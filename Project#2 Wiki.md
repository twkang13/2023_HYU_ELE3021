# Project#2 : Wiki

컴퓨터소프트웨어학부 2021025205 강태욱

# 1. Design

## A. Process Management

---

Project#2의 첫번째 요구사항은 Process Management를 구현하는 것이다.

스택용 페이지를 원하는 개수만큼 할당받는 exec2 system call, 특정 프로세스에 대해 할당받을 수 있는 메모리의 최대치를 제한하는 setmemorylimit system call을 구현해 Process Management를 구현할 수 있다.

### exec2

---

- **xv6의 기존 exec system call에 대한 분석**
    - xv6의 기존 exec system call은 그 기능을 수행할 때 1개의 스택용 페이지와 하나의 가드용 페이지를 할당받는다.
    - 본 과제를 통해 구현해야 하는 exec2 system call은 스택용 페이지를 원하는 개수만큼 할당받는 것 이외엔 모든 기능이 기존 exec system call과 동일하므로 기존 exec system call의 코드를 대부분 그대로 이용하되, 페이지 할당에 대한 부분만 수정하는 것으로 구현을 계획하였다.
    
- **스택용 페이지를 원하는 개수만큼 할당받는 기능의 설계**
    - 기존 xv6의 exec system call에서 스택용 페이지와 가드용 페이지를 할당받는 코드는 다음과 같다.
        
        ```c
        sz = PGROUNDUP(sz);
        if((sz = allocuvm(pgdir, sz, sz + 2*PGSIZE)) == 0)
        	goto bad;
        clearpteu(pgdir, (char*)(sz - 2*PGSIZE));
        sp = sz;
        ```
        
    - allocuvm을 통해 현재의 sz에서 2개의 page를 추가로 할당받는다.
    - 그 이후 clearpteu를 통해 할당받은 페이지 중 하나를 guard용 페이지로 설정한다.
    - 즉, `sz + 2*PGSIZE`에서 2는 하나의 스택용 페이지, 하나의 가드용 페이즈를 의미함을 알 수 있다.
    - 따라서, exec2 system call을 구현하기 위해선 `sz + 2*PGSIZE`를 수정하면 된다.

### setmemorylimit

---

- **process별 memory 제한에 대한 field**
    - 기존 xv6의 process는 memory 제한에 대한 field를 가지고 있지 않다.
    - 따라서, `proc.h` 내의 process 구조체에 process의 memory 제한에 대한 field를 추가하여 process의 memory limit을 설정한다.

- **추가적인 memory를 할당받을 시 memory limit의 확인**
    - xv6에서 추가적 memory 할당을 담당하는 sbrk system call은 `proc.c`의 `growproc` 함수를 통해 실행된다.
    - 따라서, `growproc`에서 추가적인 memory를 할당할 때 현재 process의 memory limit을 확인해 추가적으로 할당받고자 하는 memory가 memory limit보다 많을 경우 오류가 발생하도록 디자인을 고안했다.
    - 또, process struct 내의 field를 이용해 thread의 메모리 영역을 고려하여 프로세스의 메모리를 제한하도록 디자인했다.

### Process Manager

---

- 새로운 user program인 pmanager.c를 만들어 해당 기능을 구현한다.
- **pmanager의 background 실행**
    - pmanager는 명령어를 실행하는 과정에서 background로 작동하고 있어야 한다.
    - 이는 기존 xv6의 `sh.c`에 구현되어 있는 shell의 작동 과정과 유사하다.
    - xv6의 shell은 처음 실행될 때 fork를 통해 명령어를 수행할 shell와 그것의 종료를 기다리는 shell로 나뉜다.
    - 그 후, shell이 실행할 명령어가 BACK(background)인 경우 한번 더 fork해 child process는 exec와 같은 명령어를 수행하도록 하고, parent process는 shell을 계속 실행한다.
    - 따라서, 이 방법을 참고해 pmanager 또한 맨 처음 실행될 때 fork를 한번 수행한 후, execute 명령어에서 fork를 한번 더 수행해 child process는 execute 명령어를 실행하도록 하고 parent process는 pmanager를 background에서 실행되도록 설계했다.

- **명령어 - list**
    - process의 이름, pid 등의 정보는 user mode에서 자세한 접근이 어렵기 때문에 `proc.c`에 새로운 system call을 생성해 관련 정보를 kernel mode에서 출력하도록 한다.
    - Process가 실행한 Thread의 정보를 고려해야 하므로 Thread가 할당받은 page, memory는 Process의 정보에서 빼주어 출력하도록 한다.
    - Thread의 경우 출력되지 않도록 한다.
- **명령어  - kill**
    - kill system call을 사용하였다.
- **명령어 - execute <path> <stacksize>**
    - 위의 background 실행의 내용대로 execute 명령어의 작동을 설계했다.
    - exec2 system call에 넘겨줄 인자를 새로 정의하여 그의 0번 인자에 path를 넣어줘 exec2 system call로 넘겨준다.
- **명령어 - memlim <pid> <limit>**
    - setmemorylimit system call을 통해  pid를 가진 프로세스의 메모리를 제한한다.
- **명령어 - exit**
    - 첫번째 fork 전 exit 명령어가 입력되었는지 확인한다.
- **잘못된 명령어에 대한 처리**
    - 명령어의 잘못된 입력 외에도 아무것도 입력 받지 않은 경우 등에 대해서도 기본적인 예외 처리를 하도록 디자인하였다.

## B. Light-weight process (Thread)

---

Light-weight process는 기존 process와 상당 부분 동일하지만, 다른 LWP와 자원 및 주소공간 등을 공유하여 유저 레벨에서 멀티태스킹을 가능하도록 한다.

### Process - Thread 관계

---

- Thread가 만들어지기 전까지 모든 Process는 Process로 작동한다.
- 만약 한 Process에서 Thread가 만들어진다면, 해당 Process는 Thread를 생성한 뒤 자신도 Thread도 바뀐다. 이때 Thread로 바뀐 Process는 main thread로, 자신을 통해 생성한 모든 thread를 관리한다.
- 만약 main thread에 child thread가 존재하지 않는다면, 해당 main thread는 다시 Process로 변환된다.
- 즉 Process와 Thread는 서로 독립적인 존재가 아닌, 상호간에 변환 가능한 존재이다.

### Process struct의 수정

---

- Thread는 기본적으로 Process와 그 역할이 동일하므로 Thread에 대한 구조체를 따로 정의하는 것 대신, Process의 구조체에 Thread와 관련된 field를 추가적으로 생성하여 Thread를 논리적으로 구성하도록 디자인하였다.
- Process가 Thread인지 나타내는 isThread, Thread의 ID를 나타내는 tid등을 추가한다.
    - 이와 관련된 자세한 구현은 Implementaion 부분에 기술했다.

### thread_create

---

- Thread의 기본 기능은 Process와 동일하기 때문에 Thread를 생성하는 과정은 Process를 생성하는 과정과 동일하다.
- 따라서, 기존 xv6의 fork와 exec을 적절히 섞어 Thread의 생성을 구현할 수 있다.

- **fork의 사용**
    - fork에서는 process의 주소공간을 할당하고, parent process의 상태를 복사해 child process에 전달한다.
    - Thread는 parent process의 역할을 하는 main thread와 주소 공간을 공유한다.
    - 따라서, Thread를 생성할 때 main thread의 상태를 복사한 후 page table을 공유함으로서 Thread끼리의 자원과 주소 공간의 공유를 구현할 수 있다.
- **exec의 사용**
    - Thread가 allocate된 후 자신만의 stack을 가지기 위해서는 stack page와 guard page를 할당한 후, user stack을 정의해야 한다.
        - 이때, main thread의 주소 공간에 stack page와 guard page를 할당한다.
    - 또, 일부 register를 수정해서 Thread가 수행할 함수를 지정해줘야 한다.
    - 기존 xv6를 분석해보니 exec에 이 부분이 구현되어 있어 해당 부분의 코드를 이용하여 Thread의 생성을 디자인하였다.
    
- **추가적 구현**
    - 만약 thread_create를 호출한 thread가 main thread가 아니라면, thread를 생성할 thread를 해당 thread의 main thread로 변경한다.
    - 만약 thread_create를 호출한 process가 thread가 아니라면, 현재 process를 main thread로 변경한다.
    - thread가 생성될때마다 main thread의 child thread끼리 주소 공간을 공유해 메모리 할당 시 중복 할당을 막는다.
    - Thread를 생성한 후 생성된 Thread를 실행하기 위해 `yield()`를 호출한다.

### thread_exit

---

- Thread의 기본 기능은 Process와 동일하기 때문에 Thread를 종료하는 과정은 Process를 종료하는 과정과 동일하다.
- 따라서, 기존 xv6의 exit을 통해 Thread의 종료를 구현할 수 있다.

- **exit의 사용**
    - 기존 xv6의 exit은 open된 file들을 모두 정리하고, 현재 process의 parent를 깨운 후 현재 process를 ZOMBIE로 만들어 parent process가 자신의 자원을 정리하도록 한다.
    - 경우에 따라 orphan을 init process로 넘기기도 한다.
    - 즉, thread_exit은 기존 exit과 대부분의 기능이 유사하다.
    - 따라서, exit의 코드를 이용하되 retval를 설정하는 기능만을 추가한다.

### thread_join

---

- Thread의 기본 기능은 Process와 동일하기 때문에 Thread의 자원을 회수하는 과정은 Process의 자원을 회수하는 과정과 동일하다.
- 따라서, 기존 xv6의 wait을 통해 Thread의 종료를 구현할 수 있다.

- **wait의 사용**
    - 기존 xv6의 wait은 parent process가 ZOMBIE 상태인 child process를 찾아 그의 자원을 회수하는 역할을 한다.
    - 즉, thread_join은 기존 wait과 대부분의 기능이 유사하다.
    - 따라서, wait의 코드를 이용하되 main thread의 child thread를 판별하는 기능과 retval을 설정하는 기능, main thread에 child thread가 없다면 main thread를 process로 환원시키는 기능을 추가한다.

### System call의 수정

---

- 기존 xv6는 Thread를 지원하지 않으므로 구현한 Thread를 적절히 사용하기 위해서는 Process와 관련된 System call들을 수정해야 한다.
- 본 과제에서는 fork, exec, sbrk, kill , exit syscall을 수정한다.
- sleep, pipe syscall은 process와 thread에서 모두 그 기능이 동일하기 때문에 수정하지 않았다.

- **fork**
    - 과제 명세에 따르면, Thread를 fork하면 그 child process는 Process가 되어야 한다.
    - 따라서, fork시 child process의 Thread와 관련된 정보를 모두 초기화하도록 수정하였다.
- **exec**
    - exec가 실행되면 기존 process의 모든 thread들은 정리되어야 한다.
    - 또한, 하나의 thread에서 새로운 process가 시작하고 나머지 thread들은 종료되어야 한다.
    - 이를 구현하기 위해 main thread의 모든 thread를 정리하고 main thread를 process로 환원시키거나, thread 자신을 제외한 모든 thread(main thread 포함)를 모두 정리하는 함수인 `void killThreads(struct proc *thread)`를 구현한다.
- **sbrk**
    - sbrk system call은 `proc.c`의 growproc을 통해 작동한다.
    - 여러 thread가 동시에 메모리 할당을 요청하더라도 할당해주는 공간이 서로 겹치면 안되므로 기존 growproc에 ptable lock을 걸어준다.
    - 할당된 memory는 main thread 내의 모든 child thread가 공유할 수 있어야 하므로 main thread에 메모리를 할당하고, 이를 모든 child thread와 공유하도록 디자인하였다.
- **kill**
    - 하나 이상의 Thread가 kill되면 Process 내의 모든 Thread가 종료되어야 하므로 **exec**에서 사용한 `void killThreads(struct proc *thread)`를 이용해 Thread를 정리한다.
    - 이후 main thread를 kill한다.
- **exit**
    - Thread에서 exit을 호출하면 main thread의 모든 child thread를 정리하고 main thread도 종료한다.
    - kill과 유사한 방법으로 구현한다.

# 2. Implementaion

## A. Process Management

---

Process Management 과정에서 핵심이 되는 부분에 대한 설명을 기술했다.

### exec2

---

- **스택용 페이지를 원하는 개수만큼 할당받는 기능의 설계**
    
    ```c
    /* exec.c */
    int
    exec2(char *path, char **argv, int stacksize)
    {
    ...
      sz = PGROUNDUP(sz);
      // Allocate stack pages, stacksize * stack pages + 1 guard page
      if((sz = allocuvm(pgdir, sz, sz + **(stacksize+1)***PGSIZE)) == 0)
        goto bad;
      // Allocate the guard page
      clearpteu(pgdir, (char*)(sz - **(stacksize+1)***PGSIZE));
      sp = sz;
    ...
    }
    ```
    
    - (stacksize)개의 stack용 page, 1개의 guard용 page를 할당하기 위해 (stacksize+1)*PGSIZE만큼의 공간을 추가로 확보한다.

### setmemorylimit

---

- **process별 memory 제한에 대한 field**
    
    ```c
    struct proc {
    ...
      int memlim;           // Memory limit of process (bytes), 0 means unlimited
    ...
    };
    ```
    
    - `proc.h`의 process struct에 다음과 같은 field를 추가한다.

- **setmemorylimit System Call**
    
    ```c
    /* proc.c */
    int
    setmemorylimit(int pid, int limit)
    {
    ...
      // Get sz of process 'p' without thread's memory
      int sz = p->sz;
      if(p->isThread && p->isMain)
        sz -= 2*p->totalThread*PGSIZE;
    ...
    ```
    
    - (pid)를 pid로 가지는 process p를 찾는다.
        - 만약 p가 main thread라면 child thread가 점유하고 있는 공간을 sz에서 빼준다.
    
    ```c
    
    ...
    	if(!exist || limit < 0 || limit <= memlim){
        release(&ptable.lock);
        return -1;
      }
    ...
    ```
    
    - (pid)를 pid로 가지는 process가 존재하지 않거나, 잘못된 limit 값이 들어오면 에러를 표시하고 함수를 종료한다.
    
    ```c
    
    ...
    	// Add thread's memory to limit
      if(p->isThread && p->isMain)
        limit += 2*p->totalThread*PGSIZE;
    
      // Set process "pid"'s memory to limit
      p->memlim = limit;
      release(&ptable.lock);
      return 0;
    }
    ```
    
    - limit을 p의 memory limit으로 설정한다.
        - 만약 p가 main thread라면, child thread가 점유하고 있는 메모리 공간을 limit에 더해줘 그 limit을 memory limit으로 설정한다.
    
- **추가적인 memory를 할당받을 시 memory limit의 확인**
    
    ```c
    /* proc.c */
    int
    growproc(int n)
    {
      sz = proc->sz;
      if(n > 0){
        // cannot grow memory if n is bigger than memory limit
        **if(0 < proc->memlim && proc->memlim < sz + n){
          release(&ptable.lock);
          return -1;
        }**
    ...
      switchuvm(curproc);
      return 0;
    }
    ```
    
    - 현재 process의 memory limit을 확인해 memory를 할당받을 수 있는지 확인한다.
    

### Process Manager

---

- 새로운 user program인 `pmanager.c`를 만들어 해당 기능을 구현한다.
- **pmanager의 background 실행**
    
    ```c
    /* pmanager.c */
    int
    main(int argc, char *argv[])
    {
        static char buffer[MAXBUF] = {0, };
    
        while(getcmd(buffer, MAXBUF) >= 0){
    		...
            **if(fork1() == 0)
                runcmd(buffer);
            wait();**
        }
    
        exit();
    }
    ```
    
    - pmanager를 실행하면 fork를 통해 pmanager의 기능을 수행하는 process와 그 process의 자원을 회수하는 process를 분리한다.
    
    ```c
    /* pmanager.c */
    void
    runcmd(char *buffer)
    {
    ...
        else if(!strcmp(arg[0], "execute") && argNum == 3){
            // path = arg[1]    stacksize = arg[2]
            **int pid = fork1();**
    
            **if(pid == 0)**{
                // Set arguments
                char *args[100] = {0, };
                memset(args, 0, sizeof(args));
                strcpy(args[0], arg[1]);
    
                exec2(arg[1], args, atoi(arg[2]));
            }
            else if (pid < 0)
                printf(1, "ERROR : execute failed.\n");
        }
    ...
        exit();
    }
    ```
    
    - exec의 경우, 한번 더 fork를 실행해 execution을 수행하는 process와 pmanager를 실행하는 process를 분리한다.
        - pmanager를 실행하는 process는 fork 이후 다시 반복문을 돌며 pmanager의 기능을 수행한다.
    
- **명령어 - list**
    
    ```c
    /* proc.c */
    int
    plist()
    {
    ...
      struct proc *p;
    
      acquire(&ptable.lock);
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->state == RUNNING || p->state == RUNNABLE || p->state == SLEEPING){
          // Do not print an information of threads
          if(p->isThread && !p->isMain)
            continue;
    
          // Set allocated memory of process
          int allocmem = p->sz;
          if(p->isThread)
            allocmem -= 2*p->totalThread*PGSIZE;
    
          cprintf("pid : %d, name : %s, stack pages : %d, allocated memory : %d, ",
    							p->pid, p->name, p->stackpages, allocmem);
    
          if(p->memlim)
            cprintf("memory limit : %d\n", p->memlim);
          else
            cprintf("memory limit : unlimited\n");
        }
      }
      release(&ptable.lock);
    
      return 0;
    }
    ```
    
    - plist System Call을 통해 RUNNING, RUNNABLE, SLEEPING Process의 정보를 출력한다.
        - Thread의 정보를 출력하지 않는다. (main thread의 정보는 출력한다.)
    - stack page의 경우 process가 할당받은 stack용 page의 개수를 저장한 `p→stackpages`를 이용해 출력한다.
    - 할당받은 메모리는 thread가 할당받은 영역을 제외한 부분을 출력한다.
- **명령어 - execute <path> <stacksize>**
    
    ```c
    /* pmanager.c */
    void
    runcmd(char *buffer)
    {
    ..
        else if(!strcmp(arg[0], "execute") && argNum == 3){
            // path = arg[1]    stacksize = arg[2]
            int pid = fork1();
    
            if(pid == 0){
                // Set arguments
                **char *args[100] = {0, };
                memset(args, 0, sizeof(args));
                strcpy(args[0], arg[1]);
    
                exec2(arg[1], args, atoi(arg[2]));**
    ...
    }
    ```
    
    - exec2 system call에 넘겨줄 인자를 새로 정의하여 그의 0번 인자에 path를 넣어줘 exec2 system call로 넘겨준다.
- **명령어 - exit**
    
    ```c
    /* pmanager.c */
    int
    main(int argc, char *argv[])
    ...
        while(getcmd(buffer, MAXBUF) >= 0){
            **// exit
            if(buffer[0] == 'e' && buffer[1] == 'x' && buffer[2] == 'i' && 
    					 buffer[3] == 't' && buffer[4] == '\n')
                break;
    ...**
        exit();
    }
    ```
    
    - 첫번째 fork 전 exit 명령어가 입력되었는지 확인한다.

## B. Light-weight process (Thread)

---

Light-weight process 구현 과정에서 핵심이 되는 부분에 대한 설명을 기술했다.

### Process struct의 수정

---

```c
/* proc.h */
struct proc {
  uint sz;                  // Size of process memory (bytes)
  int memlim;               // Memory limit of process (bytes), 0 means unlimited
  int threadnum;            // Number of threads
  int totalThread;          // The number of child threads of main thread
...
  // Thread
  int isThread;             // If isThread == 1, process is a thread.
  thread_t  tid;            // Thread ID
  int isMain;               // If isMain == 1, thread is main thread.
  thread_t  nextid;         // Next thread ID
  void *arg;                // Argument for thread
  void *retval;             // Return value of thread
};
```

- `proc.h`에 Thread와 관련된 field를 추가해 특정 Process는 Thread로 작동하도록 하였다.
    - Thread 또한 ptable에 등록되기 때문에 Thread를 사용할 경우 Process의 동시 실행 개수 제한에 걸릴 것을 우려해 최대 실행 Process의 수를 64에서 128로 변경하였다.

### thread_create

---

- **fork의 일부 사용**
    
    ```c
    /* proc.c */
    int
    thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
    {
    ...
      **// Allocate thread
      if((nt = allocproc()) == 0){
        return -1;
      }**
    ...
      for(int i = 0; i < NOFILE; i++)
        if(p->ofile[i])
          nt->ofile[i] = filedup(p->ofile[i]);
      nt->cwd = idup(p->cwd);
    
      safestrcpy(nt->name, p->name, sizeof(p->name));
    
      **// Set thread RUNNBALE
      nt->state = RUNNABLE;
      switchuvm(nt);**
    
      // Thread is created
      release(&ptable.lock);
      **yield();**
      return 0;
    }
    ```
    
    - allocproc을 통해 thread를 allocate하고, 여러 과정을 수행한 후 thread를 RUNNABLE 상태로 지정한 후 yield()를 통해 생성한 thread를 실행하도록 한다.
- **exec의 사용 & 추가적 구현**
    
    ```c
    /* proc.c */
    int
    thread_create(thread_t *thread, void *(*start_routine)(void *), void *arg)
    {
    ...
      // Allocate two pages for thread at the next page boundary.
      // Make the first inaccessible.  Use the second as the user stack.
      p->sz = PGROUNDUP(p->sz);
      if((**p->sz = allocuvm(p->pgdir, p->sz, p->sz + 2*PGSIZE**)) == 0){
        release(&ptable.lock);
        return -1;
      }
      **clearpteu(p->pgdir, (char*)(p->sz - 2*PGSIZE));**
      sp = p->sz;
      ++nt->stackpages;
      ++p->totalThread;
    
      **// Share page table and size of process memory
      nt->pgdir = p->pgdir;
      nt->sz = p->sz;
      *nt->tf = *p->tf;**
    ...
    ```
    
    - main thread의 주소 공간에 thread의 stack, guard용 페이지를 할당한 후 thread와 이를 공유한다.
        - thread는 stack용 페이지를 하나 할당받았으므로 stackpage를 늘려준다.
        - main thread는 child thread의 수가 늘었으므로 totalThread를 늘려준다. 이는 memory 제한과 pmanage의 list 출력에서 thread의 정보를 고려하기 위해 사용된다.
    - Thread를 생성한 Process의 page table, sz, trap frame을 Thread와 공유한다.
    
    ```c
    	**// Share sz and pgdir of main thread with all child threads
      for(struct proc *t = ptable.proc; t < &ptable.proc[NPROC]; t++){
        if(t->isThread && !t->isMain && t->parent->pid == p->pid)
          t->sz = p->sz;
      }**
    
      // Set thread information
      nt->isThread = 1;
      nt->memlim = 0;
      if(!p->isThread && p->threadnum == 0){
        p->isThread = 1;
        p->isMain = 1;
        p->nextid = 2;
        p->tid = 1;
      }
      **// Set main thread and share pid of main thread
      nt->parent = p;
      nt->pid = p->pid;**
    ...
    ```
    
    - main thread의 새로운 sz를 현재 생성된 Thread를 제외한 child thread과 공유해 메모리 할당 시 중복 할당을 막는다. update된 page table은 공유하지 않아 thread가 생성된 thread의 주소공간을 침범하지 않도록 한다.
    - thread 정보를 설정한다. 만약 생성된 thread가 첫번째 thread면 thread를 생성한 process를 main thread로 설정한다.
    - thread끼리 같은 pid를 가지도록 thread의 부모관계와 pid를 설정한다.
    
    ```c
      **// Initialize arguments for thread
      ustack[0] = 0xffffffff;  // fake return PC
      ustack[1] = (uint)arg;**
    
      sp -= 8;
      if(copyout(nt->pgdir, sp, ustack, 8) < 0){
        release(&ptable.lock);
        return -1;
      }
    
      **// Initialize thread state (Copy state of main thread)
      nt->tf->eax = 0;
      nt->tf->eip = (uint)start_routine;
      nt->tf->esp = sp;**
    ...
      // Thread is created
      release(&ptable.lock);
      yield();
      return 0;
    }
    ```
    
    - user stack을 설정하고 thread가 start_routine 함수를 실행하도록 register를 설정한다.
    

### thread_exit

---

- **exit의 사용**
    
    ```c
    /* proc.c */
    void
    thread_exit(void *retval)
    {
    ...
      **// Set return value and decrease thread number of process
      curthd->retval = retval;
      --curthd->parent->threadnum;**
    ...
    }
    ```
    
    - thread_exit의 핵심적인 기능은 `proc.c`의 exit과 유사하기 때문에 그 코드를 이용한다.
    - 현재 thread의 retrun value인 retval를 설정하고, main thread의 현재 thread 수인 threadnum을 줄이는 기능을 추가한다.

### thread_join

---

- **wait의 사용**
    
    ```c
    /* proc.c */
    int
    thread_join(thread_t thread, void **retval)
    {
    ...
      **// Find main thread
      if(curproc->isThread && !curproc->isMain)
        curproc = curproc->parent;**
    
      for(;;){
        // Scan through table looking for exited thread with tid "thread".
        for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
          **if(!t->isThread)
            continue;
          if(t->tid != thread || t->parent->pid != curproc->pid)
            continue;**
    
          if(t->state == ZOMBIE){
    			...
            **// Reaping thread stuff
            t->isThread = 0;
            t->tid = 0;
            t->arg = 0;
            // Set return value and initialize retval
            *retval = t->retval;
            t->retval = 0;**
            
            **//If there is no thread in process, initialize main thread to process
            if(curproc->threadnum == 0){
              curproc->isThread = 0;
              curproc->isMain = 0;
              curproc->nextid = 0;
              curproc->tid = 0;
            }**
            release(&ptable.lock);
            return 0;
          }
        }
    	...
        // Wait for children to exit.  (See wakeup1 call in proc_exit.)
        sleep(curproc, &ptable.lock);  //DOC: wait-sleep
      }
    }
    ```
    
    - thread_join의 핵심적인 기능은 `proc.c`의 wait과 유사하기 때문에 그 코드를 이용한다.
    - 다음의 기능을 새롭게 추가했다.
        - thread_join을 call한 thread가 main thread가 아닌 경우 일을 처리하는 thread를 main thread로 바꾼다.
        - child thread를 찾아 process에 관한 자원과 thread에 관한 자원을 모두 찾아 정리한다.
        - 만약 정리 후 main thread에 child thread가 없다면, main thread를 process로 전환한다.

### System call의 수정

---

- **fork**
    
    ```c
    /* proc.c */
    int
    fork(void)
    {
    ...
      **// Clear information for main thread
      np->isThread = 0;
      np->isMain = 0;
      np->nextid = 0;
      np->tid = 0;
      np->threadnum = 0;
      np->totalThread = 0;
      np->stackpages = 1;**
    ...
      return pid;
    }
    ```
    
    - Thread가 fork를 호출한 경우 child process를 Process로 만들기 위해 thread와 관련된 field를 초기화하는 코드를 추가했다.
    - 또, child process는 새로운 Process이므로 stack용 page가 1개 있음을 표시하기 위해 `np→stackpages = 1;`을 한다.
    
- **exec**
    
    ```c
    /* exec.c */
    int
    exec(char *path, char **argv)
    {
    ...
      // Clear all threads of the main thread except current thread
      if(curproc->isThread)
        killThreads(curproc);
    ...
    }
    ```
    
    - exec를 실행할 시 기존 process의 모든 thread들은 정리하기 위해 `proc.c`에 `void killThreads(struct proc *thread)`를 구현한다.
    
- **`void killThreads(struct proc *thread)`**
    
    ```c
    /* proc.c */
    void
    killThreads(struct proc *thread)
    {
    ...
      for(t = ptable.proc; t < &ptable.proc[NPROC]; t++){
        if(!t->isThread)
          continue;
        // kill threads of main thread if argument is a main thread
        // kill threads of main thread except a thread which called killThread 
    		//                                               then kill main thread
        **if((thread->isMain && t->parent->pid == thread->pid) ||  
            (!thread->isMain && t->pid == thread->pid && t->tid != thread->tid)){**
          // reap process resource
          kfree(t->kstack);
          t->kstack = 0;
          t->pid = 0;
          **// reaping main thread if current thread is not a main thread
          if(!thread->isMain && t->isMain){
            thread->parent = t->parent;
            t->isMain = 0;
            t->threadnum = 0;
            t->totalThread = 0;
          }**
    ...
        }
      }
    
      release(&ptable.lock);
    }
    ```
    
    - killThreads는 threads들을 정리하는 역할을 하므로 그 구성은 wait과 유사하다.
    - killThreads는 크게 다음의 두가지 역할을 한다.
        - 인자로 받은 thread가 main thread인 경우, main thread의 child thread를 모두 정리한다.
        - 인자로 받은 thread가 main thread가 아닌 경우, 자기 자신을 제외한 main thread와 child thread들을 정리한다.
            - 이때, 인자로 받은 thread의 parent를 main thread의 parent로 설정해 해당 thread가 orphan이 되는 것을 방지한다.
            
- **sbrk**
    - sbrk system call은 `proc.c`의 growproc을 통해 작동한다.
    
    ```c
    /* proc.c */
    int
    growproc(int n)
    {
    ...
      **acquire(&ptable.lock);**
    
      **// allocate threads address without addressing
      if(curproc->isThread && !curproc->isMain)
        proc = curproc->parent;**
    ...
      **// Share address space and page table with child threads
      if(proc->isThread && proc->isMain){
        for(struct proc *t = ptable.proc; t < &ptable.proc[NPROC]; t++){
          if(t->isThread && !t->isMain && t->parent->pid == proc->pid){
            t->sz = proc->sz;
    				t->pgdir = proc->pgdir;
    			}
    	  }**
      release(&ptable.lock);
    
      switchuvm(curproc);
      return 0;
    }
    ```
    
    - 여러 thread가 동시에 메모리 할당을 요청하더라도 할당해주는 공간이 서로 겹치면 안되므로 기존 growproc에 ptable lock을 걸어준다.
    - growproc을 호출한 process가 Thread인 경우 main thread의 memory를 늘려줘야 하므로 proc을 main thread로 설정하는 기능을 추가했다.
    - memory 할당 후 child thread들과 변경된 메모리를 공유해 중복된 메모리 공간 할당을 막고, 변경된 page table을 공유해 모든 스레드가 해당 메모리를 사용할 수 있도록 하는 기능을 추가했다.

- **kill**
    
    ```c
    /* proc.c */
    int
    kill(int pid)
    {
      struct proc *p;
    
      acquire(&ptable.lock);
      for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
        if(p->pid == pid){
          p->killed = 1;
          **// Kill all thread of process if one of them is killed.
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
            main->totalThread = 0;
    
            main->killed = 1;
    
            acquire(&ptable.lock);
          }**
    ...
      }
      release(&ptable.lock);
      return -1;
    }
    ```
    
    - 하나 이상의 Thread가 kill되면 Process 내의 모든 Thread가 종료되어야 하므로 **exec**에서 사용한 `void killThreads(struct proc *thread)`를 이용해 Thread를 정리한다.
    - 이후 main thread의 thread와 관련된 field를 정리한 후 main thread를 kill한다.
    - `void killThreads(struct proc *thread)`를 사용하기 위해 해당 함수 전후로 lock을 해제했다 다시 걸어준다.
    
- **exit**
    
    ```c
    /* proc.c */
    void
    exit(void)
    {
    ...
      **// Exit all threads of calling thread when thread exit
      if(curproc->isThread){
        killThreads(curproc);
    
        // Initialize calling thread
        curproc->isThread = 0;
        curproc->isMain = 0;
        curproc->nextid = 0;
        curproc->tid = 0;
        curproc->threadnum = 0;
        curproc->totalThread = 0;
      }**
    ...
      // Jump into the scheduler, never to return.
      curproc->state = ZOMBIE;
      sched();
      panic("zombie exit");
    }
    ```
    
    - Thread에서 exit을 호출하면 main thread의 모든 child thread를 정리하기 위해 killThreads를 호출한 뒤 main thread의 thread 관련 field를 초기화해 main thread를 Process로 환원하는 기능을 추가 구현하였다.

# 3. Result

## A. 구현 및 실행 환경

---

- MacBook Air M1 8GB RAM
- Docker 사용

## B. 컴파일 및 실행 방법

---

- xv6-public 폴더 내로 접근한 후 다음의 명령어를 순서대로 입력하여 xv6를 실행한다.

```c
make
make fs.img
./bootxv6.h
```

## C. 테스트 결과 및 분석

### pmanager

---

![Screenshot 2023-05-27 at 17.50.42.png](Project#2%20Wiki%208eb04dff63694dc6a40f347fcdcb9cdf/Screenshot_2023-05-27_at_17.50.42.png)

- pmanager의 모든 기능을 테스트한 결과이다.
- execute를 통해 실행한 ‘memory_test’는 자체적으로 구현한 테스트이다.
    - 해당 테스트에서는 우선 process의 메모리 제한을 100000으로 설정하고, 그 후 process의 메모리 제한을 100으로 설정한다.
    - 따라서, 첫번째 memory 설정에서는 succsss, 두번째 memory 설정에서는 fail이 발생한다.
    - 그 후 exec2를 이용해 forktest를 수행한다.
- execute 명령어를 실행하면 pmanager와 memory_test가 동시에 실행됨을 알 수 있다.
- list 명령어를 사용할 때 출력이 잘리는 경우 terminal의 크기를 충분히 키운 뒤 다시 명령어를 실행하면 정상실행되는 결과를 확인할 수 있다.

### thread_test

---

![Screenshot 2023-05-27 at 18.02.17.png](Project#2%20Wiki%208eb04dff63694dc6a40f347fcdcb9cdf/Screenshot_2023-05-27_at_18.02.17.png)

![Screenshot 2023-05-27 at 18.03.43.png](Project#2%20Wiki%208eb04dff63694dc6a40f347fcdcb9cdf/Screenshot_2023-05-27_at_18.03.43.png)

- 모든 테스트가 정상 실행된다.

### thread_test2

---

![Screenshot 2023-05-28 at 15.48.58.png](Project#2%20Wiki%208eb04dff63694dc6a40f347fcdcb9cdf/Screenshot_2023-05-28_at_15.48.58.png)

- 본 테스트는 thread간의 자원 공유 및 thread에서의 thread_create, thread_join 정상 작동을 확인하기 위해 제작한 테스트이다.
- 본 테스트에서 thread는 공유 자원에 thread_create를 하며 얻은 arg를 더한다.
- arg가 1인 경우, 해당 thread에서는 5개의 thread를 생성하고 이를 회수한다.
    - thread에서 생성된 thread는 main thread의 자식이 된다.
    - 해당 thread들은 자신의 인자에 10을 곱한 값(즉 10)을 공유 변수에 더한다.
- 따라서, main thread에서 생성한 thread들에게선 0, 1, 3, 6, 10이 출력되고 thread에서 생성한 thread는 20, 30, 40, 50, 60을  출력하므로 thread간의 자원이 공유됨을 알 수 있다.

### thread_exec, thread_exit, thread_kill

---

![Screenshot 2023-05-27 at 18.23.28.png](Project#2%20Wiki%208eb04dff63694dc6a40f347fcdcb9cdf/Screenshot_2023-05-27_at_18.23.28.png)

- 모든 테스트가 정상 작동한다.

# 4. Trouble Shooting

## A. Thread 구조체 생성

---

처음 Thread를 설계할 땐 Thread의 구조체를 따로 정의하고, Process에 thread struct의 배열을 만들어 Process의 자식 thread들을 관리하고자 하였다. 그러나, 해당 설계를 사용할 경우 `proc.c`에 있는 process와 관련된 여러 함수들을 수정해야 했고, thread의 table을 만들어 따로 관리를 해야 하는 등 구현의 복잡도가 높아질 것이라고 예상했다.

따라서, Thread의 기본적인 기능은 Process와 동일하다는 점에서 착안하여 Process의 구조체에 Thread와 관련된 field를 추가하고, Thread를 생성한 Process를 main thread로 만들어 자식 thread들을 관리하는 현재의 구현 방안을 고안하였다.

## B.  pmanager의 background 실행

---

처음 pmanager를 구현할 땐 background 실행을 고려하지 않고 코드를 작성하였다. 해당 구현에서 execute 명령을 수행하니 pmanager와 execute 명령을 통해 수행한 함수가 동시에 작동하지 않는 점을 파악해 이를 고치고자 처음엔 단순히 execute 명령을 수행할 때에만 fork를 사용해 execute를 통해 수행되는 함수와 pmanager를 동시에 실행하고자 하였다. 그러나, 해당 구현을 통해서는 pmanager의 다른 기능과, exit이 수행되지 않는 문제가 있었다.

이 문제를 해결하기 위해 shell의 작동 방안을 참고했다. shell의 코드를 분석해보니 초기 shell의 실행에서 fork를 통해 자원 회수용 process와 shell 작동을 위한 process가 나뉜다는 점을 파악해 pmanager의 초기 실행에서도 pmanager 기능 동작 이후 그 자원을 회수하는 process와 pmanager의 기능을 수행하는 process로 나눈 후 execute에서 위의 구현을 적용하니 명세대로 정상 작동하였다.

## C. growproc에서의 page fault

---

xv6에서는 stack과 heap의 구분이 모호하기 때문에 thread에서 growproc을 실행하면 sz를 감소시키는 것을 통해 추가 memory를 할당해야 했다. 처음 thread를 구현할 땐 thread끼리 memory 크기 자체는 공유 대상이 아니라고 보아 thread 생성 과정에서 child thread가 생성되면 기존 존재하던 child thread끼리 메모리 크기 공유를 수행하지 않도록 코드를 작성했다.

그러나, 위의 고안처럼 구현하니 page fault가 발생했다. 해당 문제가 main thread가 thread들을 동시에 생성하며 추가적인 memory를 할당받을 때 발생한다는 것을 깨달아 thread_create 코드를 다시 분석해보니 thread 생성 후 main thread의 변화를 기존 child thread들은 공유받지 못하는 점을 깨달았다. 이 점을 토대로 thread_create와 growproc에서 memory 변화 이후 child thread끼리 그 변화를 공유받는 기능을 구현하니 page fault가 발생하지 않았다.

그러나, 약 5%의 확률로 thread끼리의 동기화 문제로 인해 테스트가 fail되는 경우가 발생했다. 이 문제를 해결하기 위해서는 ptable lock 이외에 thread간의 동기화를 보장하는 추가적인 lock이 필요할 것으로 파악했다. 또한, growproc의 인자로 음수가 들어올 경우 main thread의 메모리 공간이 child thread와 main thread 자신의 메모리 공간이 혼재되어 있기에 잘못된 page를 dealloc할 가능성이 높아 page fault가 발생한다는 것을 확인했다. 이 문제를 해결하기 위해선 stack 영역과 heap 영역을 명확하게 구분해 growproc을 통해 heap 영역만 늘리도록 메모리 구조를 변경하는 것이 필요할 것 같다.