#include <ctype.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 8080
#define BACKLOG 5
#define MAX_REQ_LEN 1024
#define MAX_BLOCK_SIZE 1024

#define DEFAULT_EXT "Content-Type: application/octet-stream"

void handle_connection(int cli_socket) {
    char buffer[MAX_REQ_LEN];
    char block_buf[MAX_BLOCK_SIZE];
    char *file_ext;
    char *method;
    char *url;
    char *httpvers;
    char *header, *value;
    char *contenttype = DEFAULT_EXT;
    char contentlen[32] = "Content-Length: ";

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
        char *res = "HTTP/1.1 404 NOT FOUND\r\n";
        write(cli_socket, res, strlen(res));
        write(cli_socket, "\r\n", 4);
        perror("webserver: Closing connection, file not found");
        close(cli_socket);
        exit(EXIT_FAILURE);
    }

    //Checking file size to set content-len
    struct stat file_st;
    long file_size;
    if (stat(url+1, &file_st) == 0) {
        file_size = file_st.st_size;
        char len[sizeof(long)*8+1];
        sprintf(len, "%ld", file_size);
        strcat(contentlen, len);
        strcat(contentlen, "\r\n");
    }

    //Checking file extension to set content-type
    file_ext = strrchr(url, '.');
    if (strcmp(file_ext, ".html") == 0 || strcmp(file_ext, ".htm") == 0) {
        contenttype = "Content-Type: text/html\r\n";
    }
    else if (strcmp(file_ext, "txt") == 0) { 
        contenttype = "Content-Type: text/plain\r\n"; 
    }
    else if (strcmp(file_ext, ".jpeg") == 0 || strcmp(file_ext, ".jpg") == 0) {
        contenttype = "Content-Type: image/jpg\r\n"; 
    }
    else if (strcmp(file_ext, ".png") == 0) { 
        contenttype = "Content-Type: image/png\r\n"; 
    }
    else if (strcmp(file_ext, ".pdf") == 0) { 
        contenttype = "Content-Type: application/pdf\r\n"; 
    }

    // TO DO: if file exist, save the content to buffer
    char file_buf[file_size * sizeof(char)];
    memset(file_buf, 0, file_size * sizeof(char));
    if (read(file_fd, file_buf, file_size) < 0){
		perror("webserver: Read error.");
	}

    //Send response message
    char *res = "HTTP/1.1 200 OK\r\n";
    write(cli_socket, res, strlen(res));
    write(cli_socket, contentlen, strlen(contentlen));
    write(cli_socket, contenttype, strlen(contenttype));
    write(cli_socket, "\r\n", 4);
    write(cli_socket, file_buf, strlen(file_buf));
    close(file_fd);
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
    // READ MORE: https://man7.org/linux/man-pages/man7/ip.7.html#:~:text=sin_addr%20is%20the%20IP%20host,address%20in%20network%20byte%20order.        
    // INADDR_ANY - allows socket to be binded to any IP addresses.
    struct sockaddr_in serv_addr, cli_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = INADDR_ANY;
    serv_addr.sin_port = htons(PORT);
    int addrlen = sizeof(serv_addr);
    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("webserver: setsockopt() failure error.");
        exit(EXIT_FAILURE);
    }

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

        //call handle_connection to handle requests from connected clients
        handle_connection(newsock_fd);
    }
}