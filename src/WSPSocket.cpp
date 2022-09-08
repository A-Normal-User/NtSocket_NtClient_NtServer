#include "WSPSocket.h"

FARPROC MyNtDeviceIoControlFile;
FARPROC MyNtCreateEvent;
FARPROC MyNtCreateFile;
FARPROC MyRtlIpv6StringToAddressExA;

void WSPInit() {
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll == 0) {
		//初始化出错！
		return;
	}
	MyNtDeviceIoControlFile = GetProcAddress(ntdll, "NtDeviceIoControlFile");
	MyNtCreateEvent = GetProcAddress(ntdll, "NtCreateEvent");
	MyNtCreateFile = GetProcAddress(ntdll, "NtCreateFile");
	MyRtlIpv6StringToAddressExA = GetProcAddress(ntdll, "RtlIpv6StringToAddressExA");
}

NTSTATUS inet_addr_IPv6(
	char* p,
	byte* sin6_addr
) {
	ULONG  ScopeId;
	USHORT Port;
	return ((RtlIpv6StringToAddressExA)MyRtlIpv6StringToAddressExA)(p, sin6_addr, &ScopeId, &Port);
}

NTSTATUS inet_addr_IPv6(
	const char* p,
	byte* sin6_addr
) {
	ULONG  ScopeId;
	USHORT Port;
	return ((RtlIpv6StringToAddressExA)MyRtlIpv6StringToAddressExA)((PVOID)p, sin6_addr, &ScopeId, &Port);
}

SOCKET WSPSocket(
	int AddressFamily,
	int SocketType,
	int Protocol) {
	/// <summary>
	/// 类似于Socket函数，可以创建一个Socket文件句柄
	/// </summary>
	/// <param name="AddressFamily">Address family(Support IPv6)</param>
	/// <param name="SocketType">Socket Type</param>
	/// <param name="Protocol">Protocol type</param>
	/// <returns>如果失败返回INVALID_SOCKET，成功返回Socket文件句柄</returns>
	if (AddressFamily == AF_UNSPEC && SocketType == 0 && Protocol == 0) {
		return INVALID_SOCKET;
	}
	//进行基础数据设置
	if (AddressFamily == AF_UNSPEC) {
		AddressFamily = AF_INET;
	}
	if (SocketType == 0)
	{
		switch (Protocol)
		{
		case IPPROTO_TCP:
			SocketType = SOCK_STREAM;
			break;
		case IPPROTO_UDP:
			SocketType = SOCK_DGRAM;
			break;
		case IPPROTO_RAW:
			SocketType = SOCK_RAW;
			break;
		default:
			SocketType = SOCK_STREAM;
			break;
		}
	}
	if (Protocol == 0)
	{
		switch (SocketType)
		{
		case SOCK_STREAM:
			Protocol = IPPROTO_TCP;
			break;
		case SOCK_DGRAM:
			Protocol = IPPROTO_UDP;
			break;
		case SOCK_RAW:
			Protocol = IPPROTO_RAW;
			break;
		default:
			Protocol = IPPROTO_TCP;
			break;
		}
	}
	byte EaBuffer[] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x0F, 0x1E, 0x00, 
		0x41, 0x66, 0x64, 0x4F, 0x70, 0x65, 0x6E, 0x50, 
		0x61, 0x63, 0x6B, 0x65, 0x74, 0x58, 0x58, 0x00, 
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x02, 0x00, 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 
		0x06, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 
		0x08, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
	memmove((PVOID)((__int64)EaBuffer + 32), &AddressFamily, 0x4);
	memmove((PVOID)((__int64)EaBuffer + 36), &SocketType, 0x4);
	memmove((PVOID)((__int64)EaBuffer + 40), &Protocol, 0x4);
	if (Protocol == IPPROTO_UDP)
	{
		memmove((PVOID)((__int64)EaBuffer + 24), &Protocol, 0x4);
	}
	//初始化UNICODE_STRING：
	UNICODE_STRING AfdName;
	AfdName.Buffer = L"\\Device\\Afd\\Endpoint";
	AfdName.Length = 2 * wcslen(AfdName.Buffer);
	AfdName.MaximumLength = AfdName.Length + 2;
	OBJECT_ATTRIBUTES  Object;
	IO_STATUS_BLOCK IOSB;
	//初始化OBJECT_ATTRIBUTES
	InitializeObjectAttributes(&Object,
		&AfdName,
		OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
		0,
		0);
	HANDLE MySock;
	NTSTATUS Status;
	//创建AfdSocket：
	Status = ((NtCreateFile)MyNtCreateFile)(&MySock,
		GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE,
		&Object,
		&IOSB,
		NULL,
		0,
		FILE_SHARE_READ | FILE_SHARE_WRITE,
		FILE_OPEN_IF,
		0,
		EaBuffer,
		sizeof(EaBuffer));
	if (Status != STATUS_SUCCESS) {
		return INVALID_SOCKET;
	}
	else {
		return (SOCKET)MySock;
	}
}

NTSTATUS WSPCloseSocket(
	SOCKET Handle
) {
	/// <summary>
	/// 关闭一个socket
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <returns>返回NTSTATUS，表明函数的实际调用情况</returns>
	if (Handle == 0) {
		return -1;
	}
	NTSTATUS Status;
	HANDLE SockEvent = NULL;
	//创建一个等待事件
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	byte DisconnetInfo[] = { 0x00, 0x00, 0x00, 0x00, 0xC0, 0xBD, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	//设置断开信息
	int DisconnectType = AFD_DISCONNECT_SEND | AFD_DISCONNECT_RECV | AFD_DISCONNECT_ABORT | AFD_DISCONNECT_DATAGRAM;
	//全部操作都断开
	IO_STATUS_BLOCK IOSB;
	memmove(&DisconnetInfo, &DisconnectType, 0x4);
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_DISCONNECT,
		&DisconnetInfo,
		sizeof(DisconnetInfo),
		NULL,
		0);
	//发送请求
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle((HANDLE)Handle);
	//关闭sokcet句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPBind(
	SOCKET Handle,
	sockaddr* SocketAddress,
	int ShareType
) {
	/// <summary>
	/// Bind一个地址
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="SocketAddress">绑定的IPv4地址</param>
	/// <param name="ShareType">请填写AFD_SHARE_开头常量，若无特殊需求请填AFD_SHARE_WILDCARD</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	NTSTATUS Status;
	HANDLE SockEvent = NULL;
	//创建一个等待事件
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_BindData BindConfig;
	IO_STATUS_BLOCK IOSB;
	memset(&BindConfig, 0, sizeof(BindConfig));
	BindConfig.ShareType = ShareType;
	BindConfig.Address = *SocketAddress;
	byte OutputBlock[40];
	//作反馈消息
	memset(&OutputBlock, 0, sizeof(OutputBlock));
	//发送IOCTL_AFD_BIND消息
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_BIND,
		&BindConfig,
		sizeof(BindConfig),
		&OutputBlock,
		sizeof(OutputBlock));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPBind_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress,
	int ShareType
) {
	/// <summary>
	/// Bind一个地址
	/// </summary>
	/// <param name="Handle">socket地址</param>
	/// <param name="SocketAddress">绑定的IPv6地址</param>
	/// <param name="ShareType">请填写AFD_SHARE_开头常量，若无特殊需求请填AFD_SHARE_WILDCARD</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	NTSTATUS Status;
	HANDLE SockEvent = NULL;
	//创建一个等待事件
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_BindData_IPv6 BindConfig;
	IO_STATUS_BLOCK IOSB;
	memset(&BindConfig, 0, sizeof(BindConfig));
	BindConfig.ShareType = ShareType;
	BindConfig.Address = *SocketAddress;
	byte OutputBlock[40];
	//作反馈消息
	memset(&OutputBlock, 0, sizeof(OutputBlock));
	//发送IOCTL_AFD_BIND消息
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_BIND,
		&BindConfig,
		sizeof(BindConfig),
		&OutputBlock,
		sizeof(OutputBlock));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPConnect(
	SOCKET Handle,
	sockaddr* SocketAddress
) {
	/// <summary>
	/// 连接一个IPv4地址，大致用法同Connect函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="SocketAddress">连接的地址</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	sockaddr SockAddr;
	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sa_family = AF_INET;
	NTSTATUS Status;
	Status = WSPBind(Handle, &SockAddr, AFD_SHARE_REUSE);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}
	//创建一个等待事件
	HANDLE SockEvent = NULL;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_ConnctInfo ConnectInfo;
	IO_STATUS_BLOCK IOSB;
	ConnectInfo.UseSAN = 0;
	ConnectInfo.Root = 0;
	ConnectInfo.Unknown = 0;
	ConnectInfo.Address = *SocketAddress;
	//发送IOCTL_AFD_BIND消息
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_CONNECT,
		&ConnectInfo,
		sizeof(ConnectInfo),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPConnect_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress
) {
	/// <summary>
	/// 连接一个IPv6地址，大致用法同Connect函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="SocketAddress">连接的地址</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	sockaddr_in6 SockAddr;
	RtlFillMemory(&SockAddr, sizeof(SockAddr), 0);
	SockAddr.sin6_family = AF_INET6;
	NTSTATUS Status;
	Status = WSPBind_IPv6(Handle, &SockAddr, AFD_SHARE_REUSE);
	if (Status != ERROR_SUCCESS) {
		return Status;
	}
	//创建一个等待事件
	HANDLE SockEvent = NULL;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_ConnctInfo_IPv6 ConnectInfo;
	IO_STATUS_BLOCK IOSB;
	ConnectInfo.UseSAN = 0;
	ConnectInfo.Root = 0;
	ConnectInfo.Unknown = 0;
	ConnectInfo.Address = *SocketAddress;
	//发送IOCTL_AFD_CONNECT消息
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_CONNECT,
		&ConnectInfo,
		sizeof(ConnectInfo),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPShutdown(
	SOCKET Handle,
	int HowTo
) {
	/// <summary>
	/// Shutdown指定操作，同WSAShutdown使用
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="HowTo">请填写AFD Disconnect Flags相关常量（AFD_DISCONNECT开头常量）</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	byte DisconnetInfo[] = { 0x00, 0x00, 0x00, 0x00, 0xC0, 0xBD, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	//设置断开信息
	memmove(&DisconnetInfo, &HowTo, 0x4);
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_DISCONNECT,
		&DisconnetInfo,
		sizeof(DisconnetInfo),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPListen(
	SOCKET Handle,
	int Backlog
) {
	/// <summary>
	/// 开始监听，同Listen函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="Backlog">最大等待连接数目</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_ListenInfo ListenConfig;
	ListenConfig.UseSAN = false;
	ListenConfig.UseDelayedAcceptance = false;
	ListenConfig.Backlog = Backlog;
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_START_LISTEN,
		&ListenConfig,
		sizeof(ListenConfig),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPRecv(
	SOCKET Handle,
	PVOID lpBuffers,
	PDWORD lpNumberOfBytesRead
) {
	/// <summary>
	/// 接收数据，基本实现了recv的基础功能
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="lpBuffers">缓冲区，记得自行分配内存！！</param>
	/// <param name="lpNumberOfBytesRead">读取的数据量，本参数会同时返回实际读取的长度</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_SendRecvInfo SendRecvInfo;
	memset(&SendRecvInfo, 0, sizeof(SendRecvInfo));
	//设置发送的BufferCount
	SendRecvInfo.BufferCount = 1;
	SendRecvInfo.AfdFlags = 0;
	SendRecvInfo.TdiFlags = 0x20;
	//设置发送的Buffer
	AFD_Wsbuf SendRecvBuffer;
	SendRecvBuffer.len = *lpNumberOfBytesRead;
	SendRecvBuffer.buf = lpBuffers;
	SendRecvInfo.BufferArray = &SendRecvBuffer;
	IO_STATUS_BLOCK IOSB;
	//发送IOCTL_AFD_RECV
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_RECV,
		&SendRecvInfo,
		sizeof(SendRecvInfo),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
		*lpNumberOfBytesRead = IOSB.Information;
	}
	if (Status == STATUS_SUCCESS) {
		*lpNumberOfBytesRead = IOSB.Information;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPSend(
	SOCKET Handle,
	PVOID lpBuffers,
	DWORD lpNumberOfBytesSent,
	DWORD iFlags
) {
	/// <summary>
	/// 发送数据，基本实现send功能
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="lpBuffers">要发送的数据的指针</param>
	/// <param name="lpNumberOfBytesSent">要发送的字节数目</param>
	/// <param name="iFlags">Flags，设置发送标识（比如MSG_OOB一类）</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_SendRecvInfo SendRecvInfo;
	memset(&SendRecvInfo, 0, sizeof(SendRecvInfo));
	SendRecvInfo.BufferCount = 1;
	SendRecvInfo.AfdFlags = 0;
	SendRecvInfo.TdiFlags = 0;
	//设置TDIFlag
	if (iFlags)
	{
		if (iFlags & MSG_OOB)
		{
			SendRecvInfo.TdiFlags |= TDI_SEND_EXPEDITED;
		}
		if (iFlags & MSG_PARTIAL)
		{
			SendRecvInfo.TdiFlags |= TDI_SEND_PARTIAL;
		}
	}
	IO_STATUS_BLOCK IOSB;
	DWORD lpNumberOfBytesAlreadySend = 0;
	AFD_Wsbuf SendRecvBuffer;
	//这里需要循环发送，send一次性是不能发送太大的数据的
	do 
	{
		//计算应该发送的数据长度和偏移
		SendRecvBuffer.buf = (PVOID)((__int64)lpBuffers + lpNumberOfBytesAlreadySend);
		SendRecvBuffer.len = lpNumberOfBytesSent - lpNumberOfBytesAlreadySend;
		SendRecvInfo.BufferArray = &SendRecvBuffer;
		//发送数据
		Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
			SockEvent,
			NULL,
			NULL,
			&IOSB,
			IOCTL_AFD_SEND,
			&SendRecvInfo,
			sizeof(SendRecvInfo),
			NULL,
			0);
		if (Status == STATUS_PENDING)
		{
			WaitForSingleObject(SockEvent, INFINITE);
			//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		}
		Status = IOSB.Status;
		//获取实际成功发送的长度
		lpNumberOfBytesAlreadySend = IOSB.Information;
		if (Status != STATUS_SUCCESS){
			CloseHandle(SockEvent);
			return Status;
		}
	} while (lpNumberOfBytesAlreadySend < lpNumberOfBytesSent);
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetPeerName(
	SOCKET Handle,
	sockaddr_in4* Name
) {
	/// <summary>
	/// 基本同GetPeerName函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="Name">请填写IPv4使用的结构体</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_GET_PEER_NAME,
		0,
		0,
		&Name,
		sizeof(sockaddr_in4));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetPeerName_IPv6(
	SOCKET Handle,
	sockaddr_in6* Name
) {
	/// <summary>
	/// 基本同GetPeerName函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="Name">请填写IPv6使用的结构体</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_GET_PEER_NAME,
		0,
		0,
		Name,
		sizeof(sockaddr_in6));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetSockName(
	SOCKET Handle,
	sockaddr_in4* Name
) {
	/// <summary>
	/// 基本同GetSockName函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="Name">这里填写IPv4使用的结构体</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_GET_SOCK_NAME,
		0,
		0,
		&Name,
		sizeof(sockaddr_in4));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetSockName_IPv6(
	SOCKET Handle,
	sockaddr_in6* Name
) {
	/// <summary>
	/// 基本同GetSockName函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="Name">这里填写IPv6使用的结构体</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_GET_SOCK_NAME,
		0,
		0,
		Name,
		sizeof(sockaddr_in6));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPEventSelect(
	SOCKET Handle,
	HANDLE* hEventObject,
	ULONG lNetworkEvents
) {
	/// <summary>
	/// 用法基本同EventSelect，不过事件句柄函数内已经帮你创建好了
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="hEventObject">创建出的事件句柄</param>
	/// <param name="lNetworkEvents">网络事件，请填写AFD_EVENT_开头常量</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	Status = ((NtCreateEvent)MyNtCreateEvent)(hEventObject,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建EventObject失败！
		CloseHandle(SockEvent);
		return Status;
	}
	AFD_EventSelectInfo EventSelectInfo;
	EventSelectInfo.EventObject = *hEventObject;
	EventSelectInfo.Events = lNetworkEvents;
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_EVENT_SELECT,
		&EventSelectInfo,
		sizeof(EventSelectInfo),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPEnumNetworkEvents(
	SOCKET Handle,
	HANDLE hEventObject,
	PULONG lpNetworkEvents
) {
	/// <summary>
	/// 用法基本同EnumNetworkEvents函数
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="hEventObject">事件</param>
	/// <param name="lpNetworkEvents">返回的Network事件</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return Status;
	}
	AFD_EnumNetworkEventsInfo EnumNetworkEventsInfo;
	memset(&EnumNetworkEventsInfo, 0, sizeof(EnumNetworkEventsInfo));
	//EnumNetworkEventsInfo.Event = hEventObject;
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_ENUM_NETWORK_EVENTS,
		NULL,
		0,
		&EnumNetworkEventsInfo,
		sizeof(EnumNetworkEventsInfo));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	//关闭事件句柄
	CloseHandle(SockEvent);
	*lpNetworkEvents = EnumNetworkEventsInfo.Event;
	return Status;
}

SOCKET WSPAccept(
	SOCKET Handle,
	sockaddr_in4* SocketAddress,
	int AddressFamily,
	int SocketType,
	int Protocol
) {
	/// <summary>
	/// Accept一个连接，这是IPv4版本
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="SocketAddress">建立的连接的详细信息</param>
	/// <param name="AddressFamily">同WSPSocket的参数</param>
	/// <param name="SocketType">同WSPSocket的参数</param>
	/// <param name="Protocol">同WSPSocket的参数</param>
	/// <returns></returns>
	if (Handle == 0) {
		return INVALID_SOCKET;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return INVALID_SOCKET;
	}
	AFD_BindData ListenData;
	memset(&ListenData, 0, sizeof(ListenData));
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_WAIT_FOR_LISTEN,
		NULL,
		0,
		&ListenData,
		sizeof(ListenData));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	if (Status != STATUS_SUCCESS){
		CloseHandle(SockEvent);
		return Status;
	}
	SOCKET AcceptSocket = WSPSocket(AddressFamily, SocketType, Protocol);
	if (AcceptSocket == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}
	AFD_AcceptData AcceptData;
	AcceptData.UseSAN = 0;
	AcceptData.SequenceNumber = ListenData.ShareType;
	AcceptData.ListenHandle = (HANDLE)AcceptSocket;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_ACCEPT,
		&AcceptData,
		sizeof(AcceptData),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	memmove(SocketAddress, (PVOID)&ListenData.Address, sizeof(SocketAddress));
	CloseHandle(SockEvent);
	return AcceptSocket;
}

SOCKET WSPAccept_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress,
	int AddressFamily,
	int SocketType,
	int Protocol
) {
	/// <summary>
	/// Accept一个连接，这是IPv6版本
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="SocketAddress">建立的连接的详细信息</param>
	/// <param name="AddressFamily">同WSPSocket的参数</param>
	/// <param name="SocketType">同WSPSocket的参数</param>
	/// <param name="Protocol">同WSPSocket的参数</param>
	/// <returns></returns>
	if (Handle == 0) {
		return INVALID_SOCKET;
	}
	HANDLE SockEvent = NULL;
	NTSTATUS Status;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//创建Event失败！
		return INVALID_SOCKET;
	}
	AFD_BindData_IPv6 ListenData;
	memset(&ListenData, 0, sizeof(ListenData));
	IO_STATUS_BLOCK IOSB;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_WAIT_FOR_LISTEN,
		NULL,
		0,
		&ListenData,
		sizeof(ListenData));
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	if (Status != STATUS_SUCCESS) {
		CloseHandle(SockEvent);
		return Status;
	}
	SOCKET AcceptSocket = WSPSocket(AddressFamily, SocketType, Protocol);
	if (AcceptSocket == INVALID_SOCKET) {
		return INVALID_SOCKET;
	}
	AFD_AcceptData AcceptData;
	AcceptData.UseSAN = 0;
	AcceptData.SequenceNumber = ListenData.ShareType;
	AcceptData.ListenHandle = (HANDLE)AcceptSocket;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		SockEvent,
		NULL,
		NULL,
		&IOSB,
		IOCTL_AFD_ACCEPT,
		&AcceptData,
		sizeof(AcceptData),
		NULL,
		0);
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//这里WaitForSingleObject的时间可以自己调整（相当于超时时间）
		Status = IOSB.Status;
	}
	*SocketAddress = ListenData.Address;
	CloseHandle(SockEvent);
	return AcceptSocket;
}

NTSTATUS WSPProcessAsyncSelect(
	SOCKET Handle,
	PVOID ApcRoutine,
	ULONG lNetworkEvents,
	PVOID UserContext
) {
	/// <summary>
	/// WSPProcessAsyncSelect是本程序最大的亮点，利用异步化的IOCTL_AFD_SELECT
	/// </summary>
	/// <param name="Handle"></param>
	/// <param name="ApcRoutine"></param>
	/// <param name="lNetworkEvents"></param>
	/// <returns></returns>
	AFD_AsyncData* AsyncData = (AFD_AsyncData*)malloc(sizeof(AFD_AsyncData));
	if (AsyncData == NULL)
	{
		return -1;
	}
	memset(AsyncData, 0, sizeof(AFD_AsyncData));
	AsyncData->NowSocket = Handle;
	AsyncData->PollInfo.Timeout.HighPart = 0x7FFFFFFF;
	AsyncData->PollInfo.Timeout.LowPart = 0xFFFFFFFF;
	AsyncData->PollInfo.HandleCount = 1;
	AsyncData->PollInfo.Handle = Handle;
	AsyncData->PollInfo.Events = lNetworkEvents;
	AsyncData->UserContext = UserContext;
	NTSTATUS Status;
	Status = ((NtDeviceIoControlFile)(MyNtDeviceIoControlFile))((HANDLE)Handle,
		NULL,
		ApcRoutine,
		AsyncData,
		&AsyncData->IOSB,
		IOCTL_AFD_SELECT,
		&AsyncData->PollInfo,
		sizeof(AFD_PollInfo),
		&AsyncData->PollInfo,
		sizeof(AFD_PollInfo));
	return Status;
}