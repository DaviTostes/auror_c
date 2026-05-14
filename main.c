#include "lib/http.h"
#include "lib/scpd.h"
#include "lib/utils.h"
#include <arpa/inet.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_NAME "auror"
#define MPV_BINARY "mpv"
#define MPV_SOCKET_LEN 32
#define MPV_SOCKET "/tmp/auror-mpv-%d.sock"
#define UUID_PATH "/home/toast/.config/auror/uuid"

int main() {
  int pid = getpid();
  char mpv_socket[MPV_SOCKET_LEN];
  int n = snprintf(mpv_socket, MPV_SOCKET_LEN, MPV_SOCKET, pid);
  if (n < 0 || (size_t)n >= MPV_SOCKET_LEN)
    return 1;

  char device_uuid[UUID_LEN];
  random_uuid(device_uuid);

  char *local_ip = get_local_ip();

  printf("auror starting\n");
  printf("  device name : %s\n", DEVICE_NAME);
  printf("  device UUID : %s\n", device_uuid);
  printf("  local IP    : %s\n", local_ip);
  printf("  HTTP port   : %d\n", HTTP_PORT);
  printf("  mpv binary  : %s\n", MPV_BINARY);
  printf("  mpv socket  : %s\n", mpv_socket);
  printf("  description : http://%s:%d/description.xml\n", local_ip, HTTP_PORT);
  printf("  status      : http://%s:%d/status\n\n", local_ip, HTTP_PORT);
  printf("waiting for DLNA controller to connect...\n");

  int server_fd;
  setup_socket(&server_fd);

  while (1) {
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    int client_fd =
        accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    if (client_fd < 0) {
      perror("accept");
      continue;
    }

    handle_client(client_fd);
  }

  close(server_fd);

  return 0;
}
