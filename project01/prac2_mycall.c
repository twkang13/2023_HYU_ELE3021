#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "x86.h"

int
mycall(void)
{
    cprintf("user interrupt 128 called!\n");
    return 0;
}