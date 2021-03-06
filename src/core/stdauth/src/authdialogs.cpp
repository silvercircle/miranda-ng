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

#include "stdafx.h"

INT_PTR CALLBACK DlgProcAdded(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MEVENT hDbEvent = (MEVENT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, LPGEN("View user's details"));
		Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, LPGEN("Add contact permanently to list"));

		hDbEvent = lParam;
		SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);
		{
			//blob is: uin(DWORD), hcontact(HANDLE), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ)
			DBEVENTINFO dbei = {};
			dbei.cbBlob = db_event_getBlobSize(hDbEvent);
			dbei.pBlob = (PBYTE)alloca(dbei.cbBlob);
			db_event_get(hDbEvent, &dbei);

			DWORD uin = *(PDWORD)dbei.pBlob;
			MCONTACT hContact = DbGetAuthEventContact(&dbei);
			char* nick = (char*)dbei.pBlob + sizeof(DWORD) * 2;
			char* first = nick + mir_strlen(nick) + 1;
			char* last = first + mir_strlen(first) + 1;
			char* email = last + mir_strlen(last) + 1;

			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, CallProtoService(dbei.szModule, PS_LOADICON, PLI_PROTOCOL | PLIF_SMALL, 0));
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, CallProtoService(dbei.szModule, PS_LOADICON, PLI_PROTOCOL | PLIF_LARGE, 0));

			PROTOACCOUNT* acc = Proto_GetAccount(dbei.szModule);

			wchar_t* lastT = dbei.flags & DBEF_UTF ? Utf8DecodeW(last) : mir_a2u(last);
			wchar_t* firstT = dbei.flags & DBEF_UTF ? Utf8DecodeW(first) : mir_a2u(first);
			wchar_t* nickT = dbei.flags & DBEF_UTF ? Utf8DecodeW(nick) : mir_a2u(nick);
			wchar_t* emailT = dbei.flags & DBEF_UTF ? Utf8DecodeW(email) : mir_a2u(email);

			wchar_t name[128] = L"";
			int off = 0;
			if (firstT[0] && lastT[0])
				off = mir_snwprintf(name, L"%s %s", firstT, lastT);
			else if (firstT[0])
				off = mir_snwprintf(name, L"%s", firstT);
			else if (lastT[0])
				off = mir_snwprintf(name, L"%s", lastT);
			if (nickT[0]) {
				if (off)
					mir_snwprintf(name + off, _countof(name) - off, L" (%s)", nickT);
				else
					wcsncpy_s(name, nickT, _TRUNCATE);
			}
			if (!name[0])
				wcsncpy_s(name, TranslateT("<Unknown>"), _TRUNCATE);

			wchar_t hdr[256];
			if (uin && emailT[0])
				mir_snwprintf(hdr, TranslateT("%s added you to the contact list\n%u (%s) on %s"), name, uin, emailT, acc->tszAccountName);
			else if (uin)
				mir_snwprintf(hdr, TranslateT("%s added you to the contact list\n%u on %s"), name, uin, acc->tszAccountName);
			else
				mir_snwprintf(hdr, TranslateT("%s added you to the contact list\n%s on %s"), name, emailT[0] ? emailT : TranslateT("(Unknown)"), acc->tszAccountName);

			SetDlgItemText(hwndDlg, IDC_HEADERBAR, hdr);

			mir_free(lastT);
			mir_free(firstT);
			mir_free(nickT);
			mir_free(emailT);

			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA, (LONG_PTR)hContact);

			if (hContact == INVALID_CONTACT_ID || !db_get_b(hContact, "CList", "NotOnList", 0))
				ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_ADD:
			{
				ADDCONTACTSTRUCT acs = { 0 };
				acs.hDbEvent = hDbEvent;
				acs.handleType = HANDLE_EVENT;
				acs.szProto = "";
				CallService(MS_ADDCONTACT_SHOW, (WPARAM)hwndDlg, (LPARAM)&acs);

				MCONTACT hContact = (MCONTACT)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA);
				if ((hContact == INVALID_CONTACT_ID) || !db_get_b(hContact, "CList", "NotOnList", 0))
					ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);
			}
			break;

		case IDC_DETAILS:
			{
				MCONTACT hContact = (MCONTACT)GetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA);
				CallService(MS_USERINFO_SHOWDIALOG, hContact, 0);
			}
			break;

		case IDOK:
			{
				ADDCONTACTSTRUCT acs = { 0 };
				acs.hDbEvent = hDbEvent;
				acs.handleType = HANDLE_EVENT;
				acs.szProto = "";
				CallService(MS_ADDCONTACT_SHOW, (WPARAM)hwndDlg, (LPARAM)&acs);
			}
			//fall through
		case IDCANCEL:
			DestroyWindow(hwndDlg);
			break;
		}
		break;

	case WM_DESTROY:
		Button_FreeIcon_IcoLib(hwndDlg, IDC_ADD);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_DETAILS);
		DestroyIcon((HICON)SendMessage(hwndDlg, WM_SETICON, ICON_BIG, 0));
		DestroyIcon((HICON)SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, 0));
		break;
	}
	return FALSE;
}

INT_PTR CALLBACK DlgProcAuthReq(HWND hwndDlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MEVENT hDbEvent = (MEVENT)GetWindowLongPtr(hwndDlg, GWLP_USERDATA);

	switch (msg) {
	case WM_INITDIALOG:
		TranslateDialogDefault(hwndDlg);
		Button_SetIcon_IcoLib(hwndDlg, IDC_DETAILS, SKINICON_OTHER_USERDETAILS, LPGEN("View user's details"));
		Button_SetIcon_IcoLib(hwndDlg, IDC_ADD, SKINICON_OTHER_ADDCONTACT, LPGEN("Add contact permanently to list"));
		{
			hDbEvent = lParam;
			SetWindowLongPtr(hwndDlg, GWLP_USERDATA, lParam);

			//blob is: uin(DWORD), hcontact(DWORD), nick(ASCIIZ), first(ASCIIZ), last(ASCIIZ), email(ASCIIZ), reason(ASCIIZ)
			DBEVENTINFO dbei = {};
			dbei.cbBlob = db_event_getBlobSize(hDbEvent);
			dbei.pBlob = (PBYTE)alloca(dbei.cbBlob);
			db_event_get(hDbEvent, &dbei);

			DWORD uin = *(PDWORD)dbei.pBlob;
			MCONTACT hContact = DbGetAuthEventContact(&dbei);
			char *nick = (char*)dbei.pBlob + sizeof(DWORD) * 2;
			char *first = nick + mir_strlen(nick) + 1;
			char *last = first + mir_strlen(first) + 1;
			char *email = last + mir_strlen(last) + 1;
			char *reason = email + mir_strlen(email) + 1;

			SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, CallProtoService(dbei.szModule, PS_LOADICON, PLI_PROTOCOL | PLIF_SMALL, 0));
			SendMessage(hwndDlg, WM_SETICON, ICON_BIG, CallProtoService(dbei.szModule, PS_LOADICON, PLI_PROTOCOL | PLIF_LARGE, 0));

			PROTOACCOUNT *acc = Proto_GetAccount(dbei.szModule);

			ptrW lastT(dbei.flags & DBEF_UTF ? Utf8DecodeW(last) : mir_a2u(last));
			ptrW firstT(dbei.flags & DBEF_UTF ? Utf8DecodeW(first) : mir_a2u(first));
			ptrW nickT(dbei.flags & DBEF_UTF ? Utf8DecodeW(nick) : mir_a2u(nick));
			ptrW emailT(dbei.flags & DBEF_UTF ? Utf8DecodeW(email) : mir_a2u(email));
			ptrW reasonT(dbei.flags & DBEF_UTF ? Utf8DecodeW(reason) : mir_a2u(reason));

			wchar_t name[128] = L"";
			int off = 0;
			if (firstT[0] && lastT[0])
				off = mir_snwprintf(name, L"%s %s", (wchar_t*)firstT, (wchar_t*)lastT);
			else if (firstT[0])
				off = mir_snwprintf(name, L"%s", (wchar_t*)firstT);
			else if (lastT[0])
				off = mir_snwprintf(name, L"%s", (wchar_t*)lastT);
			if (mir_wstrlen(nickT)) {
				if (off)
					mir_snwprintf(name + off, _countof(name) - off, L" (%s)", (wchar_t*)nickT);
				else
					wcsncpy_s(name, nickT, _TRUNCATE);
			}
			if (!name[0])
				wcsncpy_s(name, TranslateT("<Unknown>"), _TRUNCATE);

			wchar_t hdr[256];
			if (uin && emailT[0])
				mir_snwprintf(hdr, TranslateT("%s requested authorization\n%u (%s) on %s"), name, uin, (wchar_t*)emailT, acc->tszAccountName);
			else if (uin)
				mir_snwprintf(hdr, TranslateT("%s requested authorization\n%u on %s"), name, uin, acc->tszAccountName);
			else
				mir_snwprintf(hdr, TranslateT("%s requested authorization\n%s on %s"), name, emailT[0] ? (wchar_t*)emailT : TranslateT("(Unknown)"), acc->tszAccountName);

			SetDlgItemText(hwndDlg, IDC_HEADERBAR, hdr);
			SetDlgItemText(hwndDlg, IDC_REASON, reasonT);

			if (hContact == INVALID_CONTACT_ID || !db_get_b(hContact, "CList", "NotOnList", 0))
				ShowWindow(GetDlgItem(hwndDlg, IDC_ADD), FALSE);

			SendDlgItemMessage(hwndDlg, IDC_DENYREASON, EM_LIMITTEXT, 255, 0);
			if (CallProtoService(dbei.szModule, PS_GETCAPS, PFLAGNUM_4, 0) & PF4_NOAUTHDENYREASON) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_DENYREASON), FALSE);
				SetDlgItemText(hwndDlg, IDC_DENYREASON, TranslateT("Feature is not supported by protocol"));
			}
			if (!db_get_b(hContact, "CList", "NotOnList", 0)) {
				EnableWindow(GetDlgItem(hwndDlg, IDC_ADDCHECK), FALSE);
				CheckDlgButton(hwndDlg, IDC_ADDCHECK, BST_UNCHECKED);
			}
			else CheckDlgButton(hwndDlg, IDC_ADDCHECK, BST_CHECKED);

			SetWindowLongPtr(GetDlgItem(hwndDlg, IDC_DETAILS), GWLP_USERDATA, (LONG_PTR)hContact);
		}
		return TRUE;

	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDC_DETAILS:
			CallService(MS_USERINFO_SHOWDIALOG, GetWindowLongPtr((HWND)lParam, GWLP_USERDATA), 0);
			break;

		case IDC_DECIDELATER:
			DestroyWindow(hwndDlg);
			break;

		case IDOK:
			{
				DBEVENTINFO dbei = {};
				db_event_get(hDbEvent, &dbei);
				CallProtoService(dbei.szModule, PS_AUTHALLOW, (WPARAM)hDbEvent, 0);

				if (IsDlgButtonChecked(hwndDlg, IDC_ADDCHECK)) {
					ADDCONTACTSTRUCT acs = { 0 };
					acs.hDbEvent = hDbEvent;
					acs.handleType = HANDLE_EVENT;
					acs.szProto = "";
					CallService(MS_ADDCONTACT_SHOW, (WPARAM)hwndDlg, (LPARAM)&acs);
				}
			}
			DestroyWindow(hwndDlg);
			break;

		case IDCANCEL:
			{
				DBEVENTINFO dbei = {};
				db_event_get(hDbEvent, &dbei);

				if (IsWindowEnabled(GetDlgItem(hwndDlg, IDC_DENYREASON))) {
					wchar_t tszReason[256];
					GetDlgItemText(hwndDlg, IDC_DENYREASON, tszReason, _countof(tszReason));
					CallProtoService(dbei.szModule, PS_AUTHDENY, hDbEvent, (LPARAM)tszReason);
				}
				else CallProtoService(dbei.szModule, PS_AUTHDENY, hDbEvent, 0);
			}
			DestroyWindow(hwndDlg);
			break;
		}
		break;

	case WM_DESTROY:
		Button_FreeIcon_IcoLib(hwndDlg, IDC_ADD);
		Button_FreeIcon_IcoLib(hwndDlg, IDC_DETAILS);
		DestroyIcon((HICON)SendMessage(hwndDlg, WM_SETICON, ICON_BIG, 0));
		DestroyIcon((HICON)SendMessage(hwndDlg, WM_SETICON, ICON_SMALL, 0));
		break;
	}
	return FALSE;
}
