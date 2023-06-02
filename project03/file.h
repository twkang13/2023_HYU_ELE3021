struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE } type; // File type에 symbolic link 관련 내용 추가, path로 redirection
  int ref; // reference count
  char readable;
  char writable;
  struct pipe *pipe;
  struct inode *ip;
  uint off;
};


// in-memory copy of an inode
struct inode {
  uint dev;           // Device number
  uint inum;          // Inode number
  int ref;            // Reference count
  struct sleeplock lock; // protects everything below here
  int valid;          // inode has been read from disk?

  short type;         // copy of disk inode
  short major;
  short minor;
  short nlink;
  uint size;
  // TODO : 바꾸기, address 완전히 공유하고 있으면 hard link. inode 자체를 공유는 x
  //        pointer를 다른 file로 redirection하면 soft link
  uint addrs[NDIRECT+3];
};

// table mapping major device number to
// device functions
struct devsw {
  int (*read)(struct inode*, char*, int);
  int (*write)(struct inode*, char*, int);
};

extern struct devsw devsw[];

#define CONSOLE 1
