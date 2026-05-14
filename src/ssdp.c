#include "../lib/ssdp.h"
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

void setup_ssdp_socket(int *server_fd) {
  *server_fd = socket(AF_INET, SOCK_DGRAM, 0);
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
      .sin_port = htons(SSDP_PORT),
  };

  if (bind(*server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
    perror("bind");
    exit(EXIT_FAILURE);
  }
}

void ssdp_join_multicast(int server_fd) {
  struct ip_mreq mreq = {0};
  if (inet_pton(AF_INET, SSDP_MCAST_ADDR, &mreq.imr_multiaddr) <= 0) {
    perror("inet_pton mcast");
    exit(EXIT_FAILURE);
  }

  mreq.imr_interface.s_addr = INADDR_ANY;
  if (setsockopt(server_fd, IPPROTO_IP, IP_ADD_MEMBERSHIP, &mreq,
                 sizeof(mreq)) < 0) {
    perror("IP_ADDR_MEMBERSHIP");
    exit(EXIT_FAILURE);
  }

  unsigned char ttl = 4;
  if (setsockopt(server_fd, IPPROTO_IP, IP_MULTICAST_TTL, &ttl, sizeof(ttl)) <
      0) {
    perror("IP_MULTICAST_TTL");
    exit(EXIT_FAILURE);
  }

  unsigned char loop = 0;
  if (setsockopt(server_fd, IPPROTO_IP, IP_MULTICAST_LOOP, &loop,
                 sizeof(loop)) < 0) {
    perror("IP_MULTICAST_LOOP");
    exit(EXIT_FAILURE);
  }
}

void ssdp_send_notify(int server_fd, const ssdp_config *cfg, int alive) {
  struct sockaddr_in mcast = {
      .sin_family = AF_INET,
      .sin_port = htons(SSDP_PORT),
  };
  inet_pton(AF_INET, SSDP_MCAST_ADDR, &mcast.sin_addr);

  const char *targets[] = {"upnp:rootdevice", cfg->device_type,
                           cfg->device_uuid};
  for (size_t i = 0; i < sizeof(targets) / sizeof(targets[0]); i++) {
    char msg[SSDP_BUFFER_SIZE];
    int len;
    if (alive) {
      len = snprintf(msg, sizeof(msg),
                     "NOTIFY * HTTP/1.1\r\n"
                     "HOST: %s:%d\r\n"
                     "CACHE-CONTROL: max-age=%d\r\n"
                     "LOCATION: %s\r\n"
                     "NT: %s\r\n"
                     "NTS: ssdp:alive\r\n"
                     "SERVER: %s\r\n"
                     "USN: %s::%s\r\n"
                     "\r\n",
                     SSDP_MCAST_ADDR, SSDP_PORT, SSDP_MAX_AGE, cfg->location,
                     targets[i], cfg->server_string, cfg->device_uuid,
                     targets[i]);
    } else {
      len = snprintf(msg, sizeof(msg),
                     "NOTIFY * HTTP/1.1\r\n"
                     "HOST: %s:%d\r\n"
                     "NT: %s\r\n"
                     "NTS: ssdp:alive\r\n"
                     "USN: %s::%s\r\n"
                     "\r\n",
                     SSDP_MCAST_ADDR, SSDP_PORT, targets[i], cfg->device_uuid,
                     targets[i]);
    }
    if (sendto(server_fd, msg, len, 0, (struct sockaddr *)&mcast,
               sizeof(mcast)) < 0) {
      perror("sendto NOTIFY");
    }
  }
}

static int header_matches(const char *request, const char *header,
                          const char *value) {
  const char *line = strcasestr(request, header);
  if (!line) {
    return 0;
  }

  line += strlen(header);
  while (*line == ' ' || *line == '\t') {
    line++;
  }

  return strncasecmp(line, value, strlen(value)) == 0;
}

static void extract_st(const char *request, char *out, size_t out_size) {
  const char *line = strcasestr(request, "\r\nST:");
  if (!line) {
    out[0] = '\0';
    return;
  }

  line += 5;
  while (*line == ' ' || *line == '\t') {
    line++;
  }

  size_t i = 0;
  while (i < out_size - 1 && line[i] != '\r' && line[i] != '\n' &&
         line[i] != '\0') {
    out[i] = line[i];
    i++;
  }

  out[i] = '\0';
}

void ssdp_handle_msearch(int server_fd, const ssdp_config *cfg,
                         const char *request, struct sockaddr_in *from,
                         socklen_t from_len) {
  if (strncasecmp(request, "M-SEARCH", 8) != 0) {
    printf("[msearch] not M-SEARCH, ignoring\n");
    return;
  }
  if (!header_matches(request, "MAN:", "\"ssdp:discover\"")) {
    printf("[msearch] MAN header mismatch, rejecting\n");
    return;
  }

  char st[256];
  extract_st(request, st, sizeof(st));
  printf("[msearch] ST=[%s]\n", st);

  const char *match_st = NULL;
  if (strcmp(st, "ssdp:all") == 0) {
    match_st = "upnp:rootdevice";
  } else if (strcmp(st, "upnp:rootdevice") == 0) {
    match_st = "upnp:rootdevice";
  } else if (strcmp(st, cfg->device_type) == 0) {
    match_st = cfg->device_type;
  } else if (strcmp(st, cfg->device_uuid) == 0) {
    match_st = cfg->device_uuid;
  } else {
    printf("[msearch] ST not matched, ignoring\n");
    return;
  }
  printf("[msearch] matched, replying with ST=%s\n", match_st);

  char response[SSDP_BUFFER_SIZE];
  int len = snprintf(response, sizeof(response),
                     "HTTP/1.1 200 OK\r\n"
                     "CACHE-CONTROL: max-age=%d\r\n"
                     "EXT:\r\n"
                     "LOCATION: %s\r\n"
                     "SERVER: %s\r\n"
                     "ST: %s\r\n"
                     "USN: %s::%s\r\n"
                     "\r\n",
                     SSDP_MAX_AGE, cfg->location, cfg->server_string, match_st,
                     cfg->device_uuid, match_st);

  ssize_t sent =
      sendto(server_fd, response, len, 0, (struct sockaddr *)from, from_len);
  if (sent < 0) {
    perror("sendto M-SEARCH response");
    return;
  }

  printf("[msearch] sent %zd bytes to controller\n", sent);
}

void ssdp_run(int server_fd, const ssdp_config *cfg) {
  ssdp_send_notify(server_fd, cfg, 1);
  time_t last_notify = time(NULL);

  struct timeval tv = {.tv_sec = 1, .tv_usec = 0};
  setsockopt(server_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

  while (1) {
    char buffer[SSDP_BUFFER_SIZE];
    struct sockaddr_in client_addr;
    socklen_t client_len = sizeof(client_addr);

    ssize_t n = recvfrom(server_fd, buffer, SSDP_BUFFER_SIZE - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
    if (n > 0) {
      buffer[n] = '\0';
      char ip[INET_ADDRSTRLEN];
      inet_ntop(AF_INET, &client_addr.sin_addr, ip, sizeof(ip));
      printf("[ssdp] %zd bytes from %s:%d\n", n, ip,
             ntohs(client_addr.sin_port));
      fflush(stdout);
      ssdp_handle_msearch(server_fd, cfg, buffer, &client_addr, client_len);
    }

    time_t now = time(NULL);
    if (now - last_notify >= SSDP_NOTIFY_INTERVAL) {
      ssdp_send_notify(server_fd, cfg, 1);
      last_notify = now;
    }
  }
}
