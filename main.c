#include "lib/http.h"
#include "lib/scpd.h"
#include "lib/ssdp.h"
#include "lib/utils.h"
#include <arpa/inet.h>
#include <pthread.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

#define DEVICE_NAME "auror"
#define MPV_BINARY "mpv"
#define MPV_SOCKET_LEN 32
#define MPV_SOCKET "/tmp/auror-mpv-%d.sock"
#define UUID_PATH "/home/toast/.config/auror/uuid"

static int g_ssdp_fd = -1;
static int g_http_fd = -1;
static ssdp_config g_cfg;

static void on_shutdown(int sig) {
  (void)sig;
  if (g_ssdp_fd >= 0) {
    ssdp_send_notify(g_ssdp_fd, &g_cfg, 0);
    close(g_ssdp_fd);
    close(g_http_fd);
  }
  exit(EXIT_SUCCESS);
}

void *ssdp_thread_func(void *arg) {
  (void)arg;
  return NULL;
}

void *http_thread_func(void *arg) {
  (void)arg;
  setup_http_socket(&g_http_fd);
  return NULL;
}

int main() {
  int pid = getpid();
  char mpv_socket[MPV_SOCKET_LEN];
  int n = snprintf(mpv_socket, MPV_SOCKET_LEN, MPV_SOCKET, pid);
  if (n < 0 || (size_t)n >= MPV_SOCKET_LEN)
    return 1;

  static char device_uuid[UUID_LEN];
  random_uuid(device_uuid);

  char *local_ip = get_local_ip();

  static char description_route[64];
  snprintf(description_route, sizeof(description_route),
           "http://%s:%d/description.xml", local_ip, HTTP_PORT);
  static char status_route[64];
  snprintf(status_route, sizeof(status_route), "http://%s:%d/status.xml",
           local_ip, HTTP_PORT);

  g_cfg = (ssdp_config){
      .device_uuid = device_uuid,
      .device_type = "urn:schemas-upnp-org:device:MediaRenderer:1",
      .location = description_route,
      .server_string = "Linux/7.x UPnP/1.0 auror/0.1",
  };

  printf("auror starting\n");
  printf("  device name : %s\n", DEVICE_NAME);
  printf("  device UUID : %s\n", device_uuid);
  printf("  local IP    : %s\n", local_ip);
  printf("  HTTP port   : %d\n", HTTP_PORT);
  printf("  mpv binary  : %s\n", MPV_BINARY);
  printf("  mpv socket  : %s\n", mpv_socket);
  printf("  description : %s\n", description_route);
  printf("  status      : %s\n\n", status_route);
  printf("waiting for DLNA controller to connect...\n");

  setup_ssdp_socket(&g_ssdp_fd);
  ssdp_join_multicast(g_ssdp_fd);

  pthread_t http_tid;
  if (pthread_create(&http_tid, NULL, http_thread_func, NULL) != 0) {
    perror("pthread_create");
    return 1;
  }

  ssdp_run(g_ssdp_fd, &g_cfg);

  signal(SIGINT, on_shutdown);
  signal(SIGTERM, on_shutdown);

  pthread_join(http_tid, NULL);
  return 0;
}
