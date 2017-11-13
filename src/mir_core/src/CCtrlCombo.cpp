/*

Object UI extensions
Copyright (c) 2008  Victor Pavlychko, George Hazan
Copyright (c) 2012-17 Miranda NG project

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

/////////////////////////////////////////////////////////////////////////////////////////
// CCtrlCombo class

CCtrlCombo::CCtrlCombo(CDlgBase *dlg, int ctrlId)
	: CCtrlData(dlg, ctrlId)
{}

BOOL CCtrlCombo::OnCommand(HWND, WORD, WORD idCode)
{
	switch (idCode) {
	case CBN_CLOSEUP:  OnCloseup(this);  break;
	case CBN_DROPDOWN: OnDropdown(this); break;

	case CBN_EDITCHANGE:
	case CBN_EDITUPDATE:
	case CBN_SELCHANGE:
	case CBN_SELENDOK:
		NotifyChange();
		break;
	}
	return TRUE;
}

void CCtrlCombo::OnInit()
{
	CSuper::OnInit();
	OnReset();
}

void CCtrlCombo::OnApply()
{
	CSuper::OnApply();

	if (GetDataType() == DBVT_WCHAR) {
		int len = GetWindowTextLength(m_hwnd) + 1;
		wchar_t *buf = (wchar_t *)_alloca(sizeof(wchar_t) * len);
		GetWindowText(m_hwnd, buf, len);
		SaveText(buf);
	}
	else if (GetDataType() != DBVT_DELETED) {
		SaveInt(GetInt());
	}
}

void CCtrlCombo::OnReset()
{
	if (GetDataType() == DBVT_WCHAR)
		SetText(LoadText());
	else if (GetDataType() != DBVT_DELETED)
		SetInt(LoadInt());
}

int CCtrlCombo::AddString(const wchar_t *text, LPARAM data)
{
	int iItem = SendMessage(m_hwnd, CB_ADDSTRING, 0, (LPARAM)text);
	if (data)
		SendMessage(m_hwnd, CB_SETITEMDATA, iItem, data);
	return iItem;
}

int CCtrlCombo::AddStringA(const char *text, LPARAM data)
{
	int iItem = SendMessageA(m_hwnd, CB_ADDSTRING, 0, (LPARAM)text);
	if (data)
		SendMessage(m_hwnd, CB_SETITEMDATA, iItem, data);
	return iItem;
}

void CCtrlCombo::DeleteString(int index)
{	SendMessage(m_hwnd, CB_DELETESTRING, index, 0);
}

int CCtrlCombo::FindString(const wchar_t *str, int index, bool exact)
{	return SendMessage(m_hwnd, exact?CB_FINDSTRINGEXACT:CB_FINDSTRING, index, (LPARAM)str);
}

int CCtrlCombo::FindStringA(const char *str, int index, bool exact)
{	return SendMessageA(m_hwnd, exact?CB_FINDSTRINGEXACT:CB_FINDSTRING, index, (LPARAM)str);
}

int CCtrlCombo::GetCount()
{	return SendMessage(m_hwnd, CB_GETCOUNT, 0, 0);
}

int CCtrlCombo::GetCurSel()
{	return SendMessage(m_hwnd, CB_GETCURSEL, 0, 0);
}

bool CCtrlCombo::GetDroppedState()
{	return SendMessage(m_hwnd, CB_GETDROPPEDSTATE, 0, 0) ? true : false;
}

LPARAM CCtrlCombo::GetItemData(int index)
{	return SendMessage(m_hwnd, CB_GETITEMDATA, index, 0);
}

wchar_t* CCtrlCombo::GetItemText(int index)
{
	wchar_t *result = (wchar_t *)mir_alloc(sizeof(wchar_t) * (SendMessage(m_hwnd, CB_GETLBTEXTLEN, index, 0) + 1));
	SendMessage(m_hwnd, CB_GETLBTEXT, index, (LPARAM)result);
	return result;
}

wchar_t* CCtrlCombo::GetItemText(int index, wchar_t *buf, int size)
{
	wchar_t *result = (wchar_t *)_alloca(sizeof(wchar_t) * (SendMessage(m_hwnd, CB_GETLBTEXTLEN, index, 0) + 1));
	SendMessage(m_hwnd, CB_GETLBTEXT, index, (LPARAM)result);
	mir_wstrncpy(buf, result, size);
	return buf;
}

int CCtrlCombo::InsertString(wchar_t *text, int pos, LPARAM data)
{
	int iItem = SendMessage(m_hwnd, CB_INSERTSTRING, pos, (LPARAM)text);
	SendMessage(m_hwnd, CB_SETITEMDATA, iItem, data);
	return iItem;
}

void CCtrlCombo::ResetContent()
{	SendMessage(m_hwnd, CB_RESETCONTENT, 0, 0);
}

int CCtrlCombo::SelectString(wchar_t *str)
{	return SendMessage(m_hwnd, CB_SELECTSTRING, 0, (LPARAM)str);
}

int CCtrlCombo::SetCurSel(int index)
{	return SendMessage(m_hwnd, CB_SETCURSEL, index, 0);
}

void CCtrlCombo::SetItemData(int index, LPARAM data)
{	SendMessage(m_hwnd, CB_SETITEMDATA, index, data);
}

void CCtrlCombo::ShowDropdown(bool show)
{	SendMessage(m_hwnd, CB_SHOWDROPDOWN, show ? TRUE : FALSE, 0);
}
