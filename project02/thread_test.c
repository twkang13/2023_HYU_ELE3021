// Test code for thread
#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREAD 8

thread_t thread[NTHREAD];

void *thread_func(void *arg)
{
    printf(1, "thread : %d\n", (int)arg);
    thread_exit(arg);
    return 0;
}

int main(int argc, char *argv[])
{
    printf(1, "Thread Test\n");

    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0){
            printf(1, "thread_create failed.\n");
            exit();
        }
        sleep(100);
    }
    for(int i = 0; i < NTHREAD; i++){
        thread_join(thread[i], 0);
    }

    printf(1, "Thread Test Done\n");
    exit();
}