#include "co.h"
#include <stdlib.h>
#include <stdint.h>

struct context{
    uint64_t rax;
    uint64_t rbx;
    uint64_t rcx;
    uint64_t rdx;
    uint64_t rsi;
    uint64_t rdi;
    uint64_t rbp;
    uint64_t rsp;//register stack pointer
    uint64_t r8;
    uint64_t r9;
    uint64_t r10;
    uint64_t r11;
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
    char *name;
    void (*func)(void *);// co_start 指定的入口地址和参数
    void *arg;

    enum co_state   status;  //协程状态
    struct co *     waiterp; // 是否有其他协程在等待当前协程
    struct context  context; // 寄存器现场
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈


};

struct co* current;

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    //malloc();
    struct co *co = malloc(sizeof(struct co));
    if(co == NULL){
        return;
    }
    co->name = name;
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiterp = NULL;
    co->context.rsp = (uint64_t)co->stack + STACK_SIZE;
    
    struct co* p = current->waiterp;
    while(!p){
        p=p->waiterp;
    }
    p->waiterp = co;

    return co;
}

void co_wait(struct co *co) {
    //当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)

    free(co);
}

void co_yield() {
    //当前协程主动让出 CPU，调度执行其他协程
    // 把寄存器 (包括 Stack Pointer) 都保存下来，然后恢复另一个协程保存的寄存器 (和 Stack Pointer)
    
    current->status = CO_RUNNING;
    // 为每一个协程分配独立的堆栈 (stack)
    


    // 保存当前协程的上下文 (context)，包括寄存器 (register) 和堆栈指针 (stack pointer)

    // 保存当前协程的栈基址
    current->context.rbp = (uint64_t)current->stack + STACK_SIZE; //栈底的指针
    // 保存当前协程的程序计数器
    asm volatile(
        "movq %%rsp, %0\n"
        "movq %%rbp, %1\n"
        "movq $1f, %2\n"
        : "=m"(current->context.rsp), "=m"(current->context.rbp), "=m"(current->context.rip)
    );
    // 保存当前协程的其他寄存器
    asm volatile(
        "movq %%rax, %0\n"
        "movq %%rbx, %1\n"
        "movq %%rcx, %2\n"
        "movq %%rdx, %3\n"
        "movq %%rsi, %4\n"
        "movq %%rdi, %5\n"
        "movq %%r8, %6\n"
        "movq %%r9, %7\n"
        "movq %%r10, %8\n"
        "movq %%r11, %9\n"
        "movq %%r12, %10\n"
        "movq %%r13, %11\n"
        "movq %%r14, %12\n"
        "movq %%r15, %13\n"
        : "=m"(current->context.rax), "=m"(current->context.rbx), "=m"(current->context.rcx), "=m"(current->context.rdx), "=m"(current->context.rsi), "=m"(current->context.rdi), "=m"(current->context.r8), "=m"(current->context.r9), "=m"(current->context.r10), "=m"(current->context.r11), "=m"(current->context.r12), "=m"(current->context.r13), "=m"(current->context.r14), "=m"(current->context.r15)
    );
    // 保存当前协程的状态
    current->status = CO_WAITING;
    // 切换到下一个协程
    // 从队列中找到下一个协程
    // 恢复下一个协程的上下文 (context)，包括寄存器 (register) 和堆栈指针 (stack pointer)
    // 恢复下一个协程的栈指针
    // 恢复下一个协程的栈基址
    // 恢复下一个协程的程序计数器
    // 恢复下一个协程的其他寄存器
    // 恢复下一个协程的状态
    // 跳转到下一个协程的程序计数器
    // 从下一个协程的程序计数器开始执行
    
    
    
}

