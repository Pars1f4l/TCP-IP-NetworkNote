#include <stdio.h>
#include <windows.h>
#include <process.h> 
#define STR_LEN		100

unsigned WINAPI NumberOfA(void *arg);
unsigned WINAPI NumberOfOthers(void *arg);

static char str[STR_LEN];
static HANDLE hSemaphore;

int main(int argc, char *argv[]) 
{	
	HANDLE  hThread1, hThread2;
    //以non-signaled状态创建manual-reset模式的事件对象
	hSemaphore=CreateSemaphore(NULL, 0, 2, NULL);

	hThread1=(HANDLE)_beginthreadex(NULL, 0, NumberOfA, NULL, 0, NULL);
	hThread2=(HANDLE)_beginthreadex(NULL, 0, NumberOfOthers, NULL, 0, NULL);

	fputs("Input string: ", stdout); 
	fgets(str, STR_LEN, stdin);

    ReleaseSemaphore(hSemaphore, 2, NULL);

	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);

 	CloseHandle(hSemaphore);

    return 0;
}

unsigned WINAPI NumberOfA(void *arg) 
{
	int i, cnt=0;
	WaitForSingleObject(hSemaphore, INFINITE);
	for(i=0; str[i]!=0; i++)
	{
		if(str[i]=='A')
			cnt++;
	}
	printf("Num of A: %d \n", cnt);
	return 0;
}
unsigned WINAPI NumberOfOthers(void *arg) 
{
	int i, cnt=0;
	WaitForSingleObject(hSemaphore, INFINITE);
	for(i=0; str[i]!=0; i++) 
	{
		if(str[i]!='A')
			cnt++;
	}
	printf("Num of others: %d \n", cnt-1);
	return 0;
}