#include <stdio.h>
#include <stdlib.h>
#include <process.h>
#include <winsock2.h>
#include <windows.h>

#define BUF_SIZE 100
#define READ	3
#define	WRITE	5

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
    //����CP�������һ�����������÷�0ֵ�����Կ�����CP��������൱�ں������߳�
	hComPort=CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 2);
	GetSystemInfo(&sysInfo); //����GetSystemInfo�����Ի�ȡ��ǰϵͳ��Ϣ
    //��Ա����dwNumberOfProcessors�н�д��CPU������˫��CPUʱд��2��
    //ͨ����2�д�������CPU�����൱���߳�
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
        //��̬����PER_HANDLE_DATA�ṹ�壬��д��ͻ��������׽��ֺͿͻ��˵�ַ��Ϣ
		handleInfo=(LPPER_HANDLE_DATA)malloc(sizeof(PER_HANDLE_DATA));		
		handleInfo->hClntSock=hClntSock;
		memcpy(&(handleInfo->clntAdr), &clntAdr, addrLen);
        //���ӵ�15�д�����CP����͵�35�д������׽��֡�
		CreateIoCompletionPort((HANDLE)hClntSock, hComPort, (DWORD)handleInfo, 0);
		//��̬����PER_IO_DATA�ṹ������ռ�
		ioInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
		memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));		
		ioInfo->wsaBuf.len=BUF_SIZE;
		ioInfo->wsaBuf.buf=ioInfo->buffer;
		ioInfo->rwMode=READ; //��������������ɺ�������״̬

		WSARecv(handleInfo->hClntSock,	&(ioInfo->wsaBuf),	
			1, &recvBytes, &flags, &(ioInfo->overlapped), NULL);			
	}
	return 0;
}
//�̵߳�main����
DWORD WINAPI EchoThreadMain(LPVOID pComPort)
{
	HANDLE hComPort=(HANDLE)pComPort;
	SOCKET sock;
	DWORD bytesTrans;
	LPPER_HANDLE_DATA handleInfo;
	LPPER_IO_DATA ioInfo;
	DWORD flags=0;
	
	while(1)
	{
        //GetQueuedCompletionStatus������I/O�������ע�������Ϣʱ����
		GetQueuedCompletionStatus(hComPort, &bytesTrans, 
			(LPDWORD)&handleInfo, (LPOVERLAPPED*)&ioInfo, INFINITE);
		sock=handleInfo->hClntSock;

		if(ioInfo->rwMode==READ)//���rwMode��Ա�е�ֵ�ж���������ɻ���������
		{
			puts("message received!");
			if(bytesTrans==0)    // δ������Ϣ
			{
				closesocket(sock);
				free(handleInfo); free(ioInfo);
				continue;		
			}
            //�����������յ�����Ϣ���͸��ͻ���
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));			
			ioInfo->wsaBuf.len=bytesTrans;
			ioInfo->rwMode=WRITE;
			WSASend(sock, &(ioInfo->wsaBuf), 
				1, NULL, 0, &(ioInfo->overlapped), NULL);
            //�ٴη�����Ϣ����տͻ�����Ϣ
			ioInfo=(LPPER_IO_DATA)malloc(sizeof(PER_IO_DATA));
			memset(&(ioInfo->overlapped), 0, sizeof(OVERLAPPED));
			ioInfo->wsaBuf.len=BUF_SIZE;
			ioInfo->wsaBuf.buf=ioInfo->buffer;
			ioInfo->rwMode=READ;
			WSARecv(sock, &(ioInfo->wsaBuf), 
				1, NULL, &flags, &(ioInfo->overlapped), NULL);
		}
		else
		{
			puts("message sent!");
			free(ioInfo);
		}
	}
	return 0;
}

void ErrorHandling(char *message)
{
	fputs(message, stderr);
	fputc('\n', stderr);
	exit(1);
}