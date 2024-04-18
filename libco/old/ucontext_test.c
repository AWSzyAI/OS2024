#include <stdio.h>
#include <stdlib.h>
#include <ucontext.h>
#include <stdint.h>

#define STACK_SIZE 16384
enum co_state{
    CO_NEW,     // 新创建，还未执行过
    CO_RUNNING, // 已经执行过
    CO_WAITING, // 在 co_wait 上等待
    CO_DEAD     // 已经结束，但还未释放资源
};

typedef struct co {
    const char *name;
    void (*func)(void *);
    void *arg;
    enum co_state status;
    ucontext_t context;
    uint8_t stack[STACK_SIZE];
} co;



int g_count = 0; // 添加对 g_count 的定义

static ucontext_t main_context; // 声明主协程的上下文

static co *current_co = NULL;

static void co_func_wrapper() {
    current_co->func(current_co->arg);
    current_co->status = CO_DEAD;
    current_co = NULL;
    // 切换到主协程
    setcontext(&main_context);
}

co *co_start(const char *name, void (*func)(void *), void *arg) {
    co *new_co = (co *)malloc(sizeof(co));
    new_co->name = name;
    new_co->func = func;
    new_co->arg = arg;
    new_co->status = CO_NEW;

    getcontext(&(new_co->context));
    new_co->context.uc_stack.ss_sp = new_co->stack;
    new_co->context.uc_stack.ss_size = STACK_SIZE;
    new_co->context.uc_link = &main_context; // 设置返回主协程
    makecontext(&(new_co->context), co_func_wrapper, 0);

    return new_co;
}

void co_wait(co *co_thread) {
    while (co_thread->status != CO_DEAD) {
        // 切换到主协程
        swapcontext(&(co_thread->context), &main_context);
    }
}

void co_yield() {
    if (current_co != NULL) {
        // 切换到主协程
        swapcontext(&(current_co->context), &main_context);
    }
}

static void add_count() {
    g_count++;
}

static int get_count() {
    return g_count;
}

static void work_loop(void *arg) {
    const char *s = (const char *)arg;
    for (int i = 0; i < 100; ++i) {
        printf("%s%d  ", s, get_count());
        add_count();
        co_yield();
    }
}

static void work(void *arg) {
    work_loop(arg);
}

static void test_1() {
    co *thd1 = co_start("thread-1", work, "X");
    co *thd2 = co_start("thread-2", work, "Y");

    co_wait(thd1);
    co_wait(thd2);
}

int main() {
    test_1();
    return 0;
}