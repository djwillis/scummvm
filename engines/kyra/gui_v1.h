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

#ifndef KYRA_GUI_V1_H
#define KYRA_GUI_V1_H

#include "kyra/gui.h"

namespace Kyra {

#define GUI_V1_BUTTON(button, a, b, c, d, e, f, g, h, i, j, k) \
	button.nextButton = 0; \
	button.index = a; \
	button.unk6 = button.unk8 = 0; \
	button.data0Val1 = b; \
	button.data1Val1 = c; \
	button.data2Val1 = d; \
	button.data0ShapePtr = button.data1ShapePtr = button.data2ShapePtr = 0; \
	button.flags = e; \
	button.dimTableIndex = f; \
	button.x = g; \
	button.y = h; \
	button.width = i; \
	button.height = j; \
	button.flags2 = k; \
	button.mouseWheel = 0

#define GUI_V1_MENU(menu, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q) \
	menu.x = a; \
	menu.y = b; \
	menu.width = c; \
	menu.height = d; \
	menu.bkgdColor = e; \
	menu.color1 = f; \
	menu.color2 = g; \
	menu.menuNameString = h; \
	menu.textColor = i; \
	menu.titleX = j; \
	menu.titleY = k; \
	menu.highlightedItem = l; \
	menu.numberOfItems = m; \
	menu.scrollUpButtonX = n; \
	menu.scrollUpButtonY = o; \
	menu.scrollDownButtonX = p; \
	menu.scrollDownButtonY = q

#define GUI_V1_MENU_ITEM(item, a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v) \
	item.enabled = a; \
	item.itemString = d; \
	item.x = e; \
	item.y = g; \
	item.width = h; \
	item.height = i; \
	item.textColor = j; \
	item.highlightColor = k; \
	item.titleX = l; \
	item.bkgdColor = n; \
	item.color1 = o; \
	item.color2 = p; \
	item.saveSlot = q; \
	item.labelString = r; \
	item.labelX = s; \
	item.labelY = t; \
	item.unk1F = v

class KyraEngine_v1;

class GUI_v1 : public GUI {
	friend class KyraEngine_v1;
public:
	GUI_v1(KyraEngine_v1 *vm, Screen_v1 *screen);
	~GUI_v1();

	void processButton(Button *button);
	int processButtonList(Button *buttonList, uint16 inputFlags, int8 mouseWheel);

	int buttonMenuCallback(Button *caller);
private:
	void initStaticResource();

	Button _menuButtonData[6];
	Button _scrollUpButton;
	Button _scrollDownButton;
	Button *getButtonListData() { return _menuButtonData; }
	Button *getScrollUpButton() { return &_scrollUpButton; }
	Button *getScrollDownButton() { return &_scrollDownButton; }

	Menu *_menu;

	void setGUILabels();

	void setupSavegames(Menu &menu, int num);

	int resumeGame(Button *button);
	int loadGameMenu(Button *button);
	int saveGameMenu(Button *button);
	int gameControlsMenu(Button *button);
	int quitPlaying(Button *button);
	int quitConfirmYes(Button *button);
	int quitConfirmNo(Button *button);
	int loadGame(Button *button);
	int saveGame(Button *button);
	int savegameConfirm(Button *button);
	int cancelSubMenu(Button *button);
	int scrollUp(Button *button);
	int scrollDown(Button *button);
	int controlsChangeMusic(Button *button);
	int controlsChangeSounds(Button *button);
	int controlsChangeWalk(Button *button);
	int controlsChangeText(Button *button);
	int controlsChangeVoice(Button *button);
	int controlsApply(Button *button);

	bool quitConfirm(const char *str);
	void getInput();
	void updateSavegameString();
	void redrawTextfield();
	void fadePalette();
	void restorePalette();
	void setupControls(Menu &menu);

	uint8 defaultColor1() const { return 12; }
	uint8 defaultColor2() const { return 248; }

	const char *getMenuTitle(const Menu &menu) { return menu.menuNameString; }
	const char *getMenuItemTitle(const MenuItem &menuItem) { return menuItem.itemString; }
	const char *getMenuItemLabel(const MenuItem &menuItem) { return menuItem.labelString; }

	KyraEngine_v1 *_vm;
	Screen_v1 *_screen;

	bool _menuRestoreScreen;
	uint8 _toplevelMenu;
	int _savegameOffset;
	char _savegameName[31];
	const char *_specialSavegameString;
	Common::KeyState _keyPressed;
	int8 _mouseWheel;

	Button::Callback _scrollUpFunctor;
	Button::Callback _scrollDownFunctor;
	Button::Callback getScrollUpButtonHandler() const { return _scrollUpFunctor; }
	Button::Callback getScrollDownButtonHandler() const { return _scrollDownFunctor; }

	const char *_voiceTextString;
	const char *_textSpeedString;
	const char *_onString;
	const char *_offString;
	const char *_onCDString;
};

} // end of namespace Kyra

#endif

