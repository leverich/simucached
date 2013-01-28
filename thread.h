#ifndef THREAD_H
#define THREAD_H

enum thread_state_enum {
  THREAD_IDLE, // waiting for command
  THREAD_GOBBLE, // eating bytes after SET command
  THREAD_LAST_STATE
};

typedef struct {
  pthread_t pt; // pthread handle
  int efd; // epoll set file descriptor
  thread_state_enum state;
  int bytes_to_eat;
} thread_t;

void *thread_main(void *args);

#endif // THREAD_H
