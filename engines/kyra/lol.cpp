/* ScummVM - Graphic Adventure Engine
 *
 * ScummVM is the legal property of its developers, whose names
 * are too numerous to list here. Please refer to the COPYRIGHT
 * file distributed with this source distribution.
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
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 *
 * $URL$
 * $Id$
 *
 */

#include "kyra/lol.h"
#include "kyra/screen_lol.h"
#include "kyra/resource.h"
#include "kyra/sound.h"
#include "kyra/util.h"

#include "sound/voc.h"
#include "sound/audiostream.h"

#include "common/endian.h"
#include "base/version.h"

namespace Kyra {

LoLEngine::LoLEngine(OSystem *system, const GameFlags &flags) : KyraEngine_v1(system, flags) {
	_screen = 0;
	_gui = 0;
	_dlg = 0;

	switch (_flags.lang) {
	case Common::EN_ANY:
	case Common::EN_USA:
	case Common::EN_GRB:
		_lang = 0;
		break;

	case Common::FR_FRA:
		_lang = 1;
		break;

	case Common::DE_DEU:
		_lang = 2;
		break;

	default:
		warning("unsupported language, switching back to English");
		_lang = 0;
		break;
	}

	_chargenWSA = 0;
	_lastUsedStringBuffer = 0;
	_landsFile = 0;
	_levelLangFile = 0;

	_lastMusicTrack = -1;
	_lastSfxTrack = -1;
	_curTlkFile = -1;
	_lastSpeaker = _lastSpeechId = -1;

	memset(_moneyColumnHeight, 0, 5);
	_credits = 0;

	_itemsInPlay = 0;
	_itemProperties = 0;
	_itemInHand = 0;
	memset(_inventory, 0, 48);
	_inventoryCurItem = 0;
	_hideControls = 0;

	_itemIconShapes = _itemShapes = _gameShapes = _thrownShapes = _iceShapes = _fireballShapes = 0;
	_levelShpList = _levelDatList = 0;
	_monsterShapes = _monsterPalettes = 0;
	_buf4 = 0;
	_gameShapeMap = 0;
	memset(_monsterUnk, 0, 3);

	_charSelection = -1;
	_characters = 0;
	_spellProperties = 0;
	_updateFlags = 0;
	_selectedSpell = 0;
	_updateCharNum = _updateCharV1 = _updateCharV2 = _updateCharV3 = _updateCharV4 = _restorePalette = _hideInventory = 0;
	_palUpdateTimer = _updatePortraitNext = 0;
	_lampStatusTimer = 0xffffffff;

	_weaponsDisabled = false;
	_lastArrowButtonShape = 0;
	_arrowButtonTimer = 0;
	_selectedCharacter = 0;
	_unkFlag = 0;
	_scriptBoolSkipExec = _sceneUpdateRequired = false;
	_unkScriptByte = 0;
	_currentDirection = 0;
	_currentBlock = 0;
	memset(_scriptExecutedFuncs, 0, 18 * sizeof(uint16));

	_wllVmpMap = _wllBuffer3 = _wllBuffer4 = _wllWallFlags = 0;
	_wllShapeMap = 0;
	_lvlShapeTop = _lvlShapeBottom = _lvlShapeLeftRight = 0;
	_levelBlockProperties = 0;
	_cLevelItems = 0;
	_monsterProperties = 0;
	_lvlBlockIndex = _lvlShapeIndex = 0;
	_unkDrawLevelBool = true;
	_vcnBlocks = 0;
	_vcnShift = 0;
	_vcnExpTable = 0;
	_vmpPtr = 0;
	_trueLightTable2 = 0;
	_trueLightTable1 = 0;
	_levelShapeProperties = 0;
	_levelShapes = 0;
	_blockDrawingBuffer = 0;
	_sceneWindowBuffer = 0;
	memset (_doorShapes, 0, 2 * sizeof(uint8*));

	_lampOilStatus = _brightness = _lampStatusUnk = 0;
	_tempBuffer5120 = 0;
	_tmpData136 = 0;
	_cLevelItems = 0;
	_unkGameFlag = 0;
	_lastMouseRegion = 0;
	_preSeq_X1 = _preSeq_Y1 = _preSeq_X2 = _preSeq_Y2 = 0;

	_dscUnk1 = 0;
	_dscShapeIndex = 0;
	_dscOvlMap = 0;	
	_dscShapeScaleW = 0;
	_dscShapeScaleH = 0;
	_dscShapeX = 0;
	_dscShapeY = 0;
	_dscTileIndex = 0;	
	_dscUnk2 = 0;
	_dscDoorShpIndex = 0;
	_dscDim1 = 0;
	_dscDim2 = 0;
	_dscBlockMap = _dscDoor1 = _dscShapeOvlIndex = 0;
	_dscBlockIndex = 0;
	_dscDimMap = 0;
	_dscDoorMonsterX = _dscDoorMonsterY = 0;
	_dscDoor4 = 0;

	_ingameSoundList = 0;
	_ingameSoundIndex = 0;
	_ingameSoundListSize = 0;
	_musicTrackMap = 0;
	_curMusicTheme = -1;
	_curMusicFileExt = 0;
	_curMusicFileIndex = -1;

	_sceneDrawVar1 = _sceneDrawVar2 = _sceneDrawVar3 = _wllProcessFlag = 0;
	_partyPosX = _partyPosY = 0;
	_shpDmX = _shpDmY = _dmScaleW = _dmScaleH = 0;

	_intFlag3 = 3;
	_floatingMouseArrowControl = 0;

	memset(_activeTim, 0, 10 * sizeof(TIM*));
	memset(_activeVoiceFile, 0, sizeof(_activeVoiceFile));
		
	//_dlgAnimCallback = &TextDisplayer_LoL::portraitAnimation1;
}

LoLEngine::~LoLEngine() {
	setupPrologueData(false);

	delete[] _landsFile;
	delete[] _levelLangFile;

	delete _screen;
	delete _gui;
	delete _tim;
	delete _dlg;

	delete[]  _itemsInPlay;
	delete[]  _itemProperties;

	delete[]  _characters;

	if (_itemIconShapes) {
		for (int i = 0; i < _numItemIconShapes; i++)
			delete[]  _itemIconShapes[i];
		delete[] _itemIconShapes;
	}
	if (_itemShapes) {
		for (int i = 0; i < _numItemShapes; i++)
			delete[]  _itemShapes[i];
		delete[] _itemShapes;
	}
	if (_gameShapes) {
		for (int i = 0; i < _numGameShapes; i++)
			delete[]  _gameShapes[i];
		delete[] _gameShapes;
	}
	if (_thrownShapes) {
		for (int i = 0; i < _numThrownShapes; i++)
			delete[]  _thrownShapes[i];
		delete[] _thrownShapes;
	}
	if (_iceShapes) {
		for (int i = 0; i < _numIceShapes; i++)
			delete[]  _iceShapes[i];
		delete[] _iceShapes;
	}
	if (_fireballShapes) {
		for (int i = 0; i < _numFireballShapes; i++)
			delete[]  _fireballShapes[i];
		delete[] _fireballShapes;
	}

	if (_monsterShapes) {
		for (int i = 0; i < 48; i++)
			delete[]  _monsterShapes[i];
		delete[] _monsterShapes;
	}
	if (_monsterPalettes) {
		for (int i = 0; i < 48; i++)
			delete[]  _monsterPalettes[i];
		delete[] _monsterPalettes;
	}
	if (_buf4) {
		for (int i = 0; i < 384; i++)
			delete[]  _buf4[i];
		delete[] _buf4;
	}

	for (Common::Array<const TIMOpcode*>::iterator i = _timIntroOpcodes.begin(); i != _timIntroOpcodes.end(); ++i)
		delete *i;
	_timIntroOpcodes.clear();

	for (Common::Array<const TIMOpcode*>::iterator i = _timIngameOpcodes.begin(); i != _timIngameOpcodes.end(); ++i)
		delete *i;
	_timIngameOpcodes.clear();


	delete[] _wllVmpMap;
	delete[] _wllShapeMap;
	delete[] _wllBuffer3;
	delete[] _wllBuffer4;
	delete[] _wllWallFlags;
	delete[] _lvlShapeTop;
	delete[] _lvlShapeBottom;
	delete[] _lvlShapeLeftRight;
	delete[] _tempBuffer5120;
	delete[] _tmpData136;
	delete[] _cLevelItems;
	delete[] _levelBlockProperties;
	delete[] _monsterProperties;

	delete[] _levelFileData;
	delete[] _vcnExpTable;
	delete[] _vcnBlocks;
	delete[] _vcnShift;
	delete[] _vmpPtr;
	delete[] _trueLightTable2;
	delete[] _trueLightTable1;
	delete[] _levelShapeProperties;
	delete[] _blockDrawingBuffer;
	delete[] _sceneWindowBuffer;

	if (_levelShapes) {
		for (int i = 0; i < 400; i++)
			delete[]  _levelShapes[i];
		delete[] _levelShapes;
	}

	for (int i = 0; i < 2; i++)
		delete[] _doorShapes[i];
	
	delete _lvlShpFileHandle;

	if (_ingameSoundList) {
		for (int i = 0; i < _ingameSoundListSize; i++)
			delete[] _ingameSoundList[i];
		delete[] _ingameSoundList;	
	}
}

Screen *LoLEngine::screen() {
	return _screen;
}

GUI *LoLEngine::gui() const {
	return _gui;
}

Common::Error LoLEngine::init() {
	_screen = new Screen_LoL(this, _system);
	assert(_screen);
	_screen->setResolution();

	KyraEngine_v1::init();
	initStaticResource();

	_gui = new GUI_LoL(this);
	assert(_gui);
	_gui->initStaticData();
	initButtonList();

	_tim = new TIMInterpreter(this, _screen, _system);
	assert(_tim);

	_dlg = new TextDisplayer_LoL(this, _screen);

	_screen->setAnimBlockPtr(10000);
	_screen->setScreenDim(0);

	_itemsInPlay = new ItemInPlay[401];
	memset(_itemsInPlay, 0, sizeof(ItemInPlay) * 400);

	_characters = new LoLCharacter[4];
	memset(_characters, 0, sizeof(LoLCharacter) * 3);

	if (!_sound->init())
		error("Couldn't init sound");

	_speechFlag = speechEnabled() ? 0x48 : 0;

	_wllVmpMap = new uint8[80];
	memset(_wllVmpMap, 0, 80);
	_wllShapeMap = new int8[80];
	memset(_wllShapeMap, 0, 80);
	_wllBuffer3 = new uint8[80];
	memset(_wllBuffer3, 0, 80);
	_wllBuffer4 = new uint8[80];
	memset(_wllBuffer4, 0, 80);
	_wllWallFlags = new uint8[80];
	memset(_wllWallFlags, 0, 80);
	_lvlShapeTop = new int16[18];
	memset(_lvlShapeTop, 0, 18 * sizeof(int16));
	_lvlShapeBottom = new int16[18];
	memset(_lvlShapeBottom, 0, 18 * sizeof(int16));
	_lvlShapeLeftRight = new int16[36];
	memset(_lvlShapeLeftRight, 0, 36 * sizeof(int16));
	_levelShapeProperties = new LevelShapeProperty[100];
	memset(_levelShapeProperties, 0, 100 * sizeof(LevelShapeProperty));
	_levelShapes = new uint8*[400];
	memset(_levelShapes, 0, 400 * sizeof(uint8*));
	_blockDrawingBuffer = new uint16[1320];
	memset(_blockDrawingBuffer, 0, 1320 * sizeof(uint16));
	_sceneWindowBuffer = new uint8[21120];
	memset(_sceneWindowBuffer, 0, 21120);

	_levelBlockProperties = new LevelBlockProperty[1025];
	memset(_levelBlockProperties, 0, 1025 * sizeof(LevelBlockProperty));
	_cLevelItems = new CLevelItem[30];
	memset(_cLevelItems, 0, 30 * sizeof(CLevelItem));
	_monsterProperties = new MonsterProperty[5];
	memset(_monsterProperties, 0, 5 * sizeof(MonsterProperty));

	_vcnExpTable = new uint8[128];
	for (int i = 0; i < 128; i++)
		_vcnExpTable[i] = i & 0x0f;

	_tempBuffer5120 = new uint8[5120];
	memset(_tempBuffer5120, 0, 5120);

	_tmpData136 = new uint8[136];
	memset(_tmpData136, 0, 136);

	memset(_gameFlags, 0, 15 * sizeof(uint16));
	memset(_unkEMC46, 0, 16 * sizeof(uint16));	

	_levelFileData = 0;
	_lvlShpFileHandle = 0;

	_sceneDrawPage1 = 2;
	_sceneDrawPage2 = 6;

	_monsterShapes = new uint8*[48];
	memset(_monsterShapes, 0, 48 * sizeof(uint8*));
	_monsterPalettes = new uint8*[48];
	memset(_monsterPalettes, 0, 48 * sizeof(uint8*));

	_buf4 = new uint8*[384];
	memset(_buf4, 0, 384 * sizeof(uint8*));
	memset(&_scriptData, 0, sizeof(EMCData));
	
	_levelFlagUnk = 0;

	return Common::kNoError;
}

Common::Error LoLEngine::go() {
	setupPrologueData(true);

	if (!saveFileLoadable(0))
		showIntro();

	preInit();

	int processSelection = -1;
	while (!shouldQuit() && processSelection == -1) {
		_screen->loadBitmap("TITLE.CPS", 2, 2, _screen->getPalette(0));
		_screen->copyRegion(0, 0, 0, 0, 320, 200, 2, 0, Screen::CR_NO_P_CHECK);

		_screen->setFont(Screen::FID_6_FNT);
		// Original version: (260|193) "V CD1.02 D"
		_screen->fprintString("SVM %s", 255, 193, 0x67, 0x00, 0x04, gScummVMVersion);
		_screen->setFont(Screen::FID_9_FNT);

		_screen->fadePalette(_screen->getPalette(0), 0x1E);
		_screen->updateScreen();

		_eventList.clear();
		int selection = mainMenu();
		_screen->hideMouse();

		// Unlike the original, we add a nice fade to black
		memset(_screen->getPalette(0), 0, 768);
		_screen->fadePalette(_screen->getPalette(0), 0x54);

		switch (selection) {
		case 0:		// New game
			processSelection = 0;
			break;

		case 1:		// Show intro
			showIntro();
			break;

		case 2:		// "Lore of the Lands" (only CD version)
			break;

		case 3:		// Load game
			// For now fall through
			//processSelection = 3;
			//break;

		case 4:		// Quit game
		default:
			quitGame();
			updateInput();
			break;
		}
	}

	if (processSelection == -1)
		return Common::kNoError;

	if (processSelection == 0) {
		_sound->loadSoundFile("LOREINTR");
		_sound->playTrack(6);		
		/*int character = */chooseCharacter();
		_sound->playTrack(1);
		_screen->fadeToBlack();
	}

	setupPrologueData(false);

	if (!shouldQuit() && (processSelection == 0 || processSelection == 3))
		startup();

	if (!shouldQuit() && processSelection == 0)
		startupNew();

	if (!shouldQuit() && (processSelection == 0 || processSelection == 3)) {
		//_dlgAnimCallback = &TextDisplayer_LoL::portraitAnimation2;
		_screen->_fadeFlag = 3;
		_sceneUpdateRequired = true;
		setUnkFlags(1);
		runLoop();
	}

	return Common::kNoError;
}

#pragma mark - Initialization

void LoLEngine::preInit() {
	debugC(9, kDebugLevelMain, "LoLEngine::preInit()");

	_res->loadPakFile("GENERAL.PAK");
	if (_flags.isTalkie)
		_res->loadPakFile("STARTUP.PAK");

	_screen->loadFont(Screen::FID_9_FNT, "FONT9P.FNT");
	_screen->loadFont(Screen::FID_6_FNT, "FONT6P.FNT");

	loadTalkFile(0);

	char filename[32];
	snprintf(filename, sizeof(filename), "LANDS.%s", _languageExt[_lang]);
	_res->exists(filename, true);
	_landsFile = _res->fileData(filename, 0);
	loadItemIconShapes();
}

void LoLEngine::loadItemIconShapes() {
	debugC(9, kDebugLevelMain, "LoLEngine::loadItemIconShapes()");

	if (_itemIconShapes) {
		for (int i = 0; i < _numItemIconShapes; i++)
			delete[]  _itemIconShapes[i];
		delete[] _itemIconShapes;
	}

	_screen->loadBitmap("ITEMICN.SHP", 3, 3, 0);
	const uint8 *shp = _screen->getCPagePtr(3);
	_numItemIconShapes = READ_LE_UINT16(shp);
	_itemIconShapes = new uint8*[_numItemIconShapes];
	for (int i = 0; i < _numItemIconShapes; i++)
		_itemIconShapes[i] = _screen->makeShapeCopy(shp, i);

	_screen->setMouseCursor(0, 0, _itemIconShapes[0]);
}

void LoLEngine::setMouseCursorToIcon(int icon) {
	_screen->_drawGuiFlag |= 0x200;
	int i = _itemProperties[_itemsInPlay[_itemInHand].itemPropertyIndex].shpIndex;
	if (i == icon)
		return;
	_screen->setMouseCursor(0, 0, _itemIconShapes[icon]);
}

void LoLEngine::setMouseCursorToItemInHand() {
	_screen->_drawGuiFlag &= 0xFDFF;
	int o = (_itemInHand == 0) ? 0 : 10;
	_screen->setMouseCursor(o, o, getItemIconShapePtr(_itemInHand));
}

bool LoLEngine::posWithinRect(int mouseX, int mouseY, int x1, int y1, int x2, int y2) {
	if (mouseX < x1 || mouseX > x2 || mouseY < y1 || mouseY > y2)
		return false;
	return true;
}

uint8 *LoLEngine::getItemIconShapePtr(int index) {
	int ix = _itemProperties[_itemsInPlay[index].itemPropertyIndex].shpIndex;
	if (_itemProperties[_itemsInPlay[index].itemPropertyIndex].flags & 0x200)
		ix += (_itemsInPlay[index].shpCurFrame_flg & 0x1fff) - 1;
	
	return _itemIconShapes[ix];
}

int LoLEngine::mainMenu() {
	debugC(9, kDebugLevelMain, "LoLEngine::mainMenu()");

	bool hasSave = saveFileLoadable(0);

	MainMenu::StaticData data = {
		{ 0, 0, 0, 0, 0 },
		{ 0x01, 0x04, 0x0C, 0x04, 0x00, 0x3D, 0x9F },
		{ 0x2C, 0x19, 0x48, 0x2C },
		Screen::FID_9_FNT, 1
	};

	if (hasSave)
		++data.menuTable[3];

	static const uint16 mainMenuStrings[4][5] = {
		{ 0x4248, 0x4249, 0x42DD, 0x424A, 0x0000 },
		{ 0x4248, 0x4249, 0x42DD, 0x4001, 0x424A },
		{ 0x4248, 0x4249, 0x424A, 0x0000, 0x0000 },
		{ 0x4248, 0x4249, 0x4001, 0x424A, 0x0000 }
	};

	int tableOffs = _flags.isTalkie ? 0 : 2;

	for (int i = 0; i < 5; ++i) {
		if (hasSave)
			data.strings[i] = getLangString(mainMenuStrings[1 + tableOffs][i]);
		else
			data.strings[i] = getLangString(mainMenuStrings[tableOffs][i]);
	}

	MainMenu *menu = new MainMenu(this);
	assert(menu);
	menu->init(data, MainMenu::Animation());

	int selection = menu->handle(_flags.isTalkie ? (hasSave ? 12 : 6) : (hasSave ? 6 : 13));
	delete menu;
	_screen->setScreenDim(0);

	if (!_flags.isTalkie && selection >= 2)
		selection++;

	if (!hasSave && selection == 3)
		selection = 4;

	return selection;
}

void LoLEngine::startup() {
	_screen->clearPage(0);
	_screen->loadBitmap("PLAYFLD.CPS", 3, 3, _screen->_currentPalette);

	uint8 *tmpPal = new uint8[0x300];
	memcpy(tmpPal, _screen->_currentPalette, 0x300);
	memset(_screen->_currentPalette, 0x3f, 0x180);
	memcpy(_screen->_currentPalette + 3, tmpPal + 3, 3);
	memset(_screen->_currentPalette + 0x240, 0x3f, 12);
	_screen->generateOverlay(_screen->_currentPalette, _screen->_paletteOverlay1, 1, 6);
	_screen->generateOverlay(_screen->_currentPalette, _screen->_paletteOverlay2, 0x90, 0x41);
	memcpy(_screen->_currentPalette, tmpPal, 0x300);
	delete[] tmpPal;

	memset(_screen->getPalette(1), 0, 0x300);
	memset(_screen->getPalette(2), 0, 0x300);

	loadItemIconShapes();
	_screen->setMouseCursor(0, 0, _itemIconShapes[0x85]);

	_screen->loadBitmap("ITEMSHP.SHP", 3, 3, 0);
	const uint8 *shp = _screen->getCPagePtr(3);
	_numItemShapes = READ_LE_UINT16(shp);
	_itemShapes = new uint8*[_numItemShapes];
	for (int i = 0; i < _numItemShapes; i++)
		_itemShapes[i] = _screen->makeShapeCopy(shp, i);

	_screen->loadBitmap("GAMESHP.SHP", 3, 3, 0);
	shp = _screen->getCPagePtr(3);
	_numGameShapes = READ_LE_UINT16(shp);
	_gameShapes = new uint8*[_numGameShapes];
	for (int i = 0; i < _numGameShapes; i++)
		_gameShapes[i] = _screen->makeShapeCopy(shp, i);

	_screen->loadBitmap("THROWN.SHP", 3, 3, 0);
	shp = _screen->getCPagePtr(3);
	_numThrownShapes = READ_LE_UINT16(shp);
	_thrownShapes = new uint8*[_numThrownShapes];
	for (int i = 0; i < _numThrownShapes; i++)
		_thrownShapes[i] = _screen->makeShapeCopy(shp, i);

	_screen->loadBitmap("ICE.SHP", 3, 3, 0);
	shp = _screen->getCPagePtr(3);
	_numIceShapes = READ_LE_UINT16(shp);
	_iceShapes = new uint8*[_numIceShapes];
	for (int i = 0; i < _numIceShapes; i++)
		_iceShapes[i] = _screen->makeShapeCopy(shp, i);

	_screen->loadBitmap("FIREBALL.SHP", 3, 3, 0);
	shp = _screen->getCPagePtr(3);
	_numFireballShapes = READ_LE_UINT16(shp);
	_fireballShapes = new uint8*[_numFireballShapes];
	for (int i = 0; i < _numFireballShapes; i++)
		_fireballShapes[i] = _screen->makeShapeCopy(shp, i);

	memset(_itemsInPlay, 0, 400 * sizeof(ItemInPlay));
	for (int i = 0; i < 400; i++)
		_itemsInPlay[i].shpCurFrame_flg |= 0x8000;

	runInitScript("ONETIME.INF", 0);
	_emc->load("ITEM.INF", &_itemScript, &_opcodes);

	_trueLightTable1 = new uint8[256];
	_trueLightTable2 = new uint8[5120];
	
	_loadSuppFilesFlag = 1;

	_dlg->setAnimParameters("<MORE>", 10, 31, 0);
	_dlg->setAnimFlag(true);

	_screen->_dimLineCount = 0;

	// reconfigure TIM player for ingame scripts
	_tim->toggleDialogueSpeech(speechEnabled());
	_tim->toggleRefresh(true);

	setMouseCursorToItemInHand();
}

void LoLEngine::startupNew() {
	_selectedSpell = 0;
	_compassUnk = 0;
	_compassDirection = _compassDirectionIndex = -1;

	_lastMouseRegion = -1;
	_unkGameFlag |= 0x1B;
	/*
	_unk5 = 1;
	_unk6 = 1;
	_unk7 = 1
	_unk8 = 1*/
	_currentLevel = 1;

	giveCredits(41, 0);
	_inventory[0] = makeItem(0xd8, 0, 0);
	_inventory[1] = makeItem(0xd9, 0, 0);
	_inventory[2] = makeItem(0xda, 0, 0);

	memset(_availableSpells, -1, 7);
	_availableSpells[0] = 0;
	setupScreenDims();

	//memset(_unkWordArraySize8, 0x100, 8);

	static int selectIds[] = { -9, -1, -8, -5 };
	addCharacter(selectIds[_charSelection]);

	// TODO 

	loadLevel(1);

	_screen->showMouse();
}

int LoLEngine::setUnkFlags(int unk) {
	if (unk < 1 || unk > 14)
		return 0;

	int r = (_intFlag3 & (2 << unk)) ? 1 : 0;
	_intFlag3 |= (2 << unk);

	return r;
}

int LoLEngine::removeUnkFlags(int unk) {
	if (unk < 1 || unk > 14)
		return 0;

	int r = (_intFlag3 & (2 << unk)) ? 1 : 0;
	_intFlag3 &= ~(2 << unk);

	return r;
}

void LoLEngine::runLoop() {
	setUnkFlags(2);

	bool _runFlag = true;
	_unkFlag |= 0x800;

	while (!shouldQuit() && _runFlag) {
		if (_nextScriptFunc) {
			runResidentScript(_nextScriptFunc, 2);
			_nextScriptFunc = 0;
		}

		//processUnkAnimStructs();
		//checkFloatingPointerRegions();
		//processCharacters();
		checkInput(0, true);
		
		update();

		if (_sceneUpdateRequired)
			gui_drawScene(0);
		else
			runLoopSub4(0);

		/*if (_partyDeathFlag != -1) {
			checkForPartyDeath(_partyDeathFlag);
			_partyDeathFlag = -1;
		}*/

		_system->delayMillis(_tickLength);
	}
}

void LoLEngine::update() {
	updateWsaAnimations();

	if (_updateCharNum != -1 && _system->getMillis() > _updatePortraitNext)
		updatePortraitWithStats();

	if (_screen->_drawGuiFlag & 0x800 || !(_updateFlags & 4))
		updateLampStatus();

	if (_screen->_drawGuiFlag & 0x4000 && !(_updateFlags & 4) && (_compassDirection == -1 || (_currentDirection << 6) != _compassDirection || _compassUnk))
		updateCompass();

	snd_characterSpeaking();
	restorePaletteEntry();

	_screen->updateScreen();
}

#pragma mark - Localization

char *LoLEngine::getLangString(uint16 id) {
	debugC(9, kDebugLevelMain, "LoLEngine::getLangString(0x%.04X)", id);

	if (id == 0xFFFF)
		return 0;

	uint16 realId = id & 0x3FFF;
	uint8 *buffer = 0;

	if (id & 0x4000)
		buffer = _landsFile;
	else
		buffer = _levelLangFile;

	if (!buffer)
		return 0;

	char *string = (char *)getTableEntry(buffer, realId);

	char *srcBuffer = _stringBuffer[_lastUsedStringBuffer];
	Util::decodeString1(string, srcBuffer);
	Util::decodeString2(srcBuffer, srcBuffer);

	++_lastUsedStringBuffer;
	_lastUsedStringBuffer %= ARRAYSIZE(_stringBuffer);

	return srcBuffer;
}

uint8 *LoLEngine::getTableEntry(uint8 *buffer, uint16 id) {
	debugC(9, kDebugLevelMain, "LoLEngine::getTableEntry(%p, %d)", (const void *)buffer, id);
	if (!buffer)
		return 0;

	return buffer + READ_LE_UINT16(buffer + (id<<1));
}

bool LoLEngine::addCharacter(int id) {
	int numChars = countActiveCharacters();
	if (numChars == 4)
		return false;

	int i = 0;
	for (; i < _charDefaultsSize; i++) {
		if (_charDefaults[i].id == id) {
			memcpy(&_characters[numChars], &_charDefaults[i], sizeof(LoLCharacter));
			break;
		}
	}
	if (i == _charDefaultsSize)
		return false;

	loadCharFaceShapes(numChars, id);

	_characters[numChars].rand = _rnd.getRandomNumberRng(1, 12);

	i = 0;
	for (; i < 11; i++) {
		if (_characters[numChars].items[i]) {
			_characters[numChars].items[i] = makeItem(_characters[numChars].items[i], 0, 0);
			runItemScript(numChars, _characters[numChars].items[i], 0x80, 0, 0);
		}
	}

	calcCharPortraitXpos();
	if (numChars > 0)
		initCharacter(numChars, 2, 6, 0);

	return true;
}

void LoLEngine::initCharacter(int charNum, int firstFaceFrame, int unk2, int redraw) {
	_characters[charNum].nextFaceFrame = firstFaceFrame;
	if (firstFaceFrame || unk2)
		initCharacterUnkSub(charNum, 6, unk2, 1);
	if (redraw)
		gui_drawCharPortraitWithStats(charNum);
}

void LoLEngine::initCharacterUnkSub(int charNum, int unk1, int unk2, int unk3) {
	for (int i = 0; i < 5; i++) {
		if (_characters[charNum].arrayUnk1[i] == 0 || (unk3 && _characters[charNum].arrayUnk1[i] == unk1)) {
			_characters[charNum].arrayUnk1[i] = unk1;
			_characters[charNum].arrayUnk2[i] = unk2;

			// TODO

			break;
		}
	}
}

int LoLEngine::countActiveCharacters() {
	int i = 0;
	while (_characters[i].flags & 1)
		i++;
	return i;
}

void LoLEngine::loadCharFaceShapes(int charNum, int id) {
	if (id < 0)
		id = -id;

	char file[13];
	snprintf(file, sizeof(file), "FACE%02d.SHP", id);
	_screen->loadBitmap(file, 3, 3, 0);

	const uint8 *p = _screen->getCPagePtr(3);
	for (int i = 0; i < 40; i++)
		_characterFaceShapes[i][charNum] = _screen->makeShapeCopy(p, i);
}

void LoLEngine::updatePortraitWithStats() {
	int x = 0;
	int y = 0;
	bool redraw = false;

	if (_updateCharV2 == 0) {
		x = _activeCharsXpos[_updateCharNum];
		y = 144;
		redraw = true;
	} else if (_updateCharV2 == 1) {
		if (textEnabled()) {
			x = 90;
			y = 130;
		} else {
			x = _activeCharsXpos[_updateCharNum];
			y = 144;
		}
	} else if (_updateCharV2 == 2) {
		if (textEnabled()) {
			x = 16;
			y = 134;
		} else {
			x = _activeCharsXpos[_updateCharNum] + 10;
			y = 145;
		}
	}

	int f = _rnd.getRandomNumberRng(1, 6) - 1;
	if (f == _characters[_updateCharNum].curFaceFrame)
		f++;
	if (f > 5)
		f -= 5;
	f += 7;

	if (_speechFlag) {
		if (snd_characterSpeaking() == 2)
			_updateCharV1 = 2;
		else
			_updateCharV1 = 1;
	}

	if (--_updateCharV1) {
		setCharFaceFrame(_updateCharNum, f);
		if (redraw)
			gui_drawCharPortraitWithStats(_updateCharNum);
		else
			gui_drawCharFaceShape(_updateCharNum, x, y, 0);
		_updatePortraitNext = _system->getMillis() + 10 * _tickLength;
	} else if (_updateCharV1 == 0 && _updateCharV3 != 0) {
		faceFrameRefresh(_updateCharNum);
		if (redraw) {
			gui_drawCharPortraitWithStats(_updateCharNum);
			updatePortraitUnkTimeSub(0, 0);
		} else {
			gui_drawCharFaceShape(_updateCharNum, x, y, 0);
		}
		_updateCharNum = -1;
	}
}

void LoLEngine::updatePortraits() {
	if (_updateCharNum == -1)
		return;

	_updateCharV1 = _updateCharV3 = 1;
	updatePortraitWithStats();
	_updateCharV1 = 1;
	_updateCharNum = -1;

	if (!_updateCharV2)
		updatePortraitUnkTimeSub(0, 0);
}

void LoLEngine::updatePortraitUnkTimeSub(int unk1, int unk2) {
	if (_updateCharV4 == unk1 || !unk1) {
		_restorePalette = 1;
		_palUpdateTimer = _system->getMillis();
	}

	if (!unk2)
		return;

	updatePortraits();
	if (_hideInventory) {
		_screen->hideMouse();
		_screen->clearDim(3);
		_screen->showMouse();
	}
	
	_restorePalette = 0;
	//initGuiUnk(11);
}

void LoLEngine::charCallback4(int redraw) {
	for (int i = 0; i < 3; i++) {
		if (!(_characters[i].flags & 1) || (_characters[i].flags & 8) || (_characters[i].curFaceFrame > 1))
			continue;

		if (_characters[i].curFaceFrame == 1) {
			_characters[i].curFaceFrame = 0;
			gui_drawCharPortraitWithStats(i);
			_characters[i].rand = _rnd.getRandomNumberRng(1, 12);
		} else {
			_characters[i].rand--;
			if (_characters[i].rand <= 0 && !redraw) {
				_characters[i].curFaceFrame = 1;
				gui_drawCharPortraitWithStats(i);
				//resetAnimStructs(9, 0, 1);
			}
		}
	}
}

void LoLEngine::setCharFaceFrame(int charNum, int frameNum) {
	_characters[charNum].curFaceFrame = frameNum;
}

void LoLEngine::faceFrameRefresh(int charNum) {
	if (_characters[charNum].curFaceFrame == 1)
		initCharacter(charNum, 0, 0, 0);
	else if (_characters[charNum].curFaceFrame == 6)
		if (_characters[charNum].nextFaceFrame != 5)
			initCharacter(charNum, 0, 0, 0);
		else
			_characters[charNum].curFaceFrame = 5;
	else
		_characters[charNum].curFaceFrame = 0;
}

void LoLEngine::setupScreenDims() {
	if (textEnabled()) {
		_screen->modifyScreenDim(4, 11, 124, 28, 45);
		_screen->modifyScreenDim(5, 85, 123, 233, 54);
	} else {
		_screen->modifyScreenDim(4, 11, 124, 28, 9);
		_screen->modifyScreenDim(5, 85, 123, 233, 18);
	}	
}

void LoLEngine::initDialogueSequence(int controlMode) {
	unkHideInventory();
	gui_prepareForSequence(112, 0, 176, 120, controlMode);

	_updateFlags |= 3;

	_dlg->setupField(true);
	_dlg->expandField();
	setupScreenDims();
	gui_disableControls(controlMode);
}

void LoLEngine::toggleSelectedCharacterFrame(bool mode) {
	if (countActiveCharacters() == 1)
		return;

	int col = mode ? 212 : 1;

	int cp = _screen->setCurPage(0);
	int x = _activeCharsXpos[_selectedCharacter];

	_screen->drawBox(x, 143, x + 65, 176, col);
	_screen->setCurPage(cp);
}

void LoLEngine::unkHideInventory() {
	_hideInventory = 1;

	if (!textEnabled() || !(_hideControls & 2)) 
		charCallback4(1);

	removeUnkFlags(2);
}

void LoLEngine::gui_prepareForSequence(int x, int y, int w, int h, int unk) {
	//resetGuiUnk(x, y, w, h, unk);

	_preSeq_X1 = x;
	_preSeq_Y1 = y;
	_preSeq_X2 = x + w;
	_preSeq_Y2 = y + h;

	int mouseOffs = _itemInHand ? 10 : 0;
	_screen->setMouseCursor(mouseOffs, mouseOffs, getItemIconShapePtr(_itemInHand));

	_lastMouseRegion = -1;

	if (w == 320) {
		setLampMode(0);
		_lampStatusSuspended = true;
	}
}

void LoLEngine::restoreSceneAfterDialogueSequence(int redraw) {
	gui_enableControls();
	_dlg->setupField(false);
	_updateFlags &= 0xffdf;

	//loadLevel_initGui()

	for (int i = 0; i < 6; i++)
		_tim->freeAnimStruct(i);

	_updateFlags = 0;

	if (redraw) {
		if (_screen->_fadeFlag != 2)
			_screen->fadeClearSceneWindow(10);
		gui_drawPlayField();
		_screen->setPaletteBrightness(_screen->_currentPalette, _brightness, _lampOilStatus);
		_screen->_fadeFlag = 0;
	}

	_hideInventory = 0;
}

void LoLEngine::restorePaletteEntry() {
	if (!_restorePalette)
		return;

	_screen->copyColour(192, 252, _system->getMillis() - _palUpdateTimer, 60 * _tickLength);

	if (_hideInventory)
		return;

	_screen->clearDim(3);

	///initGuiUnk(11);

	_restorePalette = 0;
}

void LoLEngine::updateWsaAnimations() {
	if (_updateFlags & 8)
		return;

	//TODO
}

void LoLEngine::loadTalkFile(int index) {
	char file[8];

	if (index == _curTlkFile)
		return;

	if (_curTlkFile > 0 && index > 0) {
		snprintf(file, sizeof(file), "%02d.TLK", _curTlkFile);
		_res->unloadPakFile(file);
	}

	if (index > 0)
		_curTlkFile = index;

	snprintf(file, sizeof(file), "%02d.TLK", index);
	_res->loadPakFile(file);
}

bool LoLEngine::snd_playCharacterSpeech(int id, int8 speaker, int) {
	if (!_speechFlag)
		return true;

	if (speaker < 65) {
		if (_characters[speaker].flags & 1)
			speaker = (int) _characters[speaker].name[0];
		else
			speaker = 0;
	}

	if (_lastSpeechId == id && speaker == _lastSpeaker)
		return true;

	_lastSpeechId = id;
	_lastSpeaker = speaker;

	Common::List<const char*> playList;

	char pattern1[8];
	char pattern2[5];
	char file1[13];
	char file2[13];

	snprintf(pattern2, sizeof(pattern2), "%02d", id & 0x4000 ? 0 : _curTlkFile);

	if (id & 0x4000) {
		snprintf(pattern1, sizeof(pattern1), "%03X", id & 0x3fff);
	} else if (id < 1000) {
		snprintf(pattern1, sizeof(pattern1), "%03d", id);		
	} else {
		snprintf(pattern1, sizeof(pattern1), "@%04d", id - 1000);
	}

	for (char i = '0'; i != -1; i++) {
		snprintf(file1, sizeof(file1), "%s%c%c.%s", pattern1, (char)speaker, i, pattern2);
		snprintf(file2, sizeof(file2), "%s%c%c.%s", pattern1, '_', i, pattern2);
		if (_res->exists(file1)) {
			char *f = new char[strlen(file1) + 1];
			strcpy(f, file1);
			playList.push_back(f);
		} else if (_res->exists(file2)) {
			char *f = new char[strlen(file2) + 1];
			strcpy(f, file2);
			playList.push_back(f);
		} else {
			i = -2;
		}
	}

	if (playList.empty())
		return false;

	do {
		update();
		if (snd_characterSpeaking() == 0)
			break;
	} while (_sound->voiceIsPlaying());

	strcpy(_activeVoiceFile, *playList.begin());
	_tim->setActiveSpeechFile(_activeVoiceFile);

	_sound->voicePlayFromList(playList);
		
	for (Common::List<const char*>::iterator i = playList.begin(); i != playList.end(); i++)
		delete []*i;
	playList.clear();

	_tim->setDialogueCompleteFlag(0);

	return true;
}

int LoLEngine::snd_characterSpeaking() {
	if (_sound->voiceIsPlaying(_activeVoiceFile))
		return 2;
		
	_lastSpeechId = _lastSpeaker = -1;
	
	return 1;
}

int LoLEngine::snd_dialogueSpeechUpdate(int finish) {
	if (!_sound->voiceIsPlaying(_activeVoiceFile))
		return -1;
	
	//_dlgTimer = 0;

	if (finish)
		_tim->setDialogueCompleteFlag(1);
	
	return 1;
}

void LoLEngine::snd_playSoundEffect(int track, int volume) {
	debugC(9, kDebugLevelMain | kDebugLevelSound, "LoLEngine::snd_playSoundEffect(%d, %d)", track, volume);

	if (track == 1 && (_lastSfxTrack == -1 || _lastSfxTrack == 1))
		return;

	_lastSfxTrack = track;

	int16 volIndex = (int16)READ_LE_UINT16(&_ingameSoundIndex[track * 2 + 1]);

	if (volIndex > 0)
		volIndex = (volIndex * volume) >> 8;
	else
		volIndex *= -1;

	// volume TODO

	int16 vocIndex = (int16)READ_LE_UINT16(&_ingameSoundIndex[track * 2]);
	if (vocIndex != -1) {
		_sound->voicePlay(_ingameSoundList[vocIndex], true);
	} else if (_flags.platform == Common::kPlatformPC) {
		if (_sound->getSfxType() == Sound::kMidiMT32)
			track = track < _ingameMT32SoundIndexSize ? _ingameMT32SoundIndex[track] - 1 : -1;
		else if (_sound->getSfxType() == Sound::kMidiGM)
			track = track < _ingameGMSoundIndexSize ? _ingameGMSoundIndex[track] - 1: -1;

		if (track != -1)
			KyraEngine_v1::snd_playSoundEffect(track);
	}
}

void LoLEngine::snd_loadSoundFile(int track) {
	if (_unkGameFlag & 2) {
		char filename[13];
		int t = (track - 250) * 3;

		if (_curMusicFileIndex != _musicTrackMap[t] || _curMusicFileExt != (char)_musicTrackMap[t + 1]) {
			snd_stopMusic();
			snprintf(filename, sizeof(filename), "LORE%02d%c", _musicTrackMap[t], (char)_musicTrackMap[t + 1]);
			_sound->loadSoundFile(filename);
			_curMusicFileIndex = _musicTrackMap[t];
			_curMusicFileExt = (char)_musicTrackMap[t + 1];
		} else {
			snd_stopMusic();
		}
	} else {
		//XXX
	}
}

int LoLEngine::snd_playTrack(int track) {
	if (track == -1)
		return _lastMusicTrack;
	
	int res = _lastMusicTrack;
	_lastMusicTrack = track;

	if (_unkGameFlag & 2) {
		snd_loadSoundFile(track);
		int t = (track - 250) * 3;
		_sound->playTrack(_musicTrackMap[t + 2]);
	}

	return res;
}

int LoLEngine::snd_stopMusic() {
	if (_unkGameFlag & 2) {
		if (_sound->isPlaying()) {
			_sound->beginFadeOut();
			_system->delayMillis(3 * _tickLength);
		}

		_sound->haltTrack();
	}
	return snd_playTrack(-1);
}

void LoLEngine::runLoopSub4(int a) {
	cmzS7(a, _currentBlock);
}

void LoLEngine::calcCoordinates(uint16 & x, uint16 & y, int block, uint16 xOffs, uint16 yOffs) {
	x = (block & 0x1f) << 8 | xOffs;
	y = ((block & 0xffe0) << 3) | yOffs;
}

} // end of namespace Kyra

