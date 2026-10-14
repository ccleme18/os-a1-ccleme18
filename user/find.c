
#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"
#include "kernel/fcntl.h"

#define MAXPATH 512

// Match a filename against a pattern.
// '*' matches any sequence of characters, including an empty sequence.

int
match(char *pattern, char *name)
{
  if (*pattern == '\0')
    return *name == '\0';

  if (*pattern == '*') {
    // '*' can match zero characters.
    if (match(pattern + 1, name))
      return 1;

    // Or it can match one or more characters.
    if (*name != '\0')
      return match(pattern, name + 1);

    return 0;
  }

  if (*pattern == *name && *name != '\0')
    return match(pattern + 1, name + 1);

  return 0;
}


// Search recursively. The starting directory itself is not printed.
void
find(char *path, char *pattern, int type_filter, int check_open)
{
  char buf[MAXPATH];
  char *p;
  int fd;
  struct dirent de;
  struct stat st;

  if ((fd = open(path, O_RDONLY)) < 0) {
    if (check_open)
      fprintf(2, "find: cannot open %s\n", path);
    return;
  }

  if (fstat(fd, &st) < 0) {
    close(fd);
    return;
  }

  if (st.type != T_DIR) {
    close(fd);
    return;
  }

  if (strlen(path) + 1 + DIRSIZ + 1 > sizeof(buf)) {
    close(fd);
    return;
  }

  strcpy(buf, path);
  p = buf + strlen(buf);
  *p++ = '/';

  while (read(fd, &de, sizeof(de)) == sizeof(de)) {
    if (de.inum == 0)
      continue;

    memmove(p, de.name, DIRSIZ);
    p[DIRSIZ] = '\0';

    // xv6 directory names are blank-padded.
    char name[DIRSIZ + 1];
    memmove(name, de.name, DIRSIZ);
    name[DIRSIZ] = '\0';

    int len = strlen(name);
    while (len > 0 && name[len - 1] == ' ')
      name[--len] = '\0';

    if (strcmp(name, ".") == 0 || strcmp(name, "..") == 0)
      continue;

    if (stat(buf, &st) < 0)
      continue;

    int matches_type =
      (type_filter == 0) ||
      (type_filter == 'f' && (st.type == T_FILE || st.type == T_DEVICE)) ||
      (type_filter == 'd' && st.type == T_DIR);

    if (match(pattern, name) && matches_type)
      printf("%s\n", buf);

    if (st.type == T_DIR)
      find(buf, pattern, type_filter, 0);
  }

  close(fd);
}

int
main(int argc, char *argv[])
{
  char *dir;
  char *pattern;
  int type_filter = 0;

  if (argc != 3 && argc != 5) {
    fprintf(2, "find <dir> <pattern> [-type f|d]\n");
    exit(1);
  }

  dir = argv[1];
  pattern = argv[2];

  if (argc == 5) {
    if (strcmp(argv[3], "-type") != 0 ||
        (strcmp(argv[4], "f") != 0 && strcmp(argv[4], "d") != 0)) {
      fprintf(2, "find <dir> <pattern> [-type f|d]\n");
      exit(1);
    }
    type_filter = argv[4][0];
  }

  int fd = open(dir, O_RDONLY);
  struct stat st;

  if (fd < 0 || fstat(fd, &st) < 0 || st.type != T_DIR) {
    if (fd >= 0)
      close(fd);
    fprintf(2, "find: cannot open %s\n", dir);
    exit(1);
  }
  close(fd);

  find(dir, pattern, type_filter, 0);
  exit(0);
}
