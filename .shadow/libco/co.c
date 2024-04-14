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
    
    //?
    co->waiterp = NULL;
    //?
    co->context = (struct context){0};
    //?
    co->stack[STACK_SIZE-1] = 1;

    // 新创建的协程从函数 func 开始执行，并传入参数 arg。新创建的协程不会立即执行，而是调用 co_start 的协程继续执行。


    assert(co->func != NULL);
    if(!co->func)co_pool[co_pool_count++] = co;
    debugstack();

    return co;
}

//当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
void co_wait(struct co *co) {
    assert(co != NULL);
    debug("co_wait(%s)\n",co->name);
    while(co->status!=CO_DEAD){
        current->status = CO_WAITING;
        co_yield();
    }
    debug("free(%s):%s\n",co->name,"CO_DEAD");
    free(co);
    return;
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
    //stack
    int value = setjmp(ctx->env);
    if(value == 0){
        //设置新的堆栈 ？
        ctx->env[0].__jmpbuf[6] = (long)stack+STACK_SIZE-1;
        //设置新的栈基址 ？
        ctx->env[0].__jmpbuf[7] = (long)stack+STACK_SIZE-1;
        return;
    }else{//from longjmp
        
    }
    //pc
}


static inline void
stack_switch_call(void *sp, void *entry, uintptr_t arg) {
    asm volatile (//编译器不应对这段汇编代码进行优化
#if __x86_64__
        "movq %0, %%rsp; movq %2, %%rdi; jmp *%1"
          :
          : "b"((uintptr_t)sp),//stack pointer
            "d"(entry),//function pointer
            "a"(arg)   //function argument
          : "memory"
#else
        "movl %0, %%esp; movl %2, 4(%0); jmp *%1"
          :
          : "b"((uintptr_t)sp - 8),
            "d"(entry),
            "a"(arg)
          : "memory"// 告诉编译器这段代码可能会改变内存的内容
#endif
    );
}



void co_yield() {
    assert(current);
    debug("co_yield() %s->",current->name);
    

    // 保存当前的执行环境
    save_context(&current->context,current->stack);
    current->status = CO_WAITING;
    // 选择下一个待运行的协程 (相当于修改 current)
    current = next_co();
    
    debug("%s\n",current->name);//co_yield() main->Thread-1

    if(current->status==CO_NEW){//context is empty
        current->status = CO_RUNNING;
        stack_switch_call(current->stack,current->func,&current->arg);
    }else{//current->status==CO_WAITING
        current->status = CO_RUNNING;
        longjmp(current->context.env,1);
    }
    current->func(current->arg);//?
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
