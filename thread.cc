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

__thread char devnull[READ_CHUNK];

void* thread_main(void* data) {
  Thread *td = (Thread *) data;
  int efd = td->efd;

  struct epoll_event events[MAX_EVENTS];

  while (1) {
    int n = epoll_wait(efd, events, MAX_EVENTS, -1);
    if (n < 1) DIE("epoll_wait failed: %s", strerror(errno));

    for (int i = 0; i < n; i++) {
      Connection *conn = (Connection *) events[i].data.ptr;
      int fd = conn->fd;

#if 0 // Lazily detect connection closure with read() below.
      if (events[i].events & EPOLLHUP) {
        close(fd);
        delete conn;
        continue;
      }
#endif

      if (args.no_parse_given) {
        int ret = read(fd, conn->buffer, sizeof(conn->buffer));
        if (ret <= 0) {
          close(fd);
          delete conn;
          continue;
        }

        conn->buffer_idx = ret;
        conn->buffer[conn->buffer_idx] = '\0';

        char *start = conn->buffer;

        // Locate a \r\n
        char *crlf = NULL;
        while (start < &conn->buffer[conn->buffer_idx]) {
          crlf = strstr(start, "\r\n");

          if (crlf == NULL) break; // No \r\n found.

          int length = crlf - start;

          write(fd, get_reply, strlen(get_reply));
          start += length + 2;
        }

        continue;
      }

      if (conn->state == Connection::GOBBLE) {
        int ret = read(fd, devnull,
                       conn->bytes_to_eat > READ_CHUNK ?
                       READ_CHUNK : conn->bytes_to_eat);
        if (ret <= 0) {
          close(fd);
          delete conn;
          continue;
        }

        conn->bytes_to_eat -= ret;
        if (conn->bytes_to_eat <= 0) {
          conn->state = Connection::IDLE;
          write(fd, "STORED\r\n", 8);
        }
      } else {
        int ret = read(fd, &conn->buffer[conn->buffer_idx],
                       sizeof(conn->buffer) - conn->buffer_idx - 1);
        if (ret <= 0) {
          close(fd);
          delete conn;
          continue;
        }

        conn->buffer_idx += ret;
        conn->buffer[conn->buffer_idx] = '\0';

        char *start = conn->buffer;

        // Locate a \r\n
        char *crlf = NULL;
        while (start < &conn->buffer[conn->buffer_idx]) {
          crlf = strstr(start, "\r\n");

          if (crlf == NULL) break; // No \r\n found.

          int length = crlf - start;

          start[length] = '\0';

          if (!strncasecmp(start, "get", 3)) {
            write(fd, get_reply, strlen(get_reply));
            start += length + 2;
          } else if (!strncasecmp(start, "set", 3)) {
            int setsize;
            if (sscanf(start, "%*s %*s %*d %*d %d", &setsize)) {
              start += length + 2;
              int remaining = &conn->buffer[conn->buffer_idx] - start;

              // Case 1: All of the SET data is in the buffer. Eat it.
              // Case 2: We don't have enough data to complete
              // SET. Switch to GOBBLE state and start eating bytes.
              if (setsize+2 <= remaining) {
                start += setsize+2;
                write(fd, "STORED\r\n", 8);
              } else {
                conn->state = Connection::GOBBLE;
                conn->bytes_to_eat = setsize + 2 - remaining;
                start = &conn->buffer[conn->buffer_idx];
                break;
              }
            } else {
              W("Failed to parse SET command: %s", start);
              start += length + 2;
            }
          } else {
            D("Unknown command: %s", start);
            write(fd, "ERROR\r\n", 7);
            start += length + 2;
          }
        }

        // reset buffer_idx if we run out of buffer space or we've
        // successfully parse everything in the buffer.
        if ((conn->buffer_idx >= sizeof(conn->buffer) - 1) ||
            (&conn->buffer[conn->buffer_idx] == start)) {
          conn->buffer_idx = 0;
        } else { // Left-over data in buffer, skooch it up
          int shift = &conn->buffer[conn->buffer_idx] - start;
          //        D("shift = %d", shift);
          memmove(conn->buffer, start, shift);
          conn->buffer_idx = shift;
        }
      }
    }
  }
}
