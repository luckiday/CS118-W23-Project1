#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 15635
#define BACKLOG 5
#define MAX_REQ_LEN 1024



void handle_connection(int cli_socket) {
    char buffer[MAX_REQ_LEN];
    char *method;
    char *url;
    char *httpvers;
    char *header, *value;

    int valread = read(cli_socket, buffer, 1024);

    method = strtok(buffer, " ");
    printf("Method: %s\n", method);
    url = strtok(NULL, " ");
    printf("URL: %s\n", url);
    httpvers = strtok(NULL, "\r\n");
    printf("HTTP Version: %s\n", httpvers);

    //TO DO: Parse header values
    /*while ((header = strtok(NULL, ": ")) != NULL) {
        value = strtok(NULL, "\r\n");
    }
    printf("%s: %s\n", header, value);*/

    int file_fd = open(url + 1, O_RDONLY, S_IRUSR);
    
    if (file_fd == -1) {
        char *res = "HTTP/1.0 404 Not Found\r\n\r\n";
        write(cli_socket, res, 100);
        perror("webserver: Closing connection, file not found");
        close(cli_socket);
    }
    write(cli_socket, "HTTP/1.0 200 OK\r\n\r\n", 100);
    close(cli_socket);



}

int main(int argc, char const *argv[]) {
    int server_fd, newsock_fd;

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

    // Server listening indefinitely
    while(1) {

        if ((newsock_fd = accept(server_fd, (struct sockaddr *) &cli_addr, (socklen_t * ) & addrlen)) == -1) {
            perror("webserver:  accept() failure error.");
            exit(EXIT_FAILURE);
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &(cli_addr.sin_addr), client_ip, INET_ADDRSTRLEN);
        printf("webserver: connected to client: %s\n", client_ip);

        handle_connection(newsock_fd);
    }
}