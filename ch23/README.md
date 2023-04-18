## 第 23 章 IOCP

### 23.1 通过重叠I/O理解IOCP

本章的IOCP（Input Output Completion Port，输入输出完成端口）服务器端模型是很多Windows程序员关注的焦点。

#### 23.1.1 epoll和IOCP的性能比较

为了突破select等传统I/O模型的极限，每种操作系统（内核级别）都会提供特有的I/O模型以提高性能。其中 最具代表性的有Linux的epoll，BSD的kqueue及本章的Windows的IOCP。他们都在操作系统级别提供支持并完成功能。

在硬件性能和分配带宽充足的情况下，如果响应时间和并发数量出了问题，修正如下两点会解决大部分问题。
- 低效的I/O结构或低效的CPU使用
- 数据库设计和查询语句的结构

#### 23.1.2 实现非阻塞模式的套接字

利用重叠I/O模型实现回声服务器端。首先介绍创建非阻塞模式套接字的方法。在Windows中通过如下函数调用将套接字属性改为非阻塞模式。
```c
SOCKET hLisnSock,
int mode = 1;
......
hLisnSock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
ioctlsocket(hLisnSock, FIONBIO, &mode);// for non-bocking socket
```

上述代码中调用的ioctlsocket函数负责控制套接字I/O方式，其调用具有如下含义：  
“将hLisnSock句柄引用的套接字I/O模式（FIONBIO）改为变量mode中指定的形式”

也就是说，FIONBIO是用于更改套接字I/O模式的选项，该函数的第三个参数中传入的变量中若存有0，则说明套接字是阻塞模式的；如果存有非0值，则说明已将套接字模式改为非阻塞模式。改为非阻塞模式后，除了以非阻塞模式进行I/O外，还具有如下特点。
- 如果在没有客户端连接请求的状态下调用accept函数，将直接返回INVALID_SOCKET。调用WSAGetLastError函数时返回WSAEWOULDBLOCK。
- 调用accept函数时创建的套接字同样具有非阻塞属性

因此，针对非阻塞套接字调用accept函数并返回INVALID_SOCKET时，应该通过WSAGetLastError函数确认返回INVALID_SOCKET的理由，再进行适当处理。

#### 23.1.3 以纯重叠I/O方式实现回声服务器端

要想实现基于重叠I/O的服务器端，必须具备非阻塞套接字，所以先介绍了其创建方法。实际上，因为有IOCP模型，所以很少有人只用重叠I/O实现服务器端。  
即使坚持不用IOCP，也应具备仅用重叠I/O方式实现类似IOCP的服务器端的能力。这样就可以在其他操作系统平台实现类似IOCP方式的服务器端，而且不会因IOCP的限制而忽略服务器端功能的实现。  

下面使用纯重叠I/O模型实现回声服务器端，
[CmplRouEchoServ_win.c](CmplRouEchoServ_win.c)

上述示例的工作原理整理如下：
- 有新的客户端连接时调用WSARecv函数，并以非阻塞模式接收数据，接收完成后调用ReadCompRoutine函数
- 调用ReadCompRoutine函数后调用WSASend函数，并以非阻塞模式发送数据，发送完成后调用WriteCompRoutine函数
- 此时调用的WriteCompRoutine函数将再次调用WSARecv函数，并以非阻塞模式等待接收数据

通过交替调用ReadCompRoutine函数和WriteCompRoutine函数，反复执行数据的接收和发送操作。另外，每次增加1个客户端都会定义PER_IO_DATA结构体，以便将新创建的套接字句柄和缓冲信息传递给ReadCompRoutine函数和WriteCompRoutine函数。同时将该结构体地址值写入WSAOVERLAPPED结构体成员hEvent，并传递给Completion Routine函数。可概括如下：
- 使用WSAOVERLAPPED结构体成员hEvent向完成I/O时自动调用的Completion Routine函数内部传递客户端信息（套接字和缓冲）。

#### 23.1.4 重新实现客户端

其实第4章实现并使用至今的回声客户端存在一些问题，关于这些问题及解决方案已在第5章进行了充分讲解。虽然在目前为止的各种模型的服务器端中使用稍有缺陷的回声客户端也不会引起太大问题，但本章的回声服务器端则不同。因此需要按照第5章的提示解决客户端存在的问题，并结合改进后的客户端运行本章服务器端。

代码如下：[StableEchoClnt_win](StableEchoClnt_win.c)

上述代码第44行的循环语句考虑到TCP的传输特性而重复调用了recv函数，直至接受完所有数据。将上述客户端结合之前的回声服务器端运行可以得到正确的运行结果。

#### 23.1.5 从重叠I/O模型到IOCP模型

下面分析重叠I/O模型回声服务器端的缺点：
- 重复调用非阻塞模式的accept函数和以进入alertable wait状态为目的的SleepEx函数将影响性能

既不能为了处理连接请求而只调用accept函数，也不能为了Completion Routine而只调用SleepEx函数，因此轮流调用了非阻塞模式的accept函数和SleepEx函数（设置较短的超时时间）。这恰恰是影响性能的代码结构。

可以考虑如下方法：
- 让main线程调用accept函数，再单独创建1个线程负责客户端I/O。

这就是IOCP中采用的服务器端模型。换言之，IOCP将创建专用的I/O线程，该线程负责与所有客户端进行I/O。

> 负责全部I/O
> IOCP为了负责全部I/O工作，的确需要创建至少1个线程。但“再服务器端负责全部I/O”实质上相当于负责服务器端的全部工作，因此，它并不是仅负责I/O工作，而是至少创建1个线程并使其负责全部I/O的前后处理。尝试理解IOCP时不要把焦点集中于线程，二十注意观察如下两点：
> - I/O是否以非阻塞模式工作？
> - 如何确定非阻塞模式的I/O是否完成？
> 之前讲过的所有I/O模型及本章的IOCP模型都可以按上述标准进行区分。

### 23.2 分阶段实现IOCP程序

#### 23.2.1 创建“完成端口”

IOCP中已完成的I/O信息将注册到完成端口对象（Completion Port，简称CP对象），但这个过程并非单纯的注册，首先需要经过如下请求过程：
- 该套接字的I/O完成时，请把状态信息注册到指定CP对象

该过程称为“套接字和CP对象之间的连接请求”。因此，为了实现基于IOCP模型的服务器端，需要做如下2项工作。
- 创建完成端口对象
- 建立完成端口对象和套接字之间的联系

此时的套接字必须被赋予重叠属性。上述两项工作可以通过1个函数完成，但为了创建CP对象，先介绍如下函数。
```c
#include <windows.h>

HANDLE CreateIoCompletionPort(
        HANDLE FileHandle, HANDLE ExistiogCompletionPort, ULONG_PTR CompletionKey, DWORD NumberOfConcurrentThreads
        );
//成功时返回CP对象句柄，失败时返回NULL
/*
 * FileHandle：创建CP对象时传递INVALID_HANDLE_VALUE
 * ExistingCompletionPort：创建CP对象时传递NULL
 * CompletionKey：创建CP对象时传递0
 * NumberOfConcurrentThreads：分配给CP对象的用于处理I/O的线程数。例如，该参数为2时，说明分配给CP对象的可以同时运行的线程数最多为2个；如果该参数为0，系统中CPU个数就是可同时运行的最大线程数
 * */
```

以创建CP对象为目的调用上述函数时，只有最后一个参数才真正具有含义。

#### 23.2.2 连接完成端口对象和套接字

既然有了CP对象，接下来就要将该对象连接到套接字，只有这样才能使已完成的套接字I/O信息注册到CP对象。下面以建立连接为目的再次介绍CreateIoCompletionPort函数。

```c
#include <windows.h>

HANDLE CreateIoCompletionPort(
        HANDLE FileHandle, HANDLE ExistiogCompletionPort, ULONG_PTR CompletionKey, DWORD NumberOfConcurrentThreads
        );
//成功时返回CP对象句柄，失败时返回NULL
/*
 * FileHandle：要连接到CP对象的套接字句柄
 * ExistingCompletionPort：要连接套接字的CP对象句柄
 * CompletionKey：传递已完成I/O相关信息，关于该参数将在稍后介绍的GetQueuedCompletionStatus函数中共同讨论
 * NumberOfConcurrentThreads：无论传递何值，只要该函数的第二个参数非NULL就会忽略
 * */
```

上述函数的第二种功能就是将FileHandle句柄指向的套接字和ExistingCompletionPort指向的CP对象相连。

调用CreateIoCompletionPort函数后，只要针对hSock的I/O完成，相关信息就将注册到hCpObject指向的CP对象。

#### 23.2.3 确认完成端口已完成的I/O和线程的I/O处理

我们已经掌握了CP对象的创建及其与套接字建立连接的方法，接下来就要学习如何确认CP中注册的已完成的I/O。完成该功能的函数如下：

```c
#include <windows.h>

BOOL GetQueuedCompletionStatus(
        HANDLE CompletionPort, LPDWORD lpNumberOfBytes, PULONG_PTR lpCompletionKey, LPOVERLAPPED * lpOverlapped, DWORD dwMilliseconds
        );
/*
 * 成功时返回TRUE,失败时返回FALSE
 * 注册有已完成I/O信息的CP对象句柄
 * 用于保存I/O过程中传输的数据大小的变量地址值
 * 用于保存CreateIoCompletionPort函数的第三个参数值的变量地址值
 * 用于保存调用WSASend、WSARecv函数时传递的OCERLAPPED结构体地址的变量地址值。
 * 超时信息，超过该指定时间后将返回FALSE并跳出函数。传递INFINITE时，程序将阻塞，知道已完成I/O信息写入CP对象。
 * */
```

虽然只介绍了2个IOCP相关函数，但依然有些复杂，特别是上述函数的第三个和第四个参数更是如此。其实这2个参数主要是为了获取需要的信息而设置的，下面介绍这2种信息的含义。
-  通过GetQueuedCompletionStatus函数的第三个参数得到的是以连接套接字和CP对象为目的而调用的CreateCompletionPort函数的第三个参数值。
- 通过GetQueuedCompletionStatus函数的第四个参数得到的是调用WSASend、WSARecv函数时传入的WSAOVERLAPPED结构体变量地址值

如前所述，IOCP中将创建全职I/O线程，由该线程针对所有客户端进行I/O。而且CreateIoCompletionPort函数中也有参数用于指定分配给CP对象的最大线程数，所以各位或许会有如下疑问：
- 是否自动创建线程并处理I/O？

应该由程序员自行创建调用WSASend、WSARecv等I/O函数的线程，只是该线程为了确认I/O的完成会调用GetQueuedCompletionStatus函数。虽然任何线程都能调用GetQueuedCompletionStatus函数，但实际得到I/O完成信息的线程数不会超过调用GetQueuedCompletionStatus函数时指定的最大线程数。

> 分配给完成端口的合理的线程个数
> 分配给CP对象的合理的线程数应该与CPU个数相同，这是MSDN中的说明。但现在主流的CPU核数（具有运算功能的内核）大部分都是2个或更多。核数和CPU在概念上有差异，但如果是双核则可以同时运行2个线程，因此，分配给CP对象的合理的线程数可以理解为CPU中的核数。一般人不会可以区分核与CPU，而是把双核视为2个CPU。在多核技术相当成熟的背景下，这种看法不失为一种合理的选择。若想准确掌握发挥最佳性能的线程数，只能通过大量实验得到。

#### 23.2.4 实现基于IOCP的回声服务器端

虽然介绍了IOCP相关的理论知识，但离开示例很难真正掌握IOCP的使用方法。

因此，下面介绍便于理解和运用的基于IOCP的回声服务器端。
示例：[IOCPEchoServ_win.c](IOCPEchoServ_win.c)

#### 23.2.5 IOCP性能更优的原因

将其在代码级别与select进行对比，可以发现如下特点：
- 因为是非阻塞模式的I/O，所以不会由I/O引发延迟
- 查找已完成I/O时无需添加循环
- 无需将作为I/O对象的套接字句柄保存到数组进行管理
- 可以调整处理I/O的线程数，所以可在实验数据的基础上选用合适的线程数

### 23.3 习题



