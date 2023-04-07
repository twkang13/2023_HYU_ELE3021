#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    if (argc <= 1){
        exit();
    }

    if (strcmp(argv[1], "2021025205_12300_Kang") != 0){
        exit();
    }

    char* buf = "2021025205_12300_Kang";
    int ret_val;
    ret_val = myfunction(buf);
    printf(1, "Return value : 0x%x\n", ret_val);
    exit();
};