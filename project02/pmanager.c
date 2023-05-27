#include "types.h"
#include "stat.h"
#include "user.h"

#define MAXBUF 100

void panic(char*);
int fork1(void);
int getcmd(char*, int);
void runcmd(char*);

int
main(int argc, char *argv[])
{
    static char buffer[MAXBUF] = {0, };

    while(getcmd(buffer, MAXBUF) >= 0){
        // exit
        if(buffer[0] == 'e' && buffer[1] == 'x' && buffer[2] == 'i' && buffer[3] == 't' && buffer[4] == '\n')
            break;

        if(fork1() == 0)
            runcmd(buffer);
        wait();
    }

    exit();
}

void
panic(char *s)
{
  printf(2, "%s\n", s);
  exit();
}

int
fork1(void)
{
  int pid;

  pid = fork();
  if(pid == -1)
    panic("fork");
  return pid;
}

// Read buffer
int
getcmd(char *buf, int nbuf)
{
  printf(2, "pmanager> ");
  memset(buf, 0, nbuf);
  gets(buf, nbuf);
  if(buf[0] == 0) // EOF
    return -1;
  return 0;
}

// Run pmanager
void
runcmd(char *buffer)
{
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
            
    if(!strcmp(arg[0], "list") && argNum == 1){
        if(plist() < 0)
            printf(1, "ERROR : list failed.\n");
    }
    else if(!strcmp(arg[0], "kill") && argNum == 2){
        // pid = arg[1]
        if(kill(atoi(arg[1])) < 0)
            printf(1, "ERROR : kill failed.\n");
        else
            printf(1, "kill success.\n");
    }
    else if(!strcmp(arg[0], "execute") && argNum == 3){
        // path = arg[1]    stacksize = arg[2]
        int pid = fork1();

        if(pid == 0){
            // Set arguments
            char *args[100] = {0, };
            memset(args, 0, sizeof(args));
            strcpy(args[0], arg[1]);

            exec2(arg[1], args, atoi(arg[2]));
        }
        else if (pid < 0)
            printf(1, "ERROR : execute failed.\n");
    }
    else if(!strcmp(arg[0], "memlim") && argNum == 3){
        // pid = arg[1]     limit = arg[2]
        if(setmemorylimit(atoi(arg[1]), atoi(arg[2])) < 0)
            printf(1, "ERROR : memlim failed.\n");
        else
            printf(1, "memlim success.\n");
    }
    // Error handling   ex) Nothing typed, Invalid argument
    else{
        printf(1, "Invalid command! Try again.\n");
    }

    exit();
}