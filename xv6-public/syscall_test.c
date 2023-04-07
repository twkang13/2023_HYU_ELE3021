#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 2){
        exit();
    }

    int pid = atoi(argv[1]);
    int priority = atoi(argv[2]);
    setPriority(pid, priority); // for a test : setPriority

    printf(1, "level : %d\n", getLevel()); // for a test : getLevel
    yield(); // for a test : yield
    exit();
}