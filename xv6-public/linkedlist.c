#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

// Add proc to the end of the queue 'queue'
int
addListEnd(struct proc* proc, struct proc* queue)
{
    struct proc* prev = queue;

    while(prev->next)
        prev = prev->next;
    prev->next = proc;
    
    return 0;
}

// Add proc to the front of the queue 'queue'
int
addListFront(struct proc* proc, struct proc* queue)
{
    proc->next = queue->next;
    queue->next = proc;

    return 0;
}

// Return 1 if proc is the last element of the queue 'queue'
int
isLast(struct proc* proc, struct proc* queue)
{
    struct proc* cur = queue;
    while(cur->next)
        cur = cur->next;
    return proc == cur;
}

// Delele proc from the queue 'queue'
int
deleteList(struct proc* proc, struct proc* queue)
{
    struct proc* prev = queue;

    while(prev->next != 0 && prev->next != proc)
        prev = prev->next;

    if(isLast(prev, queue)){
        cprintf("ERROR : process with id '%d' is not in a queue '%d'\n", proc->pid, proc->queue);
        return 0;
    }
    
    struct proc* tmp = prev->next;
    prev->next = tmp->next;
    proc->next = 0;

    return 0;
}

// Print out all processes in L0, L1, L2 queue. (For debugging)
void
printList(void)
{
    cprintf("L0 : ");
    for(struct proc* temp = myqueue(L0)->next; temp != 0; temp = temp->next){
        cprintf("%d ", temp->pid);
    }
    cprintf("\nL1 : ");
    for(struct proc* temp = myqueue(L1)->next; temp != 0; temp = temp->next){
        cprintf("%d ", temp->pid);
    }
    cprintf("\nL2 : ");
    for(struct proc* temp = myqueue(L2)->next; temp != 0; temp = temp->next){
        cprintf("%d ", temp->pid);
    }
    cprintf("\n");
}