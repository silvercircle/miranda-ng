/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (�) 2012-17 Miranda NG project (https://miranda-ng.org),
Copyright (c) 2000-12 Miranda IM project,
all portions of this codebase are copyrighted to the people
listed in contributors.txt.

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#define DBMODULENAME "SkinHotKeys"

#define WM_HOTKEYUNREGISTERED   (WM_USER+721)

enum THotkeyType { HKT_GLOBAL, HKT_LOCAL, HKT_MANUAL };

struct THotkeyBoxData
{
	BYTE shift;
	BYTE key;
};

struct THotkeyItem
{
	THotkeyType  type;
	char        *pszService, *pszName; // pszName is valid _only_ for "root" hotkeys
	wchar_t     *pwszSection, *pwszDescription;
	LPARAM       lParam;
	WORD         DefHotkey, Hotkey;
	bool         Enabled;
	int          hLangpack;
	ATOM         idHotkey;

	THotkeyItem *rootHotkey;
	int          nSubHotkeys;
	bool         allowSubHotkeys;

	bool         OptChanged, OptDeleted, OptNew;
	WORD         OptHotkey;
	THotkeyType  OptType;
	bool         OptEnabled;

	bool         UnregisterHotkey;	// valid only during WM_APP message in options UI, used to remove unregistered hotkeys from options

	__inline char* getName() const { return (rootHotkey) ? rootHotkey->pszName : pszName; }

	__inline wchar_t* getSection() const { return TranslateW_LP(pwszSection, hLangpack); }
	__inline wchar_t* getDescr() const { return TranslateW_LP(pwszDescription, hLangpack); }
};

extern LIST<THotkeyItem> hotkeys;
extern HWND g_hwndHkOptions, g_hwndHotkeyHost;
extern DWORD g_pid, g_hkid;
extern HANDLE hEvChanged;

int HotkeyOptionsInit(WPARAM, LPARAM);

void FreeHotkey(THotkeyItem *item);
void RegisterHotkeys();
void UnregisterHotkeys();
