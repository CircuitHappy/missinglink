#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>

#include "display/ht16k33.h"

#define SOCK_PATH "/tmp/ml-display-bus"

void init_display() {
  ht16k33_init();

  ht16k33_write_ascii(0, 'L', false);
  ht16k33_write_ascii(1, 'I', false);
  ht16k33_write_ascii(2, 'N', false);
  ht16k33_write_ascii(3, 'K', false);

  ht16k33_commit();

  sleep(1);

  ht16k33_clear();

  ht16k33_write_ascii(1, 'V', false);
  ht16k33_write_ascii(2, '0', true);
  ht16k33_write_ascii(3, '1', false);

  ht16k33_commit();

  sleep(2);

  ht16k33_clear();
}

void sock_loop() {
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

    char buf[8];
    int msg_size = recv(scd, buf, 8, 0);

    if (msg_size < 0) {
      perror("failed to receive stream data\n");
    } else {
      ht16k33_write_string(buf);
    }

    close(scd);
  }
}

int main(void) {
  init_display();
  sock_loop();
  return 0;
}

