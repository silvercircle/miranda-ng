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
// CCtrlClc

CCtrlClc::CCtrlClc(CDlgBase *dlg, int ctrlId)
	: CCtrlBase(dlg, ctrlId)
{}

BOOL CCtrlClc::OnNotify(int, NMHDR *pnmh)
{
	TEventInfo evt = { this, (NMCLISTCONTROL *)pnmh };
	switch (pnmh->code) {
		case CLN_EXPANDED:       OnExpanded(&evt); break;
		case CLN_LISTREBUILT:    OnListRebuilt(&evt); break;
		case CLN_ITEMCHECKED:    OnItemChecked(&evt); break;
		case CLN_DRAGGING:       OnDragging(&evt); break;
		case CLN_DROPPED:        OnDropped(&evt); break;
		case CLN_LISTSIZECHANGE: OnListSizeChange(&evt); break;
		case CLN_OPTIONSCHANGED: OnOptionsChanged(&evt); break;
		case CLN_DRAGSTOP:       OnDragStop(&evt); break;
		case CLN_NEWCONTACT:     OnNewContact(&evt); break;
		case CLN_CONTACTMOVED:   OnContactMoved(&evt); break;
		case CLN_CHECKCHANGED:   OnCheckChanged(&evt); break;
		case NM_CLICK:           OnClick(&evt); break;
	}
	return FALSE;
}

void CCtrlClc::AddContact(MCONTACT hContact)
{	SendMessage(m_hwnd, CLM_ADDCONTACT, hContact, 0);
}

void CCtrlClc::AddGroup(HANDLE hGroup)
{	SendMessage(m_hwnd, CLM_ADDGROUP, (WPARAM)hGroup, 0);
}

void CCtrlClc::AutoRebuild()
{	SendMessage(m_hwnd, CLM_AUTOREBUILD, 0, 0);
}

void CCtrlClc::DeleteItem(HANDLE hItem)
{	SendMessage(m_hwnd, CLM_DELETEITEM, (WPARAM)hItem, 0);
}

void CCtrlClc::EditLabel(HANDLE hItem)
{	SendMessage(m_hwnd, CLM_EDITLABEL, (WPARAM)hItem, 0);
}

void CCtrlClc::EndEditLabel(bool save)
{	SendMessage(m_hwnd, CLM_ENDEDITLABELNOW, save ? 0 : 1, 0);
}

void CCtrlClc::EnsureVisible(HANDLE hItem, bool partialOk)
{	SendMessage(m_hwnd, CLM_ENSUREVISIBLE, (WPARAM)hItem, partialOk ? TRUE : FALSE);
}

void CCtrlClc::Expand(HANDLE hItem, DWORD flags)
{	SendMessage(m_hwnd, CLM_EXPAND, (WPARAM)hItem, flags);
}

HANDLE CCtrlClc::FindContact(MCONTACT hContact)
{	return (HANDLE)SendMessage(m_hwnd, CLM_FINDCONTACT, hContact, 0);
}

HANDLE CCtrlClc::FindGroup(MGROUP hGroup)
{	return (HANDLE)SendMessage(m_hwnd, CLM_FINDGROUP, hGroup, 0);
}

COLORREF CCtrlClc::GetBkColor()
{	return (COLORREF)SendMessage(m_hwnd, CLM_GETBKCOLOR, 0, 0);
}

bool CCtrlClc::GetCheck(HANDLE hItem)
{	return SendMessage(m_hwnd, CLM_GETCHECKMARK, (WPARAM)hItem, 0) ? true : false;
}

int CCtrlClc::GetCount()
{	return SendMessage(m_hwnd, CLM_GETCOUNT, 0, 0);
}

HWND CCtrlClc::GetEditControl()
{	return (HWND)SendMessage(m_hwnd, CLM_GETEDITCONTROL, 0, 0);
}

DWORD CCtrlClc::GetExpand(HANDLE hItem)
{	return SendMessage(m_hwnd, CLM_GETEXPAND, (WPARAM)hItem, 0);
}

int CCtrlClc::GetExtraColumns()
{	return SendMessage(m_hwnd, CLM_GETEXTRACOLUMNS, 0, 0);
}

BYTE CCtrlClc::GetExtraImage(HANDLE hItem, int iColumn)
{
	return (BYTE)(SendMessage(m_hwnd, CLM_GETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(iColumn, 0)) & 0xFFFF);
}

HIMAGELIST CCtrlClc::GetExtraImageList()
{	return (HIMAGELIST)SendMessage(m_hwnd, CLM_GETEXTRAIMAGELIST, 0, 0);
}

HFONT CCtrlClc::GetFont(int iFontId)
{	return (HFONT)SendMessage(m_hwnd, CLM_GETFONT, (WPARAM)iFontId, 0);
}

HANDLE CCtrlClc::GetSelection()
{	return (HANDLE)SendMessage(m_hwnd, CLM_GETSELECTION, 0, 0);
}

HANDLE CCtrlClc::HitTest(int x, int y, DWORD *hitTest)
{	return (HANDLE)SendMessage(m_hwnd, CLM_HITTEST, (WPARAM)hitTest, MAKELPARAM(x,y));
}

void CCtrlClc::SelectItem(HANDLE hItem)
{	SendMessage(m_hwnd, CLM_SELECTITEM, (WPARAM)hItem, 0);
}

void CCtrlClc::SetBkBitmap(DWORD mode, HBITMAP hBitmap)
{	SendMessage(m_hwnd, CLM_SETBKBITMAP, mode, (LPARAM)hBitmap);
}

void CCtrlClc::SetBkColor(COLORREF clBack)
{	SendMessage(m_hwnd, CLM_SETBKCOLOR, (WPARAM)clBack, 0);
}

void CCtrlClc::SetCheck(HANDLE hItem, bool check)
{	SendMessage(m_hwnd, CLM_SETCHECKMARK, (WPARAM)hItem, check ? 1 : 0);
}

void CCtrlClc::SetExtraColumns(int iColumns)
{	SendMessage(m_hwnd, CLM_SETEXTRACOLUMNS, (WPARAM)iColumns, 0);
}

void CCtrlClc::SetExtraImage(HANDLE hItem, int iColumn, int iImage)
{	SendMessage(m_hwnd, CLM_SETEXTRAIMAGE, (WPARAM)hItem, MAKELPARAM(iColumn, iImage));
}

void CCtrlClc::SetExtraImageList(HIMAGELIST hImgList)
{	SendMessage(m_hwnd, CLM_SETEXTRAIMAGELIST, 0, (LPARAM)hImgList);
}

void CCtrlClc::SetFont(int iFontId, HANDLE hFont, bool bRedraw)
{	SendMessage(m_hwnd, CLM_SETFONT, (WPARAM)hFont, MAKELPARAM(bRedraw ? 1 : 0, iFontId));
}

void CCtrlClc::SetIndent(int iIndent)
{	SendMessage(m_hwnd, CLM_SETINDENT, (WPARAM)iIndent, 0);
}

void CCtrlClc::SetItemText(HANDLE hItem, char *szText)
{	SendMessage(m_hwnd, CLM_SETITEMTEXT, (WPARAM)hItem, (LPARAM)szText);
}

void CCtrlClc::SetHideEmptyGroups(bool state)
{	SendMessage(m_hwnd, CLM_SETHIDEEMPTYGROUPS, state ? 1 : 0, 0);
}

void CCtrlClc::SetGreyoutFlags(DWORD flags)
{	SendMessage(m_hwnd, CLM_SETGREYOUTFLAGS, (WPARAM)flags, 0);
}

bool CCtrlClc::GetHideOfflineRoot()
{	return SendMessage(m_hwnd, CLM_GETHIDEOFFLINEROOT, 0, 0) ? true : false;
}

void CCtrlClc::SetHideOfflineRoot(bool state)
{	SendMessage(m_hwnd, CLM_SETHIDEOFFLINEROOT, state ? 1 : 0, 9);
}

void CCtrlClc::SetUseGroups(bool state)
{	SendMessage(m_hwnd, CLM_SETUSEGROUPS, state ? 1 : 0, 0);
}

void CCtrlClc::SetOfflineModes(DWORD modes)
{	SendMessage(m_hwnd, CLM_SETOFFLINEMODES, modes, 0);
}

DWORD CCtrlClc::GetExStyle()
{	return SendMessage(m_hwnd, CLM_GETEXSTYLE, 0, 0);
}

void CCtrlClc::SetExStyle(DWORD exStyle)
{	SendMessage(m_hwnd, CLM_SETEXSTYLE, (WPARAM)exStyle, 0);
}

int CCtrlClc::GetLefrMargin()
{	return SendMessage(m_hwnd, CLM_GETLEFTMARGIN, 0, 0);
}

void CCtrlClc::SetLeftMargin(int iMargin)
{	SendMessage(m_hwnd, CLM_SETLEFTMARGIN, (WPARAM)iMargin, 0);
}

HANDLE CCtrlClc::AddInfoItem(CLCINFOITEM *cii)
{	return (HANDLE)SendMessage(m_hwnd, CLM_ADDINFOITEM, 0, (LPARAM)cii);
}

int CCtrlClc::GetItemType(HANDLE hItem)
{	return SendMessage(m_hwnd, CLM_GETITEMTYPE, (WPARAM)hItem, 0);
}

HANDLE CCtrlClc::GetNextItem(HANDLE hItem, DWORD flags)
{	return (HANDLE)SendMessage(m_hwnd, CLM_GETNEXTITEM, (WPARAM)flags, (LPARAM)hItem);
}

COLORREF CCtrlClc::GetTextColor(int iFontId)
{	return (COLORREF)SendMessage(m_hwnd, CLM_GETTEXTCOLOR, (WPARAM)iFontId, 0);
}

void CCtrlClc::SetTextColor(int iFontId, COLORREF clText)
{	SendMessage(m_hwnd, CLM_SETTEXTCOLOR, (WPARAM)iFontId, (LPARAM)clText);
}
