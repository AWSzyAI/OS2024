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
#define STACK_SIZE 16384
struct co {
    const char *    name;// 协程的名字,用于调试,可选,可以为NULL
    void            (*func)(void *);
    void *          arg;
    enum co_state   status;  //协程状态
    ucontext_t      context;    // ucontext_t 结构,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈,用于保存当前协程的栈帧
    // struct co *     waiterp; // 是否有其他协程在等待当前协程,可选,可以为NULL,
    // struct context  context; // 寄存器现场,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
};

struct co* current=NULL;
struct co dead_co={
    .name = "dead",
    .status = CO_DEAD
};
struct co* co_pool[128];  
int co_pool_count = 0;
void debug_co_pool(){
    debug("┌────────────────────\n");
    debug("│[stack]\n");
    for(int i=co_pool_count-1;i>=0;i--){
        debug("│ %d %s ",i,co_pool[i]->name);
        if(co_pool[i]->status==CO_NEW){
            debug("CO_NEW\n");
        }else if(co_pool[i]->status==CO_RUNNING){
            debug("CO_RUNNING\n");
        }else if(co_pool[i]->status==CO_WAITING){
            debug("CO_WAITING\n");
        }else if(co_pool[i]->status==CO_DEAD){
            debug("CO_DEAD\n");
        }
    }
    debug("└────────────────────\n");
}

void refresh_co_pool(){
    for(int i=0;i<co_pool_count;i++){
        if(co_pool[i]->status==CO_DEAD||co_pool[i]==NULL){
            co_pool[i] = &dead_co;
        }
    }
}
int exist_alive(){
    for(int i=1;i<co_pool_count;i++){
        if(co_pool[i]->status!=CO_DEAD){
            return 1;
        }
    }
    return 0;
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
    co->stack[STACK_SIZE-1] = 0;

    // 创建协程上下文
    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = sizeof(co->stack);
    co->context.uc_link = &current->context;
    co->context.uc_stack.ss_flags = 0;
    co->context.uc_link = NULL;
    makecontext(&co->context, (void (*)(void))co->func, 1,co->arg);
    co_pool[co_pool_count++] = co;

    debug_co_pool();   
    
    

    return co;
}



struct co* next_co(){
    int choose = rand()%co_pool_count;
    // if(exist_alive()&&choose==0){
    //     return next_co();
    // }
    struct co* co = co_pool[choose];
    if(co->status==CO_DEAD){
        return next_co();
    }
    if(co->status==CO_RUNNING){
        return next_co();
    }
    return co;
}



//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {
    assert(co != NULL);
    debug("co_wait(%s)\n",co->name);
    
    while(co->status!=CO_DEAD){
        co_yield();
    }
    
    assert(current);
    debug("free(%s)\n", current->name);
    current->status = CO_DEAD;
    refresh_co_pool();
    free(current);
    debug_co_pool();
    return;
}


void co_yield() {
    assert(current);
    //co_yield() main->Thread-1

    debug("co_yield() %s->",current->name);
    current->status = CO_WAITING;
    // 选择下一个待运行的协程 (相当于修改 current)
    struct co* tmp = current;
    current = next_co();
    current->status = CO_RUNNING;
    debug("%s\n",current->name);
    assert(current->name!=NULL);
    debug_co_pool();

    // 保存当前协程的上下文,并切换到下一个协程的上下文
    if(current->status==CO_NEW){//context is empty
        debug("CO_NEW\n");
    }
    
    swapcontext(&tmp->context, &current->context);   
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
    co_pool[co_pool_count++] = main_co;
    debug_co_pool();
    srand(time(NULL));
}


__attribute__((destructor))
void fini() {
    debug("fini\n");
    free(current);
}
