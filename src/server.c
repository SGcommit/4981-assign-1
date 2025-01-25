#include "../include/server.h"

int main(void)
{
    return start_server();
}

static volatile sig_atomic_t server_running = 1;    // NOLINT(cppcoreguidelines-avoid-non-const-global-variables)

static void handle_signal(int signal)
{
    (void)signal;
    server_running = 0;
}

int start_server(void)
{
    int                server_fd;
    struct sockaddr_in server_addr;
#if defined(__clang__)
    #pragma clang diagnostic push
    #pragma clang diagnostic ignored "-Wdisabled-macro-expansion"
#endif

    struct sigaction sa;
    sa.sa_handler = handle_signal;    // Use the renamed signal handler
    sa.sa_flags   = 0;                // No special flags
    sigemptyset(&sa.sa_mask);         // No additional signals to block
#if defined(__clang__)
    #pragma clang diagnostic pop
#endif
    if(sigaction(SIGINT, &sa, NULL) == -1)
    {
        perror("Failed to setup SIGINT handler");
        return EXIT_FAILURE;
    }

    // Socket creation
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if(server_fd == -1)
    {
        perror("Socket failed");
        return EXIT_FAILURE;
    }

    // Server address setup
    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(PORT);

    // Binding the server
    if(bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1)
    {
        perror("Bind failed");
        close(server_fd);
        return EXIT_FAILURE;
    }

    // Listening for connections
    if(listen(server_fd, BACKLOG) == -1)
    {
        perror("Listen failed");
        close(server_fd);
        return EXIT_FAILURE;
    }
    printf("Server running on port %d\n", PORT);

    // Accepting client connections
    while(server_running)
    {
        struct sockaddr_in client_addr;
        socklen_t          addr_len  = sizeof(client_addr);
        int                client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
        if(client_fd == -1)
        {
            if(!server_running)
            {
                break;
            }
            perror("Accept failed");
            continue;
        }

        handle_client(client_fd);
        close(client_fd);
    }

    printf("Shutting down server...\n");
    close(server_fd);
    return EXIT_SUCCESS;
}

// Handle an HTTP request from a client
void handle_client(int client_socket)
{
    char    buffer[BUFFER_SIZE];
    char    method[METHOD_SIZE];
    char    path[MAX_URI_LENGTH];
    char    protocol[PROTOCOL_SIZE];
    ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer) - 1, 0);
    if(bytes_received <= 0)
    {
        return;
    }
    buffer[bytes_received] = '\0';

    parse_request(buffer, method, path, protocol);

    if(strcmp(method, "GET") == 0 || strcmp(method, "HEAD") == 0)
    {
        char file_path[BUFFER_SIZE] = "./www";
        strncat(file_path, path, sizeof(file_path) - strlen(file_path) - 1);
        if(file_path[strlen(file_path) - 1] == '/')
        {
            strcat(file_path, "index.html");
        }
        send_file(client_socket, file_path, strcmp(method, "HEAD") == 0);
    }
    else
    {
        send_status_response(client_socket, &HTTP_STATUS_METHOD_NOT_ALLOWED, "text/plain", "405 Method Not Allowed");
    }
}

void parse_request(const char *request, char *method, char *path, char *protocol)
{
    char buffer[BUFFER_SIZE];
    // Tokenize the request using strtok_r
    char       *saveptr;
    const char *token;
    // Copy the request to the buffer
    strncpy(buffer, request, BUFFER_SIZE);
    buffer[BUFFER_SIZE - 1] = '\0';    // Null-terminate the string

    // Tokenize the request using strtok_r

    token = strtok_r(buffer, " ", &saveptr);
    if(token)
    {
        strncpy(method, token, METHOD_SIZE - 1);
    }

    token = strtok_r(NULL, " ", &saveptr);
    if(token)
    {
        strncpy(path, token, MAX_URI_LENGTH - 1);
    }

    token = strtok_r(NULL, "\r\n", &saveptr);
    if(token)
    {
        strncpy(protocol, token, PROTOCOL_SIZE - 1);
    }
}

void send_status_response(int client_socket, const HttpStatus *status, const char *content_type, const char *body)
{
    size_t body_length = body ? strlen(body) : 0;
    char   header[HEADER_SIZE];
    int    header_length = snprintf(header,
                                 sizeof(header),
                                 "HTTP/1.1 %s %s\r\n"
                                    "Content-Type: %s\r\n"
                                    "Content-Length: %zu\r\n"
                                    "Connection: close\r\n\r\n",
                                 status->code,
                                 status->message,
                                 content_type,
                                 body_length);

    write(client_socket, header, (size_t)header_length);
    if(body)
    {
        write(client_socket, body, body_length);
    }
}

// Serve a requested file
void send_file(int client_socket, const char *file_path, int is_head)
{
    struct stat file_stat;
    int         fd;
    fd = open(file_path, O_RDONLY | O_CLOEXEC);
    if(fd == -1)
    {
        send_status_response(client_socket, &HTTP_STATUS_NOT_FOUND, "text/plain", "404 Not Found");
        return;
    }

    fstat(fd, &file_stat);
    send_status_response(client_socket, &HTTP_STATUS_OK, get_content_type(file_path), NULL);

    if(!is_head)
    {
        off_t offset = 0;
        sendfile(client_socket, fd, &offset, (size_t)file_stat.st_size);
    }
    close(fd);
}

const char *get_content_type(const char *path)
{
    for(int i = 0; mime_types[i].extension != NULL; i++)
    {
        if(strstr(path, mime_types[i].extension))
        {
            return mime_types[i].mime_type;
        }
    }
    return "text/plain";
}
