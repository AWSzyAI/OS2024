#include "co.h"
#include <stdlib.h>

struct co {
};

struct co *co_start(const char *name, void (*func)(void *), void *arg) {
    //malloc();
    return NULL;
}

void co_wait(struct co *co) {
    //当前协程需要等待，直到 co 协程的执行完成才能继续执行 (类似于 pthread_join)
    free(co);
}

void co_yield() {
}
