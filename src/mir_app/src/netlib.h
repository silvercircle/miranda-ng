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

#define NLH_INVALID      0
#define NLH_USER         'USER'
#define NLH_CONNECTION   'CONN'
#define NLH_BOUNDPORT    'BIND'
#define NLH_PACKETRECVER 'PCKT'
int GetNetlibHandleType(void*);

#define NLHRF_SMARTREMOVEHOST	0x00000004	 // for internal purposes only

extern struct SSL_API sslApi;

struct NetlibUser
{
	int handleType;
	NETLIBUSER user;
	NETLIBUSERSETTINGS settings;
	char *szStickyHeaders;
	int toLog;
	int inportnum;
	int outportnum;
};

struct NetlibNestedCriticalSection
{
	HANDLE hMutex;
	DWORD dwOwningThreadId;
	int lockCount;
};

struct NetlibHTTPProxyPacketQueue
{
	NetlibHTTPProxyPacketQueue *next;
	PBYTE dataBuffer;
	int dataBufferLen;
};

union SOCKADDR_INET_M
{
	SOCKADDR_IN Ipv4;
	SOCKADDR_IN6 Ipv6;
	USHORT si_family;
};

struct NetlibConnection : public MZeroedObject
{
	NetlibConnection();
	~NetlibConnection();

	int handleType;
	SOCKET s, s2;
	bool usingHttpGateway;
	bool usingDirectHttpGateway;
	bool proxyAuthNeeded;
	bool dnsThroughProxy;
	bool termRequested;
	
	NetlibUser *nlu;
	NETLIBOPENCONNECTION nloc;

	char *szNewUrl;
	
	mir_cs csHttpSequenceNums;
	HANDLE hOkToCloseEvent;
	LONG dontCloseNow;
	NetlibNestedCriticalSection ncsSend, ncsRecv;

	// SSL support
	HSSL hSsl;
	MBinBuffer foreBuf;

	// proxy support
	NETLIBHTTPPROXYINFO nlhpi;
	NetlibHTTPProxyPacketQueue *pHttpProxyPacketQueue;
	int proxyType;
	char *szProxyServer;
	WORD wProxyPort;
	CMStringA szProxyBuf;

	int pollingTimeout;
	unsigned lastPost;
};

struct NetlibBoundPort : public MZeroedObject
{
	NetlibBoundPort(HNETLIBUSER nlu, NETLIBBIND *nlb);
	~NetlibBoundPort() {
		close();
	}

	void close();

	int handleType;
	SOCKET s;
	SOCKET s6;
	WORD wPort;
	WORD wExPort;
	NetlibUser *nlu;
	NETLIBNEWCONNECTIONPROC_V2 pfnNewConnectionV2;
	HANDLE hThread;
	void *pExtra;
};

struct NetlibPacketRecver
{
	int handleType;
	NetlibConnection *nlc;
	NETLIBPACKETRECVER packetRecver;
};

//netlib.c
void NetlibFreeUserSettingsStruct(NETLIBUSERSETTINGS *settings);
void NetlibDoCloseSocket(NetlibConnection *nlc, bool noShutdown = false);
void NetlibInitializeNestedCS(NetlibNestedCriticalSection *nlncs);
void NetlibDeleteNestedCS(NetlibNestedCriticalSection *nlncs);
#define NLNCS_SEND  0
#define NLNCS_RECV  1
int NetlibEnterNestedCS(NetlibConnection *nlc, int which);
void NetlibLeaveNestedCS(NetlibNestedCriticalSection *nlncs);

extern mir_cs csNetlibUser;
extern LIST<NetlibUser> netlibUser;

// netlibautoproxy.c
void NetlibLoadIeProxy(void);
void NetlibUnloadIeProxy(void);
char* NetlibGetIeProxy(char *szUrl);
bool NetlibGetIeProxyConn(NetlibConnection *nlc, bool forceHttps);

// netlibbind.c
int NetlibFreeBoundPort(NetlibBoundPort *nlbp);
bool BindSocketToPort(const char *szPorts, SOCKET s, SOCKET s6, int* portn);

// netlibhttp.c
void NetlibHttpSetLastErrorUsingHttpResult(int result);
NETLIBHTTPREQUEST* NetlibHttpRecv(NetlibConnection* nlc, DWORD hflags, DWORD dflags, bool isConnect = false);
void NetlibConnFromUrl(const char* szUrl, bool secur, NETLIBOPENCONNECTION &nloc);

// netlibhttpproxy.c
int NetlibInitHttpConnection(NetlibConnection *nlc, NetlibUser *nlu, NETLIBOPENCONNECTION *nloc);
int NetlibHttpGatewayRecv(NetlibConnection *nlc, char *buf, int len, int flags);
int NetlibHttpGatewayPost(NetlibConnection *nlc, const char *buf, int len, int flags);
void HttpGatewayRemovePacket(NetlibConnection *nlc, int pck);

// netliblog.c
void NetlibLogShowOptions(void);
void NetlibDumpData(NetlibConnection *nlc, PBYTE buf, int len, int sent, int flags);
void NetlibLogInit(void);
void NetlibLogShutdown(void);

// netlibopenconn.c
DWORD DnsLookup(NetlibUser *nlu, const char *szHost);
int WaitUntilReadable(SOCKET s, DWORD dwTimeout, bool check = false);
int WaitUntilWritable(SOCKET s, DWORD dwTimeout);
bool NetlibDoConnect(NetlibConnection *nlc);
bool NetlibReconnect(NetlibConnection *nlc);

// netlibopts.c
int NetlibOptInitialise(WPARAM wParam, LPARAM lParam);
void NetlibSaveUserSettingsStruct(const char *szSettingsModule, const NETLIBUSERSETTINGS *settings);

// netlibsock.c
#define NL_SELECT_READ  0x0001
#define NL_SELECT_WRITE 0x0002
#define NL_SELECT_ALL   (NL_SELECT_READ+NL_SELECT_WRITE)

// netlibupnp.c
bool NetlibUPnPAddPortMapping(WORD intport, char *proto, WORD *extport, DWORD *extip, bool search);
void NetlibUPnPDeletePortMapping(WORD extport, char* proto);
void NetlibUPnPCleanup(void*);
void NetlibUPnPInit(void);
void NetlibUPnPDestroy(void);

// netlibsecurity.c
char* NtlmCreateResponseFromChallenge(HANDLE hSecurity, const char *szChallenge, const wchar_t* login, const wchar_t* psw, bool http, unsigned& complete);
