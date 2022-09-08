#pragma once
#include "Share.h"
struct sockaddr_in4 {
	short   sin_family;
	u_short sin_port;
	ULONG   sin_addr;
	char    sin_zero[8];
};

struct sockaddr_in6 {
	USHORT sin6_family;
	USHORT sin6_port;
	ULONG  sin6_flowinfo;
	byte   sin6_addr[16];
	ULONG  sin6_scope_id;
};

struct AFD_BindData {
	ULONG   ShareType;
	sockaddr Address;
};

struct AFD_BindData_IPv6 {
	ULONG        ShareType;
	sockaddr_in6 Address;
};

struct AFD_ConnctInfo{
	size_t    UseSAN;
	size_t    Root;
	size_t    Unknown;
	sockaddr  Address;
};

struct AFD_ConnctInfo_IPv6 {
	size_t        UseSAN;
	size_t        Root;
	size_t        Unknown;
	sockaddr_in6  Address;
};

struct AFD_Wsbuf {
	UINT  len;
	PVOID buf;
};

struct AFD_SendRecvInfo {
	PVOID BufferArray;
	ULONG BufferCount;
	ULONG AfdFlags;
	ULONG TdiFlags;
};

struct AFD_ListenInfo {
	BOOLEAN UseSAN;
	ULONG   Backlog;
	BOOLEAN UseDelayedAcceptance;
};

struct AFD_EventSelectInfo {
	HANDLE EventObject;
	ULONG Events;
};

struct AFD_EnumNetworkEventsInfo {
	ULONG Event;
	ULONG PollEvents;
	NTSTATUS EventStatus[12];
};

struct AFD_AcceptData {
	ULONG UseSAN;
	ULONG SequenceNumber;
	HANDLE ListenHandle;
};

struct AFD_PollInfo{
	LARGE_INTEGER Timeout;
	ULONG HandleCount;
	ULONG Exclusive;
	SOCKET Handle;
	ULONG Events;
	NTSTATUS Status;
};

struct AFD_AsyncData {
	SOCKET NowSocket;
	PVOID UserContext;
	IO_STATUS_BLOCK IOSB;
	AFD_PollInfo PollInfo;
};

void WSPInit();

SOCKET WSPSocket(
	int AddressFamily,
	int SocketType,
	int Protocol
);

NTSTATUS WSPCloseSocket(
	SOCKET Handle
);
NTSTATUS WSPBind(
	SOCKET Handle,
	sockaddr* SocketAddress,
	int ShareType
);

NTSTATUS WSPBind_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress,
	int ShareType
);

NTSTATUS WSPConnect(
	SOCKET Handle,
	sockaddr* SocketAddress
);

NTSTATUS WSPConnect_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress
);

NTSTATUS WSPListen(
	SOCKET Handle,
	int Backlog
);

NTSTATUS WSPRecv(
	SOCKET Handle,
	PVOID lpBuffers,
	PDWORD lpNumberOfBytesRead
);

NTSTATUS WSPSend(
	SOCKET Handle,
	PVOID lpBuffers,
	DWORD lpNumberOfBytesSent,
	DWORD iFlags
);

NTSTATUS WSPGetPeerName(
	SOCKET Handle,
	sockaddr_in4* Name
);

NTSTATUS WSPGetPeerName_IPv6(
	SOCKET Handle,
	sockaddr_in6* Name
);

NTSTATUS WSPGetSockName(
	SOCKET Handle,
	sockaddr_in4* Name
);

NTSTATUS WSPGetSockName_IPv6(
	SOCKET Handle,
	sockaddr_in6* Name
);

NTSTATUS WSPEventSelect(
	SOCKET Handle,
	HANDLE* hEventObject,
	ULONG lNetworkEvents
);

NTSTATUS WSPEnumNetworkEvents(
	SOCKET Handle,
	HANDLE hEventObject,
	PULONG lpNetworkEvents
);

SOCKET WSPAccept(
	SOCKET Handle,
	sockaddr_in4* SocketAddress,
	int AddressFamily,
	int SocketType,
	int Protocol
);

SOCKET WSPAccept_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress,
	int AddressFamily,
	int SocketType,
	int Protocol
);

NTSTATUS WSPProcessAsyncSelect(
	SOCKET Handle,
	PVOID ApcRoutine,
	ULONG lNetworkEvents,
	PVOID UserContext = NULL
);

NTSTATUS WSPShutdown(
	SOCKET Handle,
	int HowTo
);

NTSTATUS inet_addr_IPv6(char* p,
	byte* sin6_addr
);

NTSTATUS inet_addr_IPv6(const char* p,
	byte* sin6_addr
);