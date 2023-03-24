#include "types.h"
#include "stat.h"
#include "user.h"

int main(int argc, char* argv[]){
    /* function 실행 시 추가 문자열 parameter를 입력받지 않으면 function 종료 */
    if (argc <= 1){
        exit();
    }

    /* 문자열이 "2021025205_12300_Kang"이 아닐 때 function 종료 */
    if (strcmp(argv[1], "2021025205_12300_Kang") != 0){
        exit();
    }

    char* buf = "2021025205_12300_Kang";
    int ret_val;
    /* myfunction System call 실행 */
    ret_val = myfunction(buf);
    /* 결과 출력 */
    printf(1, "Return value : 0x%x\n", ret_val);
    exit();
};