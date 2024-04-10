#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
// #include <ucontext.h>
    // getcontext(&current->context);
    // setcontext(&current->context);
    // makecontext(&current->context, (void (*)(void))func, 1, arg);
    // swapcontext(&current->context, &co->context);


#include<stdio.h>

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif

#if __x86_64__
struct context {
    uint64_t rax, rcx, rdx, rsi, rdi, r8, r9, r10, r11, rbx, rbp, rsp, r12, r13, r14, r15, rip;
};

void getcontext(struct context *ctx) {
    asm volatile(
        "mov %%rax, %0;"
        "mov %%rcx, %1;"
        "mov %%rdx, %2;"
        "mov %%rsi, %3;"
        "mov %%rdi, %4;"
        "mov %%r8, %5;"
        "mov %%r9, %6;"
        "mov %%r10, %7;"
        "mov %%r11, %8;"
        "mov %%rbx, %9;"
        "mov %%rbp, %10;"
        "mov %%rsp, %11;"
        "mov %%r12, %12;"
        "mov %%r13, %13;"
        "mov %%r14, %14;"
        "mov %%r15, %15;"
        "lea (%%rip), %%rax;"
        "mov %%rax, %16;"
        : "=m"(ctx->rax), "=m"(ctx->rcx), "=m"(ctx->rdx), "=m"(ctx->rsi), "=m"(ctx->rdi),
          "=m"(ctx->r8), "=m"(ctx->r9), "=m"(ctx->r10), "=m"(ctx->r11), "=m"(ctx->rbx),
          "=m"(ctx->rbp), "=m"(ctx->rsp), "=m"(ctx->r12), "=m"(ctx->r13), "=m"(ctx->r14),
          "=m"(ctx->r15), "=m"(ctx->rip)
        :
        : "memory", "rax"
    );
}

void setcontext(const struct context *ctx) {
    asm volatile(
        "mov %0, %%rax;"
        "mov %1, %%rcx;"
        "mov %2, %%rdx;"
        "mov %3, %%rsi;"
        "mov %4, %%rdi;"
        "mov %5, %%r8;"
        "mov %6, %%r9;"
        "mov %7, %%r10;"
        "mov %8, %%r11;"
        "mov %9, %%rbx;"
        "mov %10, %%rbp;"
        "mov %11, %%rsp;"
        "mov %12, %%r12;"
        "mov %13, %%r13;"
        "mov %14, %%r14;"
        "mov %15, %%r15;"
        "mov %16, %%rax;"
        "jmp *%%rax;"
        :
        : "m"(ctx->rax), "m"(ctx->rcx), "m"(ctx->rdx), "m"(ctx->rsi), "m"(ctx->rdi),
          "m"(ctx->r8), "m"(ctx->r9), "m"(ctx->r10), "m"(ctx->r11), "m"(ctx->rbx),
          "m"(ctx->rbp), "m"(ctx->rsp), "m"(ctx->r12), "m"(ctx->r13), "m"(ctx->r14),
          "m"(ctx->r15), "m"(ctx->rip)
        : "rax"
    );
}
#else
struct context {
    uint32_t eax, ecx, edx, esi, edi, ebx, ebp, esp, eip;
};

void getcontext(struct context *ctx) {
    asm volatile(
        "mov %%eax, %0;"
        "mov %%ecx, %1;"
        "mov %%edx, %2;"
        "mov %%esi, %3;"
        "mov %%edi, %4;"
        "mov %%ebx, %5;"
        "mov %%ebp, %6;"
        "mov %%esp, %7;"
        "lea (%%eip), %%eax;"
        "mov %%eax, %8;"
        : "=m"(ctx->eax), "=m"(ctx->ecx), "=m"(ctx->edx), "=m"(ctx->esi), "=m"(ctx->edi),
          "=m"(ctx->ebx), "=m"(ctx->ebp), "=m"(ctx->esp), "=m"(ctx->eip)
        :
        : "memory", "eax"
    );
}

void setcontext(const struct context *ctx) {
    asm volatile(
        "mov %0, %%eax;"
        "mov %1, %%ecx;"
        "mov %2, %%edx;"
        "mov %3, %%esi;"
        "mov %4, %%edi;"
        "mov %5, %%ebx;"
        "mov %6, %%ebp;"
        "mov %7, %%esp;"
        "mov %8, %%eax;"
        "jmp *%%eax;"
        :
        : "m"(ctx->eax), "m"(ctx->ecx), "m"(ctx->edx), "m"(ctx->esi), "m"(ctx->edi),
          "m"(ctx->ebx), "m"(ctx->ebp), "m"(ctx->esp), "m"(ctx->eip)
        : "eax"
    );
}
#endif

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
    struct context  context; // 寄存器现场
    // ucontext_t context;
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈


};

struct co* current;
//协程池
struct co* co_pool[128];
int co_pool_count = 0;


//func(arg)被 co_start() 调用，从头开始运行
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    //co会被return，所以需要malloc();来保存co的数据。
    struct co *co = malloc(sizeof(struct co));
    assert(co != NULL);

#if __x86_64__
    co->context.rsp = (uint64_t)co->stack + STACK_SIZE;
    co->context.rip = (uint64_t)func;
    //(x86-64 参数在 %rdi 寄存器，而 x86 参数在堆栈中)
    co->context.rdi = (uint64_t)arg;
#else
    co->context.rsp = (uint32_t)co->stack + STACK_SIZE;
    co->context.rip = (uint32_t)func;
    co->context.rdi = (uint32_t)arg;
#endif
    // 新状态机的 %rsp 寄存器应该指向它独立的堆栈，
    // 以便在调用 co_yield 时能够恢复到这个堆栈。
    // 为了实现这一点，我们需要设置一个新的堆栈指针，
    // 并将 %rsp 寄存器指向这个新的堆栈。
    // %rip 寄存器应该指向 co_start 传递的 func 参数。
    // 根据 32/64-bit，参数也应该被保存在正确的位置 

    
    co->name = name;
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    co->waiterp = NULL;
    
    
    
    // struct co* p = current->waiterp;
    // while(!p){
    //     p=p->waiterp;
    // }
    // p->waiterp = co;
    debug("co_start\n");
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
    
    //选择一个另一个协程，
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

    // 将寄存器加载到 CPU 上    
    // 恢复选定协程的状态并运行
    getcontext(&current->context);
    printf("get context %d\n",next_co_index);
    if (current->status == CO_NEW) {
        current->status = CO_RUNNING;
        current->func(current->arg);
    } else {
        current->status = CO_RUNNING;
        setcontext(&current->context);
    }
}

