/* ScummVM - Scumm Interpreter
 * Copyright (C) 2005 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.

 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 *
 */

#include "stdafx.h"
#include <kernel.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sifrpc.h>
#include <loadfile.h>
#include <malloc.h>
#include <assert.h>
#include <fileio.h>
#include <iopcontrol.h>
#include <iopheap.h>
#include "scummsys.h"
#include "../intern.h"
#include "base/engine.h"
#include "backends/ps2/systemps2.h"
#include "backends/ps2/Gs2dScreen.h"
#include "backends/ps2/ps2input.h"
#include "sjpcm.h"
#include <cdvd_rpc.h>
#include "backends/ps2/savefile.h"
#include "common/file.h"
#include "backends/ps2/sysdefs.h"
#include <libmc.h>
#include "backends/ps2/cd.h"
#include <sio.h>

#define TIMER_STACK_SIZE (1024 * 32)
#define SOUND_STACK_SIZE (1024 * 32)
#define SMP_PER_BLOCK 800
#define FROM_BCD(a) ((a >> 4) * 10 + (a & 0xF))

#define CHECK_STACK_USAGE

#ifdef USE_PS2LINK
#define IRX_PREFIX "host:"
#define IRX_SUFFIX
#else
#define IRX_PREFIX "cdrom0:\\"
#define IRX_SUFFIX ";1"
#endif

static volatile int32 g_TimerThreadSema = -1;
static volatile int32 g_SoundThreadSema = -1;
static volatile uint64 msecCount = 0;

extern void NORETURN CDECL error(const char *s, ...);

static OSystem_PS2 *g_systemPs2 = NULL;

void readRtcTime(void);

int gBitFormat = 555;

void sioprintf(const char *zFormat, ...) {
	va_list ap;
	char resStr[2048];

	va_start(ap,zFormat);
	int res = vsnprintf(resStr, 2048, zFormat, ap);
	va_end(ap);

	sio_puts(resStr);
}

OSystem *OSystem_PS2_create(void) {
	if (!g_systemPs2)
		g_systemPs2 = new OSystem_PS2();	
	return g_systemPs2;
}

extern "C" int scummvm_main(int argc, char *argv[]);

extern "C" int main(int argc, char *argv[]) {
	SifInitRpc(0);
#ifdef USE_PS2LINK
	fioInit();
#else // reset the IOP if this is a CD build
	cdvdInit(CDVD_EXIT);
	cdvdExit();
	fioExit();
	SifExitIopHeap();
	SifLoadFileExit();
	SifExitRpc();
	sio_puts("Resetting IOP.");
	SifIopReset("rom0:UDNL rom0:EELOADCNF",0);
	while (!SifIopSync())
		;
	sio_puts("IOP synced.");
	SifInitRpc(0);
	fioInit();
	SifLoadFileInit();
    cdvdInit(CDVD_INIT_NOWAIT);
#endif

	ee_thread_t thisThread;
	int tid = GetThreadId();
	ReferThreadStatus(tid, &thisThread);

	sioprintf("Thread Start Priority = %d\n", thisThread.current_priority);
	if ((thisThread.current_priority < 5) || (thisThread.current_priority > 80)) {
		/* Depending on the way ScummVM is run, we may get here with different
		   thread priorities.
		   The PS2 BIOS executes it with priority = 0, ps2link uses priority 64.
		   Don't know about NapLink, etc.
		   The priority doesn't matter too much, but we need to be at least at prio 3,
		   so we can have the timer thread run at prio 2 and the sound thread at prio 1	*/
		sioprintf("Changing thread priority");
		int res = ChangeThreadPriority(tid, 20);
		sioprintf("Result = %d", res);
	}

	sioprintf("Creating system");
	/* The OSystem has to be created before we enter ScummVM's main.
	   It sets up the memory card, etc. */
	OSystem_PS2_create();

	sioprintf("init done. starting ScummVM.");
	return scummvm_main(argc, argv);
}

s32 timerInterruptHandler(s32 cause) {
	msecCount += 10;
	T0_MODE = 0xDC2; // same value as in initialization.

	iSignalSema(g_SoundThreadSema);
	iSignalSema(g_TimerThreadSema);
	return 0;
}

void systemTimerThread(OSystem_PS2 *system) {
	system->timerThread();
}

void systemSoundThread(OSystem_PS2 *system) {
	system->soundThread();
}

OSystem_PS2::OSystem_PS2(void) {
	sioprintf("OSystem_PS2 constructor\n");

	_soundStack = _timerStack = NULL;
	_scummTimerProc = NULL;
	_scummSoundProc = NULL;
	_scummSoundParam = NULL;

	_screen = new Gs2dScreen(320, 200, TV_DONT_CARE);
	_width = 320; 
	_height = 200;

	sioprintf("Initializing timer\n");
	initTimer();

	_screen->wantAnim(true);

	if (!loadModules()) {
		sioprintf("ERROR: Can't load modules");
		printf("ERROR: Can't load modules\n");
		_screen->wantAnim(false);
		SleepThread();
		// Todo: handle this correctly...
	}
	sioprintf("Modules: UsbMouse %sloaded, UsbKbd %sloaded, Hdd %sloaded.", _useMouse ? "" : "not ", _useKbd ? "" : "not ", _useHdd ? "" : "not ");

	sioprintf("Initializing SjPCM");
	if (SjPCM_Init(0) < 0)
		sioprintf("SjPCM Bind failed");
	
	sioprintf("Initializing LibCDVD.");
	int res = CDVD_Init();
	sioprintf("result = %d\n", res);

	_timerTid = _soundTid = -1;
	_mouseVisible = false;

	sioprintf("reading RTC");
	readRtcTime();

	sioprintf("Setting non-blocking fio");
	fioSetBlockMode(FIO_NOWAIT); // asynchronous file i/o
	
	sioprintf("Starting SavefileManager");
	_saveManager = new Ps2SaveFileManager(NULL, TO_MC, _screen);

	_soundBuf = (int16*)malloc(SMP_PER_BLOCK * 2 * sizeof(int16));
	_soundBuf2 = (int16*)malloc(SMP_PER_BLOCK * sizeof(int16));

	sioprintf("Initializing ps2Input");
	_input = new Ps2Input(this, _useMouse, _useKbd);

	sio_puts("OSystem_PS2 constructor done\n");
	_screen->wantAnim(false);
}

OSystem_PS2::~OSystem_PS2(void) {
}

void OSystem_PS2::initTimer(void) {
	// this has to be set before the timer interrupt handler gets called
	g_systemPs2 = this;

	// first setup the two threads that get activated by the timer:
	// the timerthread and the soundthread
	ee_sema_t threadSema;
	threadSema.init_count = 0;
	threadSema.max_count = 255;
	g_TimerThreadSema = CreateSema(&threadSema);
	g_SoundThreadSema = CreateSema(&threadSema);
	assert((g_TimerThreadSema >= 0) && (g_SoundThreadSema >= 0));

	ee_thread_t timerThread, soundThread, thisThread;
	ReferThreadStatus(GetThreadId(), &thisThread);

	_timerStack = (uint8*)malloc(TIMER_STACK_SIZE);
	memset(_timerStack, 0xE7, TIMER_STACK_SIZE);
	_soundStack = (uint8*)malloc(SOUND_STACK_SIZE);
	memset(_soundStack, 0xE7, SOUND_STACK_SIZE);

	// give timer thread a higher priority than main thread
	timerThread.initial_priority = thisThread.current_priority - 1;
	timerThread.stack            = _timerStack;
	timerThread.stack_size       = TIMER_STACK_SIZE;
	timerThread.func             = (void *)systemTimerThread;
	//timerThread.gp_reg         = _gp; // _gp is always NULL.. broken linkfile?
	asm("move %0, $gp\n": "=r"(timerThread.gp_reg));

	// soundthread's priority is higher than main- and timerthread
	soundThread.initial_priority = thisThread.current_priority - 2;
	soundThread.stack            = _soundStack;
	soundThread.stack_size       = SOUND_STACK_SIZE;
	soundThread.func             = (void *)systemSoundThread;
	asm("move %0, $gp\n": "=r"(soundThread.gp_reg));

	_timerTid = CreateThread(&timerThread);
	_soundTid = CreateThread(&soundThread);

	assert((_timerTid >= 0) && (_soundTid >= 0));

	StartThread(_timerTid, this);
	StartThread(_soundTid, this);

	// threads done, start the interrupt handler
	AddIntcHandler( INT_TIMER0, timerInterruptHandler, 0); // 0=first handler, 9 = cause = timer0
	EnableIntc(INT_TIMER0);
	T0_HOLD = 0;
	T0_COUNT = 0;
	T0_COMP = 5859; // (busclock / 256) / 5859 = ~ 100.0064
	T0_MODE = TIMER_MODE( 2, 0, 0, 0, 1, 1, 1, 0, 1, 1);
}

void OSystem_PS2::timerThread(void) {
	while (1) {
		WaitSema(g_TimerThreadSema);
		if (_scummTimerProc)
			_scummTimerProc(0);
		_screen->timerTick();
	}
}

void OSystem_PS2::soundThread(void) {
	ee_sema_t soundSema;
	soundSema.init_count = 1;
	soundSema.max_count = 1;
	_soundSema = CreateSema(&soundSema);
	assert(_soundSema >= 0);
	while (1) {
		WaitSema(g_SoundThreadSema);
		
		WaitSema(_soundSema);
		if (_scummSoundProc) {
			while (SjPCM_Buffered() <= 4 * SMP_PER_BLOCK) {
				// call sound mixer
				_scummSoundProc(_scummSoundParam, (uint8*)_soundBuf, SMP_PER_BLOCK * 2 * sizeof(int16));
				// split data into 2 buffers, L and R
				_soundBuf2[0] = _soundBuf[1];
				for (uint32 cnt = 1; cnt < SMP_PER_BLOCK; cnt++) {
					_soundBuf[cnt]  = _soundBuf[cnt << 1];
					_soundBuf2[cnt] = _soundBuf[(cnt << 1) | 1];
				}
				// and feed it into the SPU
				SjPCM_Enqueue((short int*)_soundBuf, (short int*)_soundBuf2, SMP_PER_BLOCK, 1);
			}
		}
		SignalSema(_soundSema);
	}
}

bool OSystem_PS2::loadModules(void) {

	_useHdd = _useMouse = _useKbd = false;

	int res;
	if ((res = SifLoadModule("rom0:SIO2MAN", 0, NULL)) < 0)
		sioprintf("Cannot load module: SIO2MAN (%d)\n", res);
	else if ((res = SifLoadModule("rom0:MCMAN", 0, NULL)) < 0)
		sioprintf("Cannot load module: MCMAN (%d)\n", res);
	else if ((res = SifLoadModule("rom0:MCSERV", 0, NULL)) < 0)
		sioprintf("Cannot load module: MCSERV (%d)\n", res);
	else if ((res = SifLoadModule("rom0:PADMAN", 0, NULL)) < 0)
		sioprintf("Cannot load module: PADMAN (%d)\n", res);
	else if ((res = SifLoadModule("rom0:LIBSD", 0, NULL)) < 0)
		sioprintf("Cannot load module: LIBSD (%d)\n", res);
	else if ((res = SifLoadModule(IRX_PREFIX "CDVD.IRX" IRX_SUFFIX, 0, NULL)) < 0)
		sioprintf("Cannot load module CDVD.IRX (%d)\n", res);
	else if ((res = SifLoadModule(IRX_PREFIX "SJPCM.IRX" IRX_SUFFIX, 0, NULL)) < 0)
		sioprintf("Cannot load module: SJPCM.IRX (%d)\n", res);
	else {
		sioprintf("modules loaded\n");
		if ((res = SifLoadModule(IRX_PREFIX "USBD.IRX" IRX_SUFFIX, 0, NULL)) < 0)
			sioprintf("Cannot load module: USBD.IRX (%d)\n", res);
#ifndef USE_PS2LINK
		else if ((res = SifLoadModule(IRX_PREFIX "IOMANX.IRX" IRX_SUFFIX, 0, NULL)) < 0)
			sioprintf("Cannot load module: IOMANX.IRX (%d)\n", res);
#endif
		else {
			if ((res = SifLoadModule(IRX_PREFIX "PS2MOUSE.IRX" IRX_SUFFIX, 0, NULL)) < 0)
				sioprintf("Cannot load module: PS2MOUSE.IRX (%d)\n", res);
			else
				_useMouse = true;
			if ((res = SifLoadModule(IRX_PREFIX "PS2KBD.IRX" IRX_SUFFIX, 0, NULL)) < 0)
				sioprintf("Cannot load module: PS2KBD.IRX (%d)\n", res);
			else
				_useKbd = true;
		}
		return true;		
	}
	return false;
}

void OSystem_PS2::initSize(uint width, uint height, int overscale) {
	printf("initializing new size: (%d/%d)...", width, height);
	_screen->newScreenSize(width, height);
	_width = width;
	_height = height;
	_screen->setMouseXy(width / 2, height / 2);
	_input->newRange(0, 0, width - 1, height - 1);
	_input->warpTo(width / 2, height / 2);

	_oldMouseX = width / 2;
	_oldMouseY = height / 2;
	printf("done\n");
}

void OSystem_PS2::setPalette(const byte *colors, uint start, uint num) {
	_screen->setPalette((const uint32*)colors, (uint8)start, (uint16)num);
}

void OSystem_PS2::copyRectToScreen(const byte *buf, int pitch, int x, int y, int w, int h) {
	if (x < 0) {
		w += x;
		buf -= x;
		x = 0;
	}
	if (y < 0) {
		h += y;
		buf -= y * pitch;
		y = 0;
	}
	if (w > x + _width)
		w = _width - x;
	if (h > y + _height)
		h = _height - y;
	if ((w > 0) && (h > 0))
		_screen->copyScreenRect((const uint8*)buf, (uint16)pitch, (uint16)x, (uint16)y, (uint16)w, (uint16)h);
}

void OSystem_PS2::updateScreen(void) {
	_screen->updateScreen();	
}

uint32 OSystem_PS2::getMillis(void) {
	return (uint32)msecCount;
}

void OSystem_PS2::delayMillis(uint msecs) {
	uint64 endTime = msecCount + msecs;

	while (endTime > msecCount) {
		// idle        
	}
}

void OSystem_PS2::setTimerCallback(OSystem::TimerProc callback, int interval) {
	if (callback && (interval != 10))
		sioprintf("unhandled timer interval: %d\n", interval);
	_scummTimerProc = callback;
}

int OSystem_PS2::getOutputSampleRate(void) const {
	return 48000;
}

bool OSystem_PS2::setSoundCallback(SoundProc proc, void *param) {
	assert(proc != NULL);

	WaitSema(_soundSema);
    _scummSoundProc = proc;
	_scummSoundParam = param;
	SjPCM_Play();
	SignalSema(_soundSema);
	return true;
}

void OSystem_PS2::clearSoundCallback(void) {
	WaitSema(_soundSema);
	_scummSoundProc = NULL;
	_scummSoundParam = NULL;
	SjPCM_Pause();
	SignalSema(_soundSema);
}

SaveFileManager *OSystem_PS2::getSavefileManager(void) {
	return _saveManager;
}

OSystem::MutexRef OSystem_PS2::createMutex(void) {
	ee_sema_t newSema;
	newSema.init_count = 1;
	newSema.max_count = 1;
	int resSema = CreateSema(&newSema);
	if (resSema < 0)
		printf("createMutex: unable to create Semaphore.\n");
	return (MutexRef)resSema;
}

void OSystem_PS2::lockMutex(MutexRef mutex) {
	WaitSema((int)mutex);
}

void OSystem_PS2::unlockMutex(MutexRef mutex) {
	SignalSema((int)mutex);
}

void OSystem_PS2::deleteMutex(MutexRef mutex) {
	DeleteSema((int)mutex);
}

void OSystem_PS2::setShakePos(int shakeOffset) {
	_screen->setShakePos(shakeOffset);
}

bool OSystem_PS2::showMouse(bool visible) {
	bool retVal = _mouseVisible;
	_screen->showMouse(visible);
	_mouseVisible = visible;
	return retVal;
}

void OSystem_PS2::warpMouse(int x, int y) {
	_input->warpTo((uint16)x, (uint16)y);
	_screen->setMouseXy(x, y);
}

void OSystem_PS2::setMouseCursor(const byte *buf, uint w, uint h, int hotspot_x, int hotspot_y, byte keycolor, int cursorTargetScale) {
	_screen->setMouseOverlay(buf, w, h, hotspot_x, hotspot_y, keycolor);
}

bool OSystem_PS2::openCD(int drive) {
	return false;
}

bool OSystem_PS2::pollCD(void) {
	return false;
}

void OSystem_PS2::playCD(int track, int num_loops, int start_frame, int duration) {
}

void OSystem_PS2::stopCD(void) {
}

void OSystem_PS2::updateCD(void) {
}

void OSystem_PS2::showOverlay(void) {
	_screen->showOverlay();
}

void OSystem_PS2::hideOverlay(void) {
	_screen->hideOverlay();
}

void OSystem_PS2::clearOverlay(void) {
	_screen->clearOverlay();
}

void OSystem_PS2::grabOverlay(OverlayColor *buf, int pitch) {
	_screen->grabOverlay((uint16*)buf, (uint16)pitch);
}

void OSystem_PS2::copyRectToOverlay(const OverlayColor *buf, int pitch, int x, int y, int w, int h) {
	_screen->copyOverlayRect((uint16*)buf, (uint16)pitch, (uint16)x, (uint16)y, (uint16)w, (uint16)h);
}

const OSystem::GraphicsMode OSystem_PS2::_graphicsMode = { NULL, NULL, 0 };

const OSystem::GraphicsMode *OSystem_PS2::getSupportedGraphicsModes(void) const {
    return &_graphicsMode;
}

bool OSystem_PS2::setGraphicsMode(int mode) {
	return (mode == 0);
}

int OSystem_PS2::getGraphicsMode(void) const {
	return 0;
}

int OSystem_PS2::getDefaultGraphicsMode(void) const {
	return 0;
}

bool OSystem_PS2::pollEvent(Event &event) {
	bool res = _input->pollEvent(&event);
	if (res && (event.type == EVENT_MOUSEMOVE))
		_screen->setMouseXy(event.mouse.x, event.mouse.y);
	return res;
}

OverlayColor OSystem_PS2::RGBToColor(uint8 r, uint8 g, uint8 b) {
	return (r >> 3) | ((g >> 3) << 5) | ((b >> 3) << 10);
}

void OSystem_PS2::colorToRGB(OverlayColor color, uint8 &r, uint8 &g, uint8 &b) {
	r = (color & 0x1F) << 3;
	g = ((color >> 5) & 0x1F) << 3;
	b = ((color >> 10) & 0x1F) << 3;
}

int16 OSystem_PS2::getHeight(void) {
	return _height;
}

int16 OSystem_PS2::getWidth(void) {
	return _width;
}

void OSystem_PS2::quit(void) {
	printf("OSystem_PS2::quit\n");
	clearSoundCallback();
	setTimerCallback(NULL, 0);
	SleepThread();
}

static uint32 g_timeSecs;
static uint8  g_day, g_month, g_year;
static uint64 g_lastTimeCheck;

void readRtcTime(void) {
	struct CdClock cdClock;
	CDVD_ReadClock(&cdClock);
	g_lastTimeCheck = msecCount;

	if (cdClock.stat)
		printf("Unable to read RTC time.\n");

	g_timeSecs = ((FROM_BCD(cdClock.hour) * 60) + FROM_BCD(cdClock.minute)) * 60 + FROM_BCD(cdClock.second);
	g_day = FROM_BCD(cdClock.day);
	g_month = FROM_BCD(cdClock.month);
	g_year = FROM_BCD(cdClock.year);
	// todo: add something here to convert from JST to the right time zone
	sioprintf("Got RTC time: %d:%02d:%02d  %d.%d.%4d\n",
		FROM_BCD(cdClock.hour), FROM_BCD(cdClock.minute), FROM_BCD(cdClock.second),
		g_day, g_month, g_year + 2000);
}

time_t time(time_t *p) {
	time_t blah;
	memset(&blah, 0, sizeof(time_t));
	return blah;
}

#define SECONDS_PER_DAY (24 * 60 * 60)

struct tm *localtime(const time_t *p) {
	uint32 currentSecs = g_timeSecs + (msecCount - g_lastTimeCheck) / 1000;
	if (currentSecs >= SECONDS_PER_DAY) {
		readRtcTime();
		currentSecs = g_timeSecs + (msecCount - g_lastTimeCheck) / 1000;
	}

	static struct tm retStruct;
	memset(&retStruct, 0, sizeof(retStruct));

	retStruct.tm_hour = currentSecs / (60 * 60);
	retStruct.tm_min  = (currentSecs / 60) % 60;
	retStruct.tm_sec  = currentSecs % 60;
	retStruct.tm_year = g_year + 100;
	retStruct.tm_mday = g_day;
	retStruct.tm_mon  = g_month;
	// tm_wday, tm_yday and tm_isdst are zero for now
    return &retStruct;	
}

