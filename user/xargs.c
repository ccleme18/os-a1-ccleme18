
#include "kernel/types.h"
#include "kernel/param.h"
#include "user/user.h"

#define MAXWORD 128

// Keep the word storage outside the function stack.
static char words[MAXARG][MAXWORD];

static void
usage(void)
{
  fprintf(2, "xargs [-n N] cmd [args...]\n");
  exit(1);
}

// Run the command with its original arguments followed by input words.
// Return 1 if the command succeeds, or 0 if it fails.
static int
run_command(char **base, int base_count, int word_count)
{
  char *args[MAXARG + 1];
  int i;
  int pid;
  int status = 0;

  if (base_count + word_count > MAXARG) {
    fprintf(2, "xargs: too many arguments\n");
    return 0;
  }

  for (i = 0; i < base_count; i++)
    args[i] = base[i];

  for (i = 0; i < word_count; i++)
    args[base_count + i] = words[i];

  args[base_count + word_count] = 0;

  pid = fork();

  if (pid < 0) {
    fprintf(2, "xargs: fork failed\n");
    return 0;
  }

  if (pid == 0) {
    exec(args[0], args);
    fprintf(2, "xargs: exec %s failed\n", args[0]);
    exit(1);
  }

  if (wait(&status) < 0) {
    fprintf(2, "xargs: wait failed\n");
    return 0;
  }

  return status == 0;
}

int
main(int argc, char *argv[])
{
  int cmd_index = 1;
  int use_n = 0;
  int n = 0;
  int base_count;
  int batch_limit;
  int word_count = 0;
  int word_len = 0;
  int all_ok = 1;
  int read_result;
  char ch;
  char **base;

  if (argc < 2)
    usage();

  // Parse the optional -n N argument.
  if (strcmp(argv[1], "-n") == 0) {
    if (argc < 4)
      usage();

    if (argv[2][0] == '\0')
      usage();

    for (int i = 0; argv[2][i] != '\0'; i++) {
      int digit;

      if (argv[2][i] < '0' || argv[2][i] > '9')
        usage();

      digit = argv[2][i] - '0';

      if (n > 214748364 ||
          (n == 214748364 && digit > 7))
        usage();

      n = n * 10 + digit;
    }

    if (n < 1)
      usage();

    use_n = 1;
    cmd_index = 3;
  }

  base = &argv[cmd_index];
  base_count = argc - cmd_index;

  if (base_count < 1 || base_count > MAXARG)
    usage();

  // Never exceed MAXARG, including the command and its fixed arguments.
  batch_limit = MAXARG - base_count;

  if (use_n && n < batch_limit)
    batch_limit = n;

  // Read standard input one character at a time.
  while ((read_result = read(0, &ch, 1)) == 1) {
    if (ch == ' ' || ch == '\t' || ch == '\n') {
      // Finish the current word, if there is one.
      if (word_len > 0) {
        words[word_count][word_len] = '\0';
        word_count++;
        word_len = 0;

        // In -n mode, run whenever the batch is full.
        if (use_n && word_count == batch_limit) {
          if (!run_command(base, base_count, word_count))
            all_ok = 0;

          word_count = 0;
        }
      }

      // Without -n, each non-empty line is a separate command.
      if (ch == '\n' && !use_n && word_count > 0) {
        if (!run_command(base, base_count, word_count))
          all_ok = 0;

        word_count = 0;
      }
    } else {
      // Make sure another input word can fit in the argument list.
      if (word_len == 0 && base_count + word_count >= MAXARG) {
        fprintf(2, "xargs: too many arguments\n");
        exit(1);
      }

      // Reject words that exceed our fixed-size word buffer.
      if (word_len >= MAXWORD - 1) {
        fprintf(2, "xargs: input word too long\n");
        exit(1);
      }

      words[word_count][word_len++] = ch;
    }
  }

  if (read_result < 0) {
    fprintf(2, "xargs: input read failed\n");
    exit(1);
  }

  // Process a final word even if the input did not end in a newline.
  if (word_len > 0) {
    words[word_count][word_len] = '\0';
    word_count++;
  }

  // Process any remaining words at EOF.
  if (word_count > 0) {
    if (!run_command(base, base_count, word_count))
      all_ok = 0;
  }

  exit(all_ok ? 0 : 1);
}
