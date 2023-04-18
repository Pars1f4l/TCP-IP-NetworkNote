#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define BUF_SIZE 1024
void CALLBACK ReadCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void CALLBACK WriteCompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void ErrorHandling(char *message);
//结构体中包含套接字句柄、缓冲及缓冲相关信息
typedef struct
{
	SOCKET hClntSock;
	char buf[BUF_SIZE];
	WSABUF wsaBuf;
} PER_IO_DATA, *LPPER_IO_DATA;

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hLisnSock, hRecvSock;	
	SOCKADDR_IN lisnAdr, recvAdr;
	LPWSAOVERLAPPED lpOvLp;
	DWORD recvBytes;
	LPPER_IO_DATA hbInfo;
	int mode=1, recvAdrSz, flagInfo=0;
	
	if(argc!=2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}
	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 

	hLisnSock=WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	ioctlsocket(hLisnSock, FIONBIO, &mode);   // for non-blocking socket

	memset(&lisnAdr, 0, sizeof(lisnAdr));
	lisnAdr.sin_family=AF_INET;
	lisnAdr.sin_addr.s_addr=htonl(INADDR_ANY);
	lisnAdr.sin_port=htons(atoi(argv[1]));

	if(bind(hLisnSock, (SOCKADDR*) &lisnAdr, sizeof(lisnAdr))==SOCKET_ERROR)
		ErrorHandling("bind() error");
	if(listen(hLisnSock, 5)==SOCKET_ERROR)
		ErrorHandling("listen() error");

	recvAdrSz=sizeof(recvAdr);    

	while(1)
	{
		SleepEx(100, TRUE);    // for alertable wait state
		hRecvSock=accept(hLisnSock, (SOCKADDR*)&recvAdr,&recvAdrSz);
		//套接字时非阻塞模式的，因此需要注意返回INVALID_SOCKET的情况
        if(hRecvSock==INVALID_SOCKET)
		{
			if(WSAGetLastError()==WSAEWOULDBLOCK)
				continue;
			else
				ErrorHandling("accept() error");
		}
		puts("Client connected.....");
        //申请重叠I/O中需要使用的结构体变量的内存空间并初始化。
        //循环内部申请，因为每个客户端都需要独立的WSAOVERLAPPED结构体变量
		lpOvLp=(LPWSAOVERLAPPED)malloc(sizeof(WSAOVERLAPPED));
		memset(lpOvLp, 0, sizeof(WSAOVERLAPPED));
        //动态分配PER_IO_DATA结构体内存空间，并写入第36行创建的套接字句柄
		hbInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		hbInfo->hClntSock=(DWORD)hRecvSock;
		(hbInfo->wsaBuf).buf=hbInfo->buf;
		(hbInfo->wsaBuf).len=BUF_SIZE;
        //WSAOVERLAPPED结构体变量的hEvent成员中将写入第49行分配过空间的变量地址值。
        //基于Completion Routine函数的重叠I/O中不需要事件对象，因此，hEvent中可以写入其他信息
		lpOvLp->hEvent=(HANDLE)hbInfo;
        //调用WSARecv函数时将ReadCompRoutine函数指定为Completion Routine。
		WSARecv(hRecvSock, &(hbInfo->wsaBuf), 
			1, &recvBytes, &flagInfo, lpOvLp, ReadCompRoutine);
	}
	closesocket(hRecvSock);
	closesocket(hLisnSock);
	WSACleanup();
	return 0;
}
//该函数的调用意味着已完成数据输入，此时需要将接收的数据发送给回声客户端
void CALLBACK ReadCompRoutine(
	DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
    //提取完成输入的套接字句柄和缓冲信息，因为WSAOVERLAPPED结构体变量的hEvent成员中保存了PER_IO_DATA结构体变量地址值
	LPPER_IO_DATA hbInfo=(LPPER_IO_DATA)(lpOverlapped->hEvent);
	SOCKET hSock=hbInfo->hClntSock;
	LPWSABUF bufInfo=&(hbInfo->wsaBuf);
	DWORD sentBytes;

	if(szRecvBytes==0) //值为0意味着收到了EOF，需要进行相应处理
	{
		closesocket(hSock);
		free(lpOverlapped->hEvent); free(lpOverlapped);
		puts("Client disconnected.....");
	}
	else    // echo!
	{
		bufInfo->len=szRecvBytes;
        //将WriteCompRoutine函数指定为Completion Routine，同时调用WSASend函数。程序通过该语句向客户端发送回声消息
		WSASend(hSock, bufInfo, 1, &sentBytes, 0, lpOverlapped, WriteCompRoutine);
	}
}

void CALLBACK WriteCompRoutine(
	DWORD dwError, DWORD szSendBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
	LPPER_IO_DATA hbInfo=(LPPER_IO_DATA)(lpOverlapped->hEvent);
	SOCKET hSock=hbInfo->hClntSock;
	LPWSABUF bufInfo=&(hbInfo->wsaBuf);
	DWORD recvBytes;
	int flagInfo=0;
    //发送回声消息后需要再次接收数据，所以执行WSARecv的函数调用
	WSARecv(hSock, bufInfo,
		1, &recvBytes, &flagInfo, lpOverlapped, ReadCompRoutine);
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
