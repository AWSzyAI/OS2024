#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <setjmp.h>
#include<stdio.h>

#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif


struct context {
    jmp_buf env;
};

void getcontext(struct context *ctx) {
    if (setjmp(ctx->env) == 0) {
        return;
    }
}

void setcontext(struct context *ctx) {
    longjmp(ctx->env, 1);
}


enum co_state{
    CO_NEW,     // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD     // 已经结束，但还未释放资源
};

#define STACK_SIZE 8192
struct co {
    const char *name;// 协程的名字,用于调试,可选,可以为NULL
    // co_start 指定的入口地址和参数,func(arg)
    void (*func)(void *);
    void *arg;
    enum co_state   status;  //协程状态
    struct co *     waiterp; // 是否有其他协程在等待当前协程,可选,可以为NULL,
                             // 用于实现co_wait,co_yield,co_start等函数
    struct context  context; // 寄存器现场,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈,用于保存当前协程的栈帧
};

struct co* current;
//协程池
// struct co* co_pool[128];
// int co_pool_count = 0;

//协程栈
struct co* co_stack[128];
int co_stack_count = 0;



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

    setjmp(co->context.env);
    // 新状态机的 %rsp 寄存器应该指向它独立的堆栈，
    // 以便在调用 co_yield 时能够恢复到这个堆栈。
    // 为了实现这一点，我们需要设置一个新的堆栈指针，
    // 并将 %rsp 寄存器指向这个新的堆栈。
    // %rip 寄存器应该指向 co_start 传递的 func 参数。
    // 根据 32/64-bit，参数也应该被保存在正确的位置 

    // struct co* p = current->waiterp;
    // while(!p){
    //     p=p->waiterp;
    // }
    // p->waiterp = co;
    debug("co_start\n");
    if(current==NULL){
        current = co;
    }else{
        co_stack[co_stack_count++] = co;
    }

    return co;
}

//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {
    // 执行co的函数
    co->status=CO_WAITING;
    debug("co_wait\n");
    co->func(co->arg); 
    co->status=CO_DEAD;
    debug("co_wait end\n");
    free(co);// 每个协程只能被 co_wait 一次
}

struct co* next_co(){
    //选择另一个状态为`CO_RUNNING`或`CO_WAITING`的协程
    struct co* co = NULL;
    for(int i=0;i<co_stack_count;i++){
        if(co_stack[i]->status==CO_RUNNING || co_stack[i]->status==CO_WAITING){
            co = co_stack[i];
            break;
        }
    }
    return co;
}


void co_yield() {
    debug("co_yield\n");

    // 1-保存当前协程的上下文 (context)，包括寄存器 (register) 和堆栈指针 (stack pointer)
    setcontext(&current->context);
    current->status = CO_RUNNING;
    
    // 2-选择一个另一个协程，
    current = next_co();

    
    // 3- 恢复选定协程的状态并运行
    getcontext(&current->context);
    
    if (current->status == CO_NEW) {
        current->status = CO_RUNNING;
        current->func(current->arg);
    } else {
        current->status = CO_RUNNING;
        setcontext(&current->context);
    }
}

