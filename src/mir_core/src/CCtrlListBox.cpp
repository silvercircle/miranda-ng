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
// CCtrlListBox class

CCtrlListBox::CCtrlListBox(CDlgBase *dlg, int ctrlId)
	: CCtrlBase(dlg, ctrlId)
{}

BOOL CCtrlListBox::OnCommand(HWND, WORD, WORD idCode)
{
	switch (idCode) {
		case LBN_DBLCLK:    OnDblClick(this); break;
		case LBN_SELCANCEL: OnSelCancel(this); break;
		case LBN_SELCHANGE: OnSelChange(this); break;
	}
	return TRUE;
}

int CCtrlListBox::AddString(wchar_t *text, LPARAM data)
{
	int iItem = ListBox_AddString(m_hwnd, text);
	ListBox_SetItemData(m_hwnd, iItem, data);
	return iItem;
}

void CCtrlListBox::DeleteString(int index)
{	ListBox_DeleteString(m_hwnd, index);
}

int CCtrlListBox::FindString(wchar_t *str, int index, bool exact)
{	return SendMessage(m_hwnd, exact?LB_FINDSTRINGEXACT:LB_FINDSTRING, index, (LPARAM)str);
}

int CCtrlListBox::GetCount()
{	return ListBox_GetCount(m_hwnd);
}

int CCtrlListBox::GetCurSel()
{	return ListBox_GetCurSel(m_hwnd);
}

LPARAM CCtrlListBox::GetItemData(int index)
{	return ListBox_GetItemData(m_hwnd, index);
}

int CCtrlListBox::GetItemRect(int index, RECT *pResult)
{	return ListBox_GetItemRect(m_hwnd, index, pResult);
}

wchar_t* CCtrlListBox::GetItemText(int index)
{
	wchar_t *result = (wchar_t *)mir_alloc(sizeof(wchar_t) * (SendMessage(m_hwnd, LB_GETTEXTLEN, index, 0) + 1));
	SendMessage(m_hwnd, LB_GETTEXT, index, (LPARAM)result);
	return result;
}

wchar_t* CCtrlListBox::GetItemText(int index, wchar_t *buf, int size)
{
	wchar_t *result = (wchar_t *)_alloca(sizeof(wchar_t) * (SendMessage(m_hwnd, LB_GETTEXTLEN, index, 0) + 1));
	SendMessage(m_hwnd, LB_GETTEXT, index, (LPARAM)result);
	mir_wstrncpy(buf, result, size);
	return buf;
}

bool CCtrlListBox::GetSel(int index)
{	return ListBox_GetSel(m_hwnd, index) ? true : false;
}

int CCtrlListBox::GetSelCount()
{	return ListBox_GetSelCount(m_hwnd);
}

int* CCtrlListBox::GetSelItems(int *items, int count)
{
	ListBox_GetSelItems(m_hwnd, count, items);
	return items;
}

int* CCtrlListBox::GetSelItems()
{
	int count = GetSelCount() + 1;
	int *result = (int *)mir_alloc(sizeof(int) * count);
	ListBox_GetSelItems(m_hwnd, count, result);
	result[count-1] = -1;
	return result;
}

int CCtrlListBox::InsertString(wchar_t *text, int pos, LPARAM data)
{
	int iItem = ListBox_InsertString(m_hwnd, pos, text);
	ListBox_SetItemData(m_hwnd, iItem, data);
	return iItem;
}

void CCtrlListBox::ResetContent()
{	ListBox_ResetContent(m_hwnd);
}

int CCtrlListBox::SelectString(wchar_t *str)
{	return ListBox_SelectString(m_hwnd, 0, str);
}

int CCtrlListBox::SetCurSel(int index)
{	return ListBox_SetCurSel(m_hwnd, index);
}

void CCtrlListBox::SetItemData(int index, LPARAM data)
{	ListBox_SetItemData(m_hwnd, index, data);
}

void CCtrlListBox::SetItemHeight(int index, int iHeight)
{	ListBox_SetItemHeight(m_hwnd, index, iHeight);
}

void CCtrlListBox::SetSel(int index, bool sel)
{	ListBox_SetSel(m_hwnd, sel ? TRUE : FALSE, index);
}
