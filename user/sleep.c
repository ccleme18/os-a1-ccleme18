
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

int
main(int argc, char *argv[])
{
    if (argc != 2) {
        fprintf(2, "sleep <ticks>\n");
        exit(1);
    }

    char *s = argv[1];

    if (*s == '\0') {
        fprintf(2, "sleep <ticks>\n");
        exit(1);
    }

    int ticks = 0;

    for (int i = 0; s[i] != '\0'; i++) {
        if (s[i] < '0' || s[i] > '9') {
            fprintf(2, "sleep <ticks>\n");
            exit(1);
        }

        int digit = s[i] - '0';

        if (ticks > 214748364 ||
            (ticks == 214748364 && digit > 7)) {
            fprintf(2, "sleep <ticks>\n");
            exit(1);
        }

        ticks = ticks * 10 + digit;
    }

    pause(ticks);
    exit(0);
}
