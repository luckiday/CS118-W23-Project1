#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 15635

int main(int argc, char const *argv[]) {
  int server_fd, new_socket, valread;
  struct sockaddr_in address;
  int opt = 1;
  int addrlen = sizeof(address);
  char buffer[1024] = {0};
  char *responsemsg =
      "HTTP/1.1 200 OK\nConnection: close\nDate: Tue, 18 Aug 2015 15:44:04 "
      "GMT\nServer: Apache/2.2.3 (CentOS)\nLast-Modified: Tue, 18 Aug 2015 "
      "15:11:03 GMT\nContent-Length: 6821\nontent-Type: text/html\n";

  char *parsedmsg[] = {0};
  char *command = "";
  char *path = "";
  char *host = "";
  bool connection = true;
  char *useragent = "";

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
    perror("socket failed");
    exit(EXIT_FAILURE);
  }

  // Forcefully attaching socket to the port 15635
  // Note:
  // https://stackoverflow.com/questions/58599070/socket-programming-setsockopt-protocol-not-available
  if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }
  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(PORT);

  // Forcefully attaching socket to the port 15635
  if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
    perror("bind failed");
    exit(EXIT_FAILURE);
  }
  if (listen(server_fd, 3) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
  if ((new_socket = accept(server_fd, (struct sockaddr *)&address,
                           (socklen_t *)&addrlen)) < 0) {
    perror("accept");
    exit(EXIT_FAILURE);
  }
  valread = read(new_socket, buffer, 1024);

  int iterator = 0;
  const char delimeter[2] = " ";
  char *token = strtok(buffer, delimeter);
  while (token != NULL) {
    // parsedmsg[iterator] = token;
    printf("%s\n", token);
    // iterator++;
    token = strtok(NULL, delimeter);
  }

  // printf("%s\n", buffer);
  send(new_socket, responsemsg, strlen(responsemsg), 0);
  return 0;
}
