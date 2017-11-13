/*
StartPosition plugin for Miranda NG

Copyright (C) 2005-2008 Felipe Brahm - souFrag
ICQ#50566818
http://www.soufrag.cl

Copyright (C) 2012-17 Miranda NG project (https://miranda-ng.org)

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation version 2
of the License.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#include "stdafx.h"


HINSTANCE g_hInst;
int hLangpack;
StartPositionPlugin* startposition;

PLUGININFOEX pluginInfo = {
    sizeof(PLUGININFOEX),
    __PLUGIN_NAME,
    PLUGIN_MAKE_VERSION(__MAJOR_VERSION, __MINOR_VERSION, __RELEASE_NUM, __BUILD_NUM),
    __DESCRIPTION,
    __AUTHOR,
    __AUTHOREMAIL,
    __COPYRIGHT,
    __AUTHORWEB,
    UNICODE_AWARE,
    // {211F6277-6F9B-4B77-A939-84D04B26B38C}
    {0x211f6277, 0x6f9b, 0x4b77, {0xa9, 0x39, 0x84, 0xd0, 0x4b, 0x26, 0xb3, 0x8c}}
};

BOOL WINAPI DllMain(HINSTANCE hinstDLL, DWORD, LPVOID)
{
    g_hInst = hinstDLL;
    return TRUE;
}

extern "C" __declspec(dllexport) PLUGININFOEX* MirandaPluginInfoEx(DWORD)
{
    return &pluginInfo;
}

extern "C" __declspec(dllexport) int Load(void)
{
    mir_getLP(&pluginInfo);
    startposition = new StartPositionPlugin;

    startposition->positionClist();

    return 0;
}

extern "C" __declspec(dllexport) int Unload(void)
{
    delete startposition;
    startposition = nullptr;

    return 0;
}
