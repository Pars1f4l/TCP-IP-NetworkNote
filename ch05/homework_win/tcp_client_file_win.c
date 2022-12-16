#include <stdio.h>
#include <stdlib.h>
#include <winsock2.h>
void ErrorHandling(char* message);
#define BUF_SIZE 100
int main(int argc, char* argv[])
{
	WSADATA wsaData;
	SOCKET hSocket; //声明Socket变量以保存socket返回值
	SOCKADDR_IN servAddr;

	char file_name[100];
    FILE * fp;
    char buf[BUF_SIZE];
    int str_Len;

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

    printf("请输入文件名：");
    scanf("%s", file_name);
    fp = fopen(file_name, "wb");

    send(hSocket, file_name, strlen(file_name), 0);
    while(str_Len = recv(hSocket, buf, BUF_SIZE, 0) != 0){
        fwrite(buf, 1, str_Len, fp);
    }

    fclose(fp);
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