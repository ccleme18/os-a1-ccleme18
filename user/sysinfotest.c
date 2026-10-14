
#include "kernel/types.h"
#include "kernel/sysinfo.h"
#include "user/user.h"

int
main(void)
{
  struct sysinfo info;

  if (sysinfo(&info) < 0) {
    fprintf(2, "sysinfotest: sysinfo failed\n");
    exit(1);
  }

  printf("Free memory: %lu bytes\n", info.freemem);
  printf("Processes:   %lu\n", info.nproc);
  printf("Open files:  %lu\n", info.nopenfile);

  exit(0);
}

