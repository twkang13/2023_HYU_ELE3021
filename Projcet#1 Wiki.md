# Projcet#1: Wiki

컴퓨터소프트웨어학부 2021025205 강태욱

## 1.  Design

---

Project#1은 xv6에 3-level MLFQ Scheduler를 구현하는 것이다.

 본 과제에서 구현해야 할 MLFQ Scheduler는 L0, L1, L2 Queue로 이루어져 있으며, L0, L1 Queue는 Round-Robin, L2 Queue는 Priority Scheduling 방식으로 Scheduler가 구성된다.

 또한, MLFQ Scheduler에 의해 스케줄링되는 프로세스보다 항상 우선적으로 처리되어야 하는 process가 있을 수 있기에 Scheduler를 Lock, Unlock하는 기능을 구현해야 한다.

### A. 필요한 자료구조

---

기본 xv6의 Scheduler는 process의 목록을 담고 있는 ptable 배열을 통해 process를 스케줄링 한다.

 그러나 Piazza와 Project#1의 과제 명세를 볼 때, ptable을 이용해 Scheduler를 구현할 경우 Unlock 시 해당 process를 L0 Queue 맨 앞에 위치시키는 방법, Priority boosting이 일어날 때 기존 L0, L1, L2 Queue 내의 process의 기본 순서를 지켜 L0 Queue에 모두 넣어야 한다는 점 등의 구현이 비효율적이라는 결론을 내렸다.

따라서, 원소 간의 삽입, 삭제, 순서 변경이 간단한 **Linked List**를 이용해 각 Queue를 구현하기로 결정했다.

### B. 구현 계획 - MLFQ Scheduler

---

- **Process의 생애 주기**
    - Process는 크게 new, ready, running, waiting, terminated의 상태를 가진다.
    - Project#1은 CPU Scheduler를 구현하는 것을 목적으로 하기 때문에, 본 과제의 MLFQ Scheduler는 ready 상태의 process를 스케줄링 한다. 따라서 각 Queue는 Ready queue로 작동해야 하며, ready 상태의 process만이 queue에 존재해야 한다.
    - 이 조건을 충족하기 위해 process가 ready 상태로 전환될 때 queue에 해당 process를 삽입하고, running 상태로 진입 시 해당 process를 queue에서 삭제하도록 기존 코드를 수정해야 한다.

- **L0, L1 Queue**
    - L0, L1 Queue는 Round-Robin 방식으로 process를 스케줄링한다.
    - RUNNABLE 상태인 Process는 RUNNABLE 상태가 된 순서대로 각 Queue에 진입한다.
    - Scheduler는 Queue 맨 앞의 Process를 context switching 한다.
    - Scheduler로 인해 RUNNING 상태로 전환된 process는 RUNNABLE 상태가 아니기 때문에 Queue에서 제거된다. 실행되고 있는 process는 1 ticks만큼 일을 처리할 수 있으며, 1 ticks이 지난 후에도 종료되지 않은 process는 RUNNABLE로 상태가 바뀐 뒤 각 Queue의 맨 뒤에 삽입된다.
    - 모든 process는 각 queue에서 머문 시간을 표시하는 runtime을 가진다.
        - L0 queue에 위치한 process는 최대 4 ticks동안 L0 queue에 머물 수 있다.
        만약 4 ticks 이상 L0 queue에 머물게 된다면, 해당 process는 L1 queue로 이동하며 process의 runtime은 초기화된다.
        - L1 queue에 위치한 process는 최대 6 ticks동안 L1 queue에 머물 수 있다.
        만약 6 ticks 이상 L1 queue에 머물게 된다면, 해당 process는 L2 queue로 이동하며 process의 runtime은 초기화된다.
    
- **L2 Queue**
    - L2 Queue는 우선순위에 따라 process를 스케줄링 한다.
    - 같은 우선순위를 가진 process끼리는 FCFS 방식으로 스케줄링 된다.
        - 일반적으로 생성 순서대로 L2 Queue에 진입하기 때문에 pid가 작을수록 우선적으로 스케줄링되도록 설계했다.
    - Scheduler로 인해 RUNNING 상태로 전환된 process는 Queue에서 제거된다. 실행되고 있는 process는 1 ticks만큼 일을 처리할 수 있으며, 1 ticks이 지난 후에도 종료되지 않은 process는 RUNNABLE로 상태가 바뀐 뒤 각 Queue의 맨 뒤에 삽입된다.
        - 같은 우선순위를 가진 process끼리는 pid가 작은 순서대로 스케줄링되므로 L2 queue 내의 process의 순서는 중요하지 않다.
    - L2 queue 내에 우선순위가 가장 높으며, FCFS를 만족하는 process가 있다면 이를 context switching한다.
    - 모든 process는 각 queue에서 머문 시간을 표시하는 runtime을 가진다.
        - L2 queue 내의 process는 8 ticks의 time quantum을 가진다.
        - 8 ticks이 지난 후에도 L2 queue에 존재하는 process는 그 우선순위를 한단계 낮춘 후, runtime을 초기화한다.
        - 만약 process의 우선순위가 0이면, 우선순위를 그대로 0으로 유지한 채 runtime을 초기화한다.
    
- **Priority boosting**
    - Global tick이 100이 되면 Queue 내의 모든 RUNNABLE Process와 현재 실행되고 있는 Process의 우선순위와 time quantum(runtime), Global tick을 초기화 하며, 모든 RUNNABLE한 process를  L0 queue로 이동시킨다.
        - 이 과정에서 L0, L1, L2 queue에 위치한 process들은 queue의 우선순위 순서대로 L0 queue에 들어간다.
        - ex) L0 : A B, L1 : C D, L2 : E F → L0 : A B C D E F
    - Priority boosting 당시에 RUNNING하고 있는 process는 RUNNABLE 상태로 변경 시 L0 queue의 맨 끝에 삽입된다.
    - 만약 Scheduler가 lock되어 있는 상태라면, 스케줄러를 unlock한다.

### C. 구현 계획 - Scheduler Lock & Unlock

---

- **Scheduler Lock**
    - 만약 생성되지 않은 process가 Scheduler Lock을 시도한다면, 오류 메세지를 출력한 후 프로세스를 종료한다.
    - 인자로 password를 받아 입력받은 password가 학번(2021025205)과 일치하면 현재 실행되고 있는 process가 scheduler를 독점하고 있음을 알린다.
        - process에 스케줄러 독점 여부를 표시하는 변수를 추가하여 이를 1로 세팅하고, Scheduler에게 스케줄러가 독점되었음을 알리는 변수를 추가하여 이 또한 1로 세팅한다.
    - 만약 인자로 받은 password가 학번과 일치하지 않으면, 오류 메세지를 출력한 후 현재 프로세스를 종료한다.

- **Scheduler Unlock**
    - 만약 스케줄러가 이미 unlock되어 있다면, 오류 메세지를 출력하고 현재 프로세스를 종료한다.
    - 인자로 password를 받아 입력받은 password가 학번(2021025205)과 일치하면 현재 실행되고 있는 process가  scheduler 독점을 해제했음을 알린다.
        - 현재 process의 priority, runtime을 초기화하고 process를 RUNNABLE 상태로 전환한 후 L0 queue의 맨 앞에 삽입한다.
    - 만약 인자로 받은 password가 학번과 일치하지 않으면, 오류 메세지를 출력한 후 현재 프로세스를 종료한다.

- **Locked Scheduler의 Context Switching**
    - Scheduler가 잠겨있는 상태라면, 스케줄러를 잠근 Process의 상태를 확인한 후, 해당 Process를 실행한다.

## 2. Implement - Process & Queue

---

### A. proc.h

```c
// Per-process state
struct proc {
  uint sz;                     // Size of process memory (bytes)
  pde_t* pgdir;                // Page table
...
  **int queue;                   // Level of queue (L0 ~ L2)
  struct proc *next;           // next process of queue
  int priority;                // Priority (0~3, L0,L1 Queue : all process's priority = 3)
  int runtime;                 // runtime of the process.
  int monopoly;                // check if the process monopolizes the scheduler.**
...
};
```

process와 관련된 정보를 담고 있는 struct proc 안에 다음과 같은 variable을 추가한다.

1. int queue : process가 위치한 queue의 level
2. struct proc *next : queue 내에서 해당 process 다음에 위치할 queue
3. int priority : process의 priority
4. int runtime : process의 time quantum
5. int monopoly : process가 스케줄러를 독점하고 있는지를 나타냄

### B. linkedlist.c

L0, L1, L2 queue를 생성하기 위해 linkedlist를 구현하였다.

해당 자료구조는 linkedlist.c 내부에 구현되어 있다.

```c
int **addListEnd**(struct proc* proc, struct proc* queue)
int **addListFront**(struct proc* proc, struct proc* queue)
int **isLast**(struct proc* proc, struct proc* queue)
int **deleteList**(struct proc* proc, struct proc* queue)
void **printList**(void)
```

해당 file 내에 구현된 함수는 다음과 같다. 해당 함수들은 아래 순서대로 다음과 같은 역할을 한다.

1. addListEnd : queue의 맨 끝에 proc을 삽입한다.
2. addListFront : queue의 맨 앞에 proc을 삽입한다.
3. isLast : proc이 queue의 맨 마지막 element인지 확인한 후 true면 1, false면 0을 return한다.
4. deleteList : queue에서 proc을 삭제한다.
5. printList : L0, L1, L2 queue 안의 모든 element를 출력한다.
    1. Debugging을 위해 해당 함수를 구현했다.

### C. proc.c

```c
struct proc L0_header; // L0 Queue
struct proc ***L0_queue** = &L0_header;

struct proc L1_header; // L1 Queue;
struct proc ***L1_queue** = &L1_header;

struct proc L2_header; // L2 Queue;
struct proc ***L2_queue** = &L2_header;
```

다음과 같이 L0, L1, L2 queue를 선언했다.

```c
struct proc*
**myqueue**(int qlevel) {
  struct proc *q = 0;
  pushcli();
  if(qlevel == L0)
    q = L0_queue;
  else if(qlevel == L1)
    q = L1_queue;
  else if(qlevel == L2)
    q = L2_queue;
  popcli();
  return q;
}
```

queue를 읽어오는 과정에서 interrupt를 disable함으로서 synchronization을 하기 위해  위 함수를 구현했다.

xv6에서 Process가 생성되는 경우는 크게 다음과 같이 두가지 경우로 나눌 수 있다.

1. xv6가  booting될 때 initial process가 생성되는 경우
2. parent process에서 fork되는 경우

위 두 경우 모두 allocproc() 함수를 실행해 process를 생성하지만, parent process에서 process가 fork되는 경우 생성된 process가 return 값(eax register)를 제외한 parent process의 모든 데이터를 복제하기 때문에 데이터 복제 후 child process의 일부를 초기화해야한다.

따라서 init process가 생성되는 userinit() 함수와 fork() 함수에 모두 process의 일부를 초기화하는 코드를 작성하였다.

또한 process의 state가 RUNNABLE로 전환된 후 process가 L0 queue에 삽입되도록 구현하였다.

추가로, init process가 생성될 때 L0, L1, L2 queue가 초기화되도록 구현하였다. 

```c
void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  p = allocproc();
  
  /**/ Initializing queues
  L0_queue->next = 0;
  L1_queue->next = 0;
  L2_queue->next = 0;**
...
	p->state = RUNNABLE;

  **p->priority = 3; // Set new process's priority to 3.
  p->runtime = 0; // Set new process's runtime to 0.
  p->monopoly = 0; // Set new process's monopoly to 0. ('0' indicates that the process does not monoploize the scheduler.)

  p->queue = L0; // Set new process's queue level to 0.
  p->next = 0; // Initialize next process
  addListEnd(p, L0_queue);**

  release(&ptable.lock);
}
```

```c
int
fork(void)
{
  int i, pid;
  struct proc *np;
...
	np->state = RUNNABLE;

  **np->priority = 3; // Set new process's priority to 3.
  np->runtime = 0; // Set new process's runtime to 0.
  np->monopoly = 0; // Set new process's monopoly to 0. ('0' indicates that the process does not monoploize the scheduler.)

  np->queue = L0; // Set new process's queue level to 0.
  np->next = 0; // Initialize next process
  addListEnd(np, L0_queue);**

  release(&ptable.lock);

  return pid;
}
```

## 3. Implement - MLFQ Scheduler

---

### A. proc.c

- **스케줄링 해야 하는 queue 구하기**
    
    ```c
    int
    getQueueLev(void)
    {
      int qlevel = -1; // Level of queue that the scheduler is processing.
    
      // Check if there is a RUNNABLE process in the L0 queue.
      if(myqueue(L0)->next)
        qlevel = L0;
      // Check if there is a RUNNABLE process in the L1 queue.
      else if(myqueue(L1)->next)
        qlevel = L1;
      // Check if there is a RUNNABLE process in the L2 queue.
      else if(myqueue(L2)->next)
        qlevel = L2;
    
      return qlevel;
    }
    ```
    
    - 위 함수는  Scheduler가 처리할 queue의 level을 구하는 역할을 한다.
        - L0, L1, L2 queue를 순서대로 탐색해 처리할 queue의 level을 반환한다.
        - 만약 처리할 process가 없다면, -1을 return한다.

### B. proc.c - void scheduler()

- **스케줄링 해야 하는 queue 구하기**
    
    ```c
    void
    scheduler(void)
    {
      struct proc *p;
    ...
    	**// Level of queue that the scheduler is processing.
    	int qlevel = getQueueLev();
      if(qlevel < 0){
    		release(&ptable.lock);
        continue;
       }**
    ...
    ```
    
    - 위 코드를 통해 스케줄러가 처리할 queue의 level을 구한 뒤, 이를 qlevel에 저장한다.
        - 만약 처리할 queue가 없다면, 스케줄러를 반복 실행한다.

- **Scheduling L0, L1 queue**
    
    ```c
    			// L0, L1 : Round-Robin
          if(qlevel == L0 || qlevel == L1){
            // Set p as a queue's first process.
            if(qlevel == L0)
              p = L0_queue->next;
            else if(qlevel == L1)
              p = L1_queue->next;
    
    				if(p->state != RUNNABLE || p->monopoly != 0)
              continue;
    ```
    
    - 현재 queue에서 접근하고 있는 process를 나타내는 p에 각 qlevel에 맞는 queue의 첫 process를  할당한다.
    - 만약 해당 p가 실행 대기 상태가 아니거나, 스케줄러를 lock하고 있다면 스케줄러를 다시 실행한다.
    
    ```c
    				c->proc = p;
            switchuvm(p);
            p->state = RUNNING;
            deleteList(p, myqueue(p->queue));
    
            swtch(&(c->scheduler), p->context);
            switchkvm();
    
            // Process is done running for now.
            // It should have changed its p->state before coming back.
            c->proc = 0;
    ```
    
    - Context Switching을 한다. 해당 부분은 기존 xv6의 코드를 대부분 그대로 이용하였다.
        - p의 state가 RUNNING으로 바뀐 뒤에는 해당 process가 queue 내부에 존재하면 안되기에 해당 process를 queue에서 삭제해주는 코드를 추가하였다.

- **Scheduling L2 queue**
    
    ```c
    			// L2 Queue : Priority Scheduling
          else if(qlevel == L2){
            struct proc *finalproc = 0;
    ```
    
    - Scheduler가 스케줄링할 process를 저장할 finalproc을 선언한다.
    
    ```c
    				// Find process which its priority is 1.
            if(finalproc == 0){
              pid = 2147483647;
              for(p = L2_queue->next; p != 0; p = p->next){
                if(p->state == RUNNABLE && p->priority == 1 && p->monopoly == 0 && p->pid < pid){
                  finalproc = p;
                  pid = finalproc->pid;
                }
              }
            }
    ```
    
    - L2 queue를 돌며 우선순위가 높은 process를 찾는다. 본 Wiki에서는 priority가 1인 process를 찾는 코드를 예시로 제공한다. 위 코드와 같은 작업을 priority가 0, 2, 3인 process들에 대해 반복한다.
        - 같은 priority를 가진 process들끼리는 FCFS로 처리되도록 구현하기 위해 process들의 pid를 확인한다.
        - L2 queue에서 RUNNING으로 전환되었다가 지정된 시간이 끝나 yield된 process의 경우 본 프로젝트의 구현에 따르면 L2 queue의 맨 끝에 삽입되게 된다. 이 상황에서 L2 queue에서 같은 priority를 가진 process 중 첫 번째로 접근하게 되는 process를 스케줄링하면 FCFS가 보장되지 않는다. 따라서 pid를 서로 비교해 가장 작은 pid를 가진 process가 스케줄링 되도록 구현하였다.
    - Context Switching의 경우 L0, L1 queue와 같은 역할을 하는 코드를 사용하였다.

## 4. Implement - Timeout

---

### A. trap.c - void trap

- **time quantum 증가**
    
    ```c
    switch(tf->trapno){
      case T_IRQ0 + IRQ_TIMER:
        if(cpuid() == 0){
          acquire(&tickslock);
          ticks++;
    
          **// Increment a runtime
          if(myproc() && myproc()->state==RUNNING)
            ++myproc()->runtime;**
          
          wakeup(&ticks);
          release(&tickslock);
    ```
    
    - Global tick이 증가할때 마다 현재 실행중인 process의 runtime을 1 증가시킨다.

- **yield 호출**
    
    ```c
    if(myproc() && myproc()->state == RUNNING &&
        **tf->trapno == T_IRQ0 + IRQ_TIMER && !schlock**)
        yield();
    ```
    
    - Scheduler가 lock되지 않은 상태에서 1 tick 마다 yield 함수가 실행되도록 구현하였다.

- **Priority boosting**
    
    ```c
    if(ticks >= 100){
        if(schlock)
          schedulerUnlock(2021025205);
        ticks = 0; // Initialize Global ticks
        boosting();
      }
    ```
    
    - Global ticks이 100 이상이면, global tick을 초기화하고 boosting() 함수를 실행한다.
        - 만약 Scheduler가 lock되어 있다면, schedulerUnlock 함수를 실행해 Scheduler의 lock을 해제하고 process 관련 작업을 한다.

### B. proc.c - void yield

- 기존 xv6의 yield 함수에 queue간의 process의 이동을 추가로 구현하였다.

- **RUNNABLE로 전환된 process에 대한 처리**
    
    ```c
    void
    yield(void)
    {
      acquire(&ptable.lock);  //DOC: yieldlock
      myproc()->state = RUNNABLE;
    
      // Add current process to queue.
      **addListEnd(myproc(), myqueue(myproc()->queue));**
    ```
    
    - 현재 실행하고 있는 process가 RUNNABLE 상태로 전환되었기 때문에 이를 process가 기존에 위치해있던 queue의 맨 끝에 삽입한다.

- **Queue 간의 이동 - L0, L1 Queue**
    
    ```c
    // L0 Queue Timeout
      if(myproc()->runtime >= 4 && myproc()->queue == L0 && !schlock){
        myproc()->queue = L1; // Move the current process to the L1 queue.
        myproc()->runtime = 0;
    
        deleteList(myproc(), myqueue(L0));
        addListEnd(myproc(), myqueue(L1));
      }
    ```
    
    - 직전에 실행중이었던 process가 각 queue에서 할당받은 time quantum을 모두 사용했다면 다음 queue로 넘어가는 기능을 구현하였다.
        - Scheduler가 lock되어 있다면 해당 기능은 작동하지 않는다.
        - 해당 process의 runtime을 초기화하고, process를 L0 queue에서 삭제한 다음 L1 queue의 맨 끝에 삽입한다.
        - 본 Wiki에서는 L0 queue에서 L1 queue로의 process 이동을 예시로 제공한다.

- **L2 Queue에서의 priority 감소**
    
    ```c
    // L2 Queue Timeout
      if(myproc()->runtime >= 8 && myproc()->queue == L2 && !schlock){
        if(myproc()->priority > 0)
          --myproc()->priority;
        myproc()->runtime = 0;
      }
    ```
    
    - L2 queue 안에 존재하는 process의 timeout이 발생하면, 해당 process의 runtime을 초기화하고 priority를 1 낮춘다.
        - Scheduler가 lock되어 있다면 해당 기능은 작동하지 않는다.
        - 만약 priority가 0이었다면, priority를 낮추지 않는다.

### C. proc.c - void boosting

- boosting() 함수는 priority boosting 관련 작업을 수행한다.

- **현재 process의 초기화**
    
    ```c
    void
    boosting(void)
    {
      acquire(&ptable.lock);
    
      // Initialzing RUNNING process if it exists.
      if(myproc()){
        myproc()->queue = L0;
        myproc()->priority = 3;
        myproc()->runtime = 0;
      }
    ```
    
    - 현재 실행중인 process의 queue level, priority, runtime을 0으로 초기화한다.

- **L0, L1, L2 queue의 process를 L0 queue로 이동**
    
    ```c
    // Initializing processes in L1 queue
      for(struct proc* p = L1_queue->next; p != 0; p = L1_queue->next){
        p->queue = L0;
        p->priority = 3;
        p->runtime = 0;
        
        deleteList(p, L1_queue);
        addListEnd(p, L0_queue);
      }
    ```
    
    - L0, L1, L2 queue 내부의 process의 queue level, priority, runtime을 초기화하고 queue 순서대로 L0 queue로 process를 이동시킨다.
        - L0 queue의 경우 queue 간의 process 이동 관련 코드가 필요하지 않다.
        - 본 Wiki에서는 L1 queue를 예시로 설명하였다.

## 5. Implement - Scheduler Lock & Unlock

---

### A. proc.c

```c
...
**int schlock;
struct proc *proc_lock;**

static struct proc *initproc;
...
```

- Scheduler가 잠겼는지 나타내는 schlock, Scheduler를 lock한 process를 담는 proc* proc_lock을 선언한다.

### B. proc.c - void schedulerLock(int)

- schedulerLock 함수는 MLFQ Scheduler를 Lock 한 후, Lock 당시의 process를 가장 먼저 처리하도록 한다.
- 아래 함수는 MLFQ Scheduler를 Lock 한 후 Scheduler가 Lock되었음을 알리는 역할을 수행한다.

- **Scheduler가 이미 Lock되었는지 확인**
    
    ```c
    void
    schedulerLock(int password)
    {
      struct proc *p = myproc();
    
      **// If the scheduler is alreay locked.
      if(schlock){
        cprintf("FAIL : Scheduler is already locked.\n");
        exit();
      }**
    ```
    
    - 만약 Scheduler가 이미 Lock 되어있다면, 에러 메세지를 출력하고 해당 process를 종료한다.
    - 이를 통해 최대 하나의 process가 Scheduler를 Lock 하도록 강제할 수 있다.

- **기존에 존재하는 process가 해당 함수(system call)을 실행하는지 확인**
    
    ```c
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
        release(&ptable.lock);
        exit();
      }
    ```
    
    - ptable을 순회하며 기존에 존재하는 process가 해당 system call을 호출하는지 파악한다.
    - 만약 기존에 존재하는 process가 해당 system call을 호출하면, 에러 메세지를 출력하고 process를 종료한다.

- **정상적인 암호가 입력된 경우**
    
    ```c
    // If password matches
      if(password == 2021025205){
        p->monopoly = 1;
        ticks = 0;
        schlock = 1;
        proc_lock = p;
    
        // Delete process which locks a scheduler from a queue.
        deleteList(p, myqueue(p->queue));
        release(&ptable.lock);
      }
    ```
    
    - 현재 실행중인 process p가 process를 독점함을 설정하고, Global tick을 초기화한다.
    - Scheduler가 Lock되었음을 표시하고, proc_lock에 현재 실행중인 process를 저장한다.
    - 그 후 현재 실행중인 process를 queue에서 삭제한다.

- **정상적인 암호가 입력되지 않은 경우**
    
    ```c
    // If not
      else {
        cprintf("ERROR : Wrong Password\n");
        cprintf("pid : %d\ttime quantum : %d\tqueue level : %d\n", p->pid, p->runtime, p->queue);
        release(&ptable.lock);
        exit();
      }
    ```
    
    - 에러 메세지를 출력하고 해당 process를 종료한다.

### C. proc.c -void schedulerUnlock(int)

- schedulerLock 함수는 MLFQ를 다시 활성화시키고, Scheduler를 Lock하던 process를 다시 queue에 삽입한다.
- 아래 함수는 현재 실행중이던 process의 Scheduler 독점을 해제한 후 이를 queue에 삽입하고, MLFQ Scheduler를 다시 활성화하는 기능을 수행한다.

- **Scheduler가 이미 Lock 되었는지 확인**
    
    ```c
    void
    schedulerUnlock(int password)
    {
      struct proc *p = myproc();
    
      **// Check if the scheduler is locked.
      if(schlock == 0){
        cprintf("ERROR : Scheduler is already unlocked.\n");
        exit();
      }**
    ```
    
    - Scheduler가 Lock되어있지 않은 상태에서 Scheduler를 Unlock할 수 없다.
    - 따라서, Scheduler가 unlock되어있는 상태에서 schedulerUnlock system call이 호출되면 오류 메세지를 출력하고 해당 process를 종료하도록 구현하였다.

- **정상적인 암호가 입력된 경우**
    
    ```c
    	acquire(&ptable.lock);
      if(password == 2021025205){
        p->monopoly = 0;
        p->runtime = 0;
        p->priority = 3;
    
        // Insert priority process to L0_queue.
        p->queue = L0;
        p->state = RUNNABLE;
        addListFront(p, L0_queue);
    
        schlock = 0;
        proc_lock = 0;
        release(&ptable.lock);
      }
    ```
    
    - Scheduler를 독점하고 있던 process가 scheduler를 더이상 독점하고 있지 않음을 나타내고, time quantum(runtime)과 priority를 초기화한다.
    - 해당 process를 RUNNABLE process로 전환하고 L0 queue의 맨 앞에 이를 삽입한다.
    - Scheduler가 unlock되었음을 나타내도록 schlock을 0으로 설정하고, proc_lock이 NULL 값을 나타내도록 초기화한다.

- **정상적인 암호가 입력되지 않은 경우**
    
    ```c
    else{
        cprintf("ERROR : Wrong Password\n");
        cprintf("pid : %d\ttime quantum : %d\tqueue level : %d\n", p->pid, p->runtime, p->queue);
        release(&ptable.lock);
        exit();
      }
    ```
    
    - 에러 메세지를 출력하고 해당 process를 종료한다.

### D. proc.c - void scheduler

- MLFQ Scheduler가 Lock 된 경우, 기존 MLFQ Scheduler 대신 Scheduler를 Lock한 Process를 우선적으로 처리하는 Scheduler를 구현하였다.

- **MLFQ Scheduler가 Lock 된 경우의 Scheduler**

```c
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
    **if(schlock){
      if(proc_lock->monopoly == 1 && proc_lock->state == RUNNABLE){
        p = proc_lock;

        c->proc = p;
        switchuvm(p);
        p->state = RUNNING;
        deleteList(p, myqueue(p->queue));

        swtch(&(c->scheduler), p->context);
        switchkvm();

        c->proc = 0;
      }
    }**
```

- 만약 Scheduler가 Lock되어 있다면, Scheduler를 lock한 process의 상태를 한번 더 확인한 다음 Context Switching 한다.

### E. trap.c : Interrupt handling

- schedulerLock, schedulerUnlock system call은 Interrupt를 통해 호출될 수 있어야 하기에 이를 위한 코드를 작성했다.

```c
void
tvinit(void)
{
  int i;

  for(i = 0; i < 256; i++)
    SETGATE(idt[i], 0, SEG_KCODE<<3, vectors[i], 0); // default : kernel mode
  SETGATE(idt[T_SYSCALL], 1, SEG_KCODE<<3, vectors[T_SYSCALL], DPL_USER); // exception : system call - user level
  SETGATE(idt[T_USERINT], 1, SEG_KCODE<<3, vectors[T_USERINT], DPL_USER); // user interrupt (Practice)
  **SETGATE(idt[T_SCHLOCK], 1, SEG_KCODE<<3, vectors[T_SCHLOCK], DPL_USER); // scheduler lock - user level
  SETGATE(idt[T_SCHUNLOCK], 1, SEG_KCODE<<3, vectors[T_SCHUNLOCK], DPL_USER); // scheduler unlock - user level**

  initlock(&tickslock, "time");
}
```

- schedulerLock, schedulerUnlock system call을 User mode에서 Interrupt로 호출할 수 있도록 설정했다.

```c
void
trap(struct trapframe *tf)
{
...
  **case T_SCHLOCK:
    schedulerLock(2021025205);
    break;
  case T_SCHUNLOCK:
    schedulerUnlock(2021025205);
    break;**
```

- 129번(T_SCHLOCK) Interrupt가 호출되면 schedulerLock system call이 호출된다.
- 130번(T_SCHUNLOCK) Interrupt가 호출되면 schedulerUnlock system call이 호출된다.

## 6. Implement - System Calls

---

### A. trap.c - void yield, void schedulerLock(int), void schedulerUnlock(int)

- 세 system call의 기능은 본 Wiki에서 이미 설명하였다.

### B. trap.c - int getLevel

```c
// Return queue level of the current process.
int
getLevel(void)
{
  return myproc()->queue;
}
```

- 현재 실행중인 process가 속해있는 queue의 level을 출력한다.

### C. trap.c - void setPriority(int, int)

- process ‘pid’의 priority를 인자로 넘겨받은 priority로 설정한다.

- **priority의 유효성 확인**
    
    ```c
    void
    setPriority(int pid, int priority)
    {  
      struct proc *p;
      if(priority < 0 || priority > 3){
        cprintf("ERROR : Invalid priority.\n");
        exit();
      }
    ```
    
    - 만약 입력받은 priority가 0~3이 아니라면, 오류 메세지를 출력하고 해당 process를 종료한다.

- **priority 설정**
    
    ```
    acquire(&ptable.lock);
      int valid = 0;
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == pid){
          p->priority = priority;
          valid = 1;
          break;
        }
      }
    ```
    
    - 인자로 넘겨받은 pid를 pid로 가지는 process를 찾아 그 process의 priority를 인자로 넘겨받은 priority로 설정한다.
    - 그 후, priority setting이 이루어졌음을 나타내는 valid를 1로 설정한 후 반복문을 빠져나온다.

- **priority setting이 제대로 이루어졌는지 확인**
    
    ```
    if(!valid)
        cprintf("ERROR : Invalid pid.\n");
    
      release(&ptable.lock);
    }
    ```
    
    - valid가 0이면, 인자로 넘겨받은 pid를 자신의 pid로 가지는 process가 없다는 뜻이다.
    - 따라서, 관련 에러 메세지를 출력한다.

### D. sysproc.c

- 해당 함수들의 Wrapper function은 sysproc.c에 구현되어 있다.
- 이 외의 System Call 등록 과정은 실습을 통해 진행한 내용과 동일하기 때문에 본 Wiki에 기술하지 않았다.

## 3. Result

---

### A. 구현 및 실행 환경

- MacBook Air M1 8GB RAM
- Docker 사용

### B. 컴파일 및 실행 방법

- xv6-public 폴더 내로 접근한 후 다음의 명령어를 순서대로 입력하여 xv6를 실행한다.

```c
make
make fs.img
./bootxv6.h
```

- **MLFQ Scheduler 테스트**
    
    ![Screenshot 2023-04-21 at 0.01.27.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.01.27.png)
    
    - xv6 실행 후 mlfq_test를 입력한다.
    - mlfq_test의 코드는 piazza에 업로드된 코드를 그대로 사용하되, 일부만 변경하였다.
        - 한 번 실행 시 Test 1, Test 2을 모두 수행하도록 변경하였다.
        - Test 2에서 fork 후 parent process가 먼저 생성된 process일수록 low priority를 가지도록 코드를 변경하였다.

- **schedulerLock, schedulerUnlock 테스트**
    - schedulerLock, schedulerUnlock system call을 테스트하는 schlock_test 프로그램을 구현하였다.
        
        ![Screenshot 2023-04-21 at 0.26.50.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.26.50.png)
        
    - xv6에서 schlock_test (argument)를 입력해 실행 가능하다.
    - 해당 테스트의 구현은 다음과 같다.
        
        ```c
        #include "types.h"
        #include "stat.h"
        #include "user.h"
        
        //schedulerLock, getLevel, schedulerUnlock systam call 사용
        
        int main(int argc, char* argv[]){
            if (argc <= 1){
                exit();
            }
        
            int password = atoi(argv[1]);
            schedulerLock(password);
            printf(1, "PID '%d'(level %d) : Locked scheduler.\n", getpid(), getLevel());
            sleep(11);
            schedulerUnlock(password);
            printf(1, "PID '%d'(level %d) : Unlocked scheduler.\n", getpid(), getLevel());
            sleep(20);
            exit();
        }
        ```
        

- **schedulerUnlock Interrupt 호출 테스트**
    - xv6에서 prac2_usercall을 입력해 schedulerUnlock system call을 실행할 수 있다.

### B. 테스트 결과

- **MLFQ Scheduler 테스트 - Test 1**
    
    ![Screenshot 2023-04-21 at 0.04.17.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.04.17.png)
    
    - 프로세스 4, 5, 6, 7는 생성 순서대로 L0, L1 Queue를 거쳐 L2 Queue로 진입한다.
    - L2 Queue에 제일 먼저 진입한 process는 자신의 일이 끝날 때 까지 priority를 줄이며 CPU를 차지한다.
    - 따라서, 제일 먼저 L2 Queue에 진입한 Process 4는 Priority Boosting을 가장 적게 받기 때문에 L0, L1 Queue에 상대적으로 적게 머문다.
    - 반면, Process 7은 L0, L1 Queue에 상대적으로 많이 머문다.
    - L0와 L1의 time quantum 비율은 2:3이다. 테스트 결과, 모든 Process에서 L0과 L1 Queue에서 머문 시간의 비율이 2:3임을 확인할 수 있다.

- **MLFQ Scheduler 테스트 - Test 2**
    
    ![Screenshot 2023-04-21 at 0.11.24.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.11.24.png)
    
    - Test 2의 기본 동작은 Test 1과 같다. 그러나, Test 2에서는 parent process에서 setPriority system call을 호출해 child의 priority를 임의 조정한다.
    - 위의 예제에서 Process 8, 9, 10, 11은 각각 fork된 후 3, 2, 1, 0으로 priority가 설정된다.
    - 100 ticks 이후 priority boosting이 일어나 모든 process들의 priority는 3으로 설정되므로 초기의 priority 설정은 결과에 영향을 주지 않는다.
    - 따라서, Test 1과 유사한 결과가 나타난다.

- **schedulerLock, schedulerUnlock 테스트**
    - 다음과 같이 Scheduler가 Lock되고 Unlock되는 것을 확인할 수 있다.
        
        ![Screenshot 2023-04-21 at 0.30.40.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.30.40.png)
        
    - 잘못된 argument를 입력할 시 오류가 발생한다.
        
        ![Screenshot 2023-04-21 at 0.28.00.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.28.00.png)
        

- **schedulerUnlock Interrupt 호출 테스트**
    - 해당 Interrupt를 호출할 때 Scheduler는 Unlock되어 있으므로 예외 처리가 발생한다.
        
        ![Screenshot 2023-04-21 at 0.36.34.png](Projcet#1%20Wiki%20863bae7a87724d2386b2be8ff63d88a8/Screenshot_2023-04-21_at_0.36.34.png)
        

## 4. Trouble Shooting

---

### A. 초기 Queue의 디자인 문제

 MLFQ Scheduler를 구현하기 위해 처음에는 Queue를 만들지 않고, process structure에 추가한 queue level만을 이용해 논리적으로 L0, L1, L2 queue를 만들고자 하였다. 그러나, Scheduler가 Unlock된 후 해당 process가 L0 queue의 맨 앞으로 이동해야 한다는 조건을 만족하는 것이 까다로웠고, Priority Boosting이 일어날 때 Queue간의 기본 순서를 보장하고자 Linked List를 이용해 Queue를 구현하는 것으로 디자인을 변경하였다.

### B. Ready Queue 구현 문제

 초기 구현에서는 구현의 편의를 위해 Ready Queue에 RUNNABLE한 Process와 RUNNING중인 Process를 모두 담고자 하였다. 그러나, 이 경우 RUNNING Process가 Queue 맨 앞에 계속 남아 RUNNABLE한 Process들은 RUNNING 중인 Process가 끝난 후에 비로소 CPU를 할당받는 문제가 발생했다. 따라서 본래의 정의에 맞게 Process가 RUNNING 상태로 변하면 Ready Queue에서 제거하고, RUNNABLE한 상태가 될 시 Ready Queue에 삽입하는 방식을 구현하였다.

### C. 동기화 문제

 L0, L1, L2 Queue는 Shared variable이므로 race condition을 방지하기 위해 이들을 동기화 해야한다. 이들을 lock을 통해 동기화하고자 Queue와 spinlock의 pointer를 담은 Queue structure를 만들고자 했으나, 해당 구현을 적용하자 xv6가 무한 부팅되는 현상이 발생했다. 메모리 할당과 관련해 문제가 발생했다고 추측했으나, 이 문제를 해결하지는 못했다. Project#1에서는 Uniprocessor를 사용하므로 Queue에 접근 할 때 Interrupt를 Disable함으로서 임시적으로 문제를 해결하고자 하였다. 다만, 이 방법은 여러 개의 Processor를 사용할 때 거의 효과가 없기 때문에 이 문제는 해결이 필요하다.