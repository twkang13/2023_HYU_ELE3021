#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
// 나중에 헤더 수정

typedef uint thread_t;

int
thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
    // TODO : Get current process

    // TODO : proc.c에서 thread_create 사용 전에 ptable lock 걸기 

    // TODO : Find a thread slot and allocate thread to it

    // TODO : Initialize thread context

    // TODO : Initialize arguments for thread

    // TODO : Initialize thread state

    // TODO : Run thread

    // Thread is created
    return 0;

    // If thread is not created
    return -1;
}

void
thread_exit(void *retval)
{

}

int
thread_join(thread_t thread, void **retval)
{
    return 0;
}