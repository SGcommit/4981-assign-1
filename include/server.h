#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#define PORT 8080
#define BUFFER_SIZE 1024
#define HEADER_SIZE 512
#define MAX_URI_LENGTH 256
#define BACKLOG 10
#define METHOD_SIZE 8
#define PROTOCOL_SIZE 16

// Struct for HTTP Status
typedef struct {
    const char *code;
    const char *message;
} HttpStatus;

static const HttpStatus HTTP_STATUS_OK = { "200", "OK" };
static const HttpStatus HTTP_STATUS_NOT_FOUND = { "404", "Not Found" };
static const HttpStatus HTTP_STATUS_METHOD_NOT_ALLOWED = { "405", "Method Not Allowed" };
static const HttpStatus HTTP_STATUS_VERSION_NOT_SUPPORTED = { "505", "HTTP Version Not Supported" };

// Struct for MIME Types
typedef struct {
    const char *extension;
    const char *mime_type;
} MimeType;

static const MimeType mime_types[] = {
    { ".html", "text/html" },
    { ".css", "text/css" },
    { ".js", "application/javascript" },
    { ".png", "image/png" },
    { ".jpg", "image/jpeg" },
    { ".jpeg", "image/jpeg" },
    { ".gif", "image/gif" },
    { NULL, "text/plain" }
};
// Function Prototypes
int         start_server(void);

void send_status_response(int client_socket, const HttpStatus *status, const char *content_type, const char *body);
void        handle_client(int client_socket);
void parse_request(const char *request, char *method, char *path, char *protocol);
void        send_file(int client_socket, const char *file_path, int is_head);
const char *get_content_type(const char *path);

#endif    // SERVER_H
