#include "types.h"
#include "stat.h"
#include "user.h"

void plist();
void pkill(int);
void execute(char*, int);
void memlim(int, int);

int
main(int argc, char *argv[])
{
    printf(1, "pmamanger executed\n");
    // TODO : parse parameters

    // TODO : execute the function

    // TODO : Error handling (Ex: Nothing typed)
    exit();
}

void
plist()
{
    // TODO : implement function "list"
}

void
pkill(int pid)
{
    // TODO : implement function "kill"
}

void
execute(char *path, int stacksize)
{
    // TODO : implement function "execute"
}

void
memlim(int pid, int limit)
{
    // TODO : implement function "memlim"
}