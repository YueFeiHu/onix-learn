#ifndef ONIX_SYSCALL_H
#define ONIX_SYSCALL_H

#include <onix/types.h>
typedef enum syscall_t
{
  SYS_NR_TEST,
  SYS_NR_EXIT = 1,
  SYS_NR_FORK = 2,
  SYS_NR_WRITE = 4,
  SYS_NR_GETPID = 20,
  SYS_NR_BRK = 45,
  SYS_NR_GETPPID = 64,
  SYS_NR_SLEEP = 158,
  SYS_NR_YIELD = 162,
}syscall_t;

u32 test();
pid_t fork();
void exit(int status);
void yield();
void sleep(u32 ms);
pid_t getpid();
pid_t getppid();
int32 brk(void *addr);
int32 write(fd_t fd, char *buf, u32 len);
#endif