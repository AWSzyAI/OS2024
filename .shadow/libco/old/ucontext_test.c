#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <ucontext.h>

#define STACK_SIZE 16384
#define NUM_THREADS 3

ucontext_t main_context;
ucontext_t coroutine_contexts[NUM_THREADS];
int current_thread = 0;

void coroutine_function(int thread_id)
{
    while (1) {
        printf("Coroutine %d: Start\n", thread_id);
        printf("Coroutine %d: Yielding to main\n", thread_id);
        swapcontext(&coroutine_contexts[thread_id], &main_context);
        printf("Coroutine %d: Resumed\n", thread_id);
        printf("Coroutine %d: End\n", thread_id);
        swapcontext(&coroutine_contexts[thread_id], &main_context);
    }
}

int next_co()
{
    current_thread = (current_thread + 1) % NUM_THREADS;
    return current_thread;
}

void* scheduler_function(void* arg)
{
    while (1) {
        int next_thread = next_co(); // 确定下一个要执行的协程
        printf("Scheduler: Resuming coroutine %d\n", next_thread);
        swapcontext(&main_context, &coroutine_contexts[next_thread]);
    }

    return NULL;
}

int main()
{
    char coroutine_stacks[NUM_THREADS][STACK_SIZE];
    pthread_t threads[NUM_THREADS + 1];
    int thread_ids[NUM_THREADS];

    // 初始化主上下文
    getcontext(&main_context);

    // 创建协程上下文
    for (int i = 0; i < NUM_THREADS; i++) {
        getcontext(&coroutine_contexts[i]);
        coroutine_contexts[i].uc_stack.ss_sp = coroutine_stacks[i];
        coroutine_contexts[i].uc_stack.ss_size = sizeof(coroutine_stacks[i]);
        coroutine_contexts[i].uc_link = &main_context;
        thread_ids[i] = i;
        makecontext(&coroutine_contexts[i], (void (*)(void))coroutine_function, 1, thread_ids[i]);
    }

    // 创建调度器线程
    pthread_create(&threads[NUM_THREADS], NULL, scheduler_function, NULL);

    // 加入主线程到调度器线程
    pthread_join(threads[NUM_THREADS], NULL);

    return 0;
}