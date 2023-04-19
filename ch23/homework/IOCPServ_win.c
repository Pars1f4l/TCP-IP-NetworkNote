#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>

#define BUF_SIZE 100
#define READ	3
#define	WRITE	5

SOCKET socklink[100];
int cnt = 0;

typedef struct    // socket info
{
	SOCKET hClntSock;
	SOCKADDR_IN clntAdr;
} PER_HANDLE_DATA, *LPPER_HANDLE_DATA;

typedef struct    // buffer info
{
	OVERLAPPED overlapped;
	WSABUF wsaBuf;
	char buffer[BUF_SIZE];
	int rwMode;    // READ or WRITE
} PER_IO_DATA, *LPPER_IO_DATA;

DWORD WINAPI EchoThreadMain(LPVOID CompletionPortIO);
void ErrorHandling(char *message);
void SendMsg(char * msg, DWORD len);

int main(int argc, char* argv[])
{
	WSADATA	wsaData;
	HANDLE hComPort;	
	SYSTEM_INFO sysInfo;
	LPPER_IO_DATA ioInfo;
	LPPER_HANDLE_DATA handleInfo;

	SOCKET hServSock;
	SOCKADDR_IN servAdr;
	int recvBytes, i, flags=0;


	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 
    //创建CP对象。最后一个参数可以用非0值，所以可以向CP对象分配相当于核数的线程
	hComPort=CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 0);
	GetSystemInfo(&sysInfo); //调用GetSystemInfo函数以获取当前系统信息
    //成员变量dwNumberOfProcessors中将写入CPU个数（双核CPU时写入2）
    //通过这2行创建了与CPU个数相当的线程
	for(i=0; i<sysInfo.dwNumberOfProcessors; i++)
		_beginthreadex(NULL, 0, EchoThreadMain, (LPVOID)hComPort, 0, NULL);

	hServSock=WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&servAdr, 0, sizeof(servAdr));
	servAdr.sin_family=AF_INET;
	servAdr.sin_addr.s_addr=htonl(INADDR_ANY);
	servAdr.sin_port=htons(atoi(argv[1]));

	bind(hServSock, (SOCKADDR*)&servAdr, sizeof(servAdr));
	listen(hServSock, 5);

	while(1)
	{	
		SOCKET hClntSock;
		SOCKADDR_IN clntAdr;		
		int addrLen=sizeof(clntAdr);
		
		hClntSock=accept(hServSock, (SOCKADDR*)&clntAdr, &addrLen);
        socklink[cnt++] = hClntSock;

        //动态分配PER_HANDLE_DATA结构体，并写入客户端相连套接字和客户端地址信息
		handleInfo=(LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));		
		handleInfo->hClntSock=hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);
        //连接第15行创建的CP对象和第35行创建的套接字。
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);
		//动态分配PER_IO_DATA结构体变量空间
		ioInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));		
		ioInfo->wsaBuf.len=BUF_SIZE;
		ioInfo->wsaBuf.buf=ioInfo->buffer;


		WSARecv(handleInfo->hClntSock,	&(ioInfo->wsaBuf),	
			1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);			
	}
	return 0;
}
//线程的main函数
DWORD WINAPI EchoThreadMain(LPVOID pComPort)
{
	HANDLE hComPort=(HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags=0;
    int i;
	
	while(1)
	{
        //GetQueuedCompletionStatus函数在I/O完成且已注册相关信息时返回
		GetQueuedCompletionStatus(hComPort, &bytesTrans, 
			(LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock=handleInfo->hClntSock;

			puts("message received!");
			if(bytesTrans==0)    // 连接关闭
			{
                for(i = 0; i < cnt; i++){
                    if(socklink[i] == sock){
                        while(i++ < cnt - 1){
                            socklink[i] = socklink[i + 1];
                        }
                        break;
                    }
                }
                cnt--;
				closesocket(sock);
				free(handleInfo); free(ioInfo);
				continue;		
			}
            SendMsg(&(ioInfo->wsaBuf), ioInfo->wsaBuf.len);
            //再次发送消息后接收客户端消息
			ioInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len=BUF_SIZE;
			ioInfo->wsaBuf.buf=ioInfo->buffer;
			ioInfo->rwMode=READ;
			WSARecv(sock, &(ioInfo->wsaBuf), 
				1, NULL, &flags, &(ioInfo->overlapped), NULL);
	}
	return 0;
}

void SendMsg(char * msg, DWORD len){
    for(int i = 0; i < cnt; i++){
        send(socklink[i], msg, len, 0);
    }
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}