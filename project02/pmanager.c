#include "types.h"
#include "stat.h"
#include "user.h"

int
main(int argc, char *argv[])
{
    printf(1, "pmanager executed.\n");

    while(1){
        // Read instructions
        char instruction[100] = {0, };
        gets(instruction, 100);

        // Pares arguments - arg[0] : command, arg[1] : argument 1, arg[2] : argument 2
        int argNum = 0, len = (int)strlen(instruction);
        char arg[3][51] = {0, };
        for(int i = 0, j = 0; i <= len; i++, j++){
            // TODO : instruction 입력의 끝일 경우 arg 맨 뒤에 '\0' 붙이기
            if(instruction[i] == 0 || instruction[i] == 32){
                arg[argNum][j] = '\0';
                ++argNum;
                j = 0;
                // arg[1]이랑 arg[2]에서 맨 앞 blank 생기는거 떄문에,, 임시로 해놓음. 수정 필요 
                ++i;
            }
            arg[argNum][j] = instruction[i];
        }

        // For debugging
        printf(1, "arg[0] : %s, len : %d\narg[1] : %s, len : %d\narg[2] : %s, len : %d\n", arg[0], strlen(arg[0]),
             arg[1], strlen(arg[1]),arg[2], strlen(arg[2]));

        if(!strcmp(arg[0], "list")){
            if(plist() < 0)
                printf(1, "ERROR : list failed.\n");
        }
        else if(!strcmp(arg[0], "kill")){
            // pid = arg[1]
            if(kill(atoi(arg[1])) < 0)
                printf(1, "ERROR : kill failed.\n");
            else
                printf(1, "kill success.\n");
        }
        else if(!strcmp(arg[0], "execute")){
            // path = arg[1]    stacksize = arg[2]
            if(exec2(arg[1], argv, atoi(arg[2])) < 0)
                printf(1, "ERROR : execution failed.\n");
        }
        else if(!strcmp(arg[0], "memlim")){
            // pid = arg[1]     limit = arg[2]
            if(setmemorylimit(atoi(arg[1]), atoi(arg[2])) < 0)
                printf(1, "ERROR : memlim failed.\n");
            else
                printf(1, "memlim success.\n");
        }
        else if(!strcmp(arg[0], "exit")){
            // TODO : invalid argument일때 error handling
            exit();
        }
        // Error handling   ex) Nothing typed, Invalid argument
        printf(1, "Invalid command! Try again.\n");
    }

    return 0;
}