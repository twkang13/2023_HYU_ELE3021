#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[])
{
    printf(1, "memory test\n");

    int pid = getpid();
    printf(1, "pid : %d\n", pid);

    if(setmemorylimit(pid, 100000) < 0)
        printf(1, "setmemorylimit failed.\n");
    else
        printf(1, "setmemorylimit success\n");
    
    if(exec2("zombie", argv, 5) < 0)
        printf(1, "exec2 failed.\n");
    else
        printf(1, "exec2 success\n");
    
    printf(1, "memory test done\n");
    exit();
}