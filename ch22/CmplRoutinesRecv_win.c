#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>

#define BUF_SIZE 1024
void CALLBACK CompRoutine(DWORD, DWORD, LPWSAOVERLAPPED, DWORD);
void ErrorHandling(char *message);

WSABUF dataBuf;
char buf[BUF_SIZE];
int recvBytes=0;

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hLisnSock, hRecvSock;	
	SOCKADDR_IN lisnAdr, recvAdr;

	WSAOVERLAPPED overlapped;
	WSAEVENT evObj;

	int idx, recvAdrSz, flags=0;
	if(argc!=2) {
		printf("Usage: %s <port>\n", argv[0]);
		exit(1);
	}	
	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!"); 

	hLisnSock=WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&lisnAdr, 0, sizeof(lisnAdr));
	lisnAdr.sin_family=AF_INET;
	lisnAdr.sin_addr.s_addr=htonl(INADDR_ANY);
	lisnAdr.sin_port=htons(atoi(argv[1]));

	if(bind(hLisnSock, (SOCKADDR*) &lisnAdr, sizeof(lisnAdr))==SOCKET_ERROR)
		ErrorHandling("bind() error");
	if(listen(hLisnSock, 5)==SOCKET_ERROR)
		ErrorHandling("listen() error");

	recvAdrSz=sizeof(recvAdr);    
	hRecvSock=accept(hLisnSock, (SOCKADDR*)&recvAdr,&recvAdrSz);
	if(hRecvSock==INVALID_SOCKET)
		ErrorHandling("accept() error");

	memset(&overlapped, 0, sizeof(overlapped));
	dataBuf.len=BUF_SIZE;
	dataBuf.buf=buf;
	evObj=WSACreateEvent();     //Dummy event object 
    //调用WSARecv函数的同时向第七个参数传递Completion Routine函数的地址值
    //必须传递第六个参数，但不必创建事件对象
	if(WSARecv(hRecvSock, &dataBuf, 1, &recvBytes, &flags, &overlapped, CompRoutine)
		==SOCKET_ERROR)
	{
		if(WSAGetLastError()==WSA_IO_PENDING)
			puts("Background data receive");
	}
    //为了使main线程进入alertable wait状态而调用的函数。使用SleepEx函数可避免Dummy Object的产生
	idx=WSAWaitForMultipleEvents(1, &evObj, FALSE, WSA_INFINITE, TRUE);
	if(idx==WAIT_IO_COMPLETION) //I/O正常结束
		puts("Overlapped I/O Completed");
	else    // If error occurred!
		ErrorHandling("WSARecv() error");

	WSACloseEvent(evObj);
	closesocket(hRecvSock);
	closesocket(hLisnSock);
	WSACleanup();
	return 0;
}
//Completion Routine函数固定不变
void CALLBACK CompRoutine(
	DWORD dwError, DWORD szRecvBytes, LPWSAOVERLAPPED lpOverlapped, DWORD flags)
{
	if(dwError!=0)
	{
		ErrorHandling("CompRoutine error");
	}
	else
	{
		recvBytes=szRecvBytes;
		printf("Received message: %s \n", buf);
	}
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}
