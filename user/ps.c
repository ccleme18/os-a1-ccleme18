
#include "kernel/types.h"
#include "kernel/uproc.h"
#include "user/user.h"

int
main(void)
{
  struct uproc procs[64];
  int n = getprocs(procs, 64);

  if (n < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  printf("PID       PPID STATE SIZE      NAME\n");

  for (int i = 0; i < n; i++) {
    char *state;

    switch (procs[i].state) {
    case 1:
      state = "used";
      break;
    case 2:
      state = "sleeping";
      break;
    case 3:
      state = "runnable";
      break;
    case 4:
      state = "running";
      break;
    case 5:
      state = "zombie";
      break;
    default:
      state = "unknown";
      break;
    }

    printf("%d\t%d\t%s\t%lu\t%s\n",
           procs[i].pid, procs[i].ppid, state,
           procs[i].sz, procs[i].name);
  }

  exit(0);
}
