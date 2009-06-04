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

/*
 * GP2X Wiz: Hardware Stuff.
 *
 */

#include "backends/platform/gp2xwiz/gp2xwiz-hw.h"

#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/ioctl.h>
#include <sys/soundcard.h>
#include <unistd.h>

namespace WIZ_HW {

int volumeLevel = VOLUME_INITIAL;

void deviceInit() {
}

void deviceDeinit() {
}

void mixerMoveVolume(int direction) {
    if (volumeLevel <= 10) {
        if (direction == VOLUME_UP)   volumeLevel += VOLUME_CHANGE_RATE/2;
        if (direction == VOLUME_DOWN) volumeLevel -= VOLUME_CHANGE_RATE/2;
    } else {
        if(direction == VOLUME_UP)   volumeLevel += VOLUME_CHANGE_RATE;
        if(direction == VOLUME_DOWN) volumeLevel -= VOLUME_CHANGE_RATE;
    }

    if (volumeLevel < VOLUME_MIN) volumeLevel = VOLUME_MIN;
    if (volumeLevel > VOLUME_MAX) volumeLevel = VOLUME_MAX;

    unsigned long soundDev = open("/dev/mixer", O_RDWR);

    if(soundDev) {
        int vol = ((volumeLevel << 8) | volumeLevel);
        ioctl(soundDev, SOUND_MIXER_WRITE_PCM, &vol);
        close(soundDev);
    }
}

} /* namespace WIZ_HW */
