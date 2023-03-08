## 第 20 章 Windows中的线程同步

### 20.1 同步方法的分类及CRITICAL_SECTION同步

#### 20.1.1 用户模式和内核模式

Windows操作系统的运行模式是双模式操作，Windows运行过程中存在两种模式：
- 用户模式：运行应用程序的基本模式，禁止访问物理设备，而且会限制访问的内存区域
- 内核模式：操作系统运行时的模式，不仅不会限制访问的内存区域，而且访问的硬件设备也不会受限

内核是操作系统的核心模块，可以简单定义为如下形式：
- 用户模式：应用程序的运行模式
- 内核模式：操作系统的运行模式

在应用程序运行过程中，Windows操作系统不会一直停留在用户模式，而是在用户模式和内核模式之间切换。例如，创建线程的请求是由应用程序的函数调用完成，但实际创建线程的是操作系统。因此无法避免向内核模式的转换。

定义两种模式能够提高安全性。应用程序的运行时错误会破坏操作系统及各种资源。但即使遇到错误的指针运算也仅停止应用程序的运行，而不会影响操作系统。像线程这种伴随着内核对象创建的资源创建过程中，都要默认经历图下模式转换过程：
- 用户模式->内核模式->用户模式

从用户模式切换到内核模式是为了创建资源，从内核模式再次切换到用户模式是为了执行应用程序的剩余部分。不仅是资源的创建，与内核对象有关的所有事务都在内核模式下进行。

#### 20.1.2 用户模式同步

用户模式同步是用户模式下进行的同步，即无需操作系统的帮助而在应用程序级别进行的同步。用户模式同步最大的优点是——速度快。无需切换到内核模式，而且使用方法相对简单。但因为这种同步方法不会借助操作系统的力量，其功能上存在一定局限性。稍后将介绍属于用户模式同步的、基于“CRITICAL_SECTION”的同步方法。

#### 20.1.3 内核模式同步

内核模式同步的优点：
- 比用户模式同步提供的功能更多
- 可以指定超时，防止产生死锁

因为是通过操作系统的帮助完成同步的，所以提供更多功能。在内核模式同步中，可以跨越进程进行线程同步。与此同时，由于无法避免用户模式和内核模式之间的切换，所以性能上会受到一定影响。

> 死锁  
> 发生死锁时，等待进入临界区的阻塞状态的线程无法退出。死锁发生的原因有很多，以第18章的Mutex为例，调用pthread_mutex_lock函数进入临界区的线程如果不调用pthread_mutex_unlock函数，将发生死锁。

#### 20.1.4 基于CRITICAL_SECTION的同步

基于CRITICAL_SECTION的同步中将创建并运用“CRITICAL_SECTION”对象，但这并非内核对象。为了进入临界区，需要得到CRITICAL_SECTION对象这把钥匙，相反，离开时应上交CRITICAL_SECTION对象。

下面是CS对象的初始化及销毁相关函数

```c
#include <windows.h>

void InitializeCriticalSection(LPCRITICAL_SECTION IpCriticalSection);
void DeleteCriticalSection(LPCRITICAL_SECTION IpCriticalSection);
//IpCriticalSection Init...函数中传入需要初始化的CRITICAL_SECTION对象的地址值，反之，Del..函数中传入需要解除的CRITICAL_SECTION对象的地址值。
```
 
上述函数的参数类型LPCRITICAL_SECTION是CRITICAL_SECTION指针类型。另外DeleteCriticalSection并不是销毁CRITICAL_SECTION对象的函数。该函数的作用是销毁CRITICAL_SECTION对象使用过的资源。

下面是获取和释放CS对象的函数，可以简单理解为获取和释放钥匙的函数。

```c
#include <windows.h>

void EnterCriticalSection(LPCRITICAL_SECTION IpCriticalSection);
void LeaveCriticalSection(LPCRITICAL_SECTION IpCriticalSection);
//IpCriticalSection 获取和释放的CRITICAL_SECTION对象的地址值
```

下面是利用CS对象将第19章的示例thread3_win.c改为同步程序。

[SyncCS_win.c](SyncCS_win.c)

运行结果：
result：0

程序中将整个循环内如临界区，主要是为了减少运行时间。如果只将访问num的语句纳入临界区，那将不知何时才能得到运行结果，因为这将导致大量获取和释放CS对象。

### 20.2 内核模式的同步方法

典型的内核模式同步方法有基于事件、信号量、互斥量等内核对象的同步。

#### 20.2.1 基于互斥量对象的同步

基于互斥量对象的同步方法与基于CS对象的同步方法类似。
下面是创建互斥量对象的函数。

```c
#include <windows.h>

HANDLE CreateMutex(
        LPSECURITY_ATTRIBUTES lpMutexAttributes, BOOL bInitialOwner, LPCTESTR lpName
        );
/*
 * 成功时返回创建的互斥量对象句柄，失败时返回NULL
 * 
 * IpMutexAttributes 传递安全相关的配置信息，使用默认安全设置时可以传递NULL
 * bInitialOwner 如果为TRUE，则创建出的互斥量对象属于调用该函数的线程，同时进入non-signaled状态；如果为FALSE，则创建出的互斥量对象不属于任何线程，此时状态为signaled。
 * lpName 用于命名互斥量对象。传入NULL时创建无名的互斥量对象
 * */
```

从上述参数说明可以看到，如果互斥量对象不属于任何拥有者，则将进入signaled状态。利用该特点进行同步。
互斥量属于内核对象，通过如下函数销毁。

```c
#include <windows.h>

BOOL CloseHandle(HANDLE hObject);
/*
 * 成功时返回TRUE,失败时返回FALSE
 * 
 * hObject 要销毁的内核对象的句柄
 * */
```

上述函数时销毁内核对象的函数，所以同样可以销毁即将介绍的信号量及事件。
获取互斥量的函数时WaitForSingleObject函数，释放的函数如下

```c
#include <windows.h>

BOOL ReleaseMutex(HANDLE hMutex);
/*
 * 成功时返回TRUE,失败时返回FALSE
 * 
 * hMutex 需要释放的互斥量对象句柄
 * */
```

互斥量被某一线程获取时为non-signaled状态，释放时进入signaled状态。因此，可以使用WaitForSingleObject函数验证互斥量是否已分配。该函数的调用结果有如下两种
- 调用后进入阻塞状态：互斥量对象已被其他线程获取，现处于non-signaled状态
- 调用后直接返回：其他线程未占用互斥量对象，现处于signaled状态。

互斥量在WaitForSingleObject函数返回时自动进入non-signaled状态，因为它是第19章介绍过的“auto-reset”模式的内核对象。

基于互斥量的临界区保护代码如下：
```c
WaitForSingleObject(hMutex, INFINITE);
//临界区的开始
//.......
//临界区的结束
ReleaseMutex(hMutex);
```

WaitForSingleObject函数使互斥量进入non-signaled状态，限制访问临界区，所以相当于临界区的门禁系统。相反，ReleaseMutex函数使互斥量重新进入signal状态，相当于临界区的出口。

示例：[SyncMutex_win.c](SyncMutex_win.c)

运行结果：SyncMutex_win.C  
result: 0

#### 20.2.2 基于信号量对象的同步

Windows中基于信号量对象的同步也与Linux下的信号量类似，两者都是利用名为“信号量值”的整数值完成同步的，而且该值都不能小于0。

下面是创建信号量对象的函数，销毁同样是利用CloseHandle函数
```c
#include <windows.h>

HANDLE CreateSemaphore(
        LPSECURITY_ATTRIBUTES lpSemaphoreAttributes, LONG lInitialCount, LONG lMaximumCount, LPCTSTR lpName
        );
/*
 * 成功时返回创建的信号量对象的句柄，失败时返回NULL
 * lpSemaphoreAttributes 安全配置信息，采用默认安全设置时传递NULL
 * lInitialCount 指定信号量的初始值，应大于0小于lMaximumCount
 * lMaximumCount 信号量的最大值。该值为1时，信号量变为只能表示0和1的二进制信号量
 * lpName 用于命名信号量对象。传递NULL时创建无名的信号量对象
 * */
```

可以利用信号量值为0时进入non-signaled状态，大于0进入signaled状态的特性进行同步。   
下面时释放信号量对象的函数
```c
#include <windows.h>

BOOL ReleaseSemaphore(
        HANDLE hSemaphore, LONG lReleaseCount, LPLONG lpPreviousCount
        );
/*
 * 成功时返回TRUE，失败时返回FALSE
 * hSemaphore 传递需要释放的信号量对象
 * lReleaseCount 释放意味着信号量值的增加，通过该参数可以指定增加的值。超过最大值则不增加，返回FALSE。
 * lpPreviousCount 用于保存修改之前值的变量地址，不需要时可传递NULL
 * */
```

信号量对象的值大于0时称为signaled状态，为0时成为non-signaled状态。因此调用WaitForSingleObject函数时，信号量大于0的情况才会返回。返回的同时将信号量值减1，同时进入non-signaled状态（仅限于信号量减1后等于0的情况）。    
基于信号量的临界区保护代码如下：
```c
WaitForSingleObject(hSemaphore, INFINITE);
//临界区的开始
//.......
//临界区的结束
ReleaseMutex(hSemaphore, 1, NULL);
```

下面时示例：
[SyncSema_win.c](SyncSema_win.c)

运行结果：     
Input num:1        
Input num:2     
Input num:3       
Input num:4        
Input num:5         
Result: 15       

#### 20.2.3 基于事件对象的同步

事件同步对象与前2种同步方法相比有很大不同，区别就在于，该方式下创建对象时，可以在自动以non-signaled状态运行的auto-reset模式和与之相反的manual-reset模式任选其一。而事件对象的主要特点是可以创建manual-reset模式的对象。

下面是用于创建事件对象的函数：
```c
#include <windows.h>

HANDLE CreateEvent(
LPSECURITY_ATTRIBUTES lpEventAttributes, BOOL nManualReset, BOOL bInitialState, LPCTSTR lpName
);
/*
* 成功时返回创建的事件对象的句柄，失败时返回NULL
* 
* lpEventAttributes 安全配置信息，采用默认安全设置时传递NULL
* nManualReset 传入TRUE时创建manual-reset模式的事件对象，传入FALSE时创建auto-reset模式的事件对象
* bInitialState 传入TRUE时创建signaled状态的事件对象，传入FALSE时创建non-signaled状态的事件对象
* lpName 用于命名信号量对象。传递NULL时创建无名的信号量对象
* */
```

在向第二个参数传入TRUE时创建manual-reset模式的事件对象，此时即使WaitForSingleObject函数返回也不会回到non-signaled状态。  
因此通过如下2个函数明确更改对象状态
```c
#include <windows.h>

BOOL ResetEvent(HANDLE hEvent); //to the non-signaled
BOOL SetEvent(HANDLE hEvent); //to the signaled
/*
* 成功时返回TRUE，失败时返回FALSE
* */
```

传递事件对象句柄并希望改为non-signaled状态时，应调用ResetEvent函数。如果希望改为signaled状态，则可以调用SetEvent函数。

示例：
[SyncEvent_win.c](SyncEvent_win.c)

运行结果：        
Input string:ABCDABC   
Num of A: 2     
Num of others: 5      

### 20.3 Windows平台下实现多线程服务器端

第18章讲完线程的创建和同步方法后，最终实现了多线程聊天服务器端和客户端。按照这种顺序，本章最后也将在Windwos平台下实现聊天服务器端和客户端。

下面是聊天服务器端的源代码：
[chat_serv_win.c](chat_serv_win.c)

下面是聊天客户端：
[chat_clnt_win.c](chat_clnt_win.c)

### 20.4 习题

1. 关于Windows操作系统的用户模式和内核模式的说法中正确的是？      
加粗部分为正确的  
a.用户模式是应用程序运行的基本模式，虽然访问的内存空间没有限制，但无法访问物理设备  
b.应用程序运行过程中绝对不会进入内核模式。应用程序只在用户模式进行  
**c.Windows为了有效使用内存空间，分别定义了用户模式和内核模式**  
d.应用程序运行过程中也有可能切换到内核模式，只是切换到内核模式后，进程将一直保持该状态  

2. 判断下列关于用户模式同步和内核模式同步描述的正误？       
加粗为正确的      
**用户模式的同步中不会切换到内核模式。即非操作系统级别的同步**       
**内核模式的同步是由操作系统提供的功能，比用户模式同步提供更多的功能**     
**需要在用户模式和内核模式之间切换，这是内核模式同步的缺点**          
除特殊情况外，原则上应使用内核模式同步。用户模式同步是操作系统提供内核模式同步机制前使用的同步方法。
       
3. 本章示例中的SyncSema_win.c的Read函数中，退出临界区需要较长时间，请给出解决方案并实现。   
    解决方案：将用户输入的num保存在一个全局数组中，这样只需要调用一次互斥量就能保证正确计算  
    实现：[SyncSema_win_homework.c](homework/SyncSema_win_homework.c)

4. 请将本章SyncEvent_win.c示例改为基于信号量的同步方式，并得出相同运行结果  
    实现：[SyncEvent_win_homework.c](homework/SyncEvent_win_homework.c)
