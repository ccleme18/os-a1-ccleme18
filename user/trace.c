#include "kernel/types.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
  int mask = 0;

  if (argc < 3) {
    fprintf(2, "Usage: trace <mask> <command> [args...]\n");
    exit(1);
  }

  char *s = argv[1];

  if (*s == '\0') {
    fprintf(2, "Usage: trace <mask> <command> [args...]\n");
    exit(1);
  }

  for (int i = 0; s[i] != '\0'; i++) {
    if (s[i] < '0' || s[i] > '9') {
      fprintf(2, "Usage: trace <mask> <command> [args...]\n");
      exit(1);
    }

    int digit = s[i] - '0';

    if (mask > 214748364 ||
        (mask == 214748364 && digit > 7)) {
      fprintf(2, "Usage: trace <mask> <command> [args...]\n");
      exit(1);
    }

    mask = mask * 10 + digit;
  }

  if (trace(mask) < 0) {
    fprintf(2, "trace: system call failed\n");
    exit(1);
  }

  exec(argv[2], &argv[2]);

  fprintf(2, "trace: exec %s failed\n", argv[2]);
  exit(1);
}
