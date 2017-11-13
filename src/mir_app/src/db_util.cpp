/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright (C) 2012-17 Miranda NG project,
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

#include "stdafx.h"
#include "profilemanager.h"

/////////////////////////////////////////////////////////////////////////////////////////

MIR_APP_DLL(int) Profile_GetPathA(size_t cbLen, char *pszDest)
{
	if (!pszDest || !cbLen)
		return 1;

	strncpy_s(pszDest, cbLen, _T2A(g_profileDir), _TRUNCATE);
	return 0;
}

MIR_APP_DLL(int) Profile_GetPathW(size_t cbLen, wchar_t *pwszDest)
{
	if (!pwszDest || !cbLen)
		return 1;

	wcsncpy_s(pwszDest, cbLen, g_profileDir, _TRUNCATE);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_APP_DLL(int) Profile_GetNameA(size_t cbLen, char *pszDest)
{
	if (!cbLen || !pszDest)
		return 1;

	strncpy_s(pszDest, cbLen, ptrA(makeFileName(g_profileName)), _TRUNCATE);
	return 0;
}

MIR_APP_DLL(int) Profile_GetNameW(size_t cbLen, wchar_t *pwszDest)
{
	if (!cbLen || !pwszDest)
		return 1;

	wcsncpy_s(pwszDest, cbLen, g_profileName, _TRUNCATE);
	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////

MIR_APP_DLL(void) Profile_SetDefault(const wchar_t *pwszPath)
{
	extern wchar_t* g_defaultProfile;
	replaceStrW(g_defaultProfile, pwszPath);
}
