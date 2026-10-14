#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"
#include "defs.h"

// Fetch the uint64 at addr from the current process.
int
fetchaddr(uint64 addr, uint64 *ip)
{
  struct proc *p = myproc();
  if (addr >= p->sz ||
      addr + sizeof(uint64) > p->sz) // both tests needed, in case of overflow
    return -1;
  if (copyin(p->pagetable, p->sz, (char *)ip, addr, sizeof(*ip)) != 0)
    return -1;
  return 0;
}

// Fetch the nul-terminated string at addr from the current process.
// Returns length of string, not including nul, or -1 for error.
int
fetchstr(uint64 addr, char *buf, int max)
{
  struct proc *p = myproc();
  if (copyinstr(p->pagetable, p->sz, buf, addr, max) < 0)
    return -1;
  return strlen(buf);
}

static uint64
argraw(int n)
{
  struct proc *p = myproc();
  switch (n) {
  case 0:
    return p->trapframe->a0;
  case 1:
    return p->trapframe->a1;
  case 2:
    return p->trapframe->a2;
  case 3:
    return p->trapframe->a3;
  case 4:
    return p->trapframe->a4;
  case 5:
    return p->trapframe->a5;
  }
  panic("argraw");
  return -1;
}

// Fetch the nth 32-bit system call argument.
void
argint(int n, int *ip)
{
  *ip = argraw(n);
}

// Retrieve an argument as a pointer.
// Doesn't check for legality, since
// copyin/copyout will do that.
void
argaddr(int n, uint64 *ip)
{
  *ip = argraw(n);
}

// Fetch the nth word-sized system call argument as a null-terminated string.
// Copies into buf, at most max.
// Returns string length if OK (not including nul), -1 if error.
int
argstr(int n, char *buf, int max)
{
  uint64 addr;
  argaddr(n, &addr);
  return fetchstr(addr, buf, max);
}

// Prototypes for the functions that handle system calls.
extern uint64 sys_fork(void);
extern uint64 sys_exit(void);
extern uint64 sys_wait(void);
extern uint64 sys_pipe(void);
extern uint64 sys_read(void);
extern uint64 sys_kill(void);
extern uint64 sys_exec(void);
extern uint64 sys_fstat(void);
extern uint64 sys_chdir(void);
extern uint64 sys_dup(void);
extern uint64 sys_getpid(void);
extern uint64 sys_sbrk(void);
extern uint64 sys_pause(void);
extern uint64 sys_uptime(void);
extern uint64 sys_open(void);
extern uint64 sys_write(void);
extern uint64 sys_mknod(void);
extern uint64 sys_unlink(void);
extern uint64 sys_link(void);
extern uint64 sys_mkdir(void);
extern uint64 sys_close(void);
extern uint64 sys_sync(void);
extern uint64 sys_trace(void);
extern uint64 sys_sysinfo(void);

// An array mapping syscall numbers from syscall.h
// to the function that handles the system call.
static uint64 (*syscalls[])(void) = {
  // clang-format off
  [SYS_fork]    = sys_fork,
  [SYS_exit]    = sys_exit,
  [SYS_wait]    = sys_wait,
  [SYS_pipe]    = sys_pipe,
  [SYS_read]    = sys_read,
  [SYS_kill]    = sys_kill,
  [SYS_exec]    = sys_exec,
  [SYS_fstat]   = sys_fstat,
  [SYS_chdir]   = sys_chdir,
  [SYS_dup]     = sys_dup,
  [SYS_getpid]  = sys_getpid,
  [SYS_sbrk]    = sys_sbrk,
  [SYS_pause]   = sys_pause,
  [SYS_uptime]  = sys_uptime,
  [SYS_open]    = sys_open,
  [SYS_write]   = sys_write,
  [SYS_mknod]   = sys_mknod,
  [SYS_unlink]  = sys_unlink,
  [SYS_link]    = sys_link,
  [SYS_mkdir]   = sys_mkdir,
  [SYS_close]   = sys_close,
  [SYS_sync]    = sys_sync,
  [SYS_trace]   = sys_trace,
  [SYS_sysinfo] = sys_sysinfo,
  // clang-format on
};

static char *syscall_names[] = {
  [SYS_fork]    = "fork",
  [SYS_exit]    = "exit",
  [SYS_wait]    = "wait",
  [SYS_pipe]    = "pipe",
  [SYS_read]    = "read",
  [SYS_kill]    = "kill",
  [SYS_exec]    = "exec",
  [SYS_fstat]   = "fstat",
  [SYS_chdir]   = "chdir",
  [SYS_dup]     = "dup",
  [SYS_getpid]  = "getpid",
  [SYS_sbrk]    = "sbrk",
  [SYS_pause]   = "pause",
  [SYS_uptime]  = "uptime",
  [SYS_open]    = "open",
  [SYS_write]   = "write",
  [SYS_mknod]   = "mknod",
  [SYS_unlink]  = "unlink",
  [SYS_link]    = "link",
  [SYS_mkdir]   = "mkdir",
  [SYS_close]   = "close",
  [SYS_sync]    = "sync",
  [SYS_trace]   = "trace",
};

void
syscall(void)
{
  int num;
  uint64 args[6];
  uint64 ret;
  struct proc *p = myproc();
  char strarg[128];
  int is_string = 0;
  int no_arg = 0;
  int have_string = 0;

  num = p->trapframe->a7;

  if (num > 0 && num < NELEM(syscalls) && syscalls[num]) {
    // Save argument registers before calling the handler.
    args[0] = p->trapframe->a0;
    args[1] = p->trapframe->a1;
    args[2] = p->trapframe->a2;
    args[3] = p->trapframe->a3;
    args[4] = p->trapframe->a4;
    args[5] = p->trapframe->a5;

    // These system calls take no argument that we print.
    if (num == SYS_fork || num == SYS_getpid ||
        num == SYS_uptime || num == SYS_sync) {
      no_arg = 1;
    }

    // These system calls take a string as their first argument.
    if (num == SYS_open || num == SYS_exec ||
        num == SYS_chdir || num == SYS_mkdir ||
        num == SYS_unlink || num == SYS_link ||
        num == SYS_mknod) {
      uint64 addr = args[0];

      is_string = 1;
      if (fetchstr(addr, strarg, sizeof(strarg)) >= 0)
        have_string = 1;
    }

    // Call the handler and save its return value.
    ret = syscalls[num]();
    p->trapframe->a0 = ret;

    // Check the mask after the handler so trace(mask) can trace itself.
    if ((p->tracemask & (1 << num)) && num != SYS_exit) {
      if (no_arg) {
        printk("[%d] %s() -> %d\n",
               p->pid, syscall_names[num], (int)ret);
      } else if (is_string) {
        if (have_string) {
          printk("[%d] %s(\"%s\") -> %d\n",
                 p->pid, syscall_names[num], strarg, (int)ret);
        } else {
          printk("[%d] %s(?) -> %d\n",
                 p->pid, syscall_names[num], (int)ret);
        }
      } else {
        printk("[%d] %s(%d) -> %d\n",
               p->pid, syscall_names[num], (int)args[0], (int)ret);
      }
    }
  } else {
    printk("%d %s: unknown sys call %d\n", p->pid, p->name, num);
    p->trapframe->a0 = -1;
  }
}
