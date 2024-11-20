
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <linux/seccomp.h>
#include <sys/prctl.h>
#include <sys/syscall.h>

int cmd_plus(char *buffer, int out_fileno) {
  int arg1, arg2, n;
  sscanf(buffer, "%d,%d", &arg1, &arg2);
  n = sprintf(buffer, "%d", arg1 + arg2);
  write(out_fileno, buffer, n);
  return 0;
}

int cmd_minus(char *buffer, int out_fileno) {
  int arg1, arg2, n;
  sscanf(buffer, "%d,%d", &arg1, &arg2);
  n = sprintf(buffer, "%d", arg1 - arg2);
  write(out_fileno, buffer, n);
  return 0;
}

int cmd_exec(const char *buffer) { return system(buffer); }

// echo "ecat /etc/passwd > ./attack.result" | ./a.out

#define READ 0
#define WRITE 1

// Select which question to run
#define Q2 0
#define Q4 1
#define Q5 2
#define Q Q5

#define BUFFER_SIZE 100

int main() {
  char buffer[BUFFER_SIZE];
#if Q == Q2
  read(STDIN_FILENO, buffer, BUFFER_SIZE);
  switch (buffer[0]) {
  case '+':
    return cmd_plus(buffer + 1, STDOUT_FILENO);
  case '-':
    return cmd_minus(buffer + 1, STDOUT_FILENO);
  case 'e':
    return cmd_exec(buffer + 1);
  }
  return 0;

#elif Q == Q4
  prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);
  read(STDIN_FILENO, buffer, BUFFER_SIZE);
  switch (buffer[0]) {
  case '+':
    syscall(SYS_exit, cmd_plus(buffer + 1, STDOUT_FILENO));
  case '-':
    syscall(SYS_exit, cmd_minus(buffer + 1, STDOUT_FILENO));
  case 'e':
    syscall(SYS_exit, cmd_exec(buffer + 1));
  }
  syscall(SYS_exit, 0);

#elif Q == Q5
  // Create two directional pipes
  int child2parent[2];
  int parent2child[2];
  pipe(child2parent);
  pipe(parent2child);

  if (fork() == 0) {
    // In child
    close(child2parent[READ]);
    close(parent2child[WRITE]);

    prctl(PR_SET_SECCOMP, SECCOMP_MODE_STRICT);

    read(parent2child[READ], buffer, BUFFER_SIZE);
    int n, writ;
    switch (buffer[0]) {
    case '+':
      n = cmd_plus(buffer + 1, child2parent[WRITE]);
      return n;
    case '-':
      n = cmd_minus(buffer + 1, child2parent[WRITE]);
      return n;
    case 'e':
      n = cmd_exec(buffer + 1);
      writ = sprintf(buffer, "%d\n", n);
      write(child2parent[WRITE], buffer, writ);
      return n;
      break;
    }

  } else {
    // In parent
    close(child2parent[WRITE]);
    close(parent2child[READ]);

    read(STDIN_FILENO, buffer, BUFFER_SIZE);
    write(parent2child[WRITE], buffer, BUFFER_SIZE);
    // sleep(1);
    int rd = read(child2parent[READ], buffer, BUFFER_SIZE);
    write(STDOUT_FILENO, buffer, rd);
  }
#endif
}
