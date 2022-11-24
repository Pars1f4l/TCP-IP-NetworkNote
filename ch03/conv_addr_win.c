//取消之前定义的宏。防止参数转换成unicode格式，给出错误的运行结果
#undef UNICODE
#undef _UNICODE
#include <stdio.h>
#include <winsock2.h>

int main(int argc, char *argv[])
{
    //给出需要转换的字符串格式的地址
	char *strAddr="203.211.218.102:9190";

	char strAddrBuf[50];
	SOCKADDR_IN servAddr;
	int size;

	WSADATA	wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);
	
	size=sizeof(servAddr);
    //调用WSAStringToAddress函数转换成结构体，保存到servAddr
	WSAStringToAddress(
		strAddr, AF_INET, NULL, (SOCKADDR*)&servAddr, &size);

	size=sizeof(strAddrBuf);
    //调用WSAAddressToString函数将结构体转换成字符串，保存到strAddrBuf
	WSAAddressToString(
		(SOCKADDR*)&servAddr, sizeof(servAddr), NULL, strAddrBuf, &size);

	printf("Second conv result: %s \n", strAddrBuf);
  	WSACleanup();
	return 0;
}