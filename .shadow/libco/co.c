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
    const char *    name;// 协程的名字,用于调试,可选,可以为NULL
    void            (*func)(void *);
    void *          arg;
    enum co_state   status;  //协程状态
    ucontext_t      context;    // ucontext_t 结构,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈,用于保存当前协程的栈帧
};

struct co* current=NULL;
struct co* main_co=NULL;
struct co dead_co={
    .name = "dead",
    .status = CO_DEAD
};
struct co* co_stack[128];  
int co_stack_count = 0;
void debug_co_stack(){
    debug("├─────────────────────────┤\n");
    for(int i=co_stack_count-1;i>=0;i--){
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%d %s", i, co_stack[i]->name);
        debug("│ %-16s ", buffer);
        if(co_stack[i]->status==CO_NEW){
            debug("🌱     │\n");
        }else if(co_stack[i]->status==CO_RUNNING){
            debug("♻️     │\n");
        }else if(co_stack[i]->status==CO_WAITING){
            debug("⌛️     │\n");
        }else if(co_stack[i]->status==CO_DEAD){
            debug("💀     │\n");
        }
    }
    debug("└─────────────────────────┘\n");
    
}

int exist_alive_co(){
    for(int i=1;i<co_stack_count;i++){
        if(!(co_stack[i]->status==CO_WAITING)){
            return 1;
        }
    }
    return 0;
}

//wrap一层，使得func(arg)执行完后，co->status==CO_DEAD
void wrapper_func(void *arg){
    struct co* co = (struct co*)arg;
    co->func(co->arg);
    co->status = CO_DEAD;
    debug_co_stack();
    co_yield();
}

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
    
    //func(arg)被 co_start() 调用，从头开始运行    
    makecontext(&co->context, (void (*)(void))wrapper_func,1,co);
    
    co_stack[co_stack_count++] = co;
    debug_co_stack();   
    return co;
}



struct co* next_co(){
    int choose = rand()%co_stack_count;
    if(exist_alive_co()&&choose==0){
        return next_co();
    }
    struct co* co = co_stack[choose];
    if(co->status==CO_DEAD){
        return next_co();
    }
    if(co->status==CO_RUNNING){
        return next_co();
    }
    return co;
}
void refresh_co_stack(){
    debug("refresh_co_stack()\n");
    for(int i=0;i<co_stack_count;i++){
        if(co_stack[i]->status==CO_DEAD||co_stack[i]==NULL){
            struct co* tmp = co_stack[i];
            for(int j=i;j<co_stack_count-1;j++){
                co_stack[j] = co_stack[j+1];
            }
            co_stack_count--;
            debug_co_stack();
            assert(tmp!=NULL);
            debug("free(%s)\n",tmp->name);
            free(tmp);
        }
    }
}

// 当在main中调用co_wait(co)时，将check一次co->status==CO_DEAD是否为真，如果为真，free(co)
// 否则co_yield()到其他co中，直到co->func(co->arg)执行完，栈指针到底，赋值使得co->status==CO_DEAD
// 然后回到co_wait(co)中原来的位置，再check一遍

//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {    
    assert(co != NULL);                                     debug("co_wait(%s)\n",co->name);
    if(co->status==CO_DEAD||co==NULL){
        return;
    }
    // current->status = CO_WAITING;
    co->status = CO_WAITING;                                debug_co_stack();
    while(co->status!=CO_DEAD){
        co_yield();
    }
    refresh_co_stack();
}


void co_yield() {
    debug("co_yield() %s->",current->name);
    current->status = CO_WAITING;
    // 选择下一个待运行的协程 (相当于修改 current)
    struct co* tmp = current;
    current = next_co();
    current->status = CO_RUNNING;
    debug("%s\n",current->name);
    debug_co_stack();
    // 保存当前协程的上下文,并切换到下一个协程的上下文
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
    main_co->stack[STACK_SIZE-1] = 0;
    getcontext(&main_co->context);
    co_stack[co_stack_count++] = main_co;
    
    
    // struct co *main_co = co_start("main",NULL,NULL);
    

    // 将主线程协程设置为当前协程
    current = main_co;
    debug_co_stack();
    srand(time(NULL));
}


__attribute__((destructor))
void fini() {
    debug("fini\n");
    free(current);
}
