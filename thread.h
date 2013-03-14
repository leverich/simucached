// -*- c++-mode -*-

#ifndef THREAD_H
#define THREAD_H

#include <vector>

#define READ_CHUNK 16384

class Thread {
public:
  pthread_t pt; // pthread handle
  int efd; // epoll set file descriptor
};

class Connection {
public:
  enum connection_state_enum {
    IDLE,
    GOBBLE,
    LAST_STATE
  };

  int fd;

  connection_state_enum state;
  int bytes_to_eat;
  char buffer[READ_CHUNK+1];
  int buffer_idx;

  Connection(int _fd) : fd(_fd), state(IDLE), bytes_to_eat(0), buffer_idx(0) {}
  Connection() = delete;
};

void *thread_main(void *args);

#endif // THREAD_H
