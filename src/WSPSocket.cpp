#include "WSPSocket.h"

FARPROC MyNtDeviceIoControlFile;
FARPROC MyNtCreateEvent;
FARPROC MyNtCreateFile;
FARPROC MyRtlIpv6StringToAddressExA;

void WSPInit() {
	HMODULE ntdll = GetModuleHandleW(L"ntdll.dll");
	if (ntdll == 0) {
		//��ʼ������
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
	/// ������Socket���������Դ���һ��Socket�ļ����
	/// </summary>
	/// <param name="AddressFamily">Address family(Support IPv6)</param>
	/// <param name="SocketType">Socket Type</param>
	/// <param name="Protocol">Protocol type</param>
	/// <returns>���ʧ�ܷ���INVALID_SOCKET���ɹ�����Socket�ļ����</returns>
	if (AddressFamily == AF_UNSPEC && SocketType == 0 && Protocol == 0) {
		return INVALID_SOCKET;
	}
	//���л�����������
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
	//��ʼ��UNICODE_STRING��
	UNICODE_STRING AfdName;
	AfdName.Buffer = L"\\Device\\Afd\\Endpoint";
	AfdName.Length = 2 * wcslen(AfdName.Buffer);
	AfdName.MaximumLength = AfdName.Length + 2;
	OBJECT_ATTRIBUTES  Object;
	IO_STATUS_BLOCK IOSB;
	//��ʼ��OBJECT_ATTRIBUTES
	InitializeObjectAttributes(&Object,
		&AfdName,
		OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
		0,
		0);
	HANDLE MySock;
	NTSTATUS Status;
	//����AfdSocket��
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
	/// �ر�һ��socket
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <returns>����NTSTATUS������������ʵ�ʵ������</returns>
	if (Handle == 0) {
		return -1;
	}
	NTSTATUS Status;
	HANDLE SockEvent = NULL;
	//����һ���ȴ��¼�
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//����Eventʧ�ܣ�
		return Status;
	}
	byte DisconnetInfo[] = { 0x00, 0x00, 0x00, 0x00, 0xC0, 0xBD, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	//���öϿ���Ϣ
	int DisconnectType = AFD_DISCONNECT_SEND | AFD_DISCONNECT_RECV | AFD_DISCONNECT_ABORT | AFD_DISCONNECT_DATAGRAM;
	//ȫ���������Ͽ�
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
	//��������
	if (Status == STATUS_PENDING)
	{
		WaitForSingleObject(SockEvent, INFINITE);
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle((HANDLE)Handle);
	//�ر�sokcet���
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPBind(
	SOCKET Handle,
	sockaddr* SocketAddress,
	int ShareType
) {
	/// <summary>
	/// Bindһ����ַ
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="SocketAddress">�󶨵�IPv4��ַ</param>
	/// <param name="ShareType">����дAFD_SHARE_��ͷ����������������������AFD_SHARE_WILDCARD</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	NTSTATUS Status;
	HANDLE SockEvent = NULL;
	//����һ���ȴ��¼�
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//����Eventʧ�ܣ�
		return Status;
	}
	AFD_BindData BindConfig;
	IO_STATUS_BLOCK IOSB;
	memset(&BindConfig, 0, sizeof(BindConfig));
	BindConfig.ShareType = ShareType;
	BindConfig.Address = *SocketAddress;
	byte OutputBlock[40];
	//��������Ϣ
	memset(&OutputBlock, 0, sizeof(OutputBlock));
	//����IOCTL_AFD_BIND��Ϣ
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPBind_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress,
	int ShareType
) {
	/// <summary>
	/// Bindһ����ַ
	/// </summary>
	/// <param name="Handle">socket��ַ</param>
	/// <param name="SocketAddress">�󶨵�IPv6��ַ</param>
	/// <param name="ShareType">����дAFD_SHARE_��ͷ����������������������AFD_SHARE_WILDCARD</param>
	/// <returns></returns>
	if (Handle == 0) {
		return -1;
	}
	NTSTATUS Status;
	HANDLE SockEvent = NULL;
	//����һ���ȴ��¼�
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//����Eventʧ�ܣ�
		return Status;
	}
	AFD_BindData_IPv6 BindConfig;
	IO_STATUS_BLOCK IOSB;
	memset(&BindConfig, 0, sizeof(BindConfig));
	BindConfig.ShareType = ShareType;
	BindConfig.Address = *SocketAddress;
	byte OutputBlock[40];
	//��������Ϣ
	memset(&OutputBlock, 0, sizeof(OutputBlock));
	//����IOCTL_AFD_BIND��Ϣ
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPConnect(
	SOCKET Handle,
	sockaddr* SocketAddress
) {
	/// <summary>
	/// ����һ��IPv4��ַ�������÷�ͬConnect����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="SocketAddress">���ӵĵ�ַ</param>
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
	//����һ���ȴ��¼�
	HANDLE SockEvent = NULL;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//����Eventʧ�ܣ�
		return Status;
	}
	AFD_ConnctInfo ConnectInfo;
	IO_STATUS_BLOCK IOSB;
	ConnectInfo.UseSAN = 0;
	ConnectInfo.Root = 0;
	ConnectInfo.Unknown = 0;
	ConnectInfo.Address = *SocketAddress;
	//����IOCTL_AFD_BIND��Ϣ
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPConnect_IPv6(
	SOCKET Handle,
	sockaddr_in6* SocketAddress
) {
	/// <summary>
	/// ����һ��IPv6��ַ�������÷�ͬConnect����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="SocketAddress">���ӵĵ�ַ</param>
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
	//����һ���ȴ��¼�
	HANDLE SockEvent = NULL;
	Status = ((NtCreateEvent)MyNtCreateEvent)(&SockEvent,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//����Eventʧ�ܣ�
		return Status;
	}
	AFD_ConnctInfo_IPv6 ConnectInfo;
	IO_STATUS_BLOCK IOSB;
	ConnectInfo.UseSAN = 0;
	ConnectInfo.Root = 0;
	ConnectInfo.Unknown = 0;
	ConnectInfo.Address = *SocketAddress;
	//����IOCTL_AFD_CONNECT��Ϣ
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPShutdown(
	SOCKET Handle,
	int HowTo
) {
	/// <summary>
	/// Shutdownָ��������ͬWSAShutdownʹ��
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="HowTo">����дAFD Disconnect Flags��س�����AFD_DISCONNECT��ͷ������</param>
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
		//����Eventʧ�ܣ�
		return Status;
	}
	byte DisconnetInfo[] = { 0x00, 0x00, 0x00, 0x00, 0xC0, 0xBD, 0xF0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
	//���öϿ���Ϣ
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPListen(
	SOCKET Handle,
	int Backlog
) {
	/// <summary>
	/// ��ʼ������ͬListen����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="Backlog">���ȴ�������Ŀ</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPRecv(
	SOCKET Handle,
	PVOID lpBuffers,
	PDWORD lpNumberOfBytesRead
) {
	/// <summary>
	/// �������ݣ�����ʵ����recv�Ļ�������
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="lpBuffers">���������ǵ����з����ڴ棡��</param>
	/// <param name="lpNumberOfBytesRead">��ȡ������������������ͬʱ����ʵ�ʶ�ȡ�ĳ���</param>
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
		//����Eventʧ�ܣ�
		return Status;
	}
	AFD_SendRecvInfo SendRecvInfo;
	memset(&SendRecvInfo, 0, sizeof(SendRecvInfo));
	//���÷��͵�BufferCount
	SendRecvInfo.BufferCount = 1;
	SendRecvInfo.AfdFlags = 0;
	SendRecvInfo.TdiFlags = 0x20;
	//���÷��͵�Buffer
	AFD_Wsbuf SendRecvBuffer;
	SendRecvBuffer.len = *lpNumberOfBytesRead;
	SendRecvBuffer.buf = lpBuffers;
	SendRecvInfo.BufferArray = &SendRecvBuffer;
	IO_STATUS_BLOCK IOSB;
	//����IOCTL_AFD_RECV
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
		*lpNumberOfBytesRead = IOSB.Information;
	}
	if (Status == STATUS_SUCCESS) {
		*lpNumberOfBytesRead = IOSB.Information;
	}
	//�ر��¼����
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
	/// �������ݣ�����ʵ��send����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="lpBuffers">Ҫ���͵����ݵ�ָ��</param>
	/// <param name="lpNumberOfBytesSent">Ҫ���͵��ֽ���Ŀ</param>
	/// <param name="iFlags">Flags�����÷��ͱ�ʶ������MSG_OOBһ�ࣩ</param>
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
		//����Eventʧ�ܣ�
		return Status;
	}
	AFD_SendRecvInfo SendRecvInfo;
	memset(&SendRecvInfo, 0, sizeof(SendRecvInfo));
	SendRecvInfo.BufferCount = 1;
	SendRecvInfo.AfdFlags = 0;
	SendRecvInfo.TdiFlags = 0;
	//����TDIFlag
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
	//������Ҫѭ�����ͣ�sendһ�����ǲ��ܷ���̫������ݵ�
	do 
	{
		//����Ӧ�÷��͵����ݳ��Ⱥ�ƫ��
		SendRecvBuffer.buf = (PVOID)((__int64)lpBuffers + lpNumberOfBytesAlreadySend);
		SendRecvBuffer.len = lpNumberOfBytesSent - lpNumberOfBytesAlreadySend;
		SendRecvInfo.BufferArray = &SendRecvBuffer;
		//��������
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
			//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		}
		Status = IOSB.Status;
		//��ȡʵ�ʳɹ����͵ĳ���
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
	/// ����ͬGetPeerName����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="Name">����дIPv4ʹ�õĽṹ��</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetPeerName_IPv6(
	SOCKET Handle,
	sockaddr_in6* Name
) {
	/// <summary>
	/// ����ͬGetPeerName����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="Name">����дIPv6ʹ�õĽṹ��</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetSockName(
	SOCKET Handle,
	sockaddr_in4* Name
) {
	/// <summary>
	/// ����ͬGetSockName����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="Name">������дIPv4ʹ�õĽṹ��</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPGetSockName_IPv6(
	SOCKET Handle,
	sockaddr_in6* Name
) {
	/// <summary>
	/// ����ͬGetSockName����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="Name">������дIPv6ʹ�õĽṹ��</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPEventSelect(
	SOCKET Handle,
	HANDLE* hEventObject,
	ULONG lNetworkEvents
) {
	/// <summary>
	/// �÷�����ͬEventSelect�������¼�����������Ѿ����㴴������
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="hEventObject">���������¼����</param>
	/// <param name="lNetworkEvents">�����¼�������дAFD_EVENT_��ͷ����</param>
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
		//����Eventʧ�ܣ�
		return Status;
	}
	Status = ((NtCreateEvent)MyNtCreateEvent)(hEventObject,
		EVENT_ALL_ACCESS,
		NULL,
		SynchronizationEvent,
		FALSE);
	if (Status != STATUS_SUCCESS) {
		//����EventObjectʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
	CloseHandle(SockEvent);
	return Status;
}

NTSTATUS WSPEnumNetworkEvents(
	SOCKET Handle,
	HANDLE hEventObject,
	PULONG lpNetworkEvents
) {
	/// <summary>
	/// �÷�����ͬEnumNetworkEvents����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="hEventObject">�¼�</param>
	/// <param name="lpNetworkEvents">���ص�Network�¼�</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
		Status = IOSB.Status;
	}
	//�ر��¼����
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
	/// Acceptһ�����ӣ�����IPv4�汾
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="SocketAddress">���������ӵ���ϸ��Ϣ</param>
	/// <param name="AddressFamily">ͬWSPSocket�Ĳ���</param>
	/// <param name="SocketType">ͬWSPSocket�Ĳ���</param>
	/// <param name="Protocol">ͬWSPSocket�Ĳ���</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
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
	/// Acceptһ�����ӣ�����IPv6�汾
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="SocketAddress">���������ӵ���ϸ��Ϣ</param>
	/// <param name="AddressFamily">ͬWSPSocket�Ĳ���</param>
	/// <param name="SocketType">ͬWSPSocket�Ĳ���</param>
	/// <param name="Protocol">ͬWSPSocket�Ĳ���</param>
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
		//����Eventʧ�ܣ�
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
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
		//����WaitForSingleObject��ʱ������Լ��������൱�ڳ�ʱʱ�䣩
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
	/// WSPProcessAsyncSelect�Ǳ������������㣬�����첽����IOCTL_AFD_SELECT
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