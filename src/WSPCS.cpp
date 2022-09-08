#include "WSPCS.h"

WSPClient::~WSPClient() {
	/// <summary>
	/// WSPClient销毁时进行的处理
	/// </summary>
	this->DeleteSocket();
}

SOCKET WSPClient::InitialSocket(bool EnableIPv6) {
	/// <summary>
	/// 初始化Client的socket，使用前必须调用此函数进行初始化
	/// </summary>
	/// <param name="EnableIPv6">是否启用IPv6模式</param>
	/// <returns>返回socket句柄</returns>
	if (this->m_socket != NULL) {
		//表示已经初始化
		return this->m_socket;
	}
	InitializeCriticalSection(&this->m_CriticalSection);
	//初始化线程临界区（为了后面部分代码需要线程安全）
	this->m_EnableIPv6 = EnableIPv6;
	this->m_socket = WSPSocket(EnableIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//初始化一个socket
	return this->m_socket;
}

VOID WSPClient::DeleteSocket() {
	/// <summary>
	/// 销毁当前Client的socket
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
	/// 连接一个地址
	/// </summary>
	/// <param name="ConnectAddr">连接的地址，初始化时如果是IPv4模式请填写IPv4的地址，反之应该填写IPv6的地址</param>
	/// <param name="Port">连接的端口号</param>
	/// <returns>返回WSPConnect的返回值</returns>
	if (this->m_EnableIPv6){
		//IPv6的处理模式
		sockaddr_in6 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin6_family = AF_INET6;
		SocketAddress.sin6_port = htons(Port);
		inet_addr_IPv6(ConnectAddr, &SocketAddress.sin6_addr[0]);
		//连接一个IPv6地址
		return WSPConnect_IPv6(this->m_socket, &SocketAddress);
	}else{
		sockaddr_in4 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_port = htons(Port);
		SocketAddress.sin_addr = inet_addr(ConnectAddr);
		//连接一个IPv6地址
		return WSPConnect(this->m_socket, (sockaddr*)&SocketAddress);
	}
}

NTSTATUS WSPClient::ShutDown(
	int HowTo
) {
	/// <summary>
	/// 关闭socket的某个模式（单向时使用）
	/// </summary>
	/// <param name="HowTo">断开的模式</param>
	/// <returns>返回WSPShutdown的返回值</returns>
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
	/// 向连接的socket发送数据
	/// </summary>
	/// <param name="lpBuffer">发送的数据</param>
	/// <param name="lpNumberOfsent">发送的数据长度</param>
	/// <returns>返回WSPSend的返回值</returns>
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
	/// 接收数据
	/// </summary>
	/// <param name="lpBuffer">接收数据的指针，需要自行分配内存</param>
	/// <param name="lpNumberOfRecv">接收的长度</param>
	/// <returns>返回WSPRecv的返回值</returns>
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
	/// 初始化服务器端的socket
	/// </summary>
	/// <param name="EnableIPv6">是否启用IPv6</param>
	/// <returns>返回一个socket句柄</returns>
	if (this->m_socket != NULL) {
		return this->m_socket;
	}
	InitializeCriticalSection(&this->m_CriticalSection);
	//初始化线程临界区（为了后面部分代码需要线程安全）
	this->m_EnableIPv6 = EnableIPv6;
	this->m_socket = WSPSocket(EnableIPv6 ? AF_INET6 : AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//初始化一个socket
	this->IsRun = true;
	//这个是给internal_APCThread使用的，让internal_APCThread能够自己结束。
	return this->m_socket;
}

VOID WSPServer::DeleteSocket(){
	/// <summary>
	/// 销毁服务器端的socket
	/// </summary>
	if (this->m_socket == NULL) {
		return;
	}
	if (this->m_ThreadHandle != NULL) {
		this->IsRun = false;
		//通知internal_APCThread跳出循环
		CloseHandle(this->m_ThreadHandle);
		//关闭它的线程句柄
		Sleep(20);
		//等待其结束
	}
	EnterCriticalSection(&this->m_CriticalSection);
	for (std::vector<SOCKET>::iterator iter = this->m_AllClientSocket.begin(); iter != this->m_AllClientSocket.end(); iter++) {
		WSPCloseSocket(*iter);
		//关闭全部连接
	}
	LeaveCriticalSection(&this->m_CriticalSection);
	DeleteCriticalSection(&this->m_CriticalSection);
	//删除临界区
	WSPCloseSocket(this->m_socket);
	//关闭服务器的socket句柄
	this->m_socket = NULL;
}

NTSTATUS WSPServer::CreateServer(
	USHORT port,
	int backlog
) {
	/// <summary>
	/// 创建服务器socket的监听
	/// </summary>
	/// <param name="port">监听的端口号</param>
	/// <param name="backlog">服务器建立连接的最大socket队列数目</param>
	/// <returns>返回WSPListen或WSPBind的返回值</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result;
	if (this->m_EnableIPv6) {
		sockaddr_in6 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin6_family = AF_INET6;
		SocketAddress.sin6_port = htons(port);
		//bind监听地址
		result = WSPBind_IPv6(this->m_socket, &SocketAddress, AFD_SHARE_REUSE);
	}
	else {
		sockaddr_in4 SocketAddress;
		memset(&SocketAddress, 0, sizeof(SocketAddress));
		SocketAddress.sin_family = AF_INET;
		SocketAddress.sin_port = htons(port);
		//bind监听地址
		result = WSPBind(this->m_socket, (sockaddr*)&SocketAddress, AFD_SHARE_REUSE);
	}
	if (result != ERROR_SUCCESS) {
		//WSPBind失败！
		LeaveCriticalSection(&this->m_CriticalSection);
		return result;
	}
	result = WSPListen(this->m_socket, backlog);
	//开始Listen，设置backlog
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

SOCKET WSPServer::AcceptClient(
	sockaddr_in4* IPv4Addr,
	sockaddr_in6* IPv6Addr
) {
	/// <summary>
	/// 和一个在队列中的socket建立连接
	/// </summary>
	/// <param name="IPv4Addr">如果是IPv4模式，在这个参数中返回IPv4的详细地址</param>
	/// <param name="IPv6Addr">如果是IPv6模式，在这个参数中返回IPv6的详细地址</param>
	/// <returns>返回成功建立连接的socket句柄</returns>
	SOCKET Client = INVALID_SOCKET;
	EnterCriticalSection(&this->m_CriticalSection);
	if (this->m_EnableIPv6) {
		sockaddr_in6 SocketAddress;
		Client = WSPAccept_IPv6(this->m_socket, &SocketAddress, AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		//建立IPv6连接
		if (IPv6Addr != NULL) {
			*IPv6Addr = SocketAddress;
		}
	}
	else {
		sockaddr_in4 SocketAddress;
		Client = WSPAccept(this->m_socket, &SocketAddress, AF_INET, SOCK_STREAM, IPPROTO_TCP);
		//建立IPv4连接
		if (IPv4Addr != NULL) {
			*IPv4Addr = SocketAddress;
		}
	}
	this->m_NeedAPCSocket.push_back(Client);
	//将该句柄加入m_NeedAPCSocket和m_AllClientSocket中
	this->m_AllClientSocket.push_back(Client);
	LeaveCriticalSection(&this->m_CriticalSection);
	return Client;
}

NTSTATUS WSPServer::ShutDown(
	SOCKET Handle,
	int HowTo
) {
	/// <summary>
	/// 同ShutDown
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="HowTo">WSPShutdown的模式</param>
	/// <returns>返回WSPShutdown的返回值</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = WSPShutdown(Handle, HowTo);
	LeaveCriticalSection(&this->m_CriticalSection);
	return result;
}

NTSTATUS WSPServer::CloseClient(
	SOCKET Handle
) {
	/// <summary>
	/// 关闭某一Client连接
	/// </summary>
	/// <param name="Handle">Client的socket句柄</param>
	/// <returns>返回WSPCloseSocket的返回值</returns>
	EnterCriticalSection(&this->m_CriticalSection);
	NTSTATUS result = ERROR_INVALID_HANDLE;
	for (int i = 0; i < this->m_AllClientSocket.size(); i++) {
		//在m_AllClientSocket中寻找该socket
		if (this->m_AllClientSocket[i] == Handle) {
			//找到就直接关闭并删除该socket。
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
	/// send消息
	/// </summary>
	/// <param name="Handle">目标socket句柄</param>
	/// <param name="lpBuffer">发送的数据</param>
	/// <param name="lpNumberOfsent">发送的数据长度</param>
	/// <returns>返回WSPSend的返回值</returns>
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
	/// recv数据
	/// </summary>
	/// <param name="Handle">socket句柄</param>
	/// <param name="lpBuffer">接收的数据（需要手动初始化缓冲区）</param>
	/// <param name="lpNumberOfRecv">接收的长度</param>
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
	/// 这是一个内部函数，也是本程序最大的亮点：APC异步select
	/// Client和服务器socket的select情况会全部调用这个函数，本函数用于分发回调事件。
	/// </summary>
	/// <param name="ApcContext"></param>
	/// <param name="IoStatusBlock"></param>
	/// <param name="Reserved"></param>
	/// <returns></returns>
	AFD_AsyncData* AsyncData = (AFD_AsyncData*)ApcContext;
	//读取出ApcContext，保存的是AsyncData，AsyncData的详细使用情况请看WSPProcessAsyncSelect函数的实现。
	WSPServer* self = (WSPServer*)AsyncData->UserContext;
	//读取出WSPServer对象，这个主要是读取出回调函数地址。
	//然后调用回调函数，参数是socket句柄和事件信息
	((WSPServerCallBack)self->m_CallBack)(AsyncData->NowSocket, AsyncData->PollInfo.Events);
	//开启下一次APC异步select
	WSPProcessAsyncSelect(AsyncData->NowSocket, internal_APCRoutine, self->m_EnableEvent, (PVOID)self);
	//释放掉原来的AsyncData
	free(AsyncData);
}


VOID WSPServer::internal_APCThread(WSPServer* Server) {
	/// <summary>
	/// 本函数是APC异步select的线程函数，用于开启每个socket的APC异步select
	/// </summary>
	/// <param name="Server">WSPServer对象</param>
	int i = 0;
	while (1) {
		EnterCriticalSection(&Server->m_CriticalSection);
		if (!Server->IsRun) {
			//已经通知本线程退出
			LeaveCriticalSection(&Server->m_CriticalSection);
			break;
		}
		i = Server->m_NeedAPCSocket.size();
		i--;
		for (i; i >= 0; i--) {
			//将m_NeedAPCSocket每一socket读取出来，开启APC异步select
			WSPProcessAsyncSelect(
				Server->m_NeedAPCSocket[i],
				internal_APCRoutine,
				Server->m_EnableEvent, 
				(PVOID)Server
			);
			//后面的APC异步select过程将由函数internal_APCRoutine完成
			Server->m_NeedAPCSocket.pop_back();
			//删除m_NeedAPCSocket对应的socket
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
	/// 开启服务器的APC异步select模式，只能初始化一次
	/// </summary>
	/// <param name="ApcCallBack">回调函数</param>
	/// <param name="lNetworkEvents">需要异步处理的事件</param>
	/// <returns>返回异步处理线程的句柄</returns>
	if (this->m_CallBack != NULL) {
		return INVALID_HANDLE_VALUE;
	}
	EnterCriticalSection(&this->m_CriticalSection);
	this->m_EnableEvent = lNetworkEvents;
	this->m_CallBack = ApcCallBack;
	//下面将把m_AllClientSocket已有的句柄拷贝到m_NeedAPCSocket
	std::copy(m_AllClientSocket.begin(), m_AllClientSocket.end(), m_NeedAPCSocket.begin());
	//将服务器socket加入m_NeedAPCSocket
	m_NeedAPCSocket.push_back(this->m_socket);
	//启动线程
	m_ThreadHandle = CreateThread(NULL, 0, (PTHREAD_START_ROUTINE)internal_APCThread, this, 0, NULL);
	LeaveCriticalSection(&this->m_CriticalSection);
	return m_ThreadHandle;
}