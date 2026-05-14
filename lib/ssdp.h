#ifndef SSDP_H
#define SSDP_H

#include <netinet/in.h>

#define SSDP_PORT 1900
#define SSDP_MCAST_ADDR "239.255.255.250"
#define SSDP_BUFFER_SIZE 4096
#define SSDP_NOTIFY_INTERVAL 30
#define SSDP_MAX_AGE 1800

typedef struct {
  const char *device_uuid;
  const char *device_type;
  const char *location;
  const char *server_string;
} ssdp_config;

void setup_ssdp_socket(int *server_fd);
void ssdp_join_multicast(int server_fd);
void ssdp_send_notify(int server_fd, const ssdp_config *cfg, int alive);
void ssdp_handle_msearch(int server_fd, const ssdp_config *cfg,
                          const char *request, struct sockaddr_in *from,
                          socklen_t from_len);
void ssdp_run(int server_fd, const ssdp_config *cfg);

#endif /* SSDP_H */
