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

/////////////////////////////////////////////////////////////////////////////////////////
// CMBaseString

class CNilMStringData : public CMStringData
{
public:
	CNilMStringData();

public:
	wchar_t achNil[2];
};

CNilMStringData::CNilMStringData()
{
	nRefs = 2;  // Never gets freed
	nDataLength = 0;
	nAllocLength = 0;
	achNil[0] = 0;
	achNil[1] = 0;
}

static CNilMStringData *m_nil = NULL;

/////////////////////////////////////////////////////////////////////////////////////////
// CMBaseString

MIR_CORE_DLL(CMStringData*) mirstr_allocate(int nChars, int nCharSize)
{
	nChars++; // nil char
	size_t nDataBytes = nCharSize * nChars;
	size_t nTotalSize = nDataBytes + sizeof(CMStringData);

	CMStringData *pData = static_cast<CMStringData*>(malloc(nTotalSize));
	if (pData == NULL)
		return NULL;

	pData->nRefs = 1;
	pData->nAllocLength = nChars - 1;
	pData->nDataLength = 0;
	return pData;
}

MIR_CORE_DLL(void) mirstr_free(CMStringData *pData)
{
	free(pData);
}

MIR_CORE_DLL(CMStringData*) mirstr_realloc(CMStringData* pData, int nChars, int nCharSize)
{
	nChars++; // nil char
	ULONG nDataBytes = nCharSize * nChars;
	ULONG nTotalSize = nDataBytes + sizeof(CMStringData);

	CMStringData *pNewData = static_cast<CMStringData*>(realloc(pData, nTotalSize));
	if (pNewData == NULL)
		return NULL;

	pNewData->nAllocLength = nChars - 1;
	return pNewData;
}

MIR_CORE_DLL(CMStringData*) mirstr_getNil()
{
	if (m_nil == NULL)
		m_nil = new CNilMStringData();
	m_nil->AddRef();
	return m_nil;
}

/////////////////////////////////////////////////////////////////////////////////////////
// CMStringData

MIR_CORE_DLL(void) mirstr_lock(CMStringData* pThis)
{
	pThis->nRefs--;  // Locked buffers can't be shared, so no interlocked operation necessary
	if (pThis->nRefs == 0)
		pThis->nRefs = -1;
}

MIR_CORE_DLL(void) mirstr_release(CMStringData* pThis)
{
	if (InterlockedDecrement(&pThis->nRefs) <= 0)
		mirstr_free(pThis);
}

MIR_CORE_DLL(void) mirstr_unlock(CMStringData* pThis)
{
	if (pThis->IsLocked())
	{
		pThis->nRefs++;  // Locked buffers can't be shared, so no interlocked operation necessary
		if (pThis->nRefs == 0)
			pThis->nRefs = 1;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////
// don't remove it
// this code just instantiates templates for CMStringW[A/W]

template CMStringW;
template MIR_CORE_EXPORT CMStringW CALLBACK operator + (const CMStringW& str1, const CMStringW& str2);
template MIR_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW& str1, const wchar_t *psz2);
template MIR_CORE_EXPORT CMStringW CALLBACK operator+(const wchar_t *psz1, const CMStringW& str2);
template MIR_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW& str1, wchar_t ch2);
template MIR_CORE_EXPORT CMStringW CALLBACK operator+(const CMStringW& str1, char ch2);
template MIR_CORE_EXPORT CMStringW CALLBACK operator+(wchar_t ch1, const CMStringW& str2);
template MIR_CORE_EXPORT CMStringW CALLBACK operator+(char ch1, const CMStringW& str2);

template CMStringA;
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, const CMStringA& str2);
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, const char *psz2);
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(const char *psz1, const CMStringA& str2);
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, wchar_t ch2);
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(const CMStringA& str1, char ch2);
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(wchar_t ch1, const CMStringA& str2);
template MIR_CORE_EXPORT CMStringA CALLBACK operator+(char ch1, const CMStringA& str2);
