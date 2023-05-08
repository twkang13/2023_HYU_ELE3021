#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 1){
        exit();
    }

    int password = atoi(argv[1]);
    schedulerLock(password);
    printf(1, "PID '%d'(level %d) : Locked scheduler.\n", getpid(), getLevel());
    sleep(11);
    schedulerUnlock(password);
    printf(1, "PID '%d'(level %d) : Unlocked scheduler.\n", getpid(), getLevel());
    sleep(20);
    exit();
}