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

#ifndef CONSOLE_DIALOG_H
#define CONSOLE_DIALOG_H

#include "dialog.h"
#include "common/str.h"
#include "common/list.h"

#include <stdarg.h>

enum {
	kBufferSize	= 32768,
	kCharWidth = 8
};

class ConsoleDialog : public Dialog {
	typedef ScummVM::String String;
protected:
	char	_buffer[kBufferSize];
	int		_linesInBuffer;

	int		_lineWidth;
	int		_linesPerPage;
	
	int 	_currentColumn;
	int		_currentLine;
	int		_scrollLine;
	

public:
	ConsoleDialog(NewGui *gui);

//	void open();
//	void close();

	virtual void drawDialog();
	virtual void handleKeyDown(uint16 ascii, int keycode, int modifiers);
//	void handleCommand(CommandSender *sender, uint32 cmd, uint32 data);

	int printf(const char *format, ...);
	int vprintf(const char *format, va_list argptr);
#undef putchar
	void putchar(int c);

protected:
	void print(const char *str);
	void nextLine();
};

#endif
