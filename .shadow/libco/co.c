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
    #define debug(...)
    // #define debug(...) printf(__VA_ARGS__)
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
struct co* co_stack[64];  
int co_stack_count = 0;


void debug_co_stack(){
    int on=0;
    if(!on)return;
    debug("├──────────────────────────────────────┤\n");
    for(int i=co_stack_count-1;i>=0;i--){
        char buffer[20];
        snprintf(buffer, sizeof(buffer), "%d %s", i, co_stack[i]->name);
        debug("│ %-16s ", buffer);
        if(co_stack[i]->status==CO_NEW){
            debug("🌱   %p \n",co_stack[i]);
        }else if(co_stack[i]->status==CO_RUNNING){
            debug("✅   %p \n",co_stack[i]);
        }else if(co_stack[i]->status==CO_WAITING){
            debug("⌛️   %p \n",co_stack[i]);
        }else if(co_stack[i]->status==CO_DEAD){
            debug("💀   %p \n",co_stack[i]);
        }
    }
    debug("└──────────────────────────────────────┘\n");
    
}



void debug_co(struct co *co){
    int on=0;
    if(!on)return;
    debug("co(%s):%s\n",co->name,co->status==CO_NEW?"CO_NEW":co->status==CO_RUNNING?"CO_RUNNING":co->status==CO_WAITING?"CO_WAITING":co->status==CO_DEAD?"CO_DEAD":"UNKNOWN");
    if(co->func!=NULL){
        debug("✅%s->func\n",co->name);
    }else{
        debug("❌%s->func\n",co->name);
    }
    if(co->arg!=NULL){
        debug("✅%s->arg\n",co->name);
    }else{
        debug("❌%s->arg\n",co->name);
    }
    
    
}

//wrap一层，使得func(arg)执行完后，co->status==CO_DEAD
void wrapper_func(void *arg){
    debug("wrapper_func(%p)\n",arg);
    struct co* co = (struct co*)arg;
    debug("co(%p)\n",co);
    debug_co_stack();
    // debug_co(co);
    co->func(co->arg);
    co->status = CO_DEAD;
    debug_co_stack();
}

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    //co会被return，所以需要malloc();来保存co的数据。
    struct co *co = malloc(sizeof(struct co));
    assert(co != NULL);
    co->name = name; debug("🟩 co_start(%s):%p\n",co->name,co);
    co->func = func;
    co->arg  = arg;
    co->status = CO_NEW;
    co->stack[STACK_SIZE-1] = 0;

    // 创建协程上下文
    getcontext(&co->context);
    co->context.uc_stack.ss_sp = co->stack;
    co->context.uc_stack.ss_size = sizeof(co->stack);
    co->context.uc_link = &current->context;
    co->context.uc_stack.ss_flags = 0;
    
    debug("🟩 makecontext(&co->context, (void (*)(void))wrapper_func,1,%p);\n",co);
    makecontext(&co->context, (void (*)(void))wrapper_func,1,co);debug("🟩 co(%s) = %p\n",co->name, co); 
    
    co_stack[co_stack_count++] = co; debug_co_stack();   
    return co;
}

void refresh_co_stack(struct co *co){
    debug("🟥 refresh_co_stack()\n");


    int i=0;
    for(;i<co_stack_count;i++){
        if(co_stack[i]==co){
            break;
        }
    }
    if(co_stack[i]->status==CO_DEAD||co_stack[i]==NULL){
        struct co* tmp = co_stack[i];
        for(int j=i;j<co_stack_count-1;j++){
            co_stack[j] = co_stack[j+1];
        }
        co_stack_count--;
        debug_co_stack();
        assert(tmp!=NULL);
        debug("🟥 free(%s) at (%s)-------------------------------------------------------\n\n\n\n\n\n",tmp->name,current->name);
        free(tmp);
        // current = main_co;
        co_yield();
        fflush(stdout);
    }
}


void co_wait(struct co *co) {    assert(co != NULL);debug("🟨 co_wait(%s)\n",co->name);
    if(co->status==CO_DEAD){
        refresh_co_stack(co);
        return;
    }
    co->status = CO_WAITING;     debug_co_stack();
    while(co->status!=CO_DEAD){
        debug("🟨 (%s) is waiting(%s)......\n",current->name,co->name);
        co_yield();
    }
    refresh_co_stack(co);
}
int exist_alive_co(){
    for(int i=1;i<co_stack_count;i++){
        if(!(co_stack[i]->status==CO_WAITING)){
            return 1;
        }
    }
    return 0;
}
struct co* next_co(){
    int choose = rand()%co_stack_count;
    // 除非全部死光，否则不允许回到主协程，一抽到主协程就立马切换到下一个协程
    // if(exist_alive_co()&&choose==0){
    //     return next_co();
    // }
    // ?
    struct co* co = co_stack[choose];
    if(co->status==CO_DEAD){
        return next_co();
    }
    return co;
}
void co_yield() {                      debug("🟦 co_yield() %s->",current->name);
    if(current->status!=CO_DEAD)current->status = CO_WAITING;
    struct co* tmp = current;
    current = next_co();
    current->status = CO_RUNNING;      debug("%s\n",current->name);
    debug_co_stack();
    // 保存当前协程的上下文,并切换到下一个协程的上下文
    swapcontext(&tmp->context, &current->context);   
}

__attribute__((constructor))
void co_init() {
    srand(time(NULL));
    //主协程
    struct co *main_co = malloc(sizeof(struct co));
    main_co->name = "main";
    main_co->status = CO_RUNNING; // 主协程已经在运行
    // main_co->func = NULL; // 主协程不需要关联任何函数
    // main_co->arg = NULL;
    main_co->stack[STACK_SIZE-1] = 0;
    // getcontext(&main_co->context);
    
    co_stack[co_stack_count++] = main_co;
    current = main_co;
    debug_co_stack();
    
}


__attribute__((destructor))
void fini() {
    debug("fini\n");
    if(!current)free(current);
    free(main_co);
}
