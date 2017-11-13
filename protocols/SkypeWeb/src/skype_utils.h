/*
Copyright (c) 2015-17 Miranda NG project (https://miranda-ng.org)

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

#ifndef _UTILS_H_
#define _UTILS_H_

class EventHandle
{
	HANDLE _hEvent;
public:
	__inline explicit EventHandle() { _hEvent = CreateEvent(NULL, 0, 0, NULL); }
	__inline explicit EventHandle(HANDLE hEvent) : _hEvent(hEvent) {}
	__inline ~EventHandle() { CloseHandle(_hEvent); }
	__inline void Set() { SetEvent(_hEvent); }
	__inline void Wait(DWORD dwMilliseconds = INFINITE) { WaitForSingleObject(_hEvent, dwMilliseconds); }
	__inline operator HANDLE() { return _hEvent; }
};

struct CFileUploadParam : public MZeroedObject {
	ptrW tszFileName;
	ptrW tszDesc;
	ptrA atr;
	ptrA fname;
	ptrA uid;
	size_t size;
	MCONTACT hContact;

	__forceinline CFileUploadParam(MCONTACT _hContact, const wchar_t* _desc, wchar_t** _files) : hContact(_hContact), tszDesc(mir_wstrdup(_desc)), tszFileName(mir_wstrdup(_files[0])) {};
	__forceinline bool IsAccess() { return ::_waccess(tszFileName, 0) == 0; }
};


#endif //_UTILS_H_