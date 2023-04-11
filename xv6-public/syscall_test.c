#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 3){
        exit();
    }

    int pid = atoi(argv[1]);
    int priority = atoi(argv[2]);
    int password = atoi(argv[3]);
    setPriority(pid, priority); // for a test : setPriority
    schedulerLock(password);

    printf(1, "level : %d\n", getLevel()); // for a test : getLevel
    yield(); // for a test : yield
    exit();
}