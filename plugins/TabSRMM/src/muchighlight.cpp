/////////////////////////////////////////////////////////////////////////////////////////
// Miranda NG: the free IM client for Microsoft* Windows*
//
// Copyright (�) 2012-17 Miranda NG project,
// Copyright (c) 2000-09 Miranda ICQ/IM project,
// all portions of this codebase are copyrighted to the people
// listed in contributors.txt.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// you should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
//
// part of tabSRMM messaging plugin for Miranda.
//
// (C) 2005-2010 by silvercircle _at_ gmail _dot_ com and contributors
//
// highlighter class for multi user chats

#include "stdafx.h"

void CMUCHighlight::cleanup()
{
	mir_free(m_NickPatternString);
	mir_free(m_TextPatternString);
	m_TextPatternString = m_NickPatternString = 0;

	mir_free(m_NickPatterns);
	mir_free(m_TextPatterns);
	m_iNickPatterns = m_iTextPatterns = 0;
	m_NickPatterns = m_TextPatterns = 0;
}

void CMUCHighlight::init()
{
	DBVARIANT dbv = { 0 };

	if (m_fInitialized)
		cleanup();							// clean up first, if we were already initialized

	m_fInitialized = true;

	if (0 == db_get_ws(0, CHAT_MODULE, "HighlightWords", &dbv)) {
		m_TextPatternString = dbv.ptszVal;
		_wsetlocale(LC_ALL, L"");
		wcslwr(m_TextPatternString);
	}

	if (0 == db_get_ws(0, CHAT_MODULE, "HighlightNames", &dbv))
		m_NickPatternString = dbv.ptszVal;

	m_dwFlags = M.GetByte(CHAT_MODULE, "HighlightEnabled", MATCH_TEXT);
	m_fHighlightMe = (M.GetByte(CHAT_MODULE, "HighlightMe", 1) ? true : false);

	tokenize(m_TextPatternString, m_TextPatterns, m_iTextPatterns);
	tokenize(m_NickPatternString, m_NickPatterns, m_iNickPatterns);
}

void CMUCHighlight::tokenize(wchar_t *tszString, wchar_t**& patterns, UINT& nr)
{
	if (tszString == 0)
		return;

	wchar_t	*p = tszString;

	if (*p == 0)
		return;

	nr = 0;

	if (*p != ' ')
		nr++;

	while (*p) {
		if (*p == ' ') {
			p++;
			while (*p && iswspace(*p))
				p++;
			if (*p)
				nr++;
		}
		p++;
	}
	patterns = (wchar_t **)mir_alloc(nr * sizeof(wchar_t*));

	p = tszString;
	nr = 0;

	if (*p != ' ')
		patterns[nr++] = p;

	while (*p) {
		if (*p == ' ') {
			*p++ = 0;
			while (*p && iswspace(*p))
				p++;
			if (*p)
				patterns[nr++] = p;
		}
		p++;
	}
}

bool CMUCHighlight::match(const GCEVENT *pgce, const SESSION_INFO *psi, DWORD dwFlags)
{
	int result = 0, nResult = 0;

	if (pgce == 0 || m_Valid == false)
		return false;

	if ((m_dwFlags & MATCH_TEXT) && (dwFlags & MATCH_TEXT) && (m_fHighlightMe || m_iTextPatterns > 0) && psi != 0) {
		wchar_t	*p = pci->RemoveFormatting(pgce->ptszText);
		p = NEWWSTR_ALLOCA(p);
		if (p == nullptr)
			return false;
		CharLower(p);

		wchar_t	*tszMe = ((psi && psi->pMe) ? NEWWSTR_ALLOCA(psi->pMe->pszNick) : 0);
		if (tszMe)
			CharLower(tszMe);

		if (m_fHighlightMe && tszMe) {
			result = wcsstr(p, tszMe) ? MATCH_TEXT : 0;
			if (0 == m_iTextPatterns)
				goto skip_textpatterns;
		}

		while (p && !result) {
			while (*p && (*p == ' ' || *p == ',' || *p == '.' || *p == ':' || *p == ';' || *p == '?' || *p == '!'))
				p++;

			if (*p == 0)
				break;

			wchar_t *p1 = p;
			while (*p1 && *p1 != ' ' && *p1 != ',' && *p1 != '.' && *p1 != ':' && *p1 != ';' && *p1 != '?' && *p1 != '!')
				p1++;

			if (*p1)
				*p1 = 0;
			else
				p1 = 0;

			for (UINT i = 0; i < m_iTextPatterns && !result; i++)
				result = wildcmpw(p, m_TextPatterns[i]) ? MATCH_TEXT : 0;

			if (p1) {
				*p1 = ' ';
				p = p1 + 1;
			}
			else p = 0;
		}
	}

skip_textpatterns:

	// optionally, match the nickname against the list of nicks to highlight
	if ((m_dwFlags & MATCH_NICKNAME) && (dwFlags & MATCH_NICKNAME) && pgce->ptszNick && m_iNickPatterns > 0) {
		for (UINT i = 0; i < m_iNickPatterns && !nResult; i++) {
			if (pgce->ptszNick)
				nResult = wildcmpw(pgce->ptszNick, m_NickPatterns[i]) ? MATCH_NICKNAME : 0;
			if ((m_dwFlags & MATCH_UIN) && pgce->ptszUserInfo)
				nResult = wildcmpw(pgce->ptszUserInfo, m_NickPatterns[i]) ? MATCH_NICKNAME : 0;
		}
	}

	return result || nResult;
}

/**
 * Dialog procedure to handle global highlight settings
 *
 * @param Standard Windows dialog procedure parameters
 */
INT_PTR CALLBACK CMUCHighlight::dlgProc(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		{
			DBVARIANT dbv = { 0 };
			if (!db_get_ws(0, CHAT_MODULE, "HighlightWords", &dbv)) {
				::SetDlgItemText(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN, dbv.ptszVal);
				::db_free(&dbv);
			}

			if (!db_get_ws(0, CHAT_MODULE, "HighlightNames", &dbv)) {
				::SetDlgItemText(hwndDlg, IDC_HIGHLIGHTNICKPATTERN, dbv.ptszVal);
				::db_free(&dbv);
			}

			DWORD dwFlags = M.GetByte(CHAT_MODULE, "HighlightEnabled", MATCH_TEXT);

			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTNICKENABLE, dwFlags & MATCH_NICKNAME ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTNICKUID, dwFlags & MATCH_UIN ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTTEXTENABLE, dwFlags & MATCH_TEXT ? BST_CHECKED : BST_UNCHECKED);
			::CheckDlgButton(hwndDlg, IDC_HIGHLIGHTME, M.GetByte(CHAT_MODULE, "HighlightMe", 1) ? BST_CHECKED : BST_UNCHECKED);

			::SendMessage(hwndDlg, WM_USER + 100, 0, 0);
		}
		return TRUE;

	case WM_USER + 100:
		Utils::enableDlgControl(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN,
			::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTTEXTENABLE) ? TRUE : FALSE);

		Utils::enableDlgControl(hwndDlg, IDC_HIGHLIGHTNICKPATTERN,
			::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKENABLE) ? TRUE : FALSE);

		Utils::enableDlgControl(hwndDlg, IDC_HIGHLIGHTNICKUID,
			::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKENABLE) ? TRUE : FALSE);

		Utils::enableDlgControl(hwndDlg, IDC_HIGHLIGHTME,
			::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTTEXTENABLE) ? TRUE : FALSE);
		return FALSE;

	case WM_COMMAND:
		if ((LOWORD(wParam) == IDC_HIGHLIGHTNICKPATTERN
			|| LOWORD(wParam) == IDC_HIGHLIGHTTEXTPATTERN)
			&& (HIWORD(wParam) != EN_CHANGE || (HWND)lParam != ::GetFocus()))
			return 0;

		::SendMessage(hwndDlg, WM_USER + 100, 0, 0);
		if (lParam != 0)
			::SendMessage(::GetParent(hwndDlg), PSM_CHANGED, 0, 0);
		break;

	case WM_NOTIFY:
		switch (((LPNMHDR)lParam)->idFrom) {
		case 0:
			switch (((LPNMHDR)lParam)->code) {
			case PSN_APPLY:
			{
				wchar_t*	szBuf = 0;
				int iLen = ::GetWindowTextLength(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTNICKPATTERN));
				if (iLen) {
					szBuf = reinterpret_cast<wchar_t *>(mir_alloc((iLen + 2) * sizeof(wchar_t)));
					::GetDlgItemText(hwndDlg, IDC_HIGHLIGHTNICKPATTERN, szBuf, iLen + 1);
					db_set_ws(0, CHAT_MODULE, "HighlightNames", szBuf);
				}

				iLen = ::GetWindowTextLength(::GetDlgItem(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN));
				if (iLen) {
					szBuf = reinterpret_cast<wchar_t *>(mir_realloc(szBuf, sizeof(wchar_t) * (iLen + 2)));
					::GetDlgItemText(hwndDlg, IDC_HIGHLIGHTTEXTPATTERN, szBuf, iLen + 1);
					db_set_ws(0, CHAT_MODULE, "HighlightWords", szBuf);
				}
				else db_set_ws(0, CHAT_MODULE, "HighlightWords", L"");

				mir_free(szBuf);
				BYTE dwFlags = (::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKENABLE) ? MATCH_NICKNAME : 0) |
					(::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTTEXTENABLE) ? MATCH_TEXT : 0);

				if (dwFlags & MATCH_NICKNAME)
					dwFlags |= (::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTNICKUID) ? MATCH_UIN : 0);

				db_set_b(0, CHAT_MODULE, "HighlightEnabled", dwFlags);
				db_set_b(0, CHAT_MODULE, "HighlightMe", ::IsDlgButtonChecked(hwndDlg, IDC_HIGHLIGHTME) ? 1 : 0);
				g_Settings.Highlight->init();
			}
			return TRUE;
			}
		}
		break;
	}
	return FALSE;
}
