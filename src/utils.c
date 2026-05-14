#include "../lib/utils.h"
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

static uint8_t random_int(int min, int max) {
  return min + rand() % (max - min + 1);
}

void random_uuid(char out[UUID_LEN]) {
  uint8_t b[16];

  srand((unsigned)time(NULL));
  for (int i = 0; i < 16; ++i) {
    b[i] = random_int(0, 255);
  }

  b[6] = (b[6] & 0x0F) | 0x40;
  b[8] = (b[8] & 0x3F) | 0x80;

  snprintf(
      out, UUID_LEN,
      "%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",
      b[0], b[1], b[2], b[3], b[4], b[5], b[6], b[7], b[8], b[9], b[10], b[11],
      b[12], b[13], b[14], b[15]);
}

char *get_local_ip() {
  char hostbuffer[256];
  gethostname(hostbuffer, sizeof(hostbuffer));
  struct hostent *host_entry = gethostbyname(hostbuffer);
  return inet_ntoa(*((struct in_addr *)host_entry->h_addr_list[0]));
}
