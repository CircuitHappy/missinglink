#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "display/oled.h"

#define SOCK_PATH "/tmp/ml-display-bus"

int main(void) {
  oled_init();
  oled_write(0, "MISSING LINK");
  oled_write(1, "v0.1");

  int ssd;
  struct sockaddr_un local;

  if ((ssd = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
    perror("failed to create socket");
    exit(1);
  }

  local.sun_family = AF_UNIX;
  strcpy(local.sun_path, SOCK_PATH);

  // Remove existing socket
  unlink(local.sun_path);

  int ssd_len = strlen(local.sun_path) + sizeof(local.sun_family);
  if (bind(ssd, (struct sockaddr *)&local, ssd_len) == -1) {
    perror("failed to bind socket");
    exit(1);
  }

  // Allow up to 4 pending client connections
  if (listen(ssd, 4) == -1) {
    perror("failed to listen on socket");
    exit(1);
  }

  while(true) {
    int scd;
    socklen_t scd_len;
    struct sockaddr_un remote;

    scd_len = sizeof(remote);
    if ((scd = accept(ssd, (struct sockaddr *)&remote, &scd_len)) == -1) {
      perror("failed to accept socket connection(s)");
      exit(1);
    }

    char buf[20];
    int msg_size = recv(scd, buf, 16, 0);

    if (msg_size < 0) {
      perror("failed to receive stream data\n");
    } else if (msg_size > 16) {
      perror("string too long to display\n");
    }

    for (int i = strlen(buf); i < 16; i++) {
      buf[i] = ' ';
    }
    buf[17] = '\0';

    oled_write(0, buf);

    close(scd);
  }

  return 0;
}

