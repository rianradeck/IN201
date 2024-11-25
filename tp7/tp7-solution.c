#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

//  echo -e "toto\ntutu" | nc localhost 8888

void error(char *msg) {
  perror(msg);
  exit(1);
}

int main(void) {
  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sockfd < 0)
    error("ERROR opening socket");
  struct sockaddr_in serv_addr, cli_addr;
  bzero((char *)&serv_addr, sizeof(serv_addr));
  const int portno = 8888;
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port = htons(portno);
  if (bind(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    error("ERROR on binding");
  listen(sockfd, 5);
  unsigned int clilen = sizeof(cli_addr);

  int nb_coonection = 0;
  while (1) {
    int newsockfd = accept(sockfd, (struct sockaddr *)&cli_addr, &clilen);
    ++nb_coonection;
    int pid = fork();
    if (pid == 0) {
      if (newsockfd < 0)
        error("ERROR on accept");

      // Remplacer à partir d’ici
      dup2(newsockfd, STDOUT_FILENO);
      dup2(newsockfd, STDIN_FILENO);
      close(sockfd);

      // char buffer[256];
      // bzero(buffer, 256);
      // int n = read(newsockfd, buffer, 255);
      // if (n < 0)
      //   error("ERROR reading from socket");
      // printf("Here is the message: %s\n", buffer);
      if (nb_coonection % 2 == 1) {
        execlp("./toupper", "toupper", NULL);
        // execlp only returns if an error occured
        printf("couldn't execute: %s", "toupper");
        return 127;
      } else {
        system("nc <INSERT_IP_HERE> 2222");
        return 0;
      }

    } else {
      close(newsockfd);
    }
    // char buffer[256];
    // bzero(buffer, 256);
    // int n = read(newsockfd, buffer, 255);
    // if (n < 0)
    //   error("ERROR reading from socket");
    // // printf("Here is the message: %s\n", buffer);

    // n = write(newsockfd, "I got your message\n", 18);
    // if (n < 0)
    //   error("ERROR writing to socket");
  }
  return 0;
}
