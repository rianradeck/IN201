#include <ctype.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>

#define BUFFER_SIZE 1024
#define HOST_NAME_LEN 100

int main() {
  char buffer[BUFFER_SIZE];
  char host[HOST_NAME_LEN];
  gethostname(host, HOST_NAME_LEN);

  while (fgets(buffer, BUFFER_SIZE, stdin) != NULL) {
    sleep(3);
    for (uint i = 0; i < BUFFER_SIZE; ++i) {
      buffer[i] = toupper(buffer[i]);
    }
    printf("%s[%d]: %s", host, getpid(), buffer);
    fflush(stdout);
  }
}
