#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdio.h>

/*debug*/
#include <assert.h>
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
struct context {
    jmp_buf env;
};
#define STACK_SIZE 8192
struct co {
    const char *name;// 协程的名字,用于调试,可选,可以为NULL
    void (*func)(void *);
    void *arg;
    enum co_state   status;  //协程状态
    struct co *     waiterp; // 是否有其他协程在等待当前协程,可选,可以为NULL,
    struct context  context; // 寄存器现场,用于保存当前协程的寄存器状态,包括栈指针,栈基址等
    uint8_t         stack[STACK_SIZE]; // 协程的堆栈,用于保存当前协程的栈帧
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
    
    co->waiterp = NULL;
    co->context = (struct context){0};
    co->stack[STACK_SIZE-1] = 1;


    assert(co->func != NULL);
    if(!co->func)co_pool[co_pool_count++] = co;
    debugstack();
    // 新状态机的 %rsp 寄存器应该指向它独立的堆栈，
    // 以便在调用 co_yield 时能够恢复到这个堆栈。
    // 为了实现这一点，我们需要设置一个新的堆栈指针，
    // 并将 %rsp 寄存器指向这个新的堆栈。
    // %rip 寄存器应该指向 co_start 传递的 func 参数。
    // 根据 32/64-bit，参数也应该被保存在正确的位置 
    return co;
}

//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {
    assert(co != NULL);
    debug("co_wait(%s)\n",co->name);

    if(co->status==CO_DEAD){
        return;
    }
    current->waiterp = co;
    co_yield();
    co->status=CO_DEAD;
    debug("free(%s):%s\n",co->name,"CO_DEAD");
    free(co);// 每个协程只能被 co_wait 一次
    
}

struct co* next_co(){
    //选择另一个状态为`CO_RUNNING`或`CO_WAITING`的协程
    struct co* co = NULL;
    for(int i=0;i<co_pool_count;i++){
        if(co_pool[i]->status==CO_RUNNING || co_pool[i]->status==CO_WAITING){
            co = co_pool[i];
            break;
        }
    }
    return co;
}

void save_context(struct context *ctx,uint8_t *stack) {
    
}
void restore_context(struct context *ctx,uint8_t *stack) {
    
}


void co_yield() {
    debug("co_yield()\n");
    if(!current)return;

    // 保存当前的执行环境
    save_context(&current->context,current->stack);
    // 选择下一个待运行的协程 (相当于修改 current)
    current = next_co();
    // 并切换到这个协程运行。
    restore_context(&current->context,current->stack);
    debug("func(%s)\n",current->name); 
    current->func(current->arg);
}


__attribute__((constructor))
void co_init() {
    debug("co_init()\n");
    current = co_start("main", NULL, NULL);
}

__attribute__((destructor))
void fini() {
    debug("fini\n");
    free(current);
}
