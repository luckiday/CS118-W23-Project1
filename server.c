#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>

#define PORT 15635
#define BACKLOG 5

int main(int argc, char const *argv[]) {
    int server_fd, newsock_fd, valread;

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

    // Create a TCP server socket using socket() syscall
    // socket() returns -1 if failed.
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("webserver: socket() creation error.");
        exit(EXIT_FAILURE);
    }

    // Initialize socket address info 
    // htons: func to convert int from host byte order to network byte order
    // READ MORE: https://man7.org/linux/man-pages/man7/ip.7.html#:~:text=sin_addr%20is%20the%20IP%20host,address%20in%20network%20byte%20order.        // INADDR_ANY - allows socket to be binded to any IP addresses.
    struct sockaddr_in serv_addr, cli_addr;
    int addrlen = sizeof(serv_addr);
    int opt = 1;

    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("webserver: setsockopt() failure error.");
        exit(EXIT_FAILURE);
    }
    
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);

    // Bind the server socket to a local IP address and port number
    if (bind(server_fd, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) == -1) {
        perror("webserver: bind() failure error.");
        exit(EXIT_FAILURE);
    }

    //Server starts to listen for connection.
    if (listen(server_fd, BACKLOG) == -1) {
        perror("webserver: listen() failure error.");
        exit(EXIT_FAILURE);
    }

    printf("webserver: server is listening on port: %d\n", PORT);

    // To listen continously
    while(1) {
        if ((newsock_fd = accept(server_fd, (struct sockaddr *) &cli_addr, (socklen_t * ) & addrlen)) == -1) {
            perror("webserver:  accept() failure error.");
            exit(EXIT_FAILURE);
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("webserver: connected to client: %s\n", client_ip);

        valread = read(newsock_fd, buffer, 1024);
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
        send(newsock_fd, responsemsg, strlen(responsemsg), 0);
    }
}