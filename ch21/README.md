## 第 21 章 异步通知I/O模型

### 21.1 理解异步通知I/O模型

#### 21.1.1 理解同步和异步

首先解释“异步”的含义。异步主要指不一致，在数据I/O中非常有用，之前的Windows示例中主要通过send&recv函数进行同步I/O。调用send函数时，完成数据传输后才能从函数返回；而调用recv函数时，只有读到期望大小的数据才能返回。因此相当于同步方式的I/O处理。

- 调用send函数的瞬间开始传输数据，send函数执行完（返回）的时刻完成数据传输
- 调用recv函数的瞬间开始接收数据，recv函数执行完（返回）的时刻完成数据传输

#### 21.1.2 同步I/O的缺点及异步方式的解决方案

同步I/O的缺点，进行I/O的过程中函数无法返回，所以不能执行其他任务  
无论数据是否完成交换都返回函数，这就意味着可以执行其他任务。  
所以说异步方式能够比同步方式更有效地使用GPU。  

#### 21.1.3 理解异步通知I/O模型

通知I/O的含义：
- 通知输入缓冲收到数据并需要读取，以及输出缓冲为空故可以发送数据

通知I/O是指发生了I/O相关的特定情况。典型的通知I/O模型是select方式。select函数就是从返回调用的函数时通知需要I/O处理的，或可以进行I/O处理的情况。  

异步通知中，指定I/O监视对象的函数和实际验证状态变化的函数时相互分离的。因此，指定监视对象后可以离开执行其他任务，最后再回来验证状态变化。

### 21.2 理解和实现异步通知I/O模型

异步通知I/O模型的实现方法有两种：一种是使用本书介绍的WSAEventSelect函数，另一种是使用WSAAsyncSelect函数。使用WSAEventSelect函数时需要指定Windows句柄以获取发生的事件。

#### 21.2.1 WSAEventSelect函数和通知

如前所述，告知I/O状态变化的操作就是“通知”。I/O的状态变化可以分为不同情况：
- 套接字的状态变化：套接字的I/O状态变化
- 发生套接字相关事件：发生套接字I/O相关事件

下面是WSAEventSelect函数，该函数用于指定某一套接字为事件监视对象
```c
#include <winsock2.h>

int WSAEventSelect(SOCKET s, WSAEVENT hEventObject, long lNetworkEvents);
//成功时返回0，失败时返回SOCKET_ERROR
//s：监视对象的套接字句柄
//hEventObject：传递事件对象句柄以验证事件发生与否
//lNetworkEvents：希望监视的事件类型信息
```

传入参数s的套接字内只要发生lNetworkEvents中指定的事件之一，WSAEventSelect函数就将hEventObject句柄所指内核对象改为signaled状态。因此，该函数又称为连接事件对象和套接字的函数

无论发生事件与否，WSAEventSelect函数调用后都会直接返回，所以可以执行其他任务。也就是说，该函数以异步方式工作。下面是该函数的第三个参数的事件类型信息，可以通过位或运算同时指定多个信息
- FD_READ:是否存在需要接受的信息
- FD_WRITE:能否以非阻塞方式传递数据
- FD_OOB:是否收到带外数据
- FD_ACCEPT:是否有新的连接请求
- FD_CLOSE:是否有断开连接请求

从前面关于WSAEventSelect函数的说明中可以看出，需要补充如下内容：
- WSAEventSelect函数的第二个参数中用到的事件对象的创建方法
- 调用WSAEventSelect函数后发生的事件的验证方法
- 验证事件发生后事件类型的查看方法

#### 21.2.2 manual-reset模式事件对象的其他创建方式

我们之前利用CreateEvent函数创建了事件对象。CreateEvent函数在创建事件对象时，可以在auto-reset和manual-reset模式中任选其一。但我们只需要manual-reset模式non-signaled状态的事件对象，所以利用如下函数创建较为方便。

```c
#include <winsock2.h>

WSAEVENT WSACreateEvent(void);
//成功时返回事件对象句柄，失败时返回WSA_INVALID_EVENT
```

上述声明中返回类型WSAEVENT的定义如下：  
#define WSAEVENT HANDLE

实际上就是内核对象句柄。为了销毁通过上述函数创建的事件对象，系统提供了如下函数。

```c
#include <winsock2.h>

BOOL WSACloseEvent(WSAEVENT hEvent);
//成功时返回TRUE,失败时返回FALSE
```

#### 21.2.3 验证是否发生事件

为了验证是否发生了事件，需要查看事件对象。完成该任务的函数如下，除了多了1个参数外，其余部分与WaitForMultipleObjects函数完全相同。

```c
#include <winsock2.h>

DWORD WSAWaitForMultipleEvents(
        DWORD cEvents, const WSAEVENT * lphEvents, BOOL fWaitAll, DWORD dwTimeout, BOOL fAlertable
        );
//成功时返回发生事件的对象信息，失败时返回WSA_INVALID_EVENT
//cEvents:需要验证是否转为signaled状态的事件对象的个数
//lphEvents：存有事件对象句柄的数据地址值
//fWaitAll：传递TRUE时，所有事件对象在signaled状态时返回；传递FALSE时，只要其中1个变为signaled状态就返回
//dwTimeout：以1/1000秒为单位指定超时，传递WSA_INFINITE时，直到变为signaled状态时才会返回
//fAlertable：传递TRUE时进入alertable wait（可警告等待）状态
//返回值：返回值减去常量WSA_WAIT_EVENT_0时，可以得到转变为signaled状态的事件对象句柄对应的索引，可以通过该索引在第二个参数指定的数组中查找句柄。如果有多个事件对象变为signaled状态，则会得到其中较小的值。发生超时返回WAIT_TIMEOUT
```

由于发生套接字事件，事件对象转为signaled后该函数才返回，所以能够确认事件发生与否。但由于最多可传递64个事件对象，如果需要监视更多句柄，就只能创建线程或扩展保存句柄的数组，并多次调用上述函数。
> 最大句柄数  
> 可通过以宏的方式声明的WSA_MAXIMUM_WAIT_EVENT常量得知WSAWaitForMultipleEvents函数可以同时监视的最大事件对象数。该常量值为64，所以最大句柄数为64.但可以修改这一限制，日后发布新版本的操作系统时可能更改该常量。

#### 21.2.4 区分事件类型

通过WSAWaitForMultipleEvents函数得到了转为signaled状态的事件对象，最后就要确定相应对象进入signaled状态的原因。为了完成该任务，我们引入如下函数。调用此函数，不仅需要signaled状态的事件对象句柄，还需要与之连接的发生事件对象的套接字句柄

```c
#include <winsock2.h>

int WSAEnumNetworkEvents(
        SOCKET s, WSAEVENT hEventObject, LPWSANETWORKEVENTS lpNetworkEvents
        );
//成功时返回0，失败时返回SOCKET_ERROR
/*
 * s：发生事件的套接字句柄
 * hEventObject：与套接字相连的signaled状态的事件对象句柄
 * lpNetworkEvents：保存发生的事件类型信息和错误信息的WSANETWORKEVENTS结构体变量地址值
 * */
```

上述函数将manual-reset模式的事件对象改为non-signaled状态，所以得到发生的事件类型后，不必单独调用ResetEvent函数。下面是与上述函数有关的WSANETWORKEVENTS结构体。

```c
typedef struct _WSANETWORKEVENTS{
    long lNetworkEvents;
    int iErrorCode[FD_MAX_EVENTS];
} WSANETWORKEVENTS, * LPWSANETWORKEVENTS;
```

上述结构体的lNetworkEvents成员将保存发生的事件信息。与WSAEventSelect函数的第三个参数相同，需要接受数据时，该成员为FD_READ;有连接请求时，该成员为FD_ACCEPT。因此，可通过如下方式查看发生的事件类型。

```c
WSANETWORKEVENTS netEvents;
....
WSAEnumNetworkEvents(hSock, hEvent, &netEvents);
if(netEvents.lNetworkEvents & FD_ACCEPT){
    //FD_ACCEPT事件的处理
}
if(netEvents.lNetworkEvents & FD_READ){
    //FD_READ事件的处理
}
if(netEvents.lNetworkEvents & FD_CLOSE){
    //FD_CLOSE事件的处理
}
```

另外，错误信息将保存到声明为成员的iErrorCode数组。验证方法如下。  
- 如果发生FD_READ相关错误，则在iErrorCode[FD_READ_BIT]中保存除0以外的其他值
- 如果发生FD_WRITE相关错误，则在iErrorCode[FD_WRITE_BIT]中保存除0以外的其他值

#### 21.2.5 利用异步通知I/O模型实现回声服务器端

示例代码：[AsynNotiEchoServ_win.c](AsynNotiEchoServ_win.c)

上述示例可以和任意回声客户端配合运行

### 21.3 习题

1. 结合send&recv函数解释同步和异步方式的I/O。并请说明同步I/O的缺点，以及怎样通过异步I/O进行解决。  

   答：同步I/O是指数据发送的时间点和函数返回的时间一致的I/O方式。即，send函数返还的时间是数据传输完毕的时间，recv函数返还的时间是数据接收完毕的时间。但是异步I/O是指I/O函数的返回时刻与数据收发的完成时刻不一致。同步I/O的缺点是，直到输入输出完成为止，都处于阻塞状态。在这段时间里，CPU实际上是没有被利用的，但由于阻塞状态，无法进行其他工作。从这一点看，异步I/O对CPU的使用效率优于同步I/O

2. 异步I/O并不是所有情况下的最佳选择。它具有哪些缺点？何种情况下同步I/O更优？可以参考异步I/O相关源代码，结合线程进行说明

   答：异步I/O有优点也有缺点，缺点在于要在完成输入输出以后去确认。如果服务器的服务非常简单，回复所需的数据量很小，就会很不方便。特别是在每一个用户都生成一个线程的服务器中，没有必要一定使用异步I/O

3. 判断下列关于select模型描述的正误

加粗的为正确的  
**select模型通过函数的返回值通知I/O相关事件，故可视为通知I/O模型**
**select模型中I/O相关事件的发生时间点和函数返回的时间点一致，故不属于异步模型**  
**WSAEventSelect函数可视为select方式的异步模型，因为该函数的I/O相关事件的通知方式为异步方式**

4. 请从源代码的角度说明select函数和WSAEventSelect函数在使用上的差异

   答：使用select函数时，要循环将观察的文件描述符传到select函数中。而如果使用WSAEventSelect函数，观察的对象会被操作系统记录下俩，无需每次都注册。而且，如果是用select函数，则到事件发生为止，应保持阻塞状态；而如果使用WSAEventSelect函数，到事件发生为止，都不应该进入阻塞。
   
5. 第17章的epoll可以在条件触发和边缘触发这两种方式下工作。那种方式更适合异步I/O模型？为什么？请概括说明。

边缘触发很好地配合异步I/O模型。边缘触发模式并不会因为输入缓冲中有数据而发起消息。即在边缘触发中，对异步的存取发生过程，不会发起消息，而只注册新的输入事件。但是在条件触发中，当输入缓冲中有数据时，仍然会发起消息

6. Linux中的epoll同样属于异步I/O模型。请说明原因

epoll方式将观察的对象的连接过程和事件发生过程分离成两种处理模式。因此，可以在事件发生后，在希望的时间确认活动是否发生。因此epoll可以说是一种异步I/O模式

7. 如何获取WSAWaitForMultipleEvents可以监视的最大句柄数？请编写代码读取该值

获取WSA_MAXIMUM_WAIT_EVENTS的值即可

8. 为何异步通知I/O模型中的事件对象必须是manual-reset模式

事件的发生，需要由WSAWaitForMultipleEvents函数调用寻找相应的句柄调用执行的；首先通过调用WSAWaitForMultipleEvents函数寻找事件发生的句柄数目，然后遍历找到具体发生的事件进行处理。如果不是manual-reset模式，就会自动转换为non-signaled状态，无法用第二次的WSAWaitForMultipleEvents函数进行操作处理

9. 代码见[AsynNotiEchoServ_win_homework.c](homework/AsynNotiEchoServ_win_homework.c)



