#include "co.h"
#include <stdlib.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>
#include <setjmp.h>
#include <stdio.h>

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
        debug("getcontext()\n");
        return;
    }
}

void setcontext(struct context *ctx) {
    debug("setcontext()\n");
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

struct co* current=NULL;
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
    debug("co_start(%s):%s\n",co->name,"CO_NEW");
    co->func = func;
    co->arg = arg;
    co->status = CO_NEW;
    //main就不要加进来了
    if(!co->func)co_stack[co_stack_count++] = co;

    // int val=setjmp(co->context.env);
    // if(val==0){
    //     debug("Calling func\n");
    // }else{
    //     debug("Back to co_start\n");
    //     // current->status = CO_RUNNING;
    //     debug("func(%s)\n",co->name);
    //     // current->func(co->arg);
    //     debug("func(%s) done\n",co->name);
    // }

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
    debug("co_wait(%s)\n",co->name);

    if(co->status==CO_DEAD){
        return;
    }
    if(co->status==CO_RUNNING || co->status==CO_WAITING || co->status==CO_NEW){
        //保存当前的执行环境
        int val = setjmp(current->context.env);
        if (val == 0) {
            current->status = CO_WAITING;
            co->waiterp = current;
            // 并切换到这个协程运行。
            current = co;
            if(current->status==CO_NEW){
                current->status = CO_RUNNING;
                current->func(current->arg);
            }else{
                longjmp(current->context.env, 1);//co_start(co)时，setjmp(co->context.env)返回1
            }
        } else {
            // 当 longjmp 被调用时，程序会回到这里,恢复当前的执行环境，继续执行
            debug("Back to co_wait(%s)\n",current->name);
        }
    }
    // 执行co的函数
    co->status=CO_DEAD;
    debug("free(%s):%s\n",co->name,"CO_DEAD");
    free(co);// 每个协程只能被 co_wait 一次
    
}

struct co* next_co(){
    //选择另一个状态为`CO_RUNNING`或`CO_WAITING`的协程
    struct co* co = NULL;
    while(co_stack_count>0){
        co = co_stack[--co_stack_count];
        if(co->status==CO_DEAD){
            continue;
        }else{
            co_stack[co_stack_count++] = co;
            break;
        }
    }
    return co;
}

struct co* next_wait(struct co* co){
    //选择另一个状态为`CO_RUNNING`或`CO_WAITING`的协程
    struct co* co_wait = NULL;
    co_wait = co->waiterp;
    if(co_wait->status==CO_DEAD){
        return next_wait(co_wait);
    }
    return co_wait;
}



void co_yield() {
    debug("co_yield()\n");
    
    // 在 co_yield() 被调用时，setjmp 保存寄存器现场后会立即返回 0，
    int val = setjmp(current->context.env);
    if (val == 0) {// 保存当前的执行环境
        // 此时我们需要选择下一个待运行的协程 (相当于修改 current)，
        debug("co_yield from (%s)\n",current->name);
        current->status = CO_WAITING;
        debug("next_co(%s):",current->name);
    
        // current = next_co();
        current = next_wait(current);
        debug("%s\n",current->name);
        //now it's always yield to main however it should be thread-2
        // 并切换到这个协程运行。
        // longjmp(current->context.env, 1);//?
        current->status = CO_RUNNING;//?
        current->func(current->arg);//?
        debug("func(%s)\n",current->name);
    } else { // 当 longjmp 被调用时，程序会回到这里,恢复当前的执行环境，继续执行
        debug("Back to co_yield\n");
        return;
    }
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
