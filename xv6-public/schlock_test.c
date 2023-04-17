#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 1){
        exit();
    }

    int password = atoi(argv[1]);
    schedulerLock(password);
    sleep(11);
    schedulerUnlock(password);
    exit();
}