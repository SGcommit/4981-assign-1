#ifndef SERVER_H
#define SERVER_H

#include <arpa/inet.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

// Define constants for various values
#define PORT 8080
#define BUFFER_SIZE 1024
#define HEADER_SIZE 512
#define MAX_URI_LENGTH 256
#define WEB_ROOT "./www"
#define HTTP_VERSION "HTTP/1.1"
#define BACKLOG 10                   // Maximum number of pending connections
#define METHOD_SIZE 8                // Size of method string buffer
#define PROTOCOL_SIZE 16             // Size of protocol string buffer
#define FILE_PATH_SIZE 512           // Maximum file path length
#define HTTP_VERSION_MSG_LEN 28      // Length of "HTTP Version Not Supported" message
#define HTTP_METHOD_MSG_LEN 23       // Length of "HTTP Method Not Allowed" message
#define HTTP_NOT_FOUND_MSG_LEN 13    // Length of "HTTP Not Found" message
// HTTP Status Codes
#define HTTP_OK "200 OK"
#define HTTP_NOT_FOUND "404 Not Found"
#define HTTP_METHOD_NOT_ALLOWED "405 Method Not Allowed"
#define HTTP_VERSION_NOT_SUPPORTED "505 HTTP Version Not Supported"

// Content Types
#define CONTENT_TYPE_TEXT "text/plain"
#define CONTENT_TYPE_HTML "text/html"
#define CONTENT_TYPE_CSS "text/css"
#define CONTENT_TYPE_JS "application/javascript"
#define CONTENT_TYPE_PNG "image/png"
#define CONTENT_TYPE_JPG "image/jpeg"
#define CONTENT_TYPE_GIF "image/gif"

// HTTP Version Magic Numbers
#define HTTP_VERSION_MAGIC "HTTP/1."
#define HTTP_VERSION_MAGIC_LENGTH 7
// Function Prototypes
int         start_server(void);
void        handle_client(int client_socket);
void        process_request(int client_socket, const char *request);
void        send_response(int client_socket, const char *status, const char *content_type, const char *body, int body_length);
void        send_file(int client_socket, const char *file_path, int is_head);
const char *get_content_type(const char *path);

#endif    // SERVER_H
