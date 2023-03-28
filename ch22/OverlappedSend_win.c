#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
void ErrorHandling(char *msg);

int main(int argc, char *argv[])
{
	WSADATA wsaData;
	SOCKET hSocket;
	SOCKADDR_IN sendAdr;

	WSABUF dataBuf;
	char msg[]="Network is Computer!";
	int sendBytes=0;

	WSAEVENT evObj;
	WSAOVERLAPPED overlapped;

	if(argc!=3) {
		printf("Usage: %s <IP> <port>\n", argv[0]);
		exit(1);
	}
	if(WSAStartup(MAKEWORD(2, 2), &wsaData)!=0)
		ErrorHandling("WSAStartup() error!"); 
	
	hSocket=WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	memset(&sendAdr, 0, sizeof(sendAdr));
	sendAdr.sin_family=AF_INET;
	sendAdr.sin_addr.s_addr=inet_addr(argv[1]);
	sendAdr.sin_port=htons(atoi(argv[2]));

	if(connect(hSocket, (SOCKADDR*)&sendAdr, sizeof(sendAdr))==SOCKET_ERROR)
		ErrorHandling("connect() error!");
	//创建事件对象并初始化待传输数据的缓冲
	evObj=WSACreateEvent();
	memset(&overlapped, 0, sizeof(overlapped));

	overlapped.hEvent=evObj;
	dataBuf.len=strlen(msg)+1;
	dataBuf.buf=msg;
    //WSASend如果不返回SOCKET_ERROR，则说明数据传输完成，所以sendBytes中的值有意义
	if(WSASend(hSocket, &dataBuf, 1, &sendBytes, 0, &overlapped, NULL)
		==SOCKET_ERROR)
	{
		if(WSAGetLastError()==WSA_IO_PENDING)
		{			
			puts("Background data send");
            //等待事件对象进入signaled状态，等待数据传输完成
			WSAWaitForMultipleEvents(1, &evObj, TRUE, WSA_INFINITE, FALSE);
			//获取传输结果
            WSAGetOverlappedResult(hSocket, &overlapped, &sendBytes, FALSE, NULL);
		}
		else
		{
			ErrorHandling("WSASend() error");
		}
	}

	printf("Send data size: %d \n", sendBytes);
	WSACloseEvent(evObj);
	closesocket(hSocket);
	WSACleanup();
	return 0;	
}

void ErrorHandling(char *msg)
{
	fputs(msg, stderr);
	fputc('\n', stderr);
	exit(1);
}