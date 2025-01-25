#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"    // Change this if testing on a different machine
#define SERVER_PORT 8080         // Make sure this matches your server's port
#define BUFFER_SIZE 1024         // Buffer size for sending and receiving data
#define REQUEST_FORMAT "GET / HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n"

int main(void)
{
    int                sockfd;
    struct sockaddr_in server_addr;
    char               buffer[BUFFER_SIZE];
    char               request[BUFFER_SIZE];
    ssize_t            bytes_received;

    /* Create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd == -1)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    /* Configure server address */
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);

    if(inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        perror("Invalid address or address not supported");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* Connect to the server */
    if(connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Connection failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    /* Prepare the request */
    snprintf(request, sizeof(request), REQUEST_FORMAT, SERVER_IP);

    /* Send the request */
    if(write(sockfd, request, strlen(request)) == -1)
    {
        perror("Write failed");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    while((bytes_received = read(sockfd, buffer, BUFFER_SIZE - 1)) > 0)
    {
        buffer[bytes_received] = '\0';    // Null-terminate the response
        printf("%s", buffer);             // Print the response
    }

    if(bytes_received == -1)
    {
        perror("Read failed");
    }

    /* Close the socket */
    close(sockfd);
    return 0;
}
