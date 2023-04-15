#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 1){
        exit();
    }

    int password = atoi(argv[1]);
    schedulerLock(password);
    // TODO : Priority boosting이 일어날때 작동하는 것으로 추정,, schedulerUnlock 되는지 확인 필요
    schedulerUnlock(password);
    exit();
}