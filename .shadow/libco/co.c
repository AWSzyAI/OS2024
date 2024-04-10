#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <ucontext.h>
    // getcontext(&current->context);
    // setcontext(&current->context);
    // makecontext(&current->context, (void (*)(void))func, 1, arg);
    // swapcontext(&current->context, &co->context);


#include<stdio.h>

struct context{
    uint64_t rax;//存储函数的返回值
    

    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t r8;
    uint64_t r9;

    uint64_t r10;
    uint64_t r11;

    //callee saved/non-volatile registers/call preserved
    uint64_t rbx;
    uint64_t rbp;
    uint64_t rsp;//register stack pointer
    uint64_t r12;
    uint64_t r13;
    uint64_t r14;
    uint64_t r15;
    

    uint64_t rip;
};

#define STACK_SIZE 8192

enum co_state{
    CO_NEW,     // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD     // 已经结束，但还未释放资源
};

struct co {
    const char *name;
    void (*func)(void *);// co_start 指定的入口地址和参数
    void *arg;

    enum co_state   status;  //协程状态
    struct co *     waiterp; // 是否有其他协程在等待当前协程
    // struct context  context; // 寄存器现场
    ucontext_t context;
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈


};

struct co* current;
//协程池
struct co* co_pool[1024];
int co_pool_count = 0;


//func(arg)被 co_start() 调用，从头开始运行
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    //co会被return，所以需要malloc();来保存co的数据。
    struct co *co = malloc(sizeof(struct co));
    assert(co != NULL);

    co->name = name;
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiterp = NULL;
    // co->context.rsp = (uint64_t)co->stack + STACK_SIZE;
    
    // struct co* p = current->waiterp;
    // while(!p){
    //     p=p->waiterp;
    // }
    // p->waiterp = co;
    printf("co_start\n");
    return co;
}

//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {
    // 执行co的函数
    co->status=CO_WAITING;
    printf("co_wait\n");
    co->func(co->arg);
    co->status=CO_DEAD;
    printf("co_wait end\n");
    free(co);// 每个协程只能被 co_wait 一次
}

void co_yield() {
    // 保存当前协程的上下文 (context)，包括寄存器 (register) 和堆栈指针 (stack pointer)
    printf("co_yield\n");
    setcontext(&current->context);
    current->status = CO_RUNNING;
    //选择另一个状态为`CO_RUNNING`或`CO_WAITING`的协程
    struct co *ready_co_list[64];
    int ready_co_count = 0;
    for (int i = 0; i < co_pool_count; i++) {
        if (co_pool[i]->status == CO_RUNNING || co_pool[i]->status == CO_WAITING) {
            ready_co_list[ready_co_count++] = co_pool[i];
        }
    }
    // 如果没有其他可运行的协程，直接返回
    if (ready_co_count == 0) {
        return;
    }

    // 初始化随机数生成器
    srand(time(NULL));

    // 随机选择一个协程
    int next_co_index = rand() % ready_co_count;

    // 切换到选定的协程
    current = ready_co_list[next_co_index];
    // 恢复选定协程的状态并运行
    getcontext(&current->context);
    if (current->status == CO_NEW) {
        current->status = CO_RUNNING;
        current->func(current->arg);
    } else {
        current->status = CO_RUNNING;
        setcontext(&current->context);
    }
}

