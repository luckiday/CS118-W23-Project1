#include <ctype.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <unistd.h>

#define PORT 15635
#define BACKLOG 5
#define MAX_MSG_LEN 1024

#define DEFAULT_EXT "application/octet-stream"

// Handle client's GET request
void handle_GET(int cli_socket, char* filePath) {
    // open requested file
    FILE* f_fd = fopen(filePath, "rb");
    if (f_fd == NULL) {
        char res_buffer[1024];
        sprintf(res_buffer, "HTTP/1.1 404 NOT FOUND\r\n"
                            "\r\n");
        send(cli_socket, res_buffer, strlen(res_buffer), 0);
        perror("webserver: ERROR404=File Not Found! Closing connection...");
        close(cli_socket);
        exit(EXIT_FAILURE);
    }

    // Checking file size to set content-len
    long f_size;
    fseek(f_fd, 0, SEEK_END);
    f_size = ftell(f_fd);
    rewind(f_fd);
    char f_len[sizeof(long)*8+1];
    sprintf(f_len, "%ld", f_size);

    // Checking file extension to set content-type
    // Two additional extensions supported over the minimum given
    char *f_ext;
    char *contenttype = DEFAULT_EXT;
    if (strchr(filePath, '.') != NULL) {
        f_ext = strrchr(filePath, '.');
        if (strcmp(f_ext, ".html") == 0 || strcmp(f_ext, ".htm") == 0) {
            contenttype = "text/html";
        } else if (strcmp(f_ext, ".css") == 0) {
            contenttype = "text/css";
        } else if (strcmp(f_ext, ".txt") == 0) { 
            contenttype = "text/plain"; 
        } else if (strcmp(f_ext, ".jpeg") == 0 || strcmp(f_ext, ".jpg") == 0) {
            contenttype = "image/jpeg"; 
        } else if (strcmp(f_ext, ".png") == 0) { 
            contenttype = "image/png"; 
        } else if (strcmp(f_ext, ".pdf") == 0) { 
            contenttype = "application/pdf"; 
        } else if (strcmp(f_ext, ".zip") == 0) { 
            contenttype = "application/zip"; 
        }
    }

    // Allocate memory for the file data
    char *f_data = malloc(f_size);
    if (!f_data) {
        perror("websever: Failed to allocate memory");
        fclose(f_fd);
        return;
    }

    // Read the file data into memory
    int bytes_read = fread(f_data, 1, f_size, f_fd);
    if (bytes_read != f_size) {
        perror("websever: Failed to read file");
        fclose(f_fd);
        free(f_data);
        return;
    }

    // Send response message
    char res_buffer[1024];
    sprintf(res_buffer, "HTTP/1.1 200 OK\r\n"
                        "Content-Type: %s\r\n"
                        "Content-Length: %s\r\n"
                        "\r\n", contenttype, f_len);
    send(cli_socket, res_buffer, strlen(res_buffer), 0);

    // Send the file data to the client
    int bytes_sent = 0;
    while (bytes_sent < f_size) {
        int result = send(cli_socket, f_data + bytes_sent, f_size - bytes_sent, 0);
        if (result < 0) {
            perror("websever: Failed to send file data");
            fclose(f_fd);
            free(f_data);
            return;
        }
        bytes_sent += result;
    }
}

void url_preprocessing(char *url) {
    char *pCheck, *pReplace;
    pCheck = pReplace = strtok(url, "/");
    // Two pointers pointing to the same characters on url
    // pCheck is used to check if the next 3 characters is "%20", if so, 
    // pReplace, pointing to those 3 character will be set to ' '
    // Otherwise, it'll 
    while(*pCheck != '\0') {

        if (*pCheck == '%' && *(pCheck + 1) == '2' && *(pCheck + 2) == '0') {
            *pReplace++ = ' ';
            pCheck += 3;
        } else {
            *pReplace++ = *pCheck++;
        }
    }
    *pCheck = '\0';
}

// Handle request from connection
void handle_connection(int cli_socket) {
    // Buffer for request
    char req_buffer[MAX_MSG_LEN];
    // Buffers to store request line 
    char *method;
    char *url;
    char *httpvers;
    char *header, *value;

    // Read and parse the request line of the header
    int valread = read(cli_socket, req_buffer, 1024);
    method = strtok(req_buffer, " ");
    printf("Method: %s\n", method);
    url = strtok(NULL, " ");
    printf("URL: %s\n", url);
    httpvers = strtok(NULL, "\r\n");
    printf("HTTP Version: %s\n", httpvers);

    // parse header values
    /*while ((header = strtok(NULL, ": ")) != NULL) {
        value = strtok(NULL, "\r\n");
    }
    printf("%s: %s\n", header, value);*/

    // Server only handle request of GET method
    if (strcmp(method, "GET") != 0) {
        char *res = "HTTP/1.1 400 BAD REQUEST\r\n";
        write(cli_socket, res, strlen(res));
        write(cli_socket, "\r\n", 4);
        perror("webserver: Invalid request method");
        close(cli_socket);
        exit(EXIT_FAILURE);
    } else {
        url = strtok(url, "/");
        url_preprocessing(url);
        handle_GET(cli_socket,url);
    }
    close(cli_socket);
}

// Set up server socket and listening for connection 
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