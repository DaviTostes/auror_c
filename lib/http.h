#ifndef HTTP_H
#define HTTP_H

#define HTTP_PORT 49152
#define BUFFER_SIZE 4096
#define BACKLOG 10

void handle_client(int client_fd);
void setup_socket(int *server_fd);

#endif /* HTTP_H */
