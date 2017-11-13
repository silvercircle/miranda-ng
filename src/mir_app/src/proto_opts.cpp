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

#define LBN_MY_RENAME	0x1000

#define WM_MY_REFRESH	(WM_USER+0x1000)

bool CheckProtocolOrder(void);

#define errMsg \
TranslateT("WARNING! The account is going to be deleted. It means that all its \
settings, contacts and histories will be also erased.\n\n\
Are you absolutely sure?")

#define upgradeMsg \
TranslateT("Your account was successfully upgraded. \
To activate it, restart of Miranda is needed.\n\n\
If you want to restart Miranda now, press Yes, if you want to upgrade another account, press No")
//	is upgradeMsg in use in any place?
#define legacyMsg \
TranslateT("This account uses legacy protocol plugin. \
Use Miranda NG options dialogs to change its preferences.")

#define welcomeMsg \
TranslateT("Welcome to Miranda NG's account manager!\n\
Here you can set up your IM accounts.\n\n\
Select an account from the list on the left to see the available options. \
Alternatively, just click on the Plus sign underneath the list to set up a new IM account.")

static class CAccountManagerDlg *pAccMgr = nullptr;

extern HANDLE hAccListChanged;

int UnloadPlugin(wchar_t* buf, int bufLen);

PROTOACCOUNT* Proto_CreateAccount(const char *szModuleName, const char *szBaseProto, const wchar_t *tszAccountName)
{
	PROTOACCOUNT *pa = (PROTOACCOUNT*)mir_calloc(sizeof(PROTOACCOUNT));
	if (pa == nullptr)
		return nullptr;

	pa->cbSize = sizeof(PROTOACCOUNT);
	pa->bIsEnabled = pa->bIsVisible = true;
	pa->iOrder = accounts.getCount();
	pa->szProtoName = mir_strdup(szBaseProto);

	// if the internal name is empty, generate new one
	if (mir_strlen(szModuleName) == 0) {
		char buf[100];
		int count = 1;
		while (true) {
			mir_snprintf(buf, "%s_%d", szBaseProto, count++);
			if (ptrA(db_get_sa(0, buf, "AM_BaseProto")) == nullptr)
				break;
		}
		pa->szModuleName = mir_strdup(buf);
	}
	else pa->szModuleName = mir_strdup(szModuleName);

	pa->tszAccountName = mir_wstrdup(tszAccountName);

	db_set_s(0, pa->szModuleName, "AM_BaseProto", szBaseProto);
	accounts.insert(pa);

	if (ActivateAccount(pa)) {
		pa->ppro->OnEvent(EV_PROTO_ONLOAD, 0, 0);
		if (!db_get_b(0, "CList", "MoveProtoMenus", true))
			pa->ppro->OnEvent(EV_PROTO_ONMENU, 0, 0);
	}

	return pa;
}

static bool FindAccountByName(const char *szModuleName)
{
	if (!mir_strlen(szModuleName))
		return false;

	for (int i = 0; i < accounts.getCount(); i++)
		if (_stricmp(szModuleName, accounts[i]->szModuleName) == 0)
			return true;

	return false;
}

///////////////////////////////////////////////////////////////////////////////////////////////////
// Account edit form
// gets PROTOACCOUNT* as a parameter, or nullptr to edit a new one

class �AccountFormDlg : public CDlgBase
{
	int m_action;
	PROTOACCOUNT *m_pa;

	CCtrlEdit m_accName, m_internalName;
	CCtrlCombo m_prototype;
	CCtrlButton m_btnOk;

public:
	�AccountFormDlg(CDlgBase *pParent, int action, PROTOACCOUNT *pa) :
		CDlgBase(g_hInst, IDD_ACCFORM),
		m_btnOk(this, IDOK),
		m_accName(this, IDC_ACCNAME),
		m_prototype(this, IDC_PROTOTYPECOMBO),
		m_internalName(this, IDC_ACCINTERNALNAME),
		m_pa(pa),
		m_action(action)
	{
		m_hwndParent = pParent->GetHwnd();
		m_btnOk.OnClick = Callback(this, &�AccountFormDlg::OnOk);
	}

	virtual void OnInitDialog() override
	{
		PROTOCOLDESCRIPTOR **proto;
		int protoCount, i, cnt = 0;
		Proto_EnumProtocols(&protoCount, &proto);
		for (i = 0; i < protoCount; i++) {
			PROTOCOLDESCRIPTOR *pd = proto[i];
			if (pd->type == PROTOTYPE_PROTOCOL && pd->cbSize == sizeof(*pd)) {
				m_prototype.AddStringA(proto[i]->szName);
				++cnt;
			}
		}

		m_prototype.SetCurSel(0);
		m_btnOk.Enable(cnt != 0);

		if (m_action == PRAC_ADDED) // new account
			SetCaption(TranslateT("Create new account"));
		else {
			wchar_t str[200];
			if (m_action == PRAC_CHANGED) { // update
				m_prototype.Disable();
				mir_snwprintf(str, L"%s: %s", TranslateT("Editing account"), m_pa->tszAccountName);
			}
			else mir_snwprintf(str, L"%s: %s", TranslateT("Upgrading account"), m_pa->tszAccountName);

			SetCaption(str);
			m_accName.SetText(m_pa->tszAccountName);
			m_internalName.SetTextA(m_pa->szModuleName);
			m_internalName.Disable();
			m_prototype.SelectString(_A2T(m_pa->szProtoName));
		}

		m_internalName.SendMsg(EM_LIMITTEXT, 40, 0);
	}

	void OnOk(CCtrlButton*)
	{
		wchar_t tszAccName[256];
		m_accName.GetText(tszAccName, _countof(tszAccName));
		rtrimw(tszAccName);
		if (tszAccName[0] == 0) {
			MessageBox(m_hwnd, TranslateT("Account name must be filled."), TranslateT("Account error"), MB_ICONERROR | MB_OK);
			return;
		}

		if (m_action == PRAC_ADDED) {
			char buf[200];
			m_internalName.GetTextA(buf, _countof(buf));
			if (FindAccountByName(rtrim(buf))) {
				MessageBox(m_hwnd, TranslateT("Account name has to be unique. Please enter unique name."), TranslateT("Account error"), MB_ICONERROR | MB_OK);
				return;
			}
		}

		if (m_action == PRAC_UPGRADED) {
			BOOL oldProto = m_pa->bOldProto;
			wchar_t szPlugin[MAX_PATH];
			mir_snwprintf(szPlugin, L"%s.dll", _A2T(m_pa->szProtoName));
			int idx = accounts.getIndex(m_pa);
			UnloadAccount(m_pa, false, false);
			accounts.remove(idx);
			if (oldProto && UnloadPlugin(szPlugin, _countof(szPlugin))) {
				wchar_t szNewName[MAX_PATH];
				mir_snwprintf(szNewName, L"%s~", szPlugin);
				MoveFile(szPlugin, szNewName);
			}
			m_action = PRAC_ADDED;
		}

		if (m_action == PRAC_ADDED) {
			char buf[200];
			GetDlgItemTextA(m_hwnd, IDC_PROTOTYPECOMBO, buf, _countof(buf));
			char *szBaseProto = NEWSTR_ALLOCA(buf);

			m_internalName.GetTextA(buf, _countof(buf));
			rtrim(buf);

			m_pa = Proto_CreateAccount(buf, szBaseProto, tszAccName);
		}
		else replaceStrW(m_pa->tszAccountName, tszAccName);

		WriteDbAccounts();
		NotifyEventHooks(hAccListChanged, m_action, (LPARAM)m_pa);

		SendMessage(GetParent(m_hwnd), WM_MY_REFRESH, 0, 0);
		EndModal(IDOK);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////
// Account manager dialog

class CAccountListCtrl : public CCtrlListBox
{
	friend class CAccountManagerDlg;

	int  m_iItem;
	RECT m_rcCheck;
	HWND m_hwndEdit;

public:
	CAccountListCtrl(CDlgBase *dlg, int ctrlId) :
		CCtrlListBox(dlg, ctrlId),
		m_iItem(-1),
		m_hwndEdit(nullptr)
	{}

	__forceinline CAccountManagerDlg* PARENT() { return (CAccountManagerDlg*)m_parentWnd; }

	virtual void OnInit()
	{
		CCtrlListBox::OnInit();
		Subclass();
	}

	virtual BOOL OnDrawItem(DRAWITEMSTRUCT *lps) override;
	virtual BOOL OnMeasureItem(MEASUREITEMSTRUCT *lps) override;

	virtual LRESULT CustomWndProc(UINT msg, WPARAM wParam, LPARAM lParam) override;

	void InitRename();
};

class CAccountManagerDlg : public CDlgBase
{
	friend class CAccountListCtrl;

	HFONT m_hfntTitle, m_hfntText;
	int m_titleHeight, m_textHeight;
	int m_selectedHeight, m_normalHeight;
	int m_iSelected;

	CAccountListCtrl m_accList;
	CCtrlButton m_btnAdd, m_btnEdit, m_btnUpgrade, m_btnRemove, m_btnOptions, m_btnNetwork;
	CCtrlButton m_btnOk, m_btnCancel;
	CCtrlBase m_name;

	void ClickButton(CCtrlButton &btn)
	{
		if (btn.Enabled())
			::PostMessage(m_hwnd, WM_COMMAND, MAKEWPARAM(btn.GetCtrlId(), BN_CLICKED), (LPARAM)btn.GetHwnd());
	}

	void SelectItem(int iItem)
	{
		if ((m_iSelected != iItem) && (m_iSelected >= 0))
			m_accList.SetItemHeight(m_iSelected, m_normalHeight);

		m_iSelected = iItem;
		m_accList.SetItemHeight(m_iSelected, m_selectedHeight);
		RedrawWindow(m_accList.GetHwnd(), nullptr, nullptr, RDW_INVALIDATE);
	}

	void UpdateAccountInfo()
	{
		int curSel = m_accList.GetCurSel();
		if (curSel != LB_ERR) {
			PROTOACCOUNT *pa = (PROTOACCOUNT *)m_accList.GetItemData(curSel);
			if (pa) {
				m_btnEdit.Enable(!pa->bOldProto && !pa->bDynDisabled);
				m_btnRemove.Enable(true);
				m_btnUpgrade.Enable(pa->bOldProto || pa->bDynDisabled);
				m_btnOptions.Enable(pa->ppro != 0);

				if (m_iSelected >= 0) {
					PROTOACCOUNT *pa_old = (PROTOACCOUNT *)m_accList.GetItemData(m_iSelected);
					if (pa_old && pa_old != pa && pa_old->hwndAccMgrUI)
						ShowWindow(pa_old->hwndAccMgrUI, SW_HIDE);
				}

				if (pa->hwndAccMgrUI) {
					ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_INFO), SW_HIDE);
					ShowWindow(pa->hwndAccMgrUI, SW_SHOW);
				}
				else if (!pa->ppro) {
					ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_INFO), SW_SHOW);
					SetDlgItemText(m_hwnd, IDC_TXT_INFO, TranslateT("Account is disabled. Please activate it to access options."));
				}
				else {
					HWND hwnd = (HWND)CallProtoService(pa->szModuleName, PS_CREATEACCMGRUI, 0, (LPARAM)m_hwnd);
					if (hwnd && (hwnd != (HWND)CALLSERVICE_NOTFOUND)) {
						RECT rc;

						ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_INFO), SW_HIDE);

						GetWindowRect(GetDlgItem(m_hwnd, IDC_TXT_INFO), &rc);
						MapWindowPoints(nullptr, m_hwnd, (LPPOINT)&rc, 2);
						SetWindowPos(hwnd, m_accList.GetHwnd(), rc.left, rc.top, 0, 0, SWP_NOSIZE | SWP_SHOWWINDOW);

						pa->hwndAccMgrUI = hwnd;
					}
					else {
						ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_INFO), SW_SHOW);
						SetDlgItemText(m_hwnd, IDC_TXT_INFO, legacyMsg);
					}
				}
				return;
			}
		}

		m_btnEdit.Disable();
		m_btnRemove.Disable();
		m_btnUpgrade.Disable();
		m_btnOptions.Disable();

		ShowWindow(GetDlgItem(m_hwnd, IDC_TXT_INFO), SW_SHOW);
		SetDlgItemText(m_hwnd, IDC_TXT_INFO, welcomeMsg);
	}

public:
	CAccountManagerDlg() :
		CDlgBase(g_hInst, IDD_ACCMGR),
		m_accList(this, IDC_ACCLIST),
		m_name(this, IDC_NAME),
		m_btnOk(this, IDOK),
		m_btnAdd(this, IDC_ADD),
		m_btnEdit(this, IDC_EDIT),
		m_btnCancel(this, IDCANCEL),
		m_btnRemove(this, IDC_REMOVE),
		m_btnUpgrade(this, IDC_UPGRADE),
		m_btnOptions(this, IDC_OPTIONS),
		m_btnNetwork(this, IDC_LNK_NETWORK)
	{
		m_name.UseSystemColors();

		m_accList.OnDblClick = Callback(this, &CAccountManagerDlg::OnListDblClick);
		m_accList.OnSelChange = Callback(this, &CAccountManagerDlg::OnListSelChange);
		m_accList.OnBuildMenu = Callback(this, &CAccountManagerDlg::OnListMenu);

		m_btnOk.OnClick = Callback(this, &CAccountManagerDlg::OnOk);
		m_btnAdd.OnClick = Callback(this, &CAccountManagerDlg::OnAdd);
		m_btnEdit.OnClick = Callback(this, &CAccountManagerDlg::OnEdit);
		m_btnRemove.OnClick = Callback(this, &CAccountManagerDlg::OnRemove);
		m_btnCancel.OnClick = Callback(this, &CAccountManagerDlg::OnCancel);
		m_btnUpgrade.OnClick = Callback(this, &CAccountManagerDlg::OnUpgrade);
		m_btnOptions.OnClick = Callback(this, &CAccountManagerDlg::OnOptions);
		m_btnNetwork.OnClick = Callback(this, &CAccountManagerDlg::OnNetwork);
	}

	virtual void OnInitDialog() override
	{
		Window_SetSkinIcon_IcoLib(m_hwnd, SKINICON_OTHER_ACCMGR);

		Button_SetIcon_IcoLib(m_hwnd, IDC_ADD, SKINICON_OTHER_ADDCONTACT, LPGEN("New account"));
		Button_SetIcon_IcoLib(m_hwnd, IDC_EDIT, SKINICON_OTHER_RENAME, LPGEN("Edit"));
		Button_SetIcon_IcoLib(m_hwnd, IDC_REMOVE, SKINICON_OTHER_DELETE, LPGEN("Remove account"));
		Button_SetIcon_IcoLib(m_hwnd, IDC_OPTIONS, SKINICON_OTHER_OPTIONS, LPGEN("Configure..."));
		Button_SetIcon_IcoLib(m_hwnd, IDC_UPGRADE, SKINICON_OTHER_ACCMGR, LPGEN("Upgrade account"));

		m_btnEdit.Disable();
		m_btnRemove.Disable();
		m_btnUpgrade.Disable();
		m_btnOptions.Disable();

		LOGFONT lf;
		GetObject((HFONT)SendMessage(m_hwnd, WM_GETFONT, 0, 0), sizeof(lf), &lf);
		m_hfntText = ::CreateFontIndirect(&lf);

		GetObject((HFONT)SendMessage(m_hwnd, WM_GETFONT, 0, 0), sizeof(lf), &lf);
		lf.lfWeight = FW_BOLD;
		m_hfntTitle = ::CreateFontIndirect(&lf);

		HDC hdc = GetDC(m_hwnd);
		HFONT hfnt = (HFONT)::SelectObject(hdc, m_hfntTitle);

		TEXTMETRIC tm;
		GetTextMetrics(hdc, &tm);
		m_titleHeight = tm.tmHeight;
		::SelectObject(hdc, m_hfntText);

		GetTextMetrics(hdc, &tm);
		m_textHeight = tm.tmHeight;
		::SelectObject(hdc, hfnt);
		::ReleaseDC(m_hwnd, hdc);

		m_normalHeight = 4 + max(m_titleHeight, g_iIconSY);
		m_selectedHeight = m_normalHeight + 4 + 2 * m_textHeight;

		m_name.SendMsg(WM_SETFONT, (WPARAM)m_hfntTitle, 0);
		SendDlgItemMessage(m_hwnd, IDC_TXT_ACCOUNT, WM_SETFONT, (WPARAM)m_hfntTitle, 0);
		SendDlgItemMessage(m_hwnd, IDC_TXT_ADDITIONAL, WM_SETFONT, (WPARAM)m_hfntTitle, 0);

		m_iSelected = -1;
		SendMessage(m_hwnd, WM_MY_REFRESH, 0, 0);

		Utils_RestoreWindowPositionNoSize(m_hwnd, 0, "AccMgr", "");
	}

	virtual void OnApply() override
	{
		PSHNOTIFY pshn;
		pshn.hdr.idFrom = 0;
		pshn.hdr.code = PSN_APPLY;
		for (int i = 0; i < accounts.getCount(); i++) {
			PROTOACCOUNT *pa = accounts[i];
			if (pa->hwndAccMgrUI && pa->bAccMgrUIChanged) {
				pshn.hdr.hwndFrom = pa->hwndAccMgrUI;
				SendMessage(pa->hwndAccMgrUI, WM_NOTIFY, 0, (LPARAM)&pshn);
				pa->bAccMgrUIChanged = FALSE;
			}
		}
	}

	virtual void OnReset() override
	{
		PSHNOTIFY pshn;
		pshn.hdr.idFrom = 0;
		pshn.hdr.code = PSN_RESET;
		for (int i = 0; i < accounts.getCount(); i++) {
			PROTOACCOUNT *pa = accounts[i];
			if (pa->hwndAccMgrUI && pa->bAccMgrUIChanged) {
				pshn.hdr.hwndFrom = pa->hwndAccMgrUI;
				SendMessage(pa->hwndAccMgrUI, WM_NOTIFY, 0, (LPARAM)&pshn);
				pa->bAccMgrUIChanged = FALSE;
			}
		}
	}

	virtual void OnDestroy() override
	{
		for (int i = 0; i < accounts.getCount(); i++) {
			PROTOACCOUNT *pa = accounts[i];
			pa->bAccMgrUIChanged = FALSE;
			if (pa->hwndAccMgrUI) {
				::DestroyWindow(pa->hwndAccMgrUI);
				pa->hwndAccMgrUI = nullptr;
			}
		}

		Window_FreeIcon_IcoLib(m_hwnd);
		Button_FreeIcon_IcoLib(m_hwnd, IDC_ADD);
		Button_FreeIcon_IcoLib(m_hwnd, IDC_EDIT);
		Button_FreeIcon_IcoLib(m_hwnd, IDC_REMOVE);
		Button_FreeIcon_IcoLib(m_hwnd, IDC_OPTIONS);
		Button_FreeIcon_IcoLib(m_hwnd, IDC_UPGRADE);
		Utils_SaveWindowPosition(m_hwnd, 0, "AccMgr", "");
		DeleteObject(m_hfntTitle);
		DeleteObject(m_hfntText);
		pAccMgr = nullptr;
	}

	void OnListMenu(void*)
	{
		POINT pt;
		GetCursorPos(&pt);

		// menu was activated with mouse = > find item under cursor & set focus to our control.
		POINT ptItem = pt;
		ScreenToClient(m_accList.GetHwnd(), &ptItem);
		int iItem = (short)LOWORD(m_accList.SendMsg(LB_ITEMFROMPOINT, 0, MAKELPARAM(ptItem.x, ptItem.y)));
		if (iItem == -1)
			return;

		m_accList.SetCurSel(iItem);
		UpdateAccountInfo();
		SelectItem(iItem);
		SetFocus(m_accList.GetHwnd());

		PROTOACCOUNT *pa = (PROTOACCOUNT*)m_accList.GetItemData(iItem);
		HMENU hMenu = CreatePopupMenu();
		if (!pa->bOldProto && !pa->bDynDisabled)
			AppendMenu(hMenu, MF_STRING, 1, TranslateT("Rename"));

		AppendMenu(hMenu, MF_STRING, 3, TranslateT("Delete"));

		if (Proto_IsAccountEnabled(pa))
			AppendMenu(hMenu, MF_STRING, 4, TranslateT("Configure"));

		if (pa->bOldProto || pa->bDynDisabled)
			AppendMenu(hMenu, MF_STRING, 5, TranslateT("Upgrade"));

		switch (TrackPopupMenu(hMenu, TPM_RETURNCMD, pt.x, pt.y, 0, m_hwnd, nullptr)) {
		case 1:
			m_accList.InitRename();
			break;

		case 2:
			ClickButton(m_btnEdit);
			break;

		case 3:
			ClickButton(m_btnRemove);
			break;

		case 4:
			ClickButton(m_btnOptions);
			break;

		case 5:
			ClickButton(m_btnUpgrade);
			break;
		}
		DestroyMenu(hMenu);
	}

	void OnListSelChange(void*)
	{
		UpdateAccountInfo();
		SelectItem(m_accList.GetCurSel());
		::SetFocus(m_accList.GetHwnd());
	}

	void OnListDblClick(void*)
	{
		m_accList.InitRename();
	}

	void OnAccountCheck(int iItem)
	{
		PROTOACCOUNT *pa = (PROTOACCOUNT*)m_accList.GetItemData(iItem);
		if (pa == nullptr || pa->bOldProto || pa->bDynDisabled)
			return;

		pa->bIsEnabled = !pa->bIsEnabled;
		if (pa->bIsEnabled) {
			if (ActivateAccount(pa)) {
				pa->ppro->OnEvent(EV_PROTO_ONLOAD, 0, 0);
				if (!db_get_b(0, "CList", "MoveProtoMenus", TRUE))
					pa->ppro->OnEvent(EV_PROTO_ONMENU, 0, 0);
			}
		}
		else {
			DWORD dwStatus = CallProtoServiceInt(0, pa->szModuleName, PS_GETSTATUS, 0, 0);
			if (dwStatus >= ID_STATUS_ONLINE) {
				wchar_t buf[200];
				mir_snwprintf(buf, TranslateT("Account %s is being disabled"), pa->tszAccountName);
				if (IDNO == ::MessageBox(m_hwnd, TranslateT("Account is online. Disable account?"), buf, MB_ICONWARNING | MB_DEFBUTTON2 | MB_YESNO))
					pa->bIsEnabled = 1; // stay enabled
			}

			if (!pa->bIsEnabled)
				DeactivateAccount(pa, true, false);
		}

		WriteDbAccounts();
		NotifyEventHooks(hAccListChanged, PRAC_CHECKED, (LPARAM)pa);
		UpdateAccountInfo();
		RedrawWindow(m_accList.GetHwnd(), nullptr, nullptr, RDW_INVALIDATE);
	}

	void OnOk(CCtrlButton*)
	{
		PSHNOTIFY pshn;
		pshn.hdr.idFrom = 0;
		pshn.hdr.code = PSN_APPLY;
		pshn.hdr.hwndFrom = m_hwnd;
		SendMessage(m_hwnd, WM_NOTIFY, 0, (LPARAM)&pshn);
	}

	void OnCancel(CCtrlButton*)
	{
		PSHNOTIFY pshn;
		pshn.hdr.idFrom = 0;
		pshn.hdr.code = PSN_RESET;
		pshn.hdr.hwndFrom = m_hwnd;
		SendMessage(m_hwnd, WM_NOTIFY, 0, (LPARAM)&pshn);
	}

	void OnAdd(CCtrlButton*)
	{
		if (IDOK == �AccountFormDlg(this, PRAC_ADDED, nullptr).DoModal())
			SendMessage(m_hwnd, WM_MY_REFRESH, 0, 0);
	}

	void OnEdit(CCtrlButton*)
	{
		int idx = m_accList.GetCurSel();
		if (idx != -1)
			m_accList.InitRename();
	}

	void OnNetwork(CCtrlButton*)
	{
		PSHNOTIFY pshn;
		pshn.hdr.idFrom = 0;
		pshn.hdr.code = PSN_APPLY;
		pshn.hdr.hwndFrom = m_hwnd;
		SendMessage(m_hwnd, WM_NOTIFY, 0, (LPARAM)&pshn);

		Options_Open(L"Network");
	}

	void OnOptions(CCtrlButton*)
	{
		int idx = m_accList.GetCurSel();
		if (idx != -1) {
			PROTOACCOUNT *pa = (PROTOACCOUNT*)m_accList.GetItemData(idx);
			if (pa->bOldProto)
				Options_Open(L"Network", _A2T(pa->szModuleName));
			else
				OpenAccountOptions(pa);
		}
	}

	void OnRemove(CCtrlButton*)
	{
		int idx = m_accList.GetCurSel();
		if (idx == -1)
			return;

		PROTOACCOUNT *pa = (PROTOACCOUNT*)m_accList.GetItemData(idx);
		wchar_t buf[200];
		mir_snwprintf(buf, TranslateT("Account %s is being deleted"), pa->tszAccountName);
		if (pa->bOldProto) {
			MessageBox(m_hwnd, TranslateT("You need to disable plugin to delete this account"), buf, MB_ICONERROR | MB_OK);
			return;
		}
		if (IDYES == MessageBox(m_hwnd, errMsg, buf, MB_ICONWARNING | MB_DEFBUTTON2 | MB_YESNO)) {
			// lock controls to avoid changes during remove process
			m_accList.SetCurSel(-1);
			UpdateAccountInfo();
			m_accList.Disable();
			m_btnAdd.Disable();

			m_accList.SetItemData(idx, 0);

			accounts.remove(pa);

			CheckProtocolOrder();

			WriteDbAccounts();
			NotifyEventHooks(hAccListChanged, PRAC_REMOVED, (LPARAM)pa);

			UnloadAccount(pa, true, true);
			SendMessage(m_hwnd, WM_MY_REFRESH, 0, 0);

			m_accList.Enable();
			m_btnAdd.Enable();
		}
	}

	void OnUpgrade(CCtrlButton*)
	{
		int idx = m_accList.GetCurSel();
		if (idx != -1)
			�AccountFormDlg(this, PRAC_UPGRADED, (PROTOACCOUNT*)m_accList.GetItemData(idx)).DoModal();
	}

	virtual INT_PTR DlgProc(UINT msg, WPARAM wParam, LPARAM lParam) override
	{
		switch (msg) {
		case WM_MY_REFRESH:
			m_iSelected = -1;
			{
				int i = m_accList.GetCurSel();
				PROTOACCOUNT *acc = (i == LB_ERR) ? nullptr : (PROTOACCOUNT *)m_accList.GetItemData(i);

				m_accList.ResetContent();
				for (i = 0; i < accounts.getCount(); i++) {
					PROTOACCOUNT *p = accounts[i];
					PROTOCOLDESCRIPTOR *pd = Proto_IsProtocolLoaded(p->szProtoName);
					if (pd != nullptr && pd->type != PROTOTYPE_PROTOCOL)
						continue;

					int iItem = m_accList.AddString(p->tszAccountName);
					m_accList.SetItemData(iItem, (LPARAM)p);

					if (p == acc)
						m_accList.SetCurSel(iItem);
				}

				m_iSelected = m_accList.GetCurSel(); // -1 if error = > nothing selected in our case
				if (m_iSelected >= 0)
					SelectItem(m_iSelected);
				else if (acc && acc->hwndAccMgrUI)
					ShowWindow(acc->hwndAccMgrUI, SW_HIDE);
			}
			UpdateAccountInfo();
			break;

		case WM_COMMAND:
			switch (LOWORD(wParam)) {
			case IDC_ACCLIST:
				switch (HIWORD(wParam)) {
				case LBN_MY_RENAME:
					int iItem = m_accList.GetCurSel();
					PROTOACCOUNT *pa = (PROTOACCOUNT *)m_accList.GetItemData(iItem);
					if (pa) {
						mir_free(pa->tszAccountName);
						pa->tszAccountName = (wchar_t*)lParam;
						WriteDbAccounts();
						NotifyEventHooks(hAccListChanged, PRAC_CHANGED, (LPARAM)pa);

						m_accList.DeleteString(iItem);
						iItem = m_accList.AddString(pa->tszAccountName, (LPARAM)pa);
						m_accList.SetCurSel(iItem);

						SelectItem(iItem);
						RedrawWindow(m_accList.GetHwnd(), nullptr, nullptr, RDW_INVALIDATE);
					}
					else mir_free((wchar_t*)lParam);
				}
				break;

			case IDC_LNK_ADDONS:
				Utils_OpenUrl("https://miranda-ng.org/");
				break;
			}
			break;

		case PSM_CHANGED:
			int idx = m_accList.GetCurSel();
			if (idx != -1) {
				PROTOACCOUNT *pa = (PROTOACCOUNT *)m_accList.GetItemData(idx);
				if (pa) {
					pa->bAccMgrUIChanged = TRUE;
					SendMessage(GetParent(m_hwnd), PSM_CHANGED, 0, 0);
				}
			}
			break;
		}

		return CDlgBase::DlgProc(msg, wParam, lParam);
	}
};

/////////////////////////////////////////////////////////////////////////////////////////

BOOL CAccountListCtrl::OnDrawItem(DRAWITEMSTRUCT *lps)
{
	HBRUSH hbrBack;
	SIZE sz;

	int cxIcon = g_iIconSX;
	int cyIcon = g_iIconSY;

	PROTOACCOUNT *acc = (PROTOACCOUNT *)lps->itemData;
	if (lps->itemID == -1 || acc == nullptr)
		return FALSE;

	SetBkMode(lps->hDC, TRANSPARENT);
	if (lps->itemState & ODS_SELECTED) {
		hbrBack = GetSysColorBrush(COLOR_HIGHLIGHT);
		SetTextColor(lps->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
	}
	else {
		hbrBack = GetSysColorBrush(COLOR_WINDOW);
		SetTextColor(lps->hDC, GetSysColor(COLOR_WINDOWTEXT));
	}
	FillRect(lps->hDC, &lps->rcItem, hbrBack);

	lps->rcItem.left += 2;
	lps->rcItem.top += 2;
	lps->rcItem.bottom -= 2;

	int tmp;
	if (acc->bOldProto)
		tmp = SKINICON_OTHER_ON;
	else if (acc->bDynDisabled)
		tmp = SKINICON_OTHER_OFF;
	else
		tmp = acc->bIsEnabled ? SKINICON_OTHER_TICK : SKINICON_OTHER_NOTICK;

	HICON hIcon = Skin_LoadIcon(tmp);
	DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top, hIcon, cxIcon, cyIcon, 0, hbrBack, DI_NORMAL);
	IcoLib_ReleaseIcon(hIcon);

	lps->rcItem.left += cxIcon + 2;

	if (acc->ppro) {
		hIcon = IcoLib_GetIconByHandle(acc->ppro->m_hProtoIcon);
		DrawIconEx(lps->hDC, lps->rcItem.left, lps->rcItem.top, hIcon, cxIcon, cyIcon, 0, hbrBack, DI_NORMAL);
		IcoLib_ReleaseIcon(hIcon);
	}
	lps->rcItem.left += cxIcon + 2;
	{
		ptrW text(GetItemText(lps->itemID));
		size_t length = mir_wstrlen(text);

		SelectObject(lps->hDC, PARENT()->m_hfntTitle);
		tmp = lps->rcItem.bottom;
		lps->rcItem.bottom = lps->rcItem.top + max(cyIcon, PARENT()->m_titleHeight);
		DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS | DT_VCENTER);
		lps->rcItem.bottom = tmp;
		GetTextExtentPoint32(lps->hDC, text, (int)length, &sz);
		lps->rcItem.top += max(cxIcon, sz.cy) + 2;
	}

	if (lps->itemID == (unsigned)PARENT()->m_iSelected) {
		SelectObject(lps->hDC, PARENT()->m_hfntText);

		CMStringW text(FORMAT, L"%s: %S", TranslateT("Protocol"), acc->szProtoName);
		DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
		GetTextExtentPoint32(lps->hDC, text, text.GetLength(), &sz);
		lps->rcItem.top += sz.cy + 2;

		if (acc->ppro && Proto_IsProtocolLoaded(acc->szProtoName)) {
			char *szIdName = (char *)acc->ppro->GetCaps(PFLAG_UNIQUEIDTEXT, 0);
			ptrW tszIdName(szIdName ? mir_a2u(szIdName) : mir_wstrdup(TranslateT("Account ID")));
			ptrW tszUniqueID(Contact_GetInfo(CNF_UNIQUEID, 0, acc->szModuleName));
			if (tszUniqueID != nullptr)
				text.Format(L"%s: %s", tszIdName, tszUniqueID);
			else
				text.Format(L"%s: %s", tszIdName, TranslateT("<unknown>"));
		}
		else text.Format(TranslateT("Protocol is not loaded."));

		DrawText(lps->hDC, text, -1, &lps->rcItem, DT_LEFT | DT_NOPREFIX | DT_SINGLELINE | DT_END_ELLIPSIS);
		GetTextExtentPoint32(lps->hDC, text, text.GetLength(), &sz);
		lps->rcItem.top += sz.cy + 2;
	}

	return TRUE;
}

BOOL CAccountListCtrl::OnMeasureItem(MEASUREITEMSTRUCT *lps)
{
	PROTOACCOUNT *acc = (PROTOACCOUNT*)lps->itemData;
	if (acc == nullptr)
		return FALSE;

	lps->itemWidth = 10;
	lps->itemHeight = PARENT()->m_normalHeight;
	return TRUE;
}

LRESULT CAccountListCtrl::CustomWndProc(UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_LBUTTONDOWN:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			int iItem = LOWORD(SendMessage(m_hwnd, LB_ITEMFROMPOINT, 0, lParam));
			GetItemRect(iItem, &m_rcCheck);

			m_rcCheck.right = m_rcCheck.left + g_iIconSX + 4;
			m_rcCheck.bottom = m_rcCheck.top + g_iIconSY + 4;
			if (PtInRect(&m_rcCheck, pt))
				m_iItem = iItem;
			else
				m_iItem = -1;
		}
		break;

	case WM_LBUTTONUP:
		{
			POINT pt = { LOWORD(lParam), HIWORD(lParam) };
			if ((m_iItem >= 0) && PtInRect(&m_rcCheck, pt))
				PARENT()->OnAccountCheck(m_iItem);
			m_iItem = -1;
		}
		break;

	case WM_CHAR:
		if (wParam == ' ') {
			int iItem = GetCurSel();
			if (iItem >= 0)
				PARENT()->OnAccountCheck(iItem);
			return 0;
		}

		if (wParam == 10 /* enter */)
			return 0;

		break;

	case WM_GETDLGCODE:
		if (wParam == VK_RETURN)
			return DLGC_WANTMESSAGE;
		break;

	case WM_KEYDOWN:
		switch (wParam) {
		case VK_F2:
			InitRename();
			return 0;

		case VK_INSERT:
			PARENT()->ClickButton(PARENT()->m_btnAdd);
			return 0;

		case VK_DELETE:
			PARENT()->ClickButton(PARENT()->m_btnRemove);
			return 0;

		case VK_RETURN:
			if (GetAsyncKeyState(VK_CONTROL))
				PARENT()->ClickButton(PARENT()->m_btnEdit);
			else
				PARENT()->ClickButton(PARENT()->m_btnOk);
			return 0;
		}
		break;
	}

	return CCtrlListBox::CustomWndProc(msg, wParam, lParam);
}

/////////////////////////////////////////////////////////////////////////////////////////

static LRESULT CALLBACK sttEditSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg) {
	case WM_KEYDOWN:
		switch (wParam) {
		case VK_RETURN:
			DestroyWindow(hwnd);
			return 0;

		case VK_ESCAPE:
			SetWindowLongPtr(hwnd, GWLP_WNDPROC, GetWindowLongPtr(hwnd, GWLP_USERDATA));
			DestroyWindow(hwnd);
			return 0;
		}
		break;

	case WM_GETDLGCODE:
		if (wParam == VK_RETURN || wParam == VK_ESCAPE)
			return DLGC_WANTMESSAGE;
		break;

	case WM_KILLFOCUS:
		int length = GetWindowTextLength(hwnd) + 1;
		wchar_t *str = (wchar_t*)mir_alloc(sizeof(wchar_t) * length);
		GetWindowText(hwnd, str, length);
		SendMessage(GetParent(GetParent(hwnd)), WM_COMMAND, MAKEWPARAM(GetWindowLongPtr(GetParent(hwnd), GWL_ID), LBN_MY_RENAME), (LPARAM)str);
		DestroyWindow(hwnd);
		return 0;
	}
	return mir_callNextSubclass(hwnd, sttEditSubclassProc, msg, wParam, lParam);
}

void CAccountListCtrl::InitRename()
{
	PROTOACCOUNT *pa = (PROTOACCOUNT *)GetItemData(GetCurSel());
	if (!pa || pa->bOldProto || pa->bDynDisabled)
		return;

	RECT rc;
	GetItemRect(GetCurSel(), &rc);
	rc.left += 2 * g_iIconSX + 4;
	rc.bottom = rc.top + max(g_iIconSX, PARENT()->m_titleHeight) + 4 - 1;
	++rc.top; --rc.right;

	m_hwndEdit = ::CreateWindow(L"EDIT", pa->tszAccountName, WS_CHILD | WS_BORDER | ES_AUTOHSCROLL, rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, m_hwnd, nullptr, g_hInst, nullptr);
	mir_subclassWindow(m_hwndEdit, sttEditSubclassProc);
	SendMessage(m_hwndEdit, WM_SETFONT, (WPARAM)PARENT()->m_hfntTitle, 0);
	SendMessage(m_hwndEdit, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN | EC_USEFONTINFO, 0);
	SendMessage(m_hwndEdit, EM_SETSEL, 0, -1);
	ShowWindow(m_hwndEdit, SW_SHOW);
	SetFocus(m_hwndEdit);
}

/////////////////////////////////////////////////////////////////////////////////////////

static INT_PTR OptProtosShow(WPARAM, LPARAM)
{
	if (!pAccMgr) {
		pAccMgr = new CAccountManagerDlg();
		pAccMgr->Show();
	}
	else ShowWindow(pAccMgr->GetHwnd(), SW_RESTORE);

	SetForegroundWindow(pAccMgr->GetHwnd());
	SetActiveWindow(pAccMgr->GetHwnd());
	return 0;
}

int OptProtosLoaded(WPARAM, LPARAM)
{
	CMenuItem mi;
	SET_UID(mi, 0xb1f74008, 0x1fa6, 0x4e98, 0x95, 0x28, 0x5a, 0x7e, 0xab, 0xfe, 0x10, 0x61);
	mi.hIcolibItem = Skin_GetIconHandle(SKINICON_OTHER_ACCMGR);
	mi.position = 1900000000;
	mi.name.a = LPGEN("&Accounts...");
	mi.pszService = MS_PROTO_SHOWACCMGR;
	Menu_AddMainMenuItem(&mi);
	return 0;
}

static int OnAccListChanged(WPARAM eventCode, LPARAM lParam)
{
	PROTOACCOUNT *pa = (PROTOACCOUNT*)lParam;

	switch (eventCode) {
	case PRAC_CHANGED:
		if (pa->ppro) {
			replaceStrW(pa->ppro->m_tszUserName, pa->tszAccountName);
			pa->ppro->OnEvent(EV_PROTO_ONRENAME, 0, lParam);

			if (pa->ppro->m_hMainMenuItem)
				Menu_ModifyItem(pa->ppro->m_hMainMenuItem, pa->tszAccountName);
		}
	}

	return 0;
}

static int ShutdownAccMgr(WPARAM, LPARAM)
{
	delete pAccMgr; pAccMgr = nullptr;
	return 0;
}

int LoadProtoOptions(void)
{
	CreateServiceFunction(MS_PROTO_SHOWACCMGR, OptProtosShow);

	HookEvent(ME_SYSTEM_MODULESLOADED, OptProtosLoaded);
	HookEvent(ME_PROTO_ACCLISTCHANGED, OnAccListChanged);
	HookEvent(ME_SYSTEM_PRESHUTDOWN, ShutdownAccMgr);
	return 0;
}
