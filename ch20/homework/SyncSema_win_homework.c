#include <stdio.h>
#include <windows.h>
#include <process.h>

unsigned WINAPI Read(void * arg);
unsigned WINAPI Accu(void * arg);

static HANDLE semOne;
static HANDLE semTwo;
#define NUM_SIZE 100
static int num_cnt;
static int Num_arr[NUM_SIZE];
static int num;

int main(int argc, char *argv[])
{
	HANDLE hThread1, hThread2;
    //创建2个信号量对象
	semOne=CreateSemaphore(NULL, 0, 1, NULL);
	semTwo=CreateSemaphore(NULL, 1, 1, NULL);

	hThread1=(HANDLE)_beginthreadex(NULL, 0, Read, NULL, 0, NULL);
	hThread2=(HANDLE)_beginthreadex(NULL, 0, Accu, NULL, 0, NULL);

	WaitForSingleObject(hThread1, INFINITE);
	WaitForSingleObject(hThread2, INFINITE);
	
	CloseHandle(semOne);
	CloseHandle(semTwo);
	return 0;
}

unsigned WINAPI Read(void * arg)
{
	int i;
    WaitForSingleObject(semTwo, INFINITE);
	for(i=0; i<5; i++)
	{
		fputs("Input num: ", stdout);
		scanf("%d", &num);
		Num_arr[num_cnt++] = num;
	}
    ReleaseSemaphore(semOne, 1, NULL);
	return 0;	
}
unsigned WINAPI Accu(void * arg)
{
	int sum=0, i;
    WaitForSingleObject(semOne, INFINITE);
	for(i=0; i< num_cnt; i++)
	{
		sum+=Num_arr[i];
	}
    ReleaseSemaphore(semTwo, 1, NULL);
	printf("Result: %d \n", sum);
	return 0;
}