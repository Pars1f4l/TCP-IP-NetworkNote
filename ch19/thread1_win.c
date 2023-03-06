#include <stdio.h>
#include <windows.h>
#include <process.h>    /* _beginthreadex, _endthreadex */

unsigned WINAPI ThreadFunc(void *arg);

int main(int argc, char *argv[]) 
{
	HANDLE hThread;
	unsigned threadID;
	int param=5;
    //将ThreadFunc作为线程的main函数，并向其传递变量param的地址值，同时请求创建线程
	hThread=(HANDLE)_beginthreadex(NULL, 0, ThreadFunc, (void*)&param, 0, &threadID);
	if(hThread==0)
	{
		puts("_beginthreadex() error");
		return -1;
	}
	Sleep(3000); //等待3秒钟
	puts("end of main");
	return 0;
}

//WINAPI是Windows固有的关键字，他用于指定参数传递方向、分配栈返回方式等函数调用相关规定。
//插入它是为了遵守_beginthradex函数要求的调用规定
unsigned WINAPI ThreadFunc(void *arg)
{
	int i;
	int cnt=*((int*)arg);
	for(i=0; i<cnt; i++)
	{
		Sleep(1000);  puts("running thread");	 
	}
	return 0;
}