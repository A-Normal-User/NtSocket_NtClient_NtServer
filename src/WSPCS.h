#pragma once
#include "WSPSocket.h"
#include <vector>
#pragma comment (lib, "ws2_32.lib")

typedef VOID (*WSPServerCallBack)(SOCKET s, int Event);

class WSPClient
{
private:
	CRITICAL_SECTION m_CriticalSection;
public:
	WSPClient() {
		memset(&m_CriticalSection, 0, sizeof(m_CriticalSection));
	}
	~WSPClient();
	SOCKET m_socket = NULL;
	bool m_EnableIPv6 = false;

public:
	SOCKET InitialSocket(bool EnableIPv6 = false);
	VOID DeleteSocket();
	NTSTATUS Connect(
		char* ConnectAddr,
		USHORT Port
	);
	NTSTATUS ShutDown(
		int HowTo
	);
	NTSTATUS Send(
		PVOID lpBuffer,
		DWORD lpNumberOfsent
	);
	NTSTATUS Recv(
		PVOID lpBuffer,
		DWORD lpNumberOfRecv
	);

};

class WSPServer
{
public:
	CRITICAL_SECTION m_CriticalSection;
	int m_EnableEvent = 0;
	std::vector<SOCKET> m_NeedAPCSocket;
	std::vector<SOCKET> m_AllClientSocket;
	WSPServerCallBack* m_CallBack = NULL;
	HANDLE m_ThreadHandle = NULL;
	SOCKET m_socket = NULL;
	bool m_EnableIPv6 = false;
	bool IsRun = false;
public:
	WSPServer() {
		memset(&m_CriticalSection, 0, sizeof(m_CriticalSection));
	}
	~WSPServer();

public:
	SOCKET InitialSocket(bool EnableIPv6 = false);
	VOID DeleteSocket();
	NTSTATUS CreateServer(
		USHORT port,
		int backlog = 5
	);
	SOCKET AcceptClient(
		sockaddr_in4* IPv4Addr = NULL,
		sockaddr_in6* IPv6Addr = NULL
	);
	NTSTATUS ShutDown(
		SOCKET Handle,
		int HowTo
	);
	NTSTATUS CloseClient(
		SOCKET Handle
	);
	NTSTATUS Send(
		SOCKET Handle,
		PVOID lpBuffer,
		DWORD lpNumberOfsent
	);
	NTSTATUS Recv(
		SOCKET Handle,
		PVOID lpBuffer,
		DWORD lpNumberOfRecv
	);
	HANDLE APCAsyncSelect(
		WSPServerCallBack* ApcCallBack,
		int lNetworkEvents
	);

	static VOID internal_APCThread(WSPServer* Server);

};
