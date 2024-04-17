#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <time.h>

#include <setjmp.h>
#include <ucontext.h>

#include <assert.h>
/*
    debug
    ✅ CFLAGS += -DLOCAL_MACHINE
    ❌ CFLAGS += -ULOCAL_MACHINE
*/
#ifdef LOCAL_MACHINE
    #define debug(...) printf(__VA_ARGS__)
#else
    #define debug(...)
#endif



enum co_state{
    CO_NEW,     // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD     // 已经结束，但还未释放资源
};
#define STACK_SIZE 8192
struct co {
    const char *name;// 协程的名字,用于调试,可选,可以为NULL
    void (*func)(void *);
    void *arg;
    enum co_state   status;  //协程状态
    ucontext_t     context;    // ucontext_t 结构,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈,用于保存当前协程的栈帧
    // struct co *     waiterp; // 是否有其他协程在等待当前协程,可选,可以为NULL,
    // struct context  context; // 寄存器现场,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
};

struct co* current=NULL;
struct co* co_pool[128];  
int co_pool_count = 0;
void debugstack(){
    debug("[stack]: ");
    for(int i=0;i<co_pool_count;i++){
        debug("%s ",co_pool[i]->name);
    }
    debug("\n");
}

//func(arg)被 co_start() 调用，从头开始运行
struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    //co会被return，所以需要malloc();来保存co的数据。
    struct co *co = malloc(sizeof(struct co));
    assert(co != NULL);
    co->name = name;
    debug("co_start(%s):%s\n",co->name,"CO_NEW");
    
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = sizeof(co->stack);
    co->context.uc_link = &current->context;
    makecontext(&co->context, (void (*)(void))co_yield, 0);
    co_pool[co_pool_count++] = co;
    debugstack();
    return co;
}



struct co* next_co(){
    //随机选择另一个状态为`CO_RUNNING`或`CO_WAITING`的协程
    struct co* co = NULL;
    int choose = time(NULL)%co_pool_count;
    for(int i=0;i<co_pool_count;i++){
        co = co_pool[choose];
        if(co->status==CO_DEAD){
            choose = (choose+1)%co_pool_count;
            continue;
        }
        // if(co->name==current->name){
        //     choose = (choose+1)%co_pool_count;
        //     continue;
        // }
        if(co->status==CO_RUNNING || co->status==CO_WAITING){
            return co;
        }
    }
    return co;
}

//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {
    assert(co != NULL);
    debug("co_wait(%s)\n",co->name);
    // co_yield();
    while(co->status!=CO_DEAD){
        // current->status = CO_WAITING;
        co_yield();
    }
    debug("free(%s):%s\n",co->name,"CO_DEAD");
    free(co);
    return;
}


void co_yield() {
    assert(current);
    // makecontext(&current->context, (void (*)(void))co_yield, 0);
    
    //co_yield() main->Thread-1
    debug("co_yield() %s->",current->name);
    current->status = CO_WAITING;
    // 选择下一个待运行的协程 (相当于修改 current)
    struct co* tmp = current;
    current = next_co();
    debug("%s\n",current->name);

    // 保存当前协程的上下文,并切换到下一个协程的上下文
    if(current->status==CO_NEW){//context is empty
        debug("CO_NEW\n");
        current->status = CO_RUNNING;
        // stack_switch_call(current->stack,current->func,(uintptr_t)current->arg);
        debug("[Run] %s->func\n",current->name);
        makecontext(&current->context, (void (*)(void))current->func, 1, current->arg);
        current->func(current->arg);
        
    }else{//current->status==CO_WAITING / CO_RUNNING
        if(current->status==CO_WAITING){
            debug("CO_WAITING\n");
        }else if(current->status==CO_RUNNING){
            debug("CO_RUNNING\n");
        }
        current->status = CO_RUNNING;
        swapcontext(&tmp->context, &current->context);
    }
    debug("%s->func\n",current->name);
    // current->func(current->arg);
}



__attribute__((constructor))
void co_init() {
    // 创建一个协程来代表主线程
    struct co *main_co = malloc(sizeof(struct co));
    main_co->name = "main";
    main_co->status = CO_RUNNING; // 主线程已经在运行
    main_co->func = NULL; // 主线程不需要关联任何函数
    main_co->arg = NULL;
    // 将主线程协程设置为当前协程
    current = main_co;
    // co_pool[co_pool_count++] = main_co;
}


__attribute__((destructor))
void fini() {
    debug("fini\n");
    free(current);
}



// static inline void
// stack_switch_call(void *sp, void *entry, uintptr_t arg) {
//     asm volatile (//编译器不应对这段汇编代码进行优化
// #if __x86_64__
//         "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
//           :
//           : "b"((uintptr_t)sp),//stack pointer
//             "d"(entry),//function pointer
//             "a"(arg)   //function argument
//           : "memory"
// #else
//         "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
//           :
//           : "b"((uintptr_t)sp - 8),
//             "d"(entry),
//             "a"(arg)
//           : "memory"// 告诉编译器这段代码可能会改变内存的内容
// #endif
//     );
// }