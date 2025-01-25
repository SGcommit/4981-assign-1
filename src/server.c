#include "../include/server.h"

int main(void)
{
    return start_server();
}

// Initialize and start the HTTP server
int start_server(void)
{
    int                server_fd;
    struct sockaddr_in server_addr;
    struct sockaddr_in client_addr;
    socklen_t          addr_len = sizeof(client_addr);

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    // Create TCP socket
    if(server_fd == -1)
    {
        perror("Socket creation failed");
        return EXIT_FAILURE;
    }

    // Configure server address
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Bind socket to port
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Start listening for connections
    if(listen(server_fd, BACKLOG) == -1)
    {
        perror("Listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    printf("Server running on port %d... Press Ctrl+C to stop.\n", PORT);

    // Accept and handle clients

    do
    {
        int client_fd;
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if(client_fd == -1)
        {
            perror("Accept failed");
            continue;
        }

        handle_client(client_fd);
        close(client_fd);    // Close connection after handling request
    } while(1);

    return EXIT_SUCCESS;
}

// Handle an HTTP request from a client
void handle_client(int client_socket)
{
    char    buffer[BUFFER_SIZE];
    ssize_t bytes_received;
    memset(buffer, 0, BUFFER_SIZE);

    // Read request from client
    bytes_received = read(client_socket, buffer, BUFFER_SIZE - 1);
    if(bytes_received < 0)
    {
        perror("Read failed");
        return;
    }

    process_request(client_socket, buffer);
}
void parse_request(const char *request, char *method, char *path, char *protocol) {
    char buffer[BUFFER_SIZE];
    strncpy(buffer, request, BUFFER_SIZE);
    buffer[BUFFER_SIZE - 1] = '\0';

    char *saveptr;
    char *token = strtok_r(buffer, " ", &saveptr);
    if (token) strncpy(method, token, METHOD_SIZE - 1);

    token = strtok_r(NULL, " ", &saveptr);
    if (token) strncpy(path, token, MAX_URI_LENGTH - 1);

    token = strtok_r(NULL, "\r\n", &saveptr);
    if (token) strncpy(protocol, token, PROTOCOL_SIZE - 1);
}

void send_status_response(int client_socket, const HttpStatus *status, const char *content_type, const char *body) {
    size_t body_length = body ? strlen(body) : 0;
    char header[HEADER_SIZE];
    int header_length = snprintf(header, sizeof(header),
        "HTTP/1.1 %s %s\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %zu\r\n"
        "Connection: close\r\n\r\n",
        status->code, status->message, content_type, body_length);

    write(client_socket, header, header_length);
    if (body) write(client_socket, body, body_length);
}

// Serve a requested file
void send_file(int client_socket, const char *file_path, int is_head) {
    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        send_status_response(client_socket, &HTTP_STATUS_NOT_FOUND, "text/plain", "404 Not Found");
        return;
    }

    struct stat file_stat;
    fstat(fd, &file_stat);
    send_status_response(client_socket, &HTTP_STATUS_OK, get_content_type(file_path), NULL);

    if (!is_head) {
        off_t offset = 0;
        sendfile(client_socket, fd, &offset, file_stat.st_size);
    }
    close(fd);
}

const char *get_content_type(const char *path) {
    for (int i = 0; mime_types[i].extension != NULL; i++) {
        if (strstr(path, mime_types[i].extension)) {
            return mime_types[i].mime_type;
        }
    }
    return "text/plain";
}
