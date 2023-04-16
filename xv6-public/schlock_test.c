#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 1){
        exit();
    }

    int password = atoi(argv[1]);
    schedulerLock(password);
    // sleep 후 schedulerUnlock 후 실행 문제 
    //sleep(2);
    schedulerUnlock(password);
    // TODO : Priority boosting이 일어날때 swithing하는 것으로 추정,, schedulerUnlock 되는지 확인 필요
    // 이거 후 acquire가 왜 되는거지
    exit();
}