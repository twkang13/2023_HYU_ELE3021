// Test code for thread
#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREAD 8

thread_t thread[NTHREAD];

void *thread_func(void *arg)
{
    printf(1, "thread : %d\n", (int)arg);
    return 0;
}

int main(int argc, char *argv[])
{
    printf(1, "Thread Test\n");

    for(int i = 0; i < NTHREAD; i++){
        int fail = thread_create(&thread[i], thread_func, (void *)i);
        if(fail){
            printf(1, "thread(%d) : thread_create failed\n", thread[i]);
            exit();
        }
        else{
            printf(1, "thread(%d) : created\n", thread[i]);
        }
        sleep(100);
    }
    // exit 안됨,, 디버깅 필요 
    for(int i = 0; i < NTHREAD; i++){
        thread_exit(&thread[i]);
    }

    printf(1, "Thread Test Done\n");
    exit();
}