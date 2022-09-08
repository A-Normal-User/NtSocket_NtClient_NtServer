#include "WSPCS.h"

WSPClient::~WSPClient() {
	/// <summary>
	/// WSPClient����ʱ���еĴ���
	/// </summary>
	this->DeleteSocket();
}

SOCKET WSPClient::InitialSocket(bool EnableIPv6) {
	/// <summary>
	/// ��ʼ��Client��socket��ʹ��ǰ������ô˺������г�ʼ��
	/// </summary>
	/// <param name="EnableIPv6">�Ƿ�����IPv6ģʽ</param>
	/// <returns>����socket���</returns>
	if (this->m_socket != NULL) {
		//��ʾ�Ѿ���ʼ��
		return this->m_socket;
	}
	InitializeCriticalSection(&this->m_CriticalSection);
	//��ʼ���߳��ٽ�����Ϊ�˺��沿�ִ�����Ҫ�̰߳�ȫ��
	this->m_EnableIPv6 = EnableIPv6;
	this->m_socket = WSPSocket(EnableIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//��ʼ��һ��socket
	return this->m_socket;
}

VOID WSPClient::DeleteSocket() {
	/// <summary>
	/// ���ٵ�ǰClient��socket
	/// </summary>
	if (m_socket = NULL) {
		return;
	}
	DeleteCriticalSection(&this->m_CriticalSection);
	WSPCloseSocket(this->m_socket);
	m_socket = NULL;
}

NTSTATUS WSPClient::Connect(
	char* ConnectAddr,
	USHORT Port
) {
	/// <summary>
	/// ����һ����ַ
	/// </summary>
	/// <param name="ConnectAddr">���ӵĵ�ַ����ʼ��ʱ�����IPv4ģʽ����дIPv4�ĵ�ַ����֮Ӧ����дIPv6�ĵ�ַ</param>
	/// <param name="Port">���ӵĶ˿ں�</param>
	/// <returns>����WSPConnect�ķ���ֵ</returns>
	if (this->m_EnableIPv6){
		//IPv6�Ĵ���ģʽ
		sockaddr_in6 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin6_family = AF_INET6;
		SocketAddress.sin6_port = htons(Port);
		inet_addr_IPv6(ConnectAddr, &SocketAddress.sin6_addr[0]);
		//����һ��IPv6��ַ
		return WSPConnect_IPv6(this->m_socket, &SocketAddress);
	}else{
		sockaddr_in4 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_port = htons(Port);
		SocketAddress.sin_addr = inet_addr(ConnectAddr);
		//����һ��IPv6��ַ
		return WSPConnect(this->m_socket, (sockaddr*)&SocketAddress);
	}
}

NTSTATUS WSPClient::ShutDown(
	int HowTo
) {
	/// <summary>
	/// �ر�socket��ĳ��ģʽ������ʱʹ�ã�
	/// </summary>
	/// <param name="HowTo">�Ͽ���ģʽ</param>
	/// <returns>����WSPShutdown�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPShutdown(this->m_socket, HowTo);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

NTSTATUS WSPClient::Send(
	PVOID lpBuffer,
	DWORD lpNumberOfsent
) {
	/// <summary>
	/// �����ӵ�socket��������
	/// </summary>
	/// <param name="lpBuffer">���͵�����</param>
	/// <param name="lpNumberOfsent">���͵����ݳ���</param>
	/// <returns>����WSPSend�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPSend(this->m_socket, lpBuffer, lpNumberOfsent, 0);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

NTSTATUS WSPClient::Recv(
	PVOID lpBuffer,
	DWORD lpNumberOfRecv
) {
	/// <summary>
	/// ��������
	/// </summary>
	/// <param name="lpBuffer">�������ݵ�ָ�룬��Ҫ���з����ڴ�</param>
	/// <param name="lpNumberOfRecv">���յĳ���</param>
	/// <returns>����WSPRecv�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPRecv(this->m_socket, lpBuffer, &lpNumberOfRecv);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}


WSPServer::~WSPServer() {
	this->DeleteSocket();
}

SOCKET WSPServer::InitialSocket(
	bool EnableIPv6
) {
	/// <summary>
	/// ��ʼ���������˵�socket
	/// </summary>
	/// <param name="EnableIPv6">�Ƿ�����IPv6</param>
	/// <returns>����һ��socket���</returns>
	if (this->m_socket != NULL) {
		return this->m_socket;
	}
	InitializeCriticalSection(&this->m_CriticalSection);
	//��ʼ���߳��ٽ�����Ϊ�˺��沿�ִ�����Ҫ�̰߳�ȫ��
	this->m_EnableIPv6 = EnableIPv6;
	this->m_socket = WSPSocket(EnableIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//��ʼ��һ��socket
	this->IsRun = true;
	//����Ǹ�internal_APCThreadʹ�õģ���internal_APCThread�ܹ��Լ�������
	return this->m_socket;
}

VOID WSPServer::DeleteSocket(){
	/// <summary>
	/// ���ٷ������˵�socket
	/// </summary>
	if (this->m_socket == NULL) {
		return;
	}
	if (this->m_ThreadHandle != NULL) {
		this->IsRun = false;
		//֪ͨinternal_APCThread����ѭ��
		CloseHandle(this->m_ThreadHandle);
		//�ر������߳̾��
		Sleep(20);
		//�ȴ������
	}
	EnterCriticalSection(&this->m_CriticalSection);
	for (std::vector<SOCKET>::iterator iter = this->m_AllClientSocket.begin(); iter != this->m_AllClientSocket.end(); iter++) {
		WSPCloseSocket(*iter);
		//�ر�ȫ������
	}
	LeaveCriticalSection(&this->m_CriticalSection);
	DeleteCriticalSection(&this->m_CriticalSection);
	//ɾ���ٽ���
	WSPCloseSocket(this->m_socket);
	//�رշ�������socket���
	this->m_socket = NULL;
}

NTSTATUS WSPServer::CreateServer(
	USHORT port,
	int backlog
) {
	/// <summary>
	/// ����������socket�ļ���
	/// </summary>
	/// <param name="port">�����Ķ˿ں�</param>
	/// <param name="backlog">�������������ӵ����socket������Ŀ</param>
	/// <returns>����WSPListen��WSPBind�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result;
	if (this->m_EnableIPv6) {
		sockaddr_in6 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin6_family = AF_INET6;
		SocketAddress.sin6_port = htons(port);
		//bind������ַ
		result = WSPBind_IPv6(this->m_socket, &SocketAddress, AFD_SHARE_REUSE);
	}
	else {
		sockaddr_in4 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_port = htons(port);
		//bind������ַ
		result = WSPBind(this->m_socket, (sockaddr*)&SocketAddress, AFD_SHARE_REUSE);
	}
	if (result != ERROR_SUCCESS) {
		//WSPBindʧ�ܣ�
		LeaveCriticalSection(&this->m_CriticalSection);
		return result;
	}
	result = WSPListen(this->m_socket, backlog);
	//��ʼListen������backlog
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

SOCKET WSPServer::AcceptClient(
	sockaddr_in4* IPv4Addr,
	sockaddr_in6* IPv6Addr
) {
	/// <summary>
	/// ��һ���ڶ����е�socket��������
	/// </summary>
	/// <param name="IPv4Addr">�����IPv4ģʽ������������з���IPv4����ϸ��ַ</param>
	/// <param name="IPv6Addr">�����IPv6ģʽ������������з���IPv6����ϸ��ַ</param>
	/// <returns>���سɹ��������ӵ�socket���</returns>
	SOCKET Client = INVALID_SOCKET;
	EnterCriticalSection(&this->m_CriticalSection);
	if (this->m_EnableIPv6) {
		sockaddr_in6 SocketAddress;
		Client = WSPAccept_IPv6(this->m_socket, &SocketAddress, AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		//����IPv6����
		if (IPv6Addr != NULL) {
			*IPv6Addr = SocketAddress;
		}
	}
	else {
		sockaddr_in4 SocketAddress;
		Client = WSPAccept(this->m_socket, &SocketAddress, AF_INET, SOCK_STREAM, IPPROTO_TCP);
		//����IPv4����
		if (IPv4Addr != NULL) {
			*IPv4Addr = SocketAddress;
		}
	}
	this->m_NeedAPCSocket.push_back(Client);
	//���þ������m_NeedAPCSocket��m_AllClientSocket��
	this->m_AllClientSocket.push_back(Client);
	LeaveCriticalSection(&this->m_CriticalSection);
	return Client;
}

NTSTATUS WSPServer::ShutDown(
	SOCKET Handle,
	int HowTo
) {
	/// <summary>
	/// ͬShutDown
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="HowTo">WSPShutdown��ģʽ</param>
	/// <returns>����WSPShutdown�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPShutdown(Handle, HowTo);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

NTSTATUS WSPServer::CloseClient(
	SOCKET Handle
) {
	/// <summary>
	/// �ر�ĳһClient����
	/// </summary>
	/// <param name="Handle">Client��socket���</param>
	/// <returns>����WSPCloseSocket�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = ERROR_INVALID_HANDLE;
	for (int i = 0; i < this->m_AllClientSocket.size(); i++) {
		//��m_AllClientSocket��Ѱ�Ҹ�socket
		if (this->m_AllClientSocket[i] == Handle) {
			//�ҵ���ֱ�ӹرղ�ɾ����socket��
			this->m_AllClientSocket.erase(this->m_AllClientSocket.begin() + i);
			result = WSPCloseSocket(Handle);
			break;
		}
	}
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

NTSTATUS WSPServer::Send(
	SOCKET Handle,
	PVOID lpBuffer,
	DWORD lpNumberOfsent
) {
	/// <summary>
	/// send��Ϣ
	/// </summary>
	/// <param name="Handle">Ŀ��socket���</param>
	/// <param name="lpBuffer">���͵�����</param>
	/// <param name="lpNumberOfsent">���͵����ݳ���</param>
	/// <returns>����WSPSend�ķ���ֵ</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPSend(Handle, lpBuffer, lpNumberOfsent, 0);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}	

NTSTATUS WSPServer::Recv(
	SOCKET Handle,
	PVOID lpBuffer,
	DWORD lpNumberOfRecv
) {
	/// <summary>
	/// recv����
	/// </summary>
	/// <param name="Handle">socket���</param>
	/// <param name="lpBuffer">���յ����ݣ���Ҫ�ֶ���ʼ����������</param>
	/// <param name="lpNumberOfRecv">���յĳ���</param>
	/// <returns></returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPRecv(Handle, lpBuffer, &lpNumberOfRecv);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

VOID __stdcall internal_APCRoutine(
	PVOID ApcContext,
	PIO_STATUS_BLOCK IoStatusBlock,
	PVOID Reserved)
{
	/// <summary>
	/// ����һ���ڲ�������Ҳ�Ǳ������������㣺APC�첽select
	/// Client�ͷ�����socket��select�����ȫ������������������������ڷַ��ص��¼���
	/// </summary>
	/// <param name="ApcContext"></param>
	/// <param name="IoStatusBlock"></param>
	/// <param name="Reserved"></param>
	/// <returns></returns>
	AFD_AsyncData* AsyncData = (AFD_AsyncData*)ApcContext;
	//��ȡ��ApcContext���������AsyncData��AsyncData����ϸʹ������뿴WSPProcessAsyncSelect������ʵ�֡�
	WSPServer* self = (WSPServer*)AsyncData->UserContext;
	//��ȡ��WSPServer���������Ҫ�Ƕ�ȡ���ص�������ַ��
	//Ȼ����ûص�������������socket������¼���Ϣ
	((WSPServerCallBack)self->m_CallBack)(AsyncData->NowSocket, AsyncData->PollInfo.Events);
	//������һ��APC�첽select
	WSPProcessAsyncSelect(AsyncData->NowSocket, internal_APCRoutine, self->m_EnableEvent, (PVOID)self);
	//�ͷŵ�ԭ����AsyncData
	free(AsyncData);
}


VOID WSPServer::internal_APCThread(WSPServer* Server) {
	/// <summary>
	/// ��������APC�첽select���̺߳��������ڿ���ÿ��socket��APC�첽select
	/// </summary>
	/// <param name="Server">WSPServer����</param>
	int i = 0;
	while (1) {
		EnterCriticalSection(&Server->m_CriticalSection);
		if (!Server->IsRun) {
			//�Ѿ�֪ͨ���߳��˳�
			LeaveCriticalSection(&Server->m_CriticalSection);
			break;
		}
		i = Server->m_NeedAPCSocket.size();
		i--;
		for (i; i >= 0; i--) {
			//��m_NeedAPCSocketÿһsocket��ȡ����������APC�첽select
			WSPProcessAsyncSelect(
				Server->m_NeedAPCSocket[i],
				internal_APCRoutine,
				Server->m_EnableEvent, 
				(PVOID)Server
			);
			//�����APC�첽select���̽��ɺ���internal_APCRoutine���
			Server->m_NeedAPCSocket.pop_back();
			//ɾ��m_NeedAPCSocket��Ӧ��socket
		}
		LeaveCriticalSection(&Server->m_CriticalSection);
		SleepEx(1, true);
	}
}

HANDLE WSPServer::APCAsyncSelect(
	WSPServerCallBack*	ApcCallBack,
	int lNetworkEvents
) {
	/// <summary>
	/// ������������APC�첽selectģʽ��ֻ�ܳ�ʼ��һ��
	/// </summary>
	/// <param name="ApcCallBack">�ص�����</param>
	/// <param name="lNetworkEvents">��Ҫ�첽������¼�</param>
	/// <returns>�����첽�����̵߳ľ��</returns>
	if (this->m_CallBack != NULL) {
		return INVALID_HANDLE_VALUE;
	}
	EnterCriticalSection(&this->m_CriticalSection);
	this->m_EnableEvent = lNetworkEvents;
	this->m_CallBack = ApcCallBack;
	//���潫��m_AllClientSocket���еľ��������m_NeedAPCSocket
	std::copy(m_AllClientSocket.begin(), m_AllClientSocket.end(), m_NeedAPCSocket.begin());
	//��������socket����m_NeedAPCSocket
	m_NeedAPCSocket.push_back(this->m_socket);
	//�����߳�
	m_ThreadHandle = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)internal_APCThread, this, 0, NULL);
	LeaveCriticalSection(&this->m_CriticalSection);
	return m_ThreadHandle;
}