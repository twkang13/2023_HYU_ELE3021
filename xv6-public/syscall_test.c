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
    // TODO : Priority boosting이 일어날때 작동하는 것으로 추정,, schedulerUnlock 되는지 확인 필요
    schedulerUnlock(password);

    printf(1, "level : %d\n", getLevel()); // for a test : getLevel
    yield(); // for a test : yield
    exit();
}