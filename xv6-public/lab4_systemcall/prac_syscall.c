#include "types.h"
#include "defs.h"

/* System call */
int myfunction(char* str){
    /* parameter로 입력받은 문자열을 출력하고 0xABCDABCD를 return */
    cprintf("%s\n", str);
    return 0xABCDABCD;
}

/* Wrapper function */
int sys_myfunction(void){
    char* str;

    if (argstr(0, &str) < 0){
        return -1;
    }
    return myfunction(str);
}
