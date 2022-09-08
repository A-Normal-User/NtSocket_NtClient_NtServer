// socket.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//
#include <iostream>
#include "WSPCS.h"
using namespace std;

VOID Callback(SOCKET s, int Event);
VOID ServerTest();
VOID ClientTest();

WSPServer MyServer;

int main()
{
	WSPInit();//必须先初始化
	cout << "输入s启动服务器，输入c启动客户端:";
	char c;
	cin >> c;
	if (c == 's') {
		ServerTest();
	}
	else {
		ClientTest();
	}
}

VOID ClientTest() {
	WSPClient MyClient;
	MyClient.InitialSocket();
	//
	char Addr[] = "14.215.177.39";
	MyClient.Connect(&Addr[0], 80);
	char GetRequst[] = "GET /index.html HTTP/1.1\r\nAccept: text/htm\r\nAccept-Language: zh-CN\r\nConnection: keep-alive\r\nHost: www.baidu.com\r\nUser-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/105.0.0.0 Safari/537.36 Edg/105.0.1343.27\r\n\r\n";
	MyClient.Send(&GetRequst[0], sizeof(GetRequst));
	byte RecvReslut[1024];
	memset(&RecvReslut, 0, sizeof(RecvReslut));
	MyClient.Recv(RecvReslut, 1024);
	cout << "收到的结果是：\r\n" << RecvReslut;
}

VOID ServerTest() {
	NTSTATUS Status;
	MyServer.InitialSocket(false);
	Status = MyServer.CreateServer(8848, 256);
	if (Status != 0) {
		cout << "启动MyServer监听8848端口失败！\n";
		MyServer.DeleteSocket();
		return;
	}
	cout << "开始监听0.0.0.0:8848\n";
	MyServer.APCAsyncSelect((WSPServerCallBack*)Callback, AFD_EVENT_RECEIVE | AFD_EVENT_ACCEPT | AFD_EVENT_DISCONNECT | AFD_EVENT_ABORT | AFD_EVENT_CLOSE);
	system("pause");
}
VOID Callback(SOCKET s, int Event) {
    if (Event & AFD_EVENT_ACCEPT) {
        SOCKET news = MyServer.AcceptClient();
        cout << "新用户进入，socket句柄：" << news << "\n";
    }
	if (Event & AFD_EVENT_RECEIVE) {
        PVOID Buffer = malloc(1024);
        memset(Buffer, 0, 1024);
		MyServer.Recv(s, Buffer,1024);
        const char* feedback = "Hello,World!";
        MyServer.Send(s, (PVOID)feedback, strlen(feedback));
		cout << "收到消息！已经发送回复消息\n";
	}
    if (Event & AFD_EVENT_DISCONNECT || Event & AFD_EVENT_ABORT || Event & AFD_EVENT_CLOSE) {
        if (s == MyServer.m_socket) {
            cout << "服务器被销毁\n";
            return;
        }
        MyServer.CloseClient(s);
        cout << "断开连接，socket句柄：" << s << "\n";
    }
}