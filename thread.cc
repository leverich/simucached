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

#define CHUNK 4096
#define MAX_EVENTS 4096

char bleh2[] = "VALUE xyz 0 200 \r\nffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff\r\nEND\r\n";

char bleh1[] = "VALUE xyz 0 1 \r\nf\r\nEND\r\n";

void* thread_main(void* args) {
  Thread *td = (Thread *) args;
  int efd = td->efd;

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(efd, events, MAX_EVENTS, -1);
    if (n < 1) DIE("epoll_wait failed: %s", strerror(errno));

    for (int i = 0; i < n; i++) {
      Connection *conn = (Connection *) events[i].data.ptr;
      int fd = conn->fd;

      if (events[i].events & EPOLLHUP) {
        close(fd);
        delete conn;
        continue;
      }

      if (conn->buffer.size() < conn->buffer_idx + CHUNK)
        conn->buffer.resize(conn->buffer_idx + CHUNK + 1);

      int ret = read(fd, &conn->buffer[conn->buffer_idx], CHUNK);
      //      int ret = read(fd, buffer + buffer_idx, sizeof(buffer) - buffer_idx - 1);
      if (ret <= 0) {
        close(fd);
        delete conn;
      } else {
        conn->buffer_idx += ret;
        conn->buffer[conn->buffer_idx] = '\0';

        char *start = &conn->buffer[0];

        // Locate a \r\n
        while (start < &conn->buffer[conn->buffer_idx]) {
          //buffer + sizeof(buffer) - 1) {
          char *crlf = strstr(start, "\r\n");
          if (crlf == NULL) { // not found.
            break;
          } else {
            int length = crlf - start;
            start += length + 2;

            write(fd, bleh2, sizeof(bleh2));
          }
        }

        if (start == &conn->buffer[conn->buffer_idx]) { // reset buffer_idx
          conn->buffer_idx = 0;
        } else {
          int shift = &conn->buffer[conn->buffer_idx] - start;
          conn->buffer.erase(conn->buffer.begin(), conn->buffer.begin() + shift);
          //          // copy everything after we munched to start of buffer
          //          memmove(buffer, start, shift);
          conn->buffer_idx = shift;
        }

        //        buffer[buffer_idx] = '\0';
      }
    }
  }
}
