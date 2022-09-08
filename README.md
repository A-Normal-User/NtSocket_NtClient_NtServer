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
