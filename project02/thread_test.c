// Test code for thread
#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREAD 5

thread_t thread[NTHREAD];
int shared = 0;
int sb = 0;

void *thread_func(void *arg)
{
    shared += (int)arg;
    printf(1, "thread : %d(%d)\n", shared, (int)arg);
    if(sb) sbrk(4096*((int)arg+1));
    thread_exit(arg);
    return 0;
}

int main(int argc, char *argv[])
{
    printf(1, "1. Basic Thread Test\n");

    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0){
            printf(1, "thread_create failed.\n");
            exit();
        }
        sleep(100);
    }
    for(int i = 0; i < NTHREAD; i++){
        int retval;
        thread_join(thread[i], (void**)&retval);
        printf(1, "thread joined : %d\n", retval);
    }

    printf(1, "Basic Thread Test Done\n\n");

    printf(1, "2. Fork Test\n");

    shared = 0;
    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0){
            printf(1, "thread_create failed.\n");
            exit();
        }
        if(i % 2 == 0)
            fork();
        sleep(100);
    }
    for(int i = 0; i < NTHREAD; i++){
        int retval;
        thread_join(thread[i], (void**)&retval);
        printf(1, "thread joined : %d\n", retval);
    }

    printf(1, "Fork Test Done\n\n");

    printf(1, "3. Sbrk Test\n");

    shared = 0;
    sb = 1;
    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0){
            printf(1, "thread_create failed.\n");
            exit();
        }
        sleep(100);
    }
    for(int i = 0; i < NTHREAD; i++){
        int retval;
        thread_join(thread[i], (void**)&retval);
        printf(1, "thread joined : %d\n", retval);
    }

    printf(1, "Sbrk Test Done\n");

    exit();
}