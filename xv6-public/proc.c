#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
} ptable;

struct proc L0_header; // L0 Queue
struct proc *L0_queue = &L0_header;

struct proc L1_header; // L1 Queue;
struct proc *L1_queue = &L1_header;

struct proc L2_header; // L2 Queue;
struct proc *L2_queue = &L2_header;

int schlock;
struct proc *proc_lock;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
struct cpu*
mycpu(void)
{
  int apicid, i;
  
  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");
  
  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid)
      return &cpus[i];
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED)
      goto found;

  release(&ptable.lock);
  return 0;

found:
  p->state = EMBRYO;
  p->pid = nextpid++;

  p->queue = L0; // Set new process's queue level to 0.
  p->next = 0; // Initialize next process
  addListFront(p, L0_queue);

  p->priority = 3; // Set new process's priority to 3.
  p->runtime = 0; // Set new process's runtime to 0.
  p->monopoly = 0; // Set new process's monopoly to 0. ('0' indicates that the process does not monoploize the scheduler.)

  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;

  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;

  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  // Initializing queues
  L0_queue->next = 0;
  L1_queue->next = 0;
  L2_queue->next = 0;

  L0_queue->next = p;

  schlock = 0;

  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");
  
  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);

  p->state = RUNNABLE;

  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->state = RUNNABLE;

  release(&ptable.lock);

  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  // Delete current process from its queue when it terminates.
  if(curproc->queue == L0)
    deleteList(curproc, L0_queue);
  else if(curproc->queue == L1)
    deleteList(curproc, L1_queue);
  else if(curproc->queue == L2)
    deleteList(curproc, L2_queue);

  // Indicates that scheduler is not locked.
  schlock = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
  
  for(;;){
    // Enable interrupts on this processor.
    sti();

    acquire(&ptable.lock);

    // If the process lock the scheduler, deal with the process first.
    if(schlock){
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->monopoly == 1 && p->state == RUNNABLE){
          c->proc = p;
          switchuvm(p);
          p->state = RUNNING;

          swtch(&(c->scheduler), p->context);
          switchkvm();

          c->proc = 0;
          break;
        }
      }
    }
    // If not, the scheduler works.
    else{
      // Level of queue that the scheduler is processing.
      int qlevel;

      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        // Check if there is a RUNNABLE process in the L0 queue.
        if(p->state == RUNNABLE && p->queue == L0){
          qlevel = L0;
          break;
        }
        // Check if there is a RUNNABLE process in the L1 queue.
        else if(p->queue == L1 && p->state == RUNNABLE){
          qlevel = L1;
          break;
        }
        // Check if there is a RUNNABLE process in the L2 queue.
        else if(p->queue == L2 && p->state == RUNNABLE){
          qlevel = L2;
          break;
        }
        qlevel = L0;
      }

      // L0, L1 : Round-Robin
      if(qlevel == L0 || qlevel == L1){
        struct proc* queue = 0;
        if(qlevel == L0)
          queue = L0_queue;
        else if(qlevel == L1)
          queue = L1_queue;

        for(p = queue->next; p != 0; p = p->next){
          if(p->state != RUNNABLE || p->monopoly != 0)
            continue;

          // Switch to chosen process.  It is the process's job
          // to release ptable.lock and then reacquire it
          // before jumping back to us.
          c->proc = p;
          switchuvm(p);
          p->state = RUNNING;

          swtch(&(c->scheduler), p->context);
          switchkvm();

          // Process is done running for now.
          // It should have changed its p->state before coming back.
          c->proc = 0;
        }
      }
      // L2 Queue : Priority Scheduling
      if(qlevel == L2){
        struct proc *finalproc = 0;

        // Find the process that its priority is 0.
        for(p = L2_queue->next; p != 0; p = p->next){
          if(p->state == RUNNABLE && p->priority == 0){
            finalproc = p;
          }
        }
        // Find the process that its priority is 1.
        if(finalproc == 0)
          for(p = L2_queue->next; p != 0; p = p->next){
            if(p->state == RUNNABLE && p->priority == 1)
              finalproc = p;
          }
        // Find the process that its priority is 2.
        if(finalproc == 0)
          for(p = L2_queue->next; p != 0; p = p->next){
            if(p->state == RUNNABLE && p->priority == 2)
              finalproc = p;
          }
        // Find the process that its priority is 3.
        if(finalproc == 0)
          for(p = L2_queue->next; p != 0; p = p->next){
            if(p->state == RUNNABLE && p->priority == 3){
              finalproc = p;
            }
          }

        // Switch to chosen process.
        if(finalproc != 0){
          // Check if the scheduler is locked.
          if(finalproc->monopoly != 0)
            cprintf("ERROR : Current process(pid : '%d') already locked the scheduler.\n", finalproc->pid);
          else{
            c->proc = finalproc;
            switchuvm(finalproc);
            finalproc->state = RUNNING;

            swtch(&(c->scheduler), finalproc->context);
            switchkvm();

            c->proc = 0;
          }
        }
        else
          panic("L2_queue is empty.\n"); // 이것도 이상함
      }
    }

    release(&ptable.lock);
  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  myproc()->state = RUNNABLE;

  sched();
  release(&ptable.lock);
}

// Return queue level of the current process.
int
getLevel(void)
{
  return myproc()->queue;
}

// Set priority of the process 'pid'
void
setPriority(int pid, int priority)
{  
  acquire(&ptable.lock);
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->priority = priority;
      break;
    }
  }
  
  release(&ptable.lock);
}

// Priority Boosting
// Initialize a queue level, priority, runtime of all processes
void
boosting(void)
{
  acquire(&ptable.lock);

  if(schlock == 0)
    schedulerUnlock(2021025205);

  // Initializing processes in L0 queue
  for(struct proc* p = L0_queue->next; p != 0; p = p->next){
    p->queue = L0;
    p->priority = 3;
    p->runtime = 0;
  }
  // Initializing processes in L1 queue
  for(struct proc* p = L1_queue->next; p != 0; p = p->next){
    p->queue = L0;
    p->priority = 3;
    p->runtime = 0;
    
    deleteList(p, L1_queue);
    addListEnd(p, L0_queue);
  }
  // Initialzing processes in L2 queue
  for(struct proc* p = L2_queue->next; p != 0; p = p->next){
    p->queue = L0;
    p->priority = 3;
    p->runtime = 0;

    deleteList(p, L2_queue);
    addListEnd(p, L0_queue);
  }
  ticks = 0; // Initialize Global ticks

  release(&ptable.lock);
}

// If password matches, Indicate that the current process monopolizes the scheduler.
// If not, print an error message ans exits the current process.
void
schedulerLock(int password)
{
  acquire(&ptable.lock);
  struct proc *p = myproc();

  // Check if the current process is in the list of existing processes.
  int lockable = 0;
  for(struct proc* tmp = ptable.proc; tmp < &ptable.proc[NPROC]; tmp++){
    if(tmp == p){
      lockable = 1;
      break;
    }
  }
  if(!lockable){
    cprintf("ERROR : current process does not exists in the ptable.\n");
    cprintf("        Failed to lock the scheduler.\n");
    exit();
  }

  // If password matches
  if(password == 2021025205){
    p->monopoly = 1;
    ticks = 0;
    schlock = 1;

    cprintf("pid '%d' : Scheduler Lock\n", myproc()->pid);
    proc_lock = p;
    
    release(&ptable.lock);
  }
  // If not
  else {
    cprintf("ERROR : Wrong Password\n");
    cprintf("pid : %d\ttime quantum : %d\tqueue level : %d\n", p->pid, p->runtime, p->queue);
    cprintf("Exit the current process.\n");
    release(&ptable.lock);
    exit();
  }
}

// If password matches, Break monopoly and initialize the current process's runtime & priority.
// Then, push the process at the head of L0_queue.
// If not, print an error message ans exits the current process.
void
schedulerUnlock(int password)
{
  acquire(&ptable.lock);
  struct proc *p = myproc();
  cprintf("test\n");

  if(password == 2021025205){
    p->monopoly = 0;
    p->runtime = 0;
    p->priority = 3;
    addListFront(p, L0_queue);

    schlock = 0;
    release(&ptable.lock);
  }
  else{
    cprintf("ERROR : Wrong Password\n");
    cprintf("pid : %d\ttime quantum : %d\tqueue level : %d\n", p->pid, p->runtime, p->queue);
    release(&ptable.lock);
    exit();
  }
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }

  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();
  
  if(p == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s %d %d %d ", p->pid, state, p->name, p->queue, p->priority, p->monopoly);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
