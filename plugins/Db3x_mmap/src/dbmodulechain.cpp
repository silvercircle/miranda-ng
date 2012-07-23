/*

Miranda NG: the free IM client for Microsoft* Windows*

Copyright 2012 Miranda NG project,
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

#include "commonheaders.h"

void CDb3Base::AddToList(char *name, DWORD len, DWORD ofs)
{
	ModuleName *mn = (ModuleName*)HeapAlloc(m_hModHeap,0,sizeof(ModuleName));
	mn->name = name;
	mn->ofs = ofs;

	if (m_lMods.getIndex(mn) != -1)
		DatabaseCorruption( _T("%s (Module Name not unique)"));
	m_lMods.insert(mn);

	if (m_lOfs.getIndex(mn) != -1)
		DatabaseCorruption( _T("%s (Module Offset not unique)"));
	m_lOfs.insert(mn);
}

int CDb3Base::InitModuleNames(void)
{
	DWORD ofsThis = m_dbHeader.ofsFirstModuleName;
	DBModuleName *dbmn = (struct DBModuleName*)DBRead(ofsThis,sizeof(struct DBModuleName),NULL);
	while (ofsThis) {
		if (dbmn->signature != DBMODULENAME_SIGNATURE)
			DatabaseCorruption(NULL);

		int nameLen = dbmn->cbName;

		char *mod = (char*)HeapAlloc(m_hModHeap,0,nameLen+1);
		CopyMemory(mod,DBRead(ofsThis + offsetof(struct DBModuleName,name),nameLen,NULL),nameLen);
		mod[nameLen] = 0;

		AddToList(mod, nameLen, ofsThis);

		ofsThis = dbmn->ofsNext;
		dbmn = (struct DBModuleName*)DBRead(ofsThis,sizeof(struct DBModuleName),NULL);
	}
	return 0;
}

DWORD CDb3Base::FindExistingModuleNameOfs(const char *szName)
{
	ModuleName mn = { (char*)szName, 0 };
	if (m_lastmn && !strcmp(mn.name, m_lastmn->name))
		return m_lastmn->ofs;

	int index = m_lMods.getIndex(&mn);
	if (index != -1) {
		ModuleName *pmn = m_lMods[index];
		m_lastmn = pmn;
		return pmn->ofs;
	}

	return 0;
}

//will create the offset if it needs to
DWORD CDb3Base::GetModuleNameOfs(const char *szName)
{
	struct DBModuleName dbmn;
	int nameLen;
	DWORD ofsNew,ofsExisting;
	char *mod;

	ofsExisting = FindExistingModuleNameOfs(szName);
	if (ofsExisting) return ofsExisting;

	nameLen = (int)strlen(szName);

	//need to create the module name
	ofsNew = CreateNewSpace(nameLen+offsetof(struct DBModuleName,name));
	dbmn.signature = DBMODULENAME_SIGNATURE;
	dbmn.cbName = nameLen;
	dbmn.ofsNext = m_dbHeader.ofsFirstModuleName;
	m_dbHeader.ofsFirstModuleName = ofsNew;
	DBWrite(0,&m_dbHeader,sizeof(m_dbHeader));
	DBWrite(ofsNew,&dbmn,offsetof(struct DBModuleName,name));
	DBWrite(ofsNew+offsetof(struct DBModuleName,name),(PVOID)szName,nameLen);
	DBFlush(0);

	//add to cache
	mod = (char*)HeapAlloc(m_hModHeap,0,nameLen+1);
	strcpy(mod,szName);
	AddToList(mod, nameLen, ofsNew);

	//quit
	return ofsNew;
}

char* CDb3Base::GetModuleNameByOfs(DWORD ofs)
{
	if (m_lastmn && m_lastmn->ofs == ofs)
		return m_lastmn->name;

	ModuleName mn = {NULL, ofs};
	int index = m_lOfs.getIndex(&mn);
	if (index != -1) {
		ModuleName *pmn = m_lOfs[index];
		m_lastmn = pmn;
		return pmn->name;
	}

	DatabaseCorruption(NULL);
	return NULL;
}

STDMETHODIMP_(BOOL) CDb3Base::EnumModuleNames(DBMODULEENUMPROC pFunc, void *pParam)
{
	for (int i = 0; i < m_lMods.getCount(); i++) {
		ModuleName *pmn = m_lMods[i];
		int ret = pFunc(pmn->name, pmn->ofs, (LPARAM)pParam);
		if (ret)
			return ret;
	}
	return 0;
}
