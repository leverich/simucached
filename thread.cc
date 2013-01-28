#include "config.h"

#include <errno.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <unistd.h>

#include "log.h"
#include "simucached.h"
#include "thread.h"

#define MAX_EVENTS 4096

char bleh2[] = "VALUE xyz 0 200 \r\nffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\r\nEND\r\n";

char bleh1[] = "VALUE xyz 0 1 \r\nf\r\nEND\r\n";

void* thread_main(void* args) {
  int efd = (int) (uint64_t) args;

  W("FIXME: need one buffer per thread... Duh");
  char buffer[4096];
  int buffer_idx = 0; // indicates location after last valid byte
  bzero(buffer, sizeof(buffer));

  D("thread checking in");

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(efd, events, MAX_EVENTS, -1);
    if (n < 1) DIE("epoll_wait failed: %s", strerror(errno));

    for (int i = 0; i < n; i++) {
      int fd = events[i].data.fd;

      if (events[i].events & EPOLLHUP) {
        close(fd);
        continue;
      }

      int ret = read(fd, buffer + buffer_idx, sizeof(buffer) - buffer_idx - 1);
      if (ret <= 0) {
        close(fd);
      } else {
        buffer_idx += ret;

        char *start = buffer;

        // Locate a \r\n
        while (start < buffer + sizeof(buffer) - 1) {
          char *crlf = strstr(start, "\r\n");
          if (crlf == NULL) { // not found.
            break;
          } else {
            int length = crlf - start;
            start += length + 2;

            write(fd, bleh2, sizeof(bleh2));
          }
        }

        if (start == buffer + buffer_idx) { // reset buffer_idx
          buffer_idx = 0;
        } else {
          int shift = buffer + buffer_idx - start;
          // copy everything after we munched to start of buffer
          memmove(buffer, start, shift);
          buffer_idx = shift;
        }

        buffer[buffer_idx] = '\0';
      }
    }
  }
}
