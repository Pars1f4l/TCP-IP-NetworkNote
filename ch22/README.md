## 第 22 章 重叠I/O模型

### 22.1 理解重叠I/O模型

#### 22.1.1 重叠I/O

同一线程内部向多个目标传输（或从多个目标接收）数据引起的I/O重叠现象称为“重叠I/O”。为了完成这项任务，调用的I/O函数应立即返回，只有这样才能发送后续数据。从结果上看，利用上述模型收发数据时，最重要的前提条件就是异步I/O。而且，为了完成异步I/O，调用的I/O函数应以非阻塞模式工作。

不管是输入还是输出，只要是非阻塞模式的，就要另外确认执行结果。Windows中的重叠I/O不仅包含所示的I/O，还包含确认I/O完成状态的方法。

#### 22.1.2 创建重叠I/O套接字

首先要创建适用于重叠I/O的套接字，可以通过如下函数完成
```c
#include <winsock2.h>

SOCKET WSASocket(
        int af, int type, int protocol, LPWSAPROTOCOL_INFO lpProtocolInfo, GROUP g, DWORD dwFlags
        );
/*
 * 成功时返回套接字句柄，失败时返回INVALID_SOCKET
 * af：协议族信息
 * type：套接字数据传输方式
 * protocol：2个套接字之间使用的协议信息
 * ipProtocolInfo：包含创建的套接字信息的WSAPROTOCOL_INFO结构体变量地址值，不需要时传递NULL
 * g：为扩展函数而预约的参数，可以使用0
 * dwFlags：套接字属性信息
 * */
```

前3个参数与之前创建时相同，第四个和第五个参数与目前的工作无关，可以简单设置为NULL和0.可以向最后一个参数传递WSA_FLAG_OVERLAPPED，赋予创建出的套接字重叠I/O特性。总之，可以通过如下函数调用创建出可以进行重叠I/O的非阻塞模式的套接字
> WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);

#### 22.1.3 执行重叠I/O的WSASend函数

创建出具有重叠I/O属性的套接字后，接下来2个套接字（服务器端/客户端之间的）连接过程与一般的套接字连接过程相同，但I/O数据时使用的函数不同。下面是重叠I/O中使用的数据输出函数。

```c
#include <winsock2.h>

int WSASend(
        SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
        LPDWORD lpNumberOfBytesSent, DWORD dwFlags, LPWSAOVERLAPPED
        lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
        );
/*
 * 成功时返回0，失败时返回SOCKET_ERROR
 * s：套接字句柄，传递具有重叠I/O属性的套接字句柄时，以重叠I/O模型输出
 * lpBuffers：WSABUF结构体变量数组的地址值，WSABUF中存有待传输数据
 * dwBufferCount：第二个参数中数组的长度
 * lpNumberOfBytesSent：用于保存实际发送字节数的变量地址值
 * dwFlags：用于更改数据传输特性，如传递MSG_OOB时发送OOB模式的数据
 * lpOverlapped：WSAOVERLAPPED结构体变量的地址值，使用事件对象，用于确认完成数据传输
 * lpCompletionRoutine：传入Completion Routine函数的入口地址值，可以通过该函数确认是否完成数据传输。
 * */
```

接下来介绍上述函数的第二个结构体参数类型，该结构体中存有待传输数据的地址和大小等信息。
```c
typedef struct __WSABUF
{
    u_long len;        //待传输数据的大小
    char FAR * buf;    //缓冲地址值
} WSABUF, * LPWSABUF;
```

下面给出上述函数的调用示例。利用上述函数传输数据时可以按如下方式编写代码
WSAEVENT event;
WSAOVERLAPPED overlapped;
WSABUF dataBuf;
char buf[BUF_SIZE] = {"待传输的数据"};
int recvBytes = 0;
.....
event = WSACreateEvent();
memset(&overlapped, 0, sizeof(overlapped));
overlapped.hEvent = event;
dataBuf.len = sizeof(buf);
dataBuf.buf = buf;
WSASend(hSocket, &dataBuf, 1, &recvBytes, 0, &overlapped, NULL);
.....

调用WSASend函数时将第三个参数设置为1，因为第二个参数中待传输数据的缓冲个数为1.另外，多于参数均设置为NULL或0，其中需要注意第六个和第七个参数。第六个参数中WSAOVERLAPPED结构体定义如下。
```c
typedef struct __WSAOVERLAPPE
{
    DWORD Internal;
    DWORD InternalHigh;
    DWORD Offset;
    DWORD OffsetHigh;
    WSAEVENT hEvent;
} WSAOVERLAPPED, * LPWSAOVERLAPPED;
```

Internal、InternalHigh成员是进行重叠I/O时操作系统内部使用的成员，而Offset、OffsetHigh同样属于具有特殊用途的成员。所以各位实际只需要关注hEvent成员。

如果向lpOverlapped传递NULL，WSASend函数的第一个参数中的句柄所指的套接字将以阻塞模式工作。
> 利用WSASend函数同时向多个目标传递数据时，需要分别构建传入第六个参数的WSAOERLAPPED结构体变量。

#### 22.1.4 关于WSASend的补充

通过WSASend函数的lpNumberOfBytesSent参数可以获得实际传输的数据大小。
实际上，WSASend函数调用过程中，函数返回时间点和数据传输完成时间点并非总不一致。如果输出缓冲是空的，且传输的数据并不大，那么函数调用后可以立即完成数据传输。此时，WSASend函数将返回0，而lpNumberOfBytesSent中将保存实际传输的数据大小的信息。反之，WSASend函数返回后仍需要传输数据时，将返回SOCKET_ERROR，并将WSA_IO_PENDING注册为错误代码，该代码可以通过WSAGetLastError函数得到。这时应该通过如下函数获取实际传输的数据大小。
```c
#include <winsock2.h>

BOOL WSAGetOverlappedResult(
        SOCKET s, LPWSAOVERLAPPED lpOverlapped, LPDWORD lpcbTransfer, BOOL fWait, LPDWORD lpdwFlags
        );
/*
 * 成功时返回TRUE，失败时返回FALSE
 * s：进行重叠I/O的套接字句柄
 * lpOverlapped：进行重叠I/O时传递的WSAOVERLAPPED结构体变量的地址值
 * lpcbTransfer：用于保存实际传输字节数的变量地址值
 * fWait：如果调用该函数时仍在进行I/O，fWait为TRUE时等待I/O完成，fWait为FALSE时将返回FALSE并跳出函数
 * lpdwFlags：调用WSARecv函数时，用于获取附加信息（例如OOB信息）。如果不需要，可以传递NULL
 * */
```

通过此函数不仅可以获取数据传输结果，还可以验证接受数据的状态。如果给出示例前进行过多理论说明会使人感到乏味，所以稍后将通过示例讲解此函数的使用方法。

#### 22.1.5 进行重叠I/O的WSARecv函数

有了WSASend函数的基础，WSARecv函数将不难理解。因为他们大同小异，只是在功能上有接收和传输之分。

```c
#include <winsock2.h>

int WSARecv(
        SOCKET s, LPWSABUF lpBuffers, DWORD dwBufferCount,
        LPDWORD lpNumberOfBytesRecvd, LPDWORD lpFlags, LPWSAOVERLAPPED
        lpOverlapped, LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine 
        )
/*
 * 成功时返回0，失败时返回SOCKET_ERROR
 * s：赋予重叠I/O属性的套接字句柄
 * lpBuffers：用于保存接收数据的WSABUF结构体数组地址值
 * dwBufferCount：第二个参数中传递的数组的长度
 * lpNumberOfBytesSent：用于保存接收的数据大小的变量地址值
 * lpFlags：用于设置或读取传输特性信息
 * lpOverlapped：WSAOVERLAPPED结构体变量的地址值
 * lpCompletionRoutine：传入Completion Routine函数的地址值
 * */
```
> Gather/Scatter I/O
> Gather/Scatter I/O是指，将多个缓冲中的数据累积到一定程度后一次性传输，将接收的数据分批保存。第13章的writev&readv函数具有Gather/Scatter I/O功能，但Windows下并没有这些函数的定义。不过，可以通过重叠I/O中的WSASend和WSARecv函数获得类似功能。

### 22.2 重叠I/O的I/O完成确认

重叠I/O中有2种方法确认I/O的完成并获取结果。
- 利用WSASend、WSARecv函数的第六个参数，基于事件对象
- 利用WSASend、WSARecv函数的第七个参数，基于Completion Routine

#### 22.2.1 使用事件对象

示例代码：[OverlappedSend_win.c](OverlappedSend_win.c)
- 完成I/O时，WSAOVERLAPPED结构体变量引用的事件对象将变为signaled状态。
- 为了验证I/O的完成和完成结果，需要调用WSAGetOverlappedResult函数

上述示例的第44行调用的WSAGetLastError函数的定义如下。调用套接字相关函数后，可以通过该函数获取错误信息
```c
#include <winsock2.h>

int WSAGetLastError(void);
//返回错误代码（表示错误原因）
```

上述示例中该函数的返回值为WSA_IO_PENDING，由此可以判断WSASend函数的调用结果并非发生了错误，而是尚未完成（Pending）的状态。
下面是Receiver示例：
[OverlappedRecv_win.c](OverlappedRecv_win.c)

#### 22.2.2 使用Completion Routine函数

注册CR具有如下含义：
> Pending的I/O完成时调用此函数

I/O完成时调用注册过的函数进行事后处理，这就是Completion Routine的运作方式。如果执行重要任务时突然调用Completion Routine，则有可能破坏程序的正常执行流。因此，操作系统通常会预先定义规则：
> 只有请求I/O的线程处于alertable wait状态时才能调用Completion Routine函数

”alertable wait状态“是等待接收操作系统消息的线程状态。调用下列函数时进入alertable wait状态。
- WaitForSingleObjectEx
- WaitForMultipleObjectsEx
- WSAWaitForMultipleEvents
- SleepEx

第一、第二、第四个函数提供的功能与WaitForSingleObjext、WaitForMultipleObjects、Sleep函数相同。上述函数只增加了1个参数，如果该参数为TRUE，则相应线程将进入alertable wait状态。因此，启动I/O任务后，执行完紧急任务时可以调用上述任一函数验证I/O完成与否。此时操作系统知道线程进入alertable wait状态，如果有已完成的I/O，则调用相应的Completion Routine函数。调用后，上述函数将全部返回WAIT_IO_COMPLETION，并开始执行接下来的程序。

示例:[CmplRoutinesRecv_win.c](CmplRoutinesRecv_win.c)

下面是传入WSARecv函数的最后一个参数的Completion Routine函数原型
void CALLBACK CompletionRoutine(
    DWORD dwError, DWORD cbTransferred, LPWSAOVERLAPPED lpOverlapped, DWORD dwFlags
);

其中第一个参数中写入错误信息（正常结束时写入0），第二个参数中写入实际收发的字节数。第三个参数中写入WSASend、WSARecv函数的参数lpOverlapped，dwFlags中写入调用I/O函数时传入的特性信息或0.另外，返回值类型void后掺入的CALLBACK关键字与main函数中声明的关键字WINAPI相同，都是用于声明函数的调用规范，所以定义Completion Routine函数时必须添加。

### 22.3 习题

1. 异步通知I/O模型与重叠I/O模型在异步处理方面有哪些区别？
    
    在异步通知I/O模型下，以异步方式处理I/O事件发生的过程
    在重叠I/O模型下，还需要异步确认I/O完成的过程

2. 请分析非阻塞I/O、异步I/O、重叠I/O之间的关系

   异步I/O意味着异步处理，确认I/O完成的过程。为了实现这一过程，I/O必须在非阻塞模式下运行。当I/O以非阻塞模式运行并且I/O以异步方式进行时，I/O会重叠

3. 阅读下面代码，指出问题并给出解决方案

   若accpet函数返回失败，WSARecv传入的overlapped会报错，因为overlapped事先没有任何传入。若accept返回成功，因为所有的OVERLAPPED结构都相同，可以使用同一个OVERLAPPED结构体变量给多个WSARecv函数调用

4. 请从源代码角度说明调用WSASend函数后如何验证进入Pending状态

    WSASend函数在返回SOCKET_ERROR的状态下，利用WSAGetLastError函数是否返回WSA_IO_PENDING来验证

5. 线程的"alertable wait状态"的含义是什么？说出能使线程进入这种状态的两个函数

    所谓"alertable wait状态"是指等待接收操作系统信息的状态，在接下来的函数调用中，线程会进入alertable wait状态

- WaitForSingleObjectEx
- WaitForMultipleObjectEx
- WSAWaitForMultipleEvents
- SleepEx
