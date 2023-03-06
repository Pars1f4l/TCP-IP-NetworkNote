## 第 19 章 Windows平台下线程的使用  

### 19.1 内核对象  

#### 19.1.1 内核对象的定义  

操作系统创建的资源有很多种，如进程、线程、文件及即将介绍的信号量、互斥量。其中大部分是由程序员创建的，而且请求方式（请求中使用的函数）各不相同，但他们存在共同点： 
- 都是由Windows操作系统创建并管理的资源

不同资源类型在管理方式上也有差异。  
- 文件管理中应注册并更新文件相关的数据I/O位置，文件的打开模式
- 线程中应注册并维护线程ID、线程所属进程  

操作系统为了以记录相关信息的方式管理各种资源，在其内部生成数据块（可以认为是结构体变量）。每种资源需要维护的信息不同，所以每种资源拥有的数据块格式也有差异。这类数据块称为内核对象。  

#### 19.1.2 内核对象归操作系统所有

线程、文件等资源的创建请求均在进程内部完成，内核对象所有者是内核（操作系统）。”所有者是内核“具有如下含义：
- 内核对象的创建、管理、销毁时机的决定等工作均有操作系统完成
内核对象就是为了管理线程、文件等资源而由操作系统创建的数据块，其创建者和所有者均为操作系统。

### 19.2 基于Windwos的线程创建

#### 19.2.1 进程和线程的关系 

”程序开始运行后，调用main函数的主题是进程还是线程？“

调用main函数的主题是线程！实际上，因为早期的操作系统不支持线程，为了创建线程，经常需要特殊的库函数支持。操作系统无法意识到线程的存在，而进程实际上成为运行的最小单位。需要线程的程序员利用特殊的哭喊是，以拆分进程运行时间的方式创建线程。现代的LInux系列、Windows系列及各种规模不等的操作系统都在操作系统级别的支持线程。

非显式创建线程的程序（基于select的服务器端）可描述如下：
- 单一线程模型的应用程序  
  显式创建线程的程序可描述如下：
- 多线程模型的应用程序  

#### 19.2.2 Windows中线程的创建方法

Windows中创建线程的函数，操作系统为了管理这些资源也将同时创建内核对象。最后返回用于区分内核对象的整数型句柄：

```c
#include <windows.h>

HANDLE CreateThread(
        LPSECURITY_ATTRIBUTES IpThreadAttributes,
        SIZE_T dwStackSize,
        LPTHREAD_START_ROUTINE IpStartAddress,
        LPCOID IpParameter,
        DWORD dwCreationFlags,
        LPDWORD IpThreadId
        );
/*
成功时返回线程句柄，失败时返回 NULL
 IpThreadAttributes ： 线程安全相关信息，使用默认设置时传递NULL
 dwStackSize ： 要分配给线程的栈大小，传递0时生成默认大小的栈
 IpStartAddress ：传递线程的main函数信息
 IpParameter ： 调用main函数时传递的参数信息
 dwCreationFlags ： 用于指定线程创建后的行为，传递0时，线程创建后立即进入可执行状态
 IpThreadId ：用于保存线程ID的变量地址值
*/
```

Windows线程的销毁时间点  
> Windows线程在首次调用的线程main函数返回时销毁，（销毁时间点和销毁方法与Linux不同）。还有其他方法可以终止线程，但最好的方法就是让线程main函数终止（返回）。

#### 19.2.3 编写多线程程序的环境设置

VC++环境下需要设置”C/C++ Runtime Library“(CRT), 这是调用C/C++标准函数时必需的库。过去的VC++6.0版默认只包含支持单线程的库，需要自行配置。在菜单中选择”项目“->“属性”，或使用快捷键Alt+F7打开库的配置页，即可看到相关界面。  

#### 19.2.4 创建“使用线程安全标准C函数”的线程

之前介绍过创建线程时使用的CreateThread函数，如果线程要调用C/C++标准函数，需要通过如下方法创建线程。因为通过CreateThread函数调用创建出的线程在使用C/C++标准函数时并不稳定。


```c
#include <process.h>

uintptr_t beginthreadex(
        void * security,
        unsigned stack_size,
        unsigned (* start_address)(void *),
        void * arglist,
        unsigned initflag,
        unsigned * thrdaddr
        );
/*
成功时返回线程句柄，失败时返回 0
*/
```

上述函数与之前的CreateThread函数相比，参数个数及各参数的含义和顺序均相同，只是变量名和参数类型有所不同。因此，用上述函数替换CreateThread函数时，只需适当更改数据类型。

> 如果查阅_beginthreadex函数相关资料，会发现还有更好用的_beginthread函数。但该函数的问题在于，他会让创建线程是返回的句柄失效，以防止访问内核对象。_beginthreadex就是为了解决这一问题而定义的函数。  

上述函数的返回值类型uintptr_t是64位unsigned整数型。下述示例通过声明CreateThread函数的返回值类型HANDLE（同样是整数型）保存返回的线程句柄。  
[thrad1_win.c](thread1_win.c) 

运行结果：thread1_win.c  
running thread  
running thread  
end of main

与Linux相同，Windows 同样在main函数返回后终止进程，也同时终止其中包含的所有线程。


### 19.3 内核对象的2种状态

资源类型不同，内核对象也含有不同信息。其中， 应用程序实现过程中需要特别关注的信息被赋予某种状态。例如，线程内核对象中需要重点关注线程是否已终止，所以终止状态又称“signaled状态”，未终止状态称为“non-signaled状态”。

#### 19.3.1 内核对象的状态及状态查看 

内核对象的signaled、non-signaled状态通过1个boolean变量表示。内核对象带有1个boolean变量、其初始值为FALSE，此时的状态就是non-signaled状态。如果发生约定的情况，会将变量改为TRUE，此时状态就是signaled状态。内核对象类型不同，进入signaled状态的情况也有所区别。

系统定义了如下函数验证状态：  
WaitForSingleObject函数针对单个内核对象验证signled状态
```c
#include <windows.h>

DWORD WaitForSingleObject(HANDLE hHandle, DWORD dwMilliseconds);
/*
 * 成功时返回事件信息，失败时返回WAIT_FAILED。
 * hHandle : 查看状态的内核对象句柄
 * dwMilliseconds ：以1/1000秒为单位指定超时，传递INFINITE时函数不会返回，直到内核对象变成signled状态
 * 返回值：进入signaled状态返回WAIT_OBJECT_0,超市返回WAIT_TIMEOUT。
 */
```

该函数由于发生事件返回时，有时会把相应内核对象再次改为non—signaled状态。这种可以再次进入non-signaled状态的内核对象称为“auto-reset模式”的呢黑对象，而不会自动跳转到non-signaled状态的对象称为“manual-reset模式”的内核对象。  


下面的函数可以验证多个内核对象状态：
```c
#include <windows.h>

DWORD WaitForMultipleObjects(DWORD nCOUNT, const HANDLE * IpHandles, BOOL bWaitAll, DWORD dwMilliseconds);
/*
 * nCount ：需验证的内核对象数
 * IpHandles ：存有内核对象句柄的数组地址值
 * bWaitAll ：如果为TRUE,则所有内核对象全部变为signaled时返回；如果为FALSE，则只要有1个验证对象的状态变为signaled就会返回
 * dwMilliseconds ：以1/1000秒为单位指定超时，传递INFINITE时函数不会返回，知道内核对象变为signaled状态
 */
```

利用WaitForSingleObject函数解决thread1_win.c的问题，示例：
[thread2_win.c](thread2_win.c)

运行结果：   
running thread  
running thread  
running thread  
running thread  
running thread  
wait result: signaled  
end of main  

结果显示，存在的问题得到解决。  


下面示例使用了WaitForMultipleObjects函数，是第18章thread4.c示例的Windows版。  

[thread3_win.c](thread3_win.c)

运行结果：  
sizeof long long: 8   
result: -10849152  

每次结果都不同，可以利用20章的同步技术得到预想的结果


### 19.4 习题

1. 下列关于内核对象的说法错误的是？  
    加粗部分为正确的  
   **a.内核对象是操作系统保存各种资源信息的数据块**   
   b.内核对象的所有者是创建该内核对象的进程。  
   c.由用户进程创建并管理内核对象  
   d.无论操作系统创建和管理的资源类型是什么，内核对象的数据块结构都完全相同。  

2.现代操作系统大部分都在操作系统级别支持线程。根据该情况判断下列描述中错误的是？
加粗部分为正确的  
**a 调用main函数的也是线程**   
b 如果进程不创建线程，则进程内不存在任何线程。   
**c 多线程模型是进程内可以创建额外线程的程序类型。**  
d 单一线程模型是进程内只额外创建1个线程的程序模型   


3.请比较从内存中完全销毁Windows线程和Linux线程的方法  

Windows上的线程销毁是随其线程main函数的返回，在内存中自动销毁的。但是linux的线程销毁必须经过pthread_join函数或者pthread_detach函数的响应才能在内存空间中完全销毁


4.通过线程创建过程解释内核对象、线程、句柄之间的关系  

线程也属于操作系统的资源，因此会伴随着内核对象的创建，并为了引用内核对象而返回句柄。整理的话，可以通过句柄区分内核对象，通过内核对象区分线程  

5.判断下列关于内核对象描述的正误  
加粗部分为正确的   
**内核对象只有signaled和non-signaled这两种状态。**     
内核对象需要转为signaled状态时，需要程序员亲自将内核对象的状态改为signaled状态。      
线程的内核对象在线程运行时处于signaled状态，线程终止则进入non-signaled状态。    


6.请解释“auto-reset模式”和“manual-reset模式”的内核对象。区分二者的内核对象特征是什么

WaitForSingleObject函数针对单个内核对象验证signaled状态时，由于发生事件(变为signaled状态)返回时，有时会把相应内核对象再次改为non-signaled状态。这种可以再次进入non-signaled状态的内核对象称为"auto-reset模式"的内核对象，而不会自动跳转到non-signaled状态的内核对象称为"manual-reset模式"的内核对象。

