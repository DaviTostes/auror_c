#include "../lib/http.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void handle_client(int client_fd, char *body) {
  char buffer[BUFFER_SIZE] = {0};
  ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);

  if (bytes_read < 0) {
    perror("read");
    close(client_fd);
    return;
  }

  char method[16], path[256], protocol[16];
  sscanf(buffer, "%15s %255s %15s", method, path, protocol);

  char response[BUFFER_SIZE];
  int response_len = snprintf(response, sizeof(response),
                              "HTTP/1.1 200 OK\r\n"
                              "Content-Type: text/xml; charset=utf-8\r\n"
                              "Content-Length: %zu\r\n"
                              "Connection: close\r\n"
                              "\r\n"
                              "%s",
                              strlen(body), body);

  write(client_fd, response, response_len);
  close(client_fd);
}

void setup_socket(int *server_fd) {
  *server_fd = socket(AF_INET, SOCK_STREAM, 0);
  if (server_fd < 0) {
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
