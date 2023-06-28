# Project#3 : Wiki

컴퓨터소프트웨어학부 2021025205 강태욱

# 1. Design

## A. Multi Indirect

---

기존 xv6는 Direct와 Single Indirect를 통해 파일의 정보를 저장한다. 이 방식은 최대 140개 블록(약 70KB)에 해당하는 파일만 쓸 수 있다는 한계를 가지고 있다. 따라서, 더 큰 파일을 다루게 할 수 있는 Multi Indirect를 구현한다.

### dinode, inode struct의 수정

---

dinode와 inode는 파일의 inode를 나타낸다. 파일 데이터의 주소는 addrs 배열에 저장되어 있으므로 이를 수정하여 Double Indirect와 Triple Indirect를 위한 테이블의 주소를 addrs 배열에 저장해 더 큰 파일을 관리할 수 있도록 한다.

`addrs[NDIRECT+1]`에 Double Indirect를 위한 테이블, `addrs[NDIRECT+2]`에 Triple Indirect를 위한 테이블을 저장해 파일 생성 및 삭제에 해당 자료구조를 이용한다.

### bmap : file 생성에 대한 처리

---

Double Indircet 자료구조의 height가 0인 node를 level 1 node, height가 1인 node를 level 2 node라고 가정했다.

Double Indirect는 binary tree와 같이 배열을 통해 tree를 구현하도록 디자인했다.

또한, Double Indirect에서 data는 1차원 배열이지만, 2차원 배열의 실제 구현과 같이 1차원 배열을 2차원처럼 해석할 수 있다.

level 1 node의 index를 l1, level 2 node의 index를 l2라고 한다면 l1과 l2는 다음과 같다.

```c
// bn = block number, NDIRECT = level node 2 inode 하나의 배열 크기
uint l1 = bn / NDIRECT;
uint l2 = bn % NDIRECT;
```

위 과정을 통해 구한 l1과 l2를 이용하여 `data[l1][l2]`를 할당받는다.

Triple Indirect 역시 같은 원리로 다음과 같이 index를 구할  수 있다.

```c
// NDOUBLEINDIRECT = level node 2 inode 하나의 배열 크기
// NDIRECT = level node 3 inode 하나의 배열 크기
uint l1 = bn / NDOUBLEINDIRECT;
uint l2 = (bn % NDOUBLEINDIRECT) / NINDIRECT;
uint l3 = (bn % NDOUBLEINDIRECT) % NINDIRECT;
```

위 과정을 통해 구한 l1, l2와 l3를 이용하여 `data[l1][l2][l3]`를 할당받는다.

### itrunc : file 삭제에 대한 처리

---

 itrunc를 통해 bmap에서 할당받은 데이터를 해제한다. 따라서, itrunc 함수는 bmap의 기능의 역순으로 데이터를 해제한다.

Double Indirect의 경우 할당받은 level 2 node의 원소를 모두 free하고, 이후 level 1 node의 원소를 free한다. 그 후 inode의 addrs[NDIRECT+1]을 초기화한다.

이를 통해 Double Indirect를 사용한 file의 데이터를 모두 free할 수 있다.

Triple Indirect의 경우 Double Indirect와 비슷하게 level 3 node의 원소를 모두 free하고, 이후 level 2 node의 원소를 모두 free하고, 마지막으로 level 1 node의 원소를 모두 free한다. 그 후 inode의 addrs[NDIRECT+2]를 초기화한다.

이를 통해 Triple Indirect를 사용한 file의 데이터를 모두 free할 수 있다.

## B. Symbolic Link

---

Symbolic Link를 통해서 한 파일이 다른 파일로 redirection하도록 할 수 있다.

원본 파일에 제거될 경우 Symbolic Link File은 남아있지만, 데이터에 접근할 수 있어선 안된다.

### 자료구조의 변경

---

Symbolic Link file임을 나타내는 새로운 parameter `T_SYMLINK`를 추가한다.

또, inode와 dinode에 symbolic link의 destination 경로를 담은 새로운 field를 추가한다.

### ln program

---

기존 hard link만 만들 수 있던 ln program을 수정해 `ln (op) (old_path) (new_path)` 명령어를 수행하도록 변경한다.

op가 -h라면 head link, -s라면 symbolic link file을 생성한다.

### sys_symlink

---

sys_symlink system call을 만들어 symbolic link file을 생성하도록 한다.

parameter로 전달받은 “new”를 path로 하는 symbolic link file을 생성한 뒤, 새로운 파일의 Symbolic Link path를 “old”로 설정한다.

### readsym

---

file의 경로를 담은 path, symbolic link의 redirection path를 저장할 sympath를 인자로 받는 readsym 함수를 만든다.

만약 path의 file inode가 symbolic link가 아니거나 symbolic link의 redirection file이 존재하지 않다면 함수를 종료한다.

그렇지 않다면, inode의 symlink를 sympath에 저장한다.

### execute

---

특정 프로그램 실행 전 readsym을 통해 실행하려고자 하는 프로그램이 symbolic link file인지 확인한다.

만약 그 프로그램이 symbolic link file이라면, 해당 file이 redirection하는 프로그램을 실행한다.

### sys_open

---

특정 file을 열기 전 readsym을 통해 open하고자 하는 프로그램이 symbolic link file인지 확인한다.

만약 그 프로그램이 symbolic link file이라면, 해당 file이 redirection하는 file을 open한다.

### iupdate, ilock

---

memory의 inode에 반영된 symlink 정보를 disk의 dinode에 반영하고, dinode의 symlink를 inode로 가져올 수 있도록 하여 재부팅 등의 상황이 발생하여도 symbolic link가 정상적으로 작동하게 한다.

### ls, syminfo

---

ls를 이용해 file의 meta data를 출력할 경우 symbolic link file의 올바른 정보를 가져와 출력해야 한다.

symbolic link file의 meta data를 가져오는 syminfo system call을 추가해 이를 구현한다.

## C. Sync

---

sync() system call을 통해 해당 syscall이 호출될 때 dirty buffer가 flush되도록 한다.

만약 buffer에 공간이 부족한 경우, 강제적으로 sync를 발생시킨다.

### sync

---

sync가 실행되면 commit을 통해 해당 시점의 dirty buffer를 flush한다.

commit 앞뒤로 begin_op와 end_op를 감싸 transcation을 보호한다.

### begin_op, end_op, commit, log_write

---

만약 buffer가 가득찼다면 begin_op 실행 후 commit이 실행되므로 begin_op에서 buffer의 크기를 검사하는 부분을 삭제한다.

group flush가 발생하지 않으므로 end_op에서 commit과 관련된 코드와 commit을 수행하는 부분을 삭제한다.

sync는 commit 후 flush한 block의 수를 return해야 하므로 commit이 flush한 block 수를 return하도록 한다. flush에 실패한 경우 -1을 return하도록 한다.

log buffer의 공간이 부족한 경우에도 log_write가 실행되므로 해당 함수에서 log buffer 공간 부족에 대한 panic을 삭제한다.

### lfull, bfull

---

log buffer가 가득 찼는지 여부를 확인하는 lfull 함수, bcache가 가득 찼는지 여부를 확인하는 bfull 함수를 구현한다.

### writei

---

file에 데이터를 write하기 전 log buffer 혹은 bcache의 공간이 충분한지 확인한다.

만약 log buffer 혹은 bcache의 공간이 부족하다면 sync()를 호출한다.

# 2. Implement

## A. Multi Indirect

### 자료구조의 수정

---

```c
// NDIRECT should be a factor of BSIZE
#define NDIRECT 6
#define NINDIRECT (BSIZE / sizeof(uint))
#define MAXFILE (NDIRECT + NINDIRECT + NDOUBLEINDIRECT + NTRIPLEINDIRECT)
#define NDOUBLEINDIRECT (NINDIRECT * NINDIRECT)
#define NTRIPLEINDIRECT (NDOUBLEINDIRECT * NINDIRECT)
```

`fs.h`에 다음과 같은 parameter를 추가한다.

```c
// On-disk inode structure
struct dinode {
...
  uint addrs[**NDIRECT+3**];   // Data block addressess
                           // addrs[NDIRECT]   : indirect block address
                           // addrs[NDIRECT+1] : double indirect block address
                           // addrs[NDIRECT+2] : triple indirect block address
...
};
```

```c
// file.h
// in-memory copy of an inode
struct inode {
...
  uint addrs[**NDIRECT+3**];
...
};
```

`struct dinode`와 `struct inode`를 다음과 같이 수정한다.

```c
// param.h
#define FSSIZE  2000000  // size of file system in blocks
```

Multi Indirect를 사용하기 위해 FSSIZE를 확장하였다.

### bmap

---

- **Double Indirect block의 할당**
    
    ```c
    // fs.c
    static uint
    bmap(struct inode *ip, uint bn)
    {
    ...
      // double indirect block
      if(bn < NDOUBLEINDIRECT){
        // Load double indirect block, allocating if necessary.
        if((addr = ip->addrs[NDIRECT+1]) == 0)
          **ip->addrs[NDIRECT+1] = addr = balloc(ip->dev);**
        bp = bread(ip->dev, addr);
        a = (uint*)bp->data;
    ...
    ```
    
    - block의 위치가 Double Indirect를 사용해야하는 위치인지 확인한다.
    - addrs[NDIRECT+1]이 할당되어있지 않으면 이를 할당한다.
    
- **block의 할당 - Double Indirect**
    
    ```c
    
        **// l1 : level 1 index, l2 : level 2 index
        uint l1 = bn / NINDIRECT;
        uint l2 = bn % NINDIRECT;**
    
        // Set level 1 indirect block
        if((addr = a[l1]) == 0){
          **a[l1] = addr = balloc(ip->dev);**
          log_write(bp);
        }
        brelse(bp);
    
        // Set level 2 indirect block
        bp = bread(ip->dev, addr);
        **a = (uint*)bp->data;**
    
        if((addr = a[l2]) == 0){
          **a[l2] = addr = balloc(ip->dev);**
          log_write(bp);
        }
        brelse(bp);
        return addr;
      }
      bn -= NDOUBLEINDIRECT;
    ```
    
    - level 1 index l1, level 2 index l2를 구한다.
    - level 1 node가 할당되어있지 않으면 이를 새로 할당한다.
    - level 2 node에 원소를 할당해 block을 저장한다.
    
- **Triple Indirect block의 할당**
    
    ```c
    	// triple indirect block
      if(bn < NTRIPLEINDIRECT){
        // Load triple indirect block, allocating if necessary.
        if((addr = ip->addrs[NDIRECT+2]) == 0)
          **ip->addrs[NDIRECT+2] = addr = balloc(ip->dev);**
        bp = bread(ip->dev, addr);
        a = (uint*)bp->data;
    ```
    
    - block의 위치가 Triple Indirect를 사용해야하는 위치인지 확인한다.
    - addrs[NDIRECT+2]이 할당되어있지 않으면 이를 할당한다.
    
- **block의 할당 - Triple Indirect**
    
    ```c
    		**// l1 : level 1 index, l2 : level 2 index, l3 : level 3 index
        uint l1 = bn / NDOUBLEINDIRECT;
        uint tmpbn = bn % NDOUBLEINDIRECT;
        uint l2 = tmpbn / NINDIRECT;
        uint l3 = tmpbn % NINDIRECT;**
    
        // Set level 1 indirect block
        if((addr = a[l1]) == 0){
          **a[l1] = addr = balloc(ip->dev);**
          log_write(bp);
        }
        brelse(bp);
    
        // Set level 2 indirect block
        bp = bread(ip->dev, addr);
        **a = (uint*)bp->data;**
    
        if((addr = a[l2]) == 0){
          **a[l2] = addr = balloc(ip->dev);**
          log_write(bp);
        }
        brelse(bp);
    
        // Set level 3 indirect block
        bp = bread(ip->dev, addr);
        **a = (uint*)bp->data;**
    
        if((addr = a[l3]) == 0){
          **a[l3] = addr = balloc(ip->dev);**
          log_write(bp);
        }
        brelse(bp);
        return addr;
      }
    ```
    
    - level 1 index l1, level 2 index l2, level 3 index l3를 구한다.
    - level 1 node가 할당되어있지 않으면 이를 새로 할당한다.
    - level 2 node가 할당되어있지 않으면 이를 새로 할당한다.
    - level 3 node에 원소를 할당해 block을 저장한다.

### itrunc

---

- **Double Indirect의 free**
    
    ```c
    // fs.c
    static void
    itrunc(struct inode *ip)
    {
    ...
      // truncate double indirect block
      **if(ip->addrs[NDIRECT+1]){**
        int m, n;
    
        bp = bread(ip->dev, ip->addrs[NDIRECT+1]);
        a = (uint*)bp->data;
    
        // free first level block
        **for(n = 0; n < NINDIRECT; n++){**
          if(a[n]){
            struct buf *bpsec = bread(ip->dev, a[n]);
            **uint *asec = (uint*)bpsec->data;**
    
            **// free second level block
            for(m = 0; m < NINDIRECT; m++){
              if(asec[m])
                bfree(ip->dev, asec[m]);
            }**
    
            brelse(bpsec);
            **bfree(ip->dev, a[n]);**
          }
        }
        brelse(bp);
        **bfree(ip->dev, ip->addrs[NDIRECT+1]);
        ip->addrs[NDIRECT+1] = 0;**
      }
    ```
    
    - Double Indirect가 존재하는지 확인한다.
    - 만약 Double Indirect가 존재한다면, level 2 node의 원소를 먼저 free한다.
    - 이후 level 1 node의 원소를 free한다.
    - 마지막으로 ****`addrs[NDIRECT+1]`을 초기화 해 Double Indirect를 삭제한다.
    
- **Triple Indirect의 free**
    
    ```c
    	// truncate triple indirect block
      **if(ip->addrs[NDIRECT+2]){**
        int m, n, k;
    
        bp = bread(ip->dev, ip->addrs[NDIRECT+2]);
        a = (uint*)bp->data;
    
        // free first level block
        **for(k = 0; k < NINDIRECT; k++){**
          if(a[k]){
            struct buf *bpsec = bread(ip->dev, a[k]);
            **uint *asec = (uint*)bpsec->data;**
    
            // free second level block
            **for(m = 0; m < NINDIRECT; m++){**
              if(asec[m]){
                struct buf *bpthird = bread(ip->dev, asec[m]);
                **uint *athird = (uint*)bpthird->data;**
    
                // free third level block
                **for(n = 0; n < NINDIRECT; n++){**
                  if(athird[n])
                    **bfree(ip->dev, athird[n]);**
                }
    
                brelse(bpthird);
                **bfree(ip->dev, asec[m]);**
              }
            }
    
            brelse(bpsec);
            **bfree(ip->dev, a[k]);**
          }
        }
        brelse(bp);
        **bfree(ip->dev, ip->addrs[NDIRECT+2]);
        ip->addrs[NDIRECT+2] = 0;**
      }
    ```
    
    - Triple Indirect가 존재하는지 확인한다.
    - 만약 Triple Indirect가 존재한다면, level 3 node의 원소를 먼저 free한다.
    - 이후 level 2 node의 원소를 free한다.
    - 이후 level 1 node의 원소를 free한다.
    - 마지막으로 ****`addrs[NDIRECT+2]`을 초기화 해 Triple Indirect를 삭제한다.
    

## B. Symbolic Link

### 자료구조의 수정

---

```c
// stat.h
#define T_SYMLINK 4 // Symbolic link
```

`stat.h`에 symbolic link file임을 표시하는 parameter를 추가한다.

```c
// Max length of symbolic link path
**#define MAXSYM 16**

// On-disk inode structure
struct dinode {
...
  **char symlink[MAXSYM]; // Path of symbolic link**
};
```

```c
// file.h
// in-memory copy of an inode
struct inode {
...
  **char symlink[MAXSYM]; // path of symbolic link**
};
```

Symbolic link path의 최대 길이인 MAXSYM을 16으로 설정한다. 즉, symbolic link를 통해 redirection하는 file path의 최대 길이는 16이다.

dinode와 inode에 redirection path를 저장하는 문자열 symlink field를 추가한다.

### In program

---

```c
// ln.c
int
main(int argc, char *argv[])
{
  if(argc != 4){
    printf(2, "Usage: ln op old new\n");
    exit();
  }

  if(!strcmp(argv[1], "-h")){
    if(link(argv[2], argv[3]) < 0)
      printf(2, "link %s %s %s: failed\n", argv[1], argv[2], argv[3]);
    exit();
  }

  if(!strcmp(argv[1], "-s")){
    if(symlink(argv[2], argv[3]) < 0)
      printf(2, "symlink %s %s %s: failed\n", argv[1], argv[2], argv[3]);
    exit();
  }

  exit();
}
```

다음과 같이 프로그램 코드를 수정한다.

### sys_symlink

---

- **new file 생성**
    
    ```c
    // sysfile.c
    int
    sys_symlink(void)
    {
    ...
      **if((dp = create(new, T_SYMLINK, 0, 0)) == 0){**
        end_op();
        return -1;
      }
    ...
    ```
    
    - “new”를 path로 가지는 symbolic link file을 생성한다.
    
- **Redirection link 설정**
    
    ```c
    	// Set symbolic link
      safestrcpy(dp->symlink, old, PGSIZE);
    ...
      return 0;
    }
    ```
    
    - path “old”를 symbolic link file의 redirection path(symlink)로 설정한다.
    

### readsym

---

- **file이 유효한 symbolic link file인지 확인**
    
    ```c
    // fs.c
    int
    readsym(char *path, char *sympath)
    {
      struct inode *ip;
    
      // Get inode of path
      if((ip = namei(path)) == 0)
        return -1;
      ilock(ip);
      
      // Check if inode is a symbolic link
      if(ip->type != T_SYMLINK){
        iunlock(ip);
        return -1;
      }
    
      if((namei(ip->symlink)) == 0){
        iunlock(ip);
        return -1;
      }
    ...
    ```
    
    - path를 바탕으로 inode를 불러온다.
    - 해당 inode가 symbolic link file의 inode가 아니거나, redirection file이 존재하지 않으면 -1을 return하고 함수를 종료한다.
    
- **Redirection path 저장**
    
    ```c
    	// Read the path of the desitation file (ex. symbolic -> symbolic -> file)
      if(namei(ip->symlink)->type == T_SYMLINK){
        readsym(ip->symlink, sympath);
      }
      // Set sympath to the path of symbolic link
      else{
        safestrcpy(sympath, ip->symlink, MAXSYM);
      }
      
      iunlock(ip);
      return 0;
    }
    ```
    
    - 만약 symbolic link의 redirection file이 symbolic link file이라면, readsym을 recursive하게 실행해 최종 redirection path를 저장한다.
    - sympath에 redirection path를 저장한다.

### execute

---

- **Redirection 확인**
    
    ```c
    // exec.c
    int
    exec(char *path, char **argv)
    {
    ...
      // Path of symbolic link
      char sympath[16] = {0, };
    ...
      // Get path of symbolic link
      if(readsym(path, sympath) == 0){
        if((ip = namei(sympath)) == 0){
          end_op();
          cprintf("exec: fail\n");
          return -1;
        }
      }
      // Get path of normal file
      else{
        if((ip = namei(path)) == 0){
          end_op();
          cprintf("exec: fail\n");
          return -1;
        }
      }
    ...
    ```
    
    - 만약 path의 프로그램이 symbolic link라면, readsym을 통해 전달받은 sympath의 프로그램 inode (redirection inode)를 가져온다.
    - 만약 path의 프로그램이 symbolic link가 아니라면, path의 프로그램 inode를 가져온다.
    

### sys_open

---

- **Redirection 확인**
    
    ```c
    // sysfile.c
    int
    sys_open(void)
    {
      char *path;
      char sympath[16] = {0, };
    ...
      } else {
        // Get path of symbolic link
        if(readsym(path, sympath) == 0){
          if((ip = namei(sympath)) == 0){
            end_op();
            return -1;
          }
        }
        // Get path of normal file
        else{
          if((ip = namei(path)) == 0){
            end_op();
            return -1;
          }
        }
    ...
    ```
    
    - 만약 path의 file이 symbolic link라면, readsym을 통해 전달받은 sympath의 file inode를 가져온다.
    - 만약 path의 프로그램이 symbolic link가 아니라면, path의 file inode를 가져온다.

### iupdate, ilock

---

- **iupdate**
    
    ```c
    // fs.c
    void
    iupdate(struct inode *ip)
    {
    ...
      // Save symbolic link to disk inode
      if(ip->type == T_SYMLINK)
        safestrcpy(dip->symlink, ip->symlink, MAXSYM);
    ...
    }
    ```
    
    - memory에 존재하는 inode의 symbolic link를 disk에 반영한다.
    
- **ilock**
    
    ```c
    // fs.c
    void
    ilock(struct inode *ip)
    {
    ...
        // Load symbolic link from disk inode
        if(ip->type == T_SYMLINK)
          safestrcpy(ip->symlink, dip->symlink, MAXSYM);
    ...
    }
    ```
    
    - disk에 존재하는 dinode의 symbolic link를 inode로 가져온다.

### ls, symlink

---

- **symlink system call을 통한 올바른 meta data 출력**
    
    ```c
    // sysfile.c
    int
    syminfo(char *path, struct stat *st)
    {
      struct inode *ip;
    
      if((ip = namei(path)) == 0)
        return 0;
    
      ilock(ip);
      if(ip->type != T_SYMLINK){
        iunlockput(ip);
        return 0;
      }
    
      stati(ip, st);
      iunlockput(ip);
      return 1;
    }
    ```
    
    - 다음의 system call을 통해 meta data를 출력하고자 하는 file이 symbolic file인 경우 redirection file의 meta data가 아닌 symbolic file의 meta data를 stat에 저장해 이를 출력하도록 한다.
    
- **symlink system call의 사용**
    
    ```c
    // ls.c
    void
    ls(char *path)
    {
      ...
          if(stat(buf, &st) < 0){
            printf(1, "ls: cannot stat %s\n", buf);
            continue;
          }
          // Find a correct information
          syminfo(buf, &st);
          printf(1, "%s %d %d %d\n", fmtname(buf), st.type, st.ino, st.size);
    ...
    }
    ```
    
    - file의 meta data를 출력하는 과정에서 file이 symbolic link인지 확인한 후, 만약 symbolic link라면 stat st를 업데이트해 이를 출력하도록 한다.

## C. Sync

### sync

---

```c
// log.c
int
sync(void)
{
  begin_op();
  int i = commit();
  end_op();

  // Return the number of flushed blocks
  return i;
}
```

다음과 같이 sync()를 구현한다.

### begin_op, end_op, commit, log_write

---

- **begin_op**
    
    ```c
    // log.c
    void
    begin_op(void)
    {
      acquire(&log.lock);
      while(1){
        if(log.committing){
          sleep(&log, &log.lock);
        } else {
          log.outstanding += 1;
          release(&log.lock);
          break;
        }
      }
    }
    ```
    
    - 만약 buffer가 가득찼다면 begin_op 실행 후 commit이 실행되므로 begin_op에서 buffer의 크기를 검사하는 부분을 삭제한다.
    
- **end_op**
    
    ```c
    // log.c
    void
    end_op(void)
    {
      acquire(&log.lock);
      log.outstanding -= 1;
      if(log.committing)
        panic("log.committing");
      if(log.outstanding > 0) {
        // begin_op() may be waiting for log space,
        // and decrementing log.outstanding has decreased
        // the amount of reserved space.
        wakeup(&log);
      }
      release(&log.lock);
    }
    ```
    
    - group flush가 발생하지 않으므로 end_op에서 commit과 관련된 코드와 commit을 수행하는 부분을 삭제한다.
    
- **commit**
    
    ```c
    // log.c
    static int
    commit()
    {
      int n = -1;
    
      if (log.lh.n > 0) {
        n = log.lh.n;
        write_log();     // Write modified blocks from cache to log
        write_head();    // Write header to disk -- the real commit
        install_trans(); // Now install writes to home locations
        log.lh.n = 0;
        write_head();    // Erase the transaction from the log
      }
    
      return n;
    }
    ```
    
    - commit 후 flush한 block의 수를 return한다. 만약 commit에 실패했다면 -1을 return한다.
    
- **log_write**
    
    ```c
    // log.c
    void
    log_write(struct buf *b)
    {
      int i;
    // ------- DELETED ------- //
      if (log.outstanding < 1)
        panic("log_write outside of trans");
    
      acquire(&log.lock);
      for (i = 0; i < log.lh.n; i++) {
        if (log.lh.block[i] == b->blockno)   // log absorbtion
          break;
      }
      log.lh.block[i] = b->blockno;
      if (i == log.lh.n)
        log.lh.n++;
      b->flags |= B_DIRTY; // prevent eviction
      release(&log.lock);
    }
    ```
    
    - log buffer의 공간이 부족한 경우에도 log_write가 실행되므로 해당 함수에서 log buffer 공간 부족에 대한 panic을 삭제한다.

### lfull, bfull

---

- **lfull**
    
    ```c
    // log.c
    // Check if log buffer is about to full.
    int
    lfull(void)
    {
      int full = 0;
    
      acquire(&log.lock);
      if (log.lh.n + (log.outstanding+1)*MAXOPBLOCKS > LOGSIZE || log.lh.n >= log.size - 1){
        full = 1;
      }
      release(&log.lock);
    
      return full;
    }
    ```
    
    - log buffer에 데이터를 write할 공간이 부족할 시 1을 return한다.
    
- **bfull**
    
    ```c
    // bio.c
    // Check whether the buffer cache is full or not.
    // Return 1 if full, 0 if not full.
    int
    bfull(void)
    {
      struct buf *buffer;
    
      // Traverse the buffer cache to find an empty buffer.
      for(buffer = bcache.head.next; buffer != &bcache.head; buffer = buffer->next){
        if(buffer->refcnt != 0 || buffer->flags != B_DIRTY){
          return 0;
        }
      }
    
      // If buffer is full, return 1
      return 1;
    }
    ```
    
    - bcache의 모든 원소가 dirty할 시 1을 return한다.
    

### writei

---

- **buffer 용량 확인**
    
    ```c
    // fs.c
    int
    writei(struct inode *ip, char *src, uint off, uint n)
    {
    ...
      // sync if log buffer or cache is full
      if(lfull() || bfull())
        sync();
    ...
    }
    ```
    
    - write 하기 전 log buffer 혹은 bcache의 공간이 부족한지 확인한다.
    - 만약 공간이 부족하다면 sync를 호출해 buffer를 disk로 flush한다.
    

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

## C. Multi Indirect Test

---

- **Test의 구성**
    - Multi Indirect Test를 수행하기 위해 multi_test program을 만들었다.
    - 8MB file, 16MB file을 write, read, close하여 multi indirect가 잘 작동하는지 확인한다.
    - 8MB file을 생성하는 Test 1은 Double Indirect의 정상 작동을 확인한다. Test 1에서는 file을 삭제한 후, file이 잘 삭제되었는지를 확인한다.
    - 16MB file을 생성하는 Test 2는 Triple Indirect의 정상 작동을 확인한다.

- **Test 결과**
    - 다음과 같이 테스트가 정상 작동한다.
        
        ![Screenshot 2023-06-12 at 17.32.53.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.32.53.png)
        
    - 다음과 같이 16MB file이 정상 생성되었음을 확인할 수 있다.
        
        ![Screenshot 2023-06-12 at 17.33.38.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.33.38.png)
        

## D. Symbolic Link Test

---

- **Test의 구성**
    - ln program을 통해 README file에 대한 symbolic link file과 hard link file을 생성한다.
    - cat 명령어를 통해 각 file을 open한다.
    - README file을 삭제한 후 다시 cat 명령어를 이용해 file들을 open하여 정상 작동 여부를 확인한다.
    - ln program을 통해 zombie 명령어에 대한 symbolic link program과 hard link program을 생성한다.
    - 각 program을 실행한다.
    - zombie program을 삭제한 후 다시 각 program을 실행해 정상 작동 여부를 확인한다.

- **Test 결과**
    - 정상적으로 두 file 모두 open된다.
        
        ![Screenshot 2023-06-12 at 17.35.12.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.35.12.png)
        
        ![Screenshot 2023-06-12 at 17.35.46.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.35.46.png)
        
        ![Screenshot 2023-06-12 at 17.36.45.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.36.45.png)
        
    - README를 삭제하니 symbolic link file은 열리지 않는다.
        
        ![Screenshot 2023-06-12 at 17.37.34.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.37.34.png)
        
    - Symbolic link program과 hard link program이 모두 생성 및 실행된다.
        
        ![Screenshot 2023-06-12 at 17.39.30.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.39.30.png)
        
    - zombie를 삭제하니 symbolic link program은 실행되지 않는다.
        
        ![Screenshot 2023-06-12 at 17.40.21.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.40.21.png)
        

## E. Sync Test

---

- **Test의 구성**
    - Sync Test를 수행하기 위해 sync_test program을 만들었다.
    - 본질적인 구성은 Multi_test와 동일하다.
    - Test 0는 1KB file을 write, read, close한다. 이때 close 후 main에서 sync를 호출하도록 하여 Test 0 실행 과정에서 close에서 flush가 일어났는지를 확인한다.
    - Test 1은 8MB file을 write, read, close, remove하여 sync와 multi indirect 사이의 오류 여부를 확인한다. Test 1에서는 sync를 호출하지 않으며, buffer의 공간이 부족한 경우 sync가 잘 작동하는지 확인한다.
    - Test 2는 16MB file은 write, read, close, remove한다. write 후 sync를 호출하여 flush된 block의 수를 확인한다.
    
- **Test 결과**
    - Test 0가 종료된 후 sync를 호출하니 해당 시점에 buffer가 flush된다. 이를 통해 close에서는 buffer를 flush하지 않음을 알 수 있다.
        
        ![Screenshot 2023-06-12 at 17.46.24.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.46.24.png)
        
    - 8MB file을 정상적으로 만들고 삭제했다. 이를 통해 sync과 multi indirect를 동시에 사용하더라도 오류가 발생하지 않음을 알 수 있다.
        
        ![Screenshot 2023-06-12 at 17.47.42.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.47.42.png)
        
    - 16MB file을 정상적으로 만들고 삭제했다. write 수행 후 sync 호출로 41개 block이 commit되었음을 확인할 수 있다.
        
        ![Screenshot 2023-06-12 at 17.51.40.png](Project#3%20Wiki%207166163af0114c6e9f9fbf39945592ed/Screenshot_2023-06-12_at_17.51.40.png)
        

# 4. Trouble Shooting

## A. ls 명령어 사용 시 file의 meta data 출력 오류

---

ls 명령어를 사용해 file의 meta data를 출력할 시, 기존의 구현에서는 symbolic link의 file 정보가 redirection되는 file의 정보로 출력되는 문제가 있었다. 이는 symbolic link와 open의 구현 문제로, open에서는 file이 symbolic link라면 inode인 ip를 redirection inode로 변경하고, 최종 inode의 정보를 stat에 저장해 출력했기에 redirection file의 meta data가 출력된 것이다.

이를 해결하기 위해 file이 symbolic link인지 확인하고, 만약 symbolic link라면 symbolic link file의 meta data를 stat에 복사해 이를 출력하도록 하는 system call을 추가했다.

그러나, 해당 방식은 ls 명령어 실행 중 모든 file에 대해 symbolic link인지 여부를 확인해야 하기에 그 효율성이 떨어진다. 이 문제를 근본적으로 해결하기 위해서는 read-only open 시 file의 symbolic link 여부를 확인하지 않고 file 그 자체의 meta data를 file 구조체에 저장하게 하거나, symbolic link 구현 과정에서 symbolic link file에 old의 주소 공간을 할당해주는 등의 방식을 사용해야 할 것 같다.