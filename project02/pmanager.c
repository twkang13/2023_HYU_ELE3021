#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXBUF 100

int
main(int argc, char *argv[])
{
    while(1){
        printf(1, "pmanager> ");

        // Read buffer
        char buffer[MAXBUF] = {0, };
        memset(buffer, 0, MAXBUF);
        gets(buffer, MAXBUF);

        // Pares arguments - arg[0] : command, arg[1] : argument 1, arg[2] : argument 2
        char arg[3][51] = {0, };
        int argNum = 0, len = (int)strlen(buffer);
    
        for(int i = 0, j = 0; i <= len; i++, j++){
            if(buffer[i] == '\n' || buffer[i] == 32){
                arg[argNum][j] = '\0';
                ++argNum;
                j = -1;
            }
            else
                arg[argNum][j] = buffer[i];
        }

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
            exec2(arg[1], argv, atoi(arg[2]));
        }
        else if(!strcmp(arg[0], "memlim")){
            // pid = arg[1]     limit = arg[2]
            if(setmemorylimit(atoi(arg[1]), atoi(arg[2])) < 0)
                printf(1, "ERROR : memlim failed.\n");
            else
                printf(1, "memlim success.\n");
        }
        else if(!strcmp(arg[0], "exit") && argNum == 1){
            exit();
        }
        // Error handling   ex) Nothing typed, Invalid argument
        else{
            printf(1, "Invalid command! Try again.\n");
        }
    }

    return 0;
}