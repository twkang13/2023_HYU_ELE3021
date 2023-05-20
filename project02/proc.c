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
myproc(void)
{
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

  p->memlim = 0;
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
  struct proc *proc = curproc;

  acquire(&ptable.lock);

  // allocate threads address without addressing
  if(curproc->isThread && !curproc->isMain)
    proc = curproc->parent;

  sz = proc->sz;
  if(n > 0){
    // cannot grow memory if n is bigger than memory limit
    if(0 < proc->memlim && proc->memlim < sz + n){
      release(&ptable.lock);
      return -1;
    }
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0){
      release(&ptable.lock);
      return -1;
    }
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0){
      release(&ptable.lock);
      return -1;
    }
  }
  proc->sz = sz;
  switchuvm(curproc);
  release(&ptable.lock);
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

  // Clear information for main thread
  np->isThread = 0;
  np->isMain = 0;
  np->nextid = 0;
  np->tid = 0;
  np->threadnum = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;

  acquire(&ptable.lock);

  np->memlim = 0;
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

  // Exit all threads of main thread when main thread exit
  if(curproc->isThread && curproc->isMain){
    killThreads(curproc);

    // Initialize main thread
  curproc->isThread = 0;
  curproc->isMain = 0;
  curproc->nextid = 0;
  curproc->tid = 0;
  curproc->threadnum = 0;
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

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

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
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
      // Kill all thread of process if one of them is killed.
      if(p->isThread){
        release(&ptable.lock);

        struct proc *main;
        if(p->isMain)
          main = p;
        else
          main = p->parent;
        killThreads(main);

        // Initialize main thread
        main->isThread = 0;
        main->isMain = 0;
        main->nextid = 0;
        main->tid = 0;
        main->threadnum = 0;

        // Initialize main thread
        main->killed = 1;

        acquire(&ptable.lock);
      }
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

// Set the memory limit of process with pid "pid"
// Return 0 if the operation is successfully done.
// Return -1 if there is an error.
int
setmemorylimit(int pid, int limit)
{
  struct proc *p;
  int exist = 0;

  // Find a process with pid "pid"
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      exist = 1;
      break;
    }
  }

  // When process with pid "pid" does not exist
  // or limit is an invalid value
  // or limit is not a PGSIZE(4KB) multiple, ERROR
  int memlim = 0;
  if(p->memlim)
    p->sz < p->memlim ? (memlim = p->sz) : (memlim = p->memlim);
  else
    memlim = p->sz;

  if(!exist || limit < 0 || limit <= memlim || limit % PGSIZE != 0){
    release(&ptable.lock);
    return -1;
  }

  // Set process "pid"'s memory to limit
  p->memlim = limit;
  release(&ptable.lock);
  return 0;
}

// Print informations of all RUNNING or RUNNABLE processes
int
plist()
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == RUNNING || p->state == RUNNABLE){
      cprintf("pid : %d, name : %s, allocated memory : %d, ", p->pid, p->name, p->sz);

      if(p->memlim)
        cprintf("memory limit : %d\n", p->memlim);
      else
        cprintf("memory limit : unlimited\n");
    }
  }
  release(&ptable.lock);

  return 0;
}

// THREAD

// Create thread
// Run on main thread
// Return 0 if thread is succssefully created
// Return -1 if there is an error
// Modified code from fork() and exec()
int
thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
{
  // Define guard page, user stack with page size(4KB)
  uint sp, ustack[4];
  struct proc *nt;
  struct proc *p = myproc();

  // Allocate thread
  if((nt = allocproc()) == 0){
    return -1;
  }

  acquire(&ptable.lock);

  // Allocate two pages for thread at the next page boundary.
  // Make the first inaccessible.  Use the second as the user stack.
  p->sz = PGROUNDUP(p->sz);
  if((p->sz = allocuvm(p->pgdir, p->sz, p->sz + 2*PGSIZE)) == 0){
    release(&ptable.lock);
    return -1;
  }
  clearpteu(p->pgdir, (char*)(p->sz - 2*PGSIZE));
  sp = p->sz;

  // Share page table and size of process memory
  nt->pgdir = p->pgdir;
  nt->sz = p->sz;
  *nt->tf = *p->tf;

  // Set thread information
  nt->isThread = 1;
  if(!p->isThread && p->threadnum == 0){
    p->isThread = 1;
    p->isMain = 1;
    p->nextid = 2;
    p->tid = 1;
  }
  // Set main thread and share pid of main thread
  if(p->isMain){
    nt->parent = p;
    nt->pid = p->pid;
  }
  else{
    nt->parent = p->parent;
    nt->pid = nt->parent->pid;
  }

  // Set thread id
  ++nt->parent->threadnum;
  nt->tid = nt->parent->nextid++;
  *thread = nt->tid;

  // Initialize arguments for thread
  ustack[0] = 0xffffffff;  // fake return PC
  ustack[1] = (uint)arg;
  ustack[2] = 0x0;
  ustack[3] = 0x0;

  sp -= 2*4;
  if(copyout(nt->pgdir, sp, ustack, 2*4) < 0){
    release(&ptable.lock);
    return -1;
  }

  // Initialize thread state (Copy state of main thread)
  nt->tf->eax = 0;
  nt->tf->eip = (uint)start_routine;
  nt->tf->esp = sp;

  for(int i = 0; i < NOFILE; i++)
    if(p->ofile[i])
      nt->ofile[i] = filedup(p->ofile[i]);
  nt->cwd = idup(p->cwd);

  safestrcpy(nt->name, p->name, sizeof(p->name));

  // Set thread RUNNBALE
  nt->state = RUNNABLE;
  switchuvm(nt);

  // Thread is created
  release(&ptable.lock);
  yield();
  return 0;
}


// Exit thread
// Modified code from exit()
void
thread_exit(void *retval)
{
  struct proc *curthd = myproc();
  struct proc *p;
  int fd;

  if(curthd == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curthd->ofile[fd]){
      fileclose(curthd->ofile[fd]);
      curthd->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curthd->cwd);
  end_op();
  curthd->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curthd->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curthd){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Set return value and decrease thread number of process
  curthd->retval = retval;
  --curthd->parent->threadnum;

  // Jump into the scheduler, never to return.
  curthd->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Join thread
// Run on main thread
// Return 0 if thread is succssefully joined
// Return -1 if there is an error
// Modified code from wait()
int
thread_join(thread_t thread, void **retval)
{
  struct proc *t;
  struct proc *curproc = myproc();
  
  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited thread with tid "thread".
    for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
      if(!t->isThread)
        continue;
      // Check whether the process who called join is a parent of thread with tid "thread"
      if(t->tid != thread || t->parent->pid != curproc->pid)
        continue;

      if(t->state == ZOMBIE){
        kfree(t->kstack);
        t->kstack = 0;
        // Reaping process stuff
        t->pid = 0;
        t->parent = 0;
        t->name[0] = 0;
        t->killed = 0;
        t->state = UNUSED;
        // Reaping thread stuff
        t->isThread = 0;
        t->tid = 0;
        t->arg = 0;
        // Set return value and initialize retval
        *retval = t->retval;
        t->retval = 0;
        
        // If there is no thread in process, initialize main thread to process
        if(curproc->threadnum == 0){
          curproc->isThread = 0;
          curproc->isMain = 0;
          curproc->nextid = 0;
          curproc->tid = 0;
        }
        release(&ptable.lock);
        return 0;
      }
    }

    // No point waiting if we don't have any children.
    if(curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}

// Kill and reap threads of process
void
killThreads(struct proc *thread)
{
  struct proc *t;

  acquire(&ptable.lock);

  for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
    if(!t->isThread)
      continue;
    // kill threads of main thread if argument is a main thread
    // kill threads of main thread except a thread which called killThread then kill main thread
    if((thread->isMain && t->parent->pid == thread->pid) ||  
        (!thread->isMain && t->pid == thread->pid && t->tid != thread->tid)){
      // reap process resource
      kfree(t->kstack);
      t->kstack = 0;
      t->pid = 0;
      if(!thread->isMain)
        thread->parent = t->parent;
      t->parent = 0;
      t->name[0] = 0;
      t->killed = 0;
      t->state = UNUSED;
      // reap thread resource
      t->isThread = 0;
      t->tid = 0;
      t->arg = 0;
      t->retval = 0;
    }
  }

  release(&ptable.lock);
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
    cprintf("%d %s %s", p->pid, state, p->name);
    
    if(p->isThread)
      cprintf(" (thread %d of %d)", p->tid, p->pid);
    if(p->isMain)
      cprintf(" (main)");

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}
