#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
void ErrorHandling(char* message);

int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hSocket; //声明Socket变量以保存socket返回值
	SOCKADDR_IN servAddr;

	char message[100];
    char message1[]="Hello World!";
    char message2[]="Nice to meet you too!";
    char message3[]="Have a good time!";
    char* str_arr[] = {message1, message2, message3};
	int str_Len=0;
	int idx=0, readLen=0;

	if(argc!=3)
	{
		printf("Usage : %s <IP> <port>\n", argv[0]);
		exit(1);
	}

	if(WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
		ErrorHandling("WSAStartup() error!");  

    //调用socket函数创建TCP套接字
	hSocket=socket(PF_INET, SOCK_STREAM, 0);
	if(hSocket==INVALID_SOCKET)
		ErrorHandling("hSocket() error");
	
	memset(&servAddr, 0, sizeof(servAddr));
	servAddr.sin_family=AF_INET;
	servAddr.sin_addr.s_addr=inet_addr(argv[1]);
	servAddr.sin_port=htons(atoi(argv[2]));
	
	if(connect(hSocket, (SOCKADDR*)&servAddr, sizeof(servAddr))==SOCKET_ERROR)
		ErrorHandling("connect() error!");

    for(int i = 0; i < 3; i++){
        str_Len = strlen(str_arr[i]) + 1;
        send(hSocket,(char*) &str_Len, 4, 0);
        send(hSocket,str_arr[i], str_Len, 0);
        recv(hSocket, (char *) &str_Len, 4, 0);
        recv(hSocket, message, str_Len, 0);
        puts(message);
    }

	closesocket(hSocket);
	WSACleanup();
	return 0;
}

void ErrorHandling(char* message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}