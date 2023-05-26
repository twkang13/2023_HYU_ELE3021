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
    if(sb) malloc(4096*((int)arg+1));
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
    }
    for(int i = 0; i < NTHREAD; i++){
        int retval;
        thread_join(thread[i], (void**)&retval);
        printf(1, "thread joined : %d\n", retval);
    }

    printf(1, "Basic Thread Test Done\n\n");

    printf(1, "2. Sbrk Sleep Test\n");

    shared = 0;
    sb = 1;
    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0){
            printf(1, "thread_create failed.\n");
            exit();
        }
        else sleep(2);
    }
    for(int i = 0; i < NTHREAD; i++){
        int retval;
        thread_join(thread[i], (void**)&retval);
        printf(1, "thread joined : %d\n", retval);
    }

    printf(1, "Sbrk Sleep Test Done\n\n");

    printf(1, "3. Sbrk Test\n");

    shared = 0;
    sb = 1;
    // 여러개의 thread가 동시에 sbrk를 호출할때 trap 14 발생 
    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0){
            printf(1, "thread_create failed.\n");
            exit();
        }
    }
    for(int i = 0; i < NTHREAD; i++){
        int retval;
        thread_join(thread[i], (void**)&retval);
        printf(1, "thread joined : %d\n", retval);
    }

    printf(1, "Sbrk Test Done\n");

    exit();
}
