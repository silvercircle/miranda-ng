/*
Copyright (c) 2013-17 Miranda NG project (https://miranda-ng.org)

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

#include "stdafx.h"

//////////////////////////////////////////////////////////////////////////////

int CVkProto::RecvMsg(MCONTACT hContact, PROTORECVEVENT *pre)
{
	debugLogA("CVkProto::RecvMsg");
	Proto_RecvMessage(hContact, pre);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////

void CVkProto::SendMsgAck(void *param)
{
	debugLogA("CVkProto::SendMsgAck");
	CVkSendMsgParam *ack = (CVkSendMsgParam *)param;
	Sleep(100);
	ProtoBroadcastAck(ack->hContact, ACKTYPE_MESSAGE, ACKRESULT_SUCCESS, (HANDLE)ack->iMsgID);
	delete ack;
}

int CVkProto::SendMsg(MCONTACT hContact, int, const char *szMsg)
{
	debugLogA("CVkProto::SendMsg");
	if (!IsOnline())
		return 0;

	bool bIsChat = isChatRoom(hContact);
	LONG iUserID = getDword(hContact, bIsChat ? "vk_chat_id" : "ID", VK_INVALID_USER);

	if (iUserID == VK_INVALID_USER || iUserID == VK_FEED_USER) {
		ForkThread(&CVkProto::SendMsgAck, new CVkSendMsgParam(hContact));
		return 0;
	}

	int StickerId = 0;
	ptrA pszRetMsg(GetStickerId(szMsg, StickerId));

	ULONG uMsgId = ::InterlockedIncrement(&m_msgId);
	AsyncHttpRequest *pReq = new AsyncHttpRequest(this, REQUEST_POST, "/method/messages.send.json", true,
		bIsChat ? &CVkProto::OnSendChatMsg : &CVkProto::OnSendMessage, AsyncHttpRequest::rpHigh)
		<< INT_PARAM(bIsChat ? "chat_id" : "peer_id", iUserID)
		<< INT_PARAM("random_id", ((LONG)time(nullptr)) * 100 + uMsgId % 100);
	pReq->AddHeader("Content-Type", "application/x-www-form-urlencoded");

	if (StickerId)
		pReq << INT_PARAM("sticker_id", StickerId);
	else {
		pReq << CHAR_PARAM("message", szMsg);
		if (m_vkOptions.bSendVKLinksAsAttachments) {
			CMStringA szAttachments = GetAttachmentsFromMessage(szMsg);
			if (!szAttachments.IsEmpty()) {
				debugLogA("CVkProto::SendMsg Attachments = %s", szAttachments.c_str());
				pReq << CHAR_PARAM("attachment", szAttachments);
			}
		}
	}

	if (!bIsChat)
		pReq->pUserInfo = new CVkSendMsgParam(hContact, uMsgId);

	Push(pReq);

	if (!m_vkOptions.bServerDelivery && !bIsChat)
		ForkThread(&CVkProto::SendMsgAck, new CVkSendMsgParam(hContact, uMsgId));

	if (!IsEmpty(pszRetMsg))
		SendMsg(hContact, 0, pszRetMsg);
	else if (m_iStatus == ID_STATUS_INVISIBLE)
		Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/account.setOffline.json", true, &CVkProto::OnReceiveSmth));

	return uMsgId;
}

void CVkProto::OnSendMessage(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	int iResult = ACKRESULT_FAILED;
	if (pReq->pUserInfo == nullptr) {
		debugLogA("CVkProto::OnSendMessage failed! (pUserInfo == nullptr)");
		return;
	}
	CVkSendMsgParam *param = (CVkSendMsgParam *)pReq->pUserInfo;

	debugLogA("CVkProto::OnSendMessage %d", reply->resultCode);
	if (reply->resultCode == 200) {
		JSONNode jnRoot;
		const JSONNode &jnResponse = CheckJsonResponse(pReq, reply, jnRoot);
		if (jnResponse) {
			debugLogA("CVkProto::OnSendMessage jnResponse %d", jnResponse.as_int());
			UINT mid;
			switch (jnResponse.type()) {
			case JSON_NUMBER:
				mid = jnResponse.as_int();
				break;
			case JSON_STRING:
				if (swscanf(jnResponse.as_mstring(), L"%d", &mid) != 1)
					mid = 0;
				break;
			case JSON_ARRAY:
				mid = jnResponse.as_array()[json_index_t(0)].as_int();
				break;
			default:
				mid = 0;
			}

			if (param->iMsgID != -1)
				m_sendIds.insert((HANDLE)mid);

			if (mid > getDword(param->hContact, "lastmsgid"))
				setDword(param->hContact, "lastmsgid", mid);

			if (m_vkOptions.iMarkMessageReadOn >= MarkMsgReadOn::markOnReply)
				MarkMessagesRead(param->hContact);

			iResult = ACKRESULT_SUCCESS;
		}
	}

	if (param->pFUP) {
		ProtoBroadcastAck(param->hContact, ACKTYPE_FILE, iResult, (HANDLE)(param->pFUP));
		if (!pReq->bNeedsRestart || m_bTerminated)
			delete param->pFUP;
	}
	else if (m_vkOptions.bServerDelivery)
		ProtoBroadcastAck(param->hContact, ACKTYPE_MESSAGE, iResult, (HANDLE)(param->iMsgID));

	if (!pReq->bNeedsRestart || m_bTerminated) {
		delete param;
		pReq->pUserInfo = nullptr;
	}
}

/////////////////////////////////////////////////////////////////////////////////////////

int CVkProto::OnDbEventRead(WPARAM, LPARAM hDbEvent)
{
	debugLogA("CVkProto::OnDbEventRead");
	MCONTACT hContact = db_event_getContact(hDbEvent);
	if (!hContact)
		return 0;

	CMStringA szProto(GetContactProto(hContact));
	if (szProto.IsEmpty() || szProto != m_szModuleName)
		return 0;

	if (m_vkOptions.iMarkMessageReadOn == MarkMsgReadOn::markOnRead)
		MarkMessagesRead(hContact);
	return 0;
}

INT_PTR CVkProto::SvcMarkMessagesAsRead(WPARAM hContact, LPARAM)
{
	MarkDialogAsRead(hContact);
	MarkMessagesRead(hContact);
	return 0;
}

void CVkProto::MarkMessagesRead(const CMStringA &mids)
{
	debugLogA("CVkProto::MarkMessagesRead (mids)");
	if (!IsOnline() || mids.IsEmpty())
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.markAsRead.json", true, &CVkProto::OnReceiveSmth, AsyncHttpRequest::rpLow)
		<< CHAR_PARAM("message_ids", mids));
}

void CVkProto::MarkMessagesRead(const MCONTACT hContact)
{
	debugLogA("CVkProto::MarkMessagesRead (hContact)");
	if (!IsOnline() || !hContact)
		return;

	if (!IsEmpty(ptrW(db_get_wsa(hContact, m_szModuleName, "Deactivated"))))
		return;

	LONG userID = getDword(hContact, "ID", VK_INVALID_USER);
	if (userID == VK_INVALID_USER || userID == VK_FEED_USER)
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/messages.markAsRead.json", true, &CVkProto::OnReceiveSmth, AsyncHttpRequest::rpLow)
		<< INT_PARAM("start_message_id", 0)
		<< INT_PARAM("peer_id", userID));
}

void CVkProto::RetrieveMessagesByIds(const CMStringA &mids)
{
	debugLogA("CVkProto::RetrieveMessagesByIds");
	if (!IsOnline() || mids.IsEmpty())
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/execute.RetrieveMessagesByIds", true, &CVkProto::OnReceiveMessages, AsyncHttpRequest::rpHigh)
		<< CHAR_PARAM("mids", mids)
	);
}

void CVkProto::RetrieveUnreadMessages()
{
	debugLogA("CVkProto::RetrieveUnreadMessages");
	if (!IsOnline())
		return;

	Push(new AsyncHttpRequest(this, REQUEST_GET, "/method/execute.RetrieveUnreadMessages", true, &CVkProto::OnReceiveDlgs, AsyncHttpRequest::rpHigh));
}

void CVkProto::OnReceiveMessages(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveMessages %d", reply->resultCode);
	if (reply->resultCode != 200)
		return;

	JSONNode jnRoot;
	const JSONNode &jnResponse = CheckJsonResponse(pReq, reply, jnRoot);
	if (!jnResponse)
		return;
	if (!jnResponse["Msgs"])
		return;

	CMStringA mids;
	int numMessages = jnResponse["Msgs"]["count"].as_int();
	const JSONNode &jnMsgs = jnResponse["Msgs"]["items"];
	const JSONNode &jnFUsers = jnResponse["fwd_users"];

	debugLogA("CVkProto::OnReceiveMessages numMessages = %d", numMessages);

	for (auto it = jnMsgs.begin(); it != jnMsgs.end(); ++it) {
		const JSONNode &jnMsg = (*it);
		if (!jnMsg) {
			debugLogA("CVkProto::OnReceiveMessages pMsg == nullptr");
			break;
		}

		UINT mid = jnMsg["id"].as_int();
		CMStringW wszBody(jnMsg["body"].as_mstring());
		int datetime = jnMsg["date"].as_int();
		int isOut = jnMsg["out"].as_int();
		int isRead = jnMsg["read_state"].as_int();
		int uid = jnMsg["user_id"].as_int();

		const JSONNode &jnFwdMessages = jnMsg["fwd_messages"];
		if (jnFwdMessages) {
			CMStringW wszFwdMessages = GetFwdMessages(jnFwdMessages, jnFUsers, m_vkOptions.BBCForAttachments());
			if (!wszBody.IsEmpty())
				wszFwdMessages = L"\n" + wszFwdMessages;
			wszBody += wszFwdMessages;
		}

		CMStringW wszAttachmentDescr;
		const JSONNode &jnAttachments = jnMsg["attachments"];
		if (jnAttachments) {
			wszAttachmentDescr = GetAttachmentDescr(jnAttachments, m_vkOptions.BBCForAttachments());
			if (!wszBody.IsEmpty())
				wszBody += L"\n";
			wszBody += wszAttachmentDescr;
		}

		if (m_vkOptions.bAddMessageLinkToMesWAtt && (jnAttachments || jnFwdMessages))
			wszBody += SetBBCString(TranslateT("Message link"), m_vkOptions.BBCForAttachments(), vkbbcUrl,
				CMStringW(FORMAT, L"https://vk.com/im?sel=%d&msgid=%d", uid, mid));

		MCONTACT hContact = 0;
		int chat_id = jnMsg["chat_id"].as_int();
		if (chat_id == 0)
			hContact = FindUser(uid, true);

		char szMid[40];
		_itoa(mid, szMid, 10);
		if (m_vkOptions.iMarkMessageReadOn == MarkMsgReadOn::markOnReceive || chat_id != 0) {
			if (!mids.IsEmpty())
				mids.AppendChar(',');
			mids.Append(szMid);
		}

		if (chat_id != 0) {
			debugLogA("CVkProto::OnReceiveMessages chat_id != 0");
			CMStringW action_chat = jnMsg["action"].as_mstring();
			int action_mid = _wtoi(jnMsg["action_mid"].as_mstring());
			if ((action_chat == L"chat_kick_user") && (action_mid == m_myUserId))
				KickFromChat(chat_id, uid, jnMsg, jnFUsers);
			else {
				MCONTACT chatContact = FindChat(chat_id);
				if (chatContact && getBool(chatContact, "kicked", true))
					db_unset(chatContact, m_szModuleName, "kicked");
				AppendChatMessage(chat_id, jnMsg, jnFUsers, false);
			}
			continue;
		}

		PROTORECVEVENT recv = { 0 };
		bool bUseServerReadFlag = m_vkOptions.bSyncReadMessageStatusFromServer ? true : !m_vkOptions.bMesAsUnread;
		if (isRead && bUseServerReadFlag)
			recv.flags |= PREF_CREATEREAD;
		if (isOut)
			recv.flags |= PREF_SENT;
		else if (m_vkOptions.bUserForceInvisibleOnActivity && time(nullptr) - datetime < 60 * m_vkOptions.iInvisibleInterval)
			SetInvisible(hContact);

		T2Utf pszBody(wszBody);
		recv.timestamp = m_vkOptions.bUseLocalTime ? time(nullptr) : datetime;
		recv.szMessage = pszBody;
		recv.lParam = isOut;
		recv.pCustomData = szMid;
		recv.cbCustomDataSize = (int)mir_strlen(szMid);
		Sleep(100);

		debugLogA("CVkProto::OnReceiveMessages mid = %d, datetime = %d, isOut = %d, isRead = %d, uid = %d", mid, datetime, isOut, isRead, uid);

		if (!CheckMid(m_sendIds, mid)) {
			debugLogA("CVkProto::OnReceiveMessages ProtoChainRecvMsg");
			ProtoChainRecvMsg(hContact, &recv);
			if (mid > getDword(hContact, "lastmsgid", -1))
				setDword(hContact, "lastmsgid", mid);
			if (!isOut)
				m_incIds.insert((HANDLE)mid);
		}
		else if (m_vkOptions.bLoadSentAttachments && !wszAttachmentDescr.IsEmpty() && isOut) {
			T2Utf pszAttach(wszAttachmentDescr);
			recv.timestamp = time(nullptr); // only local time
			recv.szMessage = pszAttach;
			ProtoChainRecvMsg(hContact, &recv);
		}
	}

	if (!mids.IsEmpty())
		MarkMessagesRead(mids);
}

void CVkProto::OnReceiveDlgs(NETLIBHTTPREQUEST *reply, AsyncHttpRequest *pReq)
{
	debugLogA("CVkProto::OnReceiveDlgs %d", reply->resultCode);
	if (reply->resultCode != 200)
		return;

	JSONNode jnRoot;
	const JSONNode &jnResponse = CheckJsonResponse(pReq, reply, jnRoot);
	if (!jnResponse)
		return;

	const JSONNode &jnDialogs = jnResponse["dialogs"];
	if (!jnDialogs)
		return;

	const JSONNode &jnDlgs = jnDialogs["items"];
	if (!jnDlgs)
		return;

	LIST<void> lufUsers(20, PtrKeySortT);
	const JSONNode &jnUsers = jnResponse["users"];
	if (jnUsers)
		for (auto it = jnUsers.begin(); it != jnUsers.end(); ++it) {
			int iUserId = (*it)["user_id"].as_int();
			int iStatus = (*it)["friend_status"].as_int();

			// iStatus == 3 - user is friend
			// uid < 0 - user is group
			if (iUserId < 0 || iStatus != 3 || lufUsers.indexOf((HANDLE)iUserId) != -1)
				continue;

			lufUsers.insert((HANDLE)iUserId);
		}

	const JSONNode &jnGroups = jnResponse["groups"];
	if (jnGroups)
		for (auto it = jnGroups.begin(); it != jnGroups.end(); ++it) {
			int iUserId = 1000000000 + (*it).as_int();

			if (lufUsers.indexOf((HANDLE)iUserId) != -1)
				continue;

			lufUsers.insert((HANDLE)iUserId);
		}

	CMStringA szGroupIds;

	for (auto it = jnDlgs.begin(); it != jnDlgs.end(); ++it) {
		if (!(*it))
			break;
		int numUnread = (*it)["unread"].as_int();
		const JSONNode &jnDlg = (*it)["message"];
		if (!jnDlg)
			break;

		int uid = 0;
		MCONTACT hContact(0);

		int chatid = jnDlg["chat_id"].as_int();

		if (!chatid) {
			uid = jnDlg["user_id"].as_int();
			int iSearchId = (uid < 0) ? 1000000000 - uid : uid;
			int iIndex = lufUsers.indexOf((HANDLE)iSearchId);
			debugLogA("CVkProto::OnReceiveDlgs UserId = %d, iIndex = %d, numUnread = %d", uid, iIndex, numUnread);

			if (m_vkOptions.bLoadOnlyFriends && numUnread == 0 && iIndex == -1)
				continue;

			hContact = FindUser(uid, true);
			debugLogA("CVkProto::OnReceiveDlgs add UserId = %d", uid);

			if (IsGroupUser(hContact))
				szGroupIds.AppendFormat(szGroupIds.IsEmpty() ? "%d" : ",%d", -1 * uid);

			if (ServiceExists(MS_MESSAGESTATE_UPDATE)) {
				time_t tLastReadMessageTime = jnDlg["date"].as_int();
				bool isOut = jnDlg["out"].as_bool();
				bool isRead = jnDlg["read_state"].as_bool();

				if (isRead && isOut) {
					MessageReadData data(tLastReadMessageTime, MRD_TYPE_MESSAGETIME);
					CallService(MS_MESSAGESTATE_UPDATE, hContact, (LPARAM)&data);
				}
			}
		}

		if (chatid) {
			debugLogA("CVkProto::OnReceiveDlgs chatid = %d", chatid);
			if (m_chats.find((CVkChatInfo*)&chatid) == nullptr)
				AppendChat(chatid, jnDlg);
		}
		else if (m_vkOptions.iSyncHistoryMetod) {
			int mid = jnDlg["id"].as_int();
			m_bNotifyForEndLoadingHistory = false;

			if (getDword(hContact, "lastmsgid", -1) == -1 && numUnread && !getBool(hContact, "ActiveHistoryTask")) {
				setByte(hContact, "ActiveHistoryTask", 1);
				GetServerHistory(hContact, 0, numUnread, 0, 0, true);
			}
			else
				GetHistoryDlg(hContact, mid);

			if (m_vkOptions.iMarkMessageReadOn == MarkMsgReadOn::markOnReceive && numUnread)
				MarkMessagesRead(hContact);
		}
		else if (numUnread && !getBool(hContact, "ActiveHistoryTask")) {

			m_bNotifyForEndLoadingHistory = false;
			setByte(hContact, "ActiveHistoryTask", 1);
			GetServerHistory(hContact, 0, numUnread, 0, 0, true);

			if (m_vkOptions.iMarkMessageReadOn == MarkMsgReadOn::markOnReceive)
				MarkMessagesRead(hContact);
		}
	}

	lufUsers.destroy();
	RetrieveUsersInfo();
	RetrieveGroupInfo(szGroupIds);
}