# NtSocket_NtClient_NtServer：
   - Using NtCreateFile and NtDeviceIoControlFile to realize the function of winsock
   - 利用NtCreateFile和NtDeviceIoControlFile实现winsock的功能。
   - 部分功能参考ReactOS源码实现。
   - 现在已经成功封装出了一个NtClient和一个NtServer。
   - 源码主体实现了以下函数：
      - WSPSocket（创建socket句柄）
      - WSPCloseSocket（关闭socket）
      - WSPBind（Bind一个地址，支持IPv6）
      - WSPConnect（连接一个地址，大致用法同Connect函数，支持IPv6）
      - WSPShutdown（Shutdown指定操作，可以同WSAShutdown使用）
      - WSPListen（基本同Listen函数）
      - WSPRecv（接收数据，基本同Recv）
      - WSPSend（发送数据，基本同send）
      - WSPGetPeerName（基本同GetPeerName，支持IPv6）
      - WSPGetSockName（基本同GetSockName，支持IPv6）
      - WSPEventSelect（事件选择模型，基本同WSAEventSelect）
      - WSPEnumNetworkEvents（事件选择模型，基本同WSAEnumNetworkEvents）
      - WSPAccept（基本完整实现accept功能）
      - WSPProcessAsyncSelect（本源码最大的亮点，APC异步Select模型，这是winsock没有开放的模型，IOCP模型本质上也依赖这个完成）

# 前言：
   - 本程序只能使用于Windows。
   - 本程序开发环境：Win11 x64.
   - 本程序理论上兼容x64和x86环境，不过具体出现问题还得具体分析。

# 为什么写它？
   - 之前看了一篇文章：[NTSockets - Downloading a file via HTTP using the NtCreateFile and NtDeviceIoControlFile syscalls](https://www.x86matthew.com/view_post?id=ntsockets "NTSockets - Downloading a file via HTTP using the NtCreateFile and NtDeviceIoControlFile syscalls")
   - 我觉得这篇文章非常有意思，**直接利用NtCreateFile和Afd驱动建立通信，然后利用NtDeviceIoControlFile实现向afd发送socket控制的信息**。
   - 不过美中不足的是这个程序支持x86（原因是结构体定义的问题），同时也仅实现了Client，其他部分还没有完善。
   - 于是便有了这个程序。

# 正文：
   - 最基础的内容，利用NtCreateFile创建socket句柄：
```
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
```
   - 从总体上看来，这部分代码是最简单的，用NtCreateFile创建\\Device\\Afd\\Endpoint的连接，麻烦一点的可能是传递的EaBuffer，不过这部分可以直接通过CE拦截的方式获取数据，也可以参考ReactOS的结构体，我选择了一种介于两者之间的方法实现。
   - 这部分的兼容性也是最难实现的，因为不知道不同的系统的结构体会不会有差异，不过我测试基本是没有问题的。
   - 创建出socket句柄后，剩下的事情就非常简单了，只需要根据不同的“IOCTL_AFD_”写出各个函数即可。
