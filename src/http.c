#include "../lib/http.h"
#include "../lib/scpd.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void setup_socket(int *server_fd) {
  *server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (*server_fd < 0) {
    perror("socket");
    exit(EXIT_FAILURE);
  }

  int opt = 1;
  if (setsockopt(*server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    perror("setsockopt");
    exit(EXIT_FAILURE);
  }

  struct sockaddr_in addr = {
      .sin_family = AF_INET,
      .sin_addr.s_addr = INADDR_ANY,
      .sin_port = htons(HTTP_PORT),
  };

  if (bind(*server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }

  if (listen(*server_fd, BACKLOG) < 0) {
    perror("listen");
    exit(EXIT_FAILURE);
  }
}

void send_response(int client_fd, int status, const char *status_text,
                   const char *content_type, const char *body) {
  char response[BUFFER_SIZE];
  int len = snprintf(response, sizeof(response),
                     "HTTP/1.1 %d %s\r\n"
                     "Content-Type: %s\r\n"
                     "Content-Length: %zu\r\n"
                     "Connection: close\r\n"
                     "\r\n"
                     "%s",
                     status, status_text, content_type, strlen(body), body);

  write(client_fd, response, len);
}

void handle_description(int client_fd, const char *body) {
  send_response(client_fd, 200, "OK", "text/xml", body);
}

void handle_not_found(int client_fd, const char *body) {
  (void)body;
  send_response(client_fd, 404, "Not Found", "text/plain", "404 Not Found\n");
}

void handle_method_not_allowed(int client_fd) {
  send_response(client_fd, 405, "Method Not Allowed", "text/plain",
                "405 Method Not Allowed\n");
}

void handle_client(int client_fd) {
  char buffer[BUFFER_SIZE] = {0};
  ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

  if (bytes_read < 0) {
    perror("read");
    close(client_fd);
    return;
  }

  char method[16], path[256], protocol[16];
  sscanf(buffer, "%15s %255s %15s", method, path, protocol);

  char *query = strchr(path, '?');
  if (query) {
    *query = '\0';
  }

  const char *body = strstr(buffer, "\r\n\r\n");
  if (body) {
    body += 4;
  }

  int path_matched = 0;
  if (strcmp(method, "GET") == 0) {
    if (strcmp("/description.xml", path) == 0) {
      path_matched = 1;
      body = deviceDescriptionXML("auror", "teste");
      handle_description(client_fd, body);
      close(client_fd);
      return;
    }
    handle_not_found(client_fd, NULL);
    close(client_fd);
    return;
  }

  if (path_matched) {
    handle_method_not_allowed(client_fd);
  } else {
    handle_not_found(client_fd, NULL);
  }
  close(client_fd);
}
