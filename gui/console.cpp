/* ScummVM - Scumm Interpreter
 * Copyright (C) 2002 The ScummVM project
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 * $Header$
 */

#include "stdafx.h"
#include "console.h"
#include "newgui.h"

#include "common/engine.h"

/*

 _   _           _                                 _                   _   _             
| | | |_ __   __| | ___ _ __    ___ ___  _ __  ___| |_ _ __ _   _  ___| |_(_) ___  _ __  
| | | | '_ \ / _` |/ _ \ '__|  / __/ _ \| '_ \/ __| __| '__| | | |/ __| __| |/ _ \| '_ \ 
| |_| | | | | (_| |  __/ |    | (_| (_) | | | \__ \ |_| |  | |_| | (__| |_| | (_) | | | |
 \___/|_| |_|\__,_|\___|_|     \___\___/|_| |_|___/\__|_|   \__,_|\___|\__|_|\___/|_| |_|
                                                                                         
This code is not finished, so please don't complain :-)

*/

/* TODO:
 * - it is very inefficient to redraw the full thingy when just one char is added/removed.
 *   Instead, we could just copy the GFX of the blank console (i.e. after the transparent
 *   background is drawn, before any text is drawn). Then using that, it becomes trivial
 *   to erase a single character, do scrolling etc.
 * - add a scrollbar widget to allow scrolling in the history
 * - a *lot* of others things, this code is in no way complete and heavily under progress
 */
ConsoleDialog::ConsoleDialog(NewGui *gui)
	: Dialog(gui, 5, 0, 320-2*5, 5*kLineHeight+2)
{
	_lineWidth = (_w - 2) / kCharWidth;
	_linesPerPage = (_h - 2) / kLineHeight;

	memset(_buffer, ' ', kBufferSize);
	_linesInBuffer = kBufferSize / _lineWidth;
	
	_currentColumn = 0;
	_currentLine = 0;
	_scrollLine = 0;
	
	print("ScummVM "SCUMMVM_VERSION" (" SCUMMVM_CVS ")\n");
	print("Console is ready\n");
}

void ConsoleDialog::drawDialog()
{
	_gui->blendRect(_x, _y, _w, _h, _gui->_bgcolor);
	
	// Draw a border (might want to use different colors :-)
	_gui->vline(_x, _y, _y+_h-1, _gui->_textcolorhi);
	_gui->hline(_x, _y+_h-1, _x+_w-1, _gui->_textcolor);
	_gui->vline(_x+_w-1, _y, _y+_h-1, _gui->_textcolor);

	// Draw text
	int start = _scrollLine - _linesPerPage + 1;
	int y = _y + 1;
	if (start < 0)
		start = 0;
	for (int line = 0; line < _linesPerPage; line++) {
		int x = _x + 1;
		for (int column = 0; column < _lineWidth; column++) {
			int l = (start+line) % _linesInBuffer;
			byte c = _buffer[l * _lineWidth + column];
			_gui->drawChar(c, x, y, _gui->_textcolor);
			x += kCharWidth;
		}
		y += kLineHeight;
	}

	_gui->addDirtyRect(_x, _y, _w, _h);
}

void ConsoleDialog::nextLine()
{
	_currentColumn = 0;
	if (_currentLine == _scrollLine)
		_scrollLine++;
	_currentLine++;
}

int ConsoleDialog::printf(const char *format, ...)
{
	va_list	argptr;

	va_start(argptr, format);
	int count = this->vprintf(format, argptr);
	va_end (argptr);
	return count;
}

int ConsoleDialog::vprintf(const char *format, va_list argptr)
{
	char	buf[2048];

	int count = vsnprintf(buf, sizeof(buf), format, argptr);
	print(buf);
	return count;
}

void ConsoleDialog::putchar(int c)
{
	if (c == '\n')
		nextLine();
	else {
		int pos = (_currentLine % _linesInBuffer) * _lineWidth + _currentColumn;
		_buffer[pos] = (char)c;
		_currentColumn++;
		if (_currentColumn >= _lineWidth)
			nextLine();
	}
	draw();	// FIXME - not nice to redraw the full console just for one char!
}

void ConsoleDialog::print(const char *str)
{
	int pos = (_currentLine % _linesInBuffer) * _lineWidth + _currentColumn;
	while (*str) {
		if (*str == '\n') {
			nextLine();
			pos += _lineWidth - _currentColumn;
		} else {
			_buffer[pos++] = *str;
			_currentColumn++;
			if (_currentColumn >= _lineWidth)
				nextLine();
		}
		pos %= kBufferSize;
		str++;
	}
	draw();
}

void ConsoleDialog::handleKeyDown(uint16 ascii, int keycode, int modifiers) {
	if (ascii == '~' || (keycode == 27) || ascii == '#') {		// Total abort on tilde or escape
		close();
	} else if (ascii == '\r' || ascii == '\n') {		// Run command on enter/newline
		nextLine();
		draw();
	} else if (keycode == 8) {				// Backspace
		if (_currentColumn == 0) {
			_currentColumn = _lineWidth - 1;
			if (_currentLine > 0)
				_currentLine--;
		} else
			_currentColumn--;
		_buffer[(_currentLine % _linesInBuffer) * _lineWidth + _currentColumn] = ' ';
		draw();	// FIXME - not nice to redraw the full console just for one char!
	} else if ((ascii >= 31) && (ascii <= 122)) {	// Printable ASCII, add to string
		putchar(ascii);
	} else {
		debug(2, "Unhandled keycode from ConsoleDialog: %d\n", keycode);
	}
}
