// Test code for thread
#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREAD 5

thread_t thread[NTHREAD];
int shared = 0;

void *thread_func(void *arg)
{
    shared += (int)arg;
    printf(1, "thread : %d(%d)\n", shared, (int)arg);
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

    exit();
}
