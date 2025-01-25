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

// Process the HTTP request and determine the response
void process_request(int client_socket, const char *request)
{
    char method[METHOD_SIZE];
    char path[MAX_URI_LENGTH];
    char protocol[PROTOCOL_SIZE];
    char file_path[FILE_PATH_SIZE];
    int  is_head;

    // Parse request line
    sscanf(request, "%7s %255s %15s", method, path, protocol);

    // Ensure it's an HTTP request
    if(strncmp(protocol, HTTP_VERSION_MAGIC, HTTP_VERSION_MAGIC_LENGTH) != 0)
    {
        send_response(client_socket, HTTP_VERSION_NOT_SUPPORTED, CONTENT_TYPE_TEXT, HTTP_VERSION_NOT_SUPPORTED, HTTP_VERSION_MSG_LEN);
        return;
    }

    // Reject unsupported methods
    if(strcmp(method, "GET") != 0 && strcmp(method, "HEAD") != 0)
    {
        send_response(client_socket, HTTP_METHOD_NOT_ALLOWED, CONTENT_TYPE_TEXT, HTTP_METHOD_NOT_ALLOWED, HTTP_METHOD_MSG_LEN);
        return;
    }

    // Construct file path
    snprintf(file_path, sizeof(file_path), "%s%s", WEB_ROOT, strcmp(path, "/") == 0 ? "/index.html" : path);
    is_head = strcmp(method, "HEAD") == 0;

    // Serve the requested file
    send_file(client_socket, file_path, is_head);
}

// Send an HTTP response with a status and optional body
void send_response(int client_socket, const char *status, const char *content_type, const char *body, int body_length)
{
    char   header[HEADER_SIZE];
    size_t header_length;

    header_length = (size_t)snprintf(header,
                                     sizeof(header),
                                     "%s %s\r\n"
                                     "Content-Type: %s\r\n"
                                     "Content-Length: %d\r\n"
                                     "Connection: close\r\n\r\n",
                                     HTTP_VERSION,
                                     status,
                                     content_type,
                                     body_length);

    write(client_socket, header, header_length);
    if(body_length > 0)
    {
        write(client_socket, body, (size_t)body_length);    // Explicit type cast to avoid signedness issue
    }
}

// Serve a requested file
void send_file(int client_socket, const char *file_path, int is_head)
{
    int         fd;
    struct stat file_stat;
    char        header[HEADER_SIZE];
    size_t      header_length;
    const char *content_type;

    fd = open(file_path, O_RDONLY | O_CLOEXEC);
    if(fd == -1)
    {
        send_response(client_socket, HTTP_NOT_FOUND, CONTENT_TYPE_TEXT, HTTP_NOT_FOUND, HTTP_NOT_FOUND_MSG_LEN);
        return;
    }

    // Get file size
    fstat(fd, &file_stat);

    // Send HTTP header
    content_type  = get_content_type(file_path);
    header_length = (size_t)snprintf(header,
                                     sizeof(header),
                                     "%s 200 OK\r\n"
                                     "Content-Type: %s\r\n"
                                     "Content-Length: %ld\r\n"
                                     "Connection: close\r\n\r\n",
                                     HTTP_VERSION,
                                     content_type,
                                     file_stat.st_size);

    write(client_socket, header, header_length);

    // If HEAD request, do not send body
    if(!is_head)
    {
        ssize_t bytes_read;
        char    buffer[BUFFER_SIZE];
        while((bytes_read = read(fd, buffer, BUFFER_SIZE)) > 0)
        {
            write(client_socket, buffer, (size_t)bytes_read);    // Explicit cast to avoid signedness issue
        }
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
