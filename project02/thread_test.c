// Test code for thread
#include "types.h"
#include "stat.h"
#include "user.h"

#define NTHREAD 8

thread_t thread[NTHREAD];

void *thread_func(void *arg)
{
    printf(1, "thread : %d", (int)arg);
    thread_exit(arg);
    return 0;
}

int main(int argc, char *argv[])
{
    printf(1, "Thread Test\n");

    for(int i = 0; i < NTHREAD; i++){
        if(thread_create(&thread[i], thread_func, (void *)i) < 0)
            exit();
        thread_join(thread[i], 0);
        sleep(100);
    }

    printf(1, "Thread Test Done\n");
    exit();
}