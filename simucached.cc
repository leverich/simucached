#include "config.h"

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <pthread.h>
#include <sched.h>
#include <stdlib.h>
#include <string.h>
#include <sys/epoll.h>
#include <sys/types.h>
#include <sys/socket.h>

#include "cmdline.h"
#include "log.h"
#include "simucached.h"
#include "thread.h"

/* simucached

   Model A
   One listener, one epoll set per child.

     The main thread responsibilities:
     * Create epoll set for each child.
     * Spawn children.
     * Open socket for listening.
     * Add new connections to children epoll sets, round-robin.

     Child thread responsibilities:
     * Wait for events from epoll set.
     * Close hung-up connections, housekeep.
     * Read from fds, parse commands, generate responses.

   Model 2
   One listener, one epoll set total.

   Model 3
   One listener, one epoll set per child, fds assigned to random pair of sets.
  
 */

using namespace std;

gengetopt_args_info args;

static int open_listen_socket(int port) {
  struct sockaddr_in sa;
  int optval = 1;

  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) DIE("socket() failed: %s", strerror(errno));

  if (setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(optval)))
    DIE("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));

  bzero(&sa, sizeof(sa));
  sa.sin_family = AF_INET;
  sa.sin_addr.s_addr = INADDR_ANY;
  sa.sin_port = htons(port);

  if (bind(fd, (struct sockaddr *) &sa, sizeof(sa)) < 0)
    DIE("bind(port=%d) failed: %s", port, strerror(errno));

  if (listen(fd, 1024) < 0)
    DIE("listen() failed: %s", strerror(errno));

  return fd;
}

void spawn_thread(Thread* td) {
  pthread_attr_t attr;
  pthread_attr_init(&attr);

  if (args.affinity_given) {
    static int current_cpu = -1;
    int max_cpus = 8 * sizeof(cpu_set_t);
    cpu_set_t m;
    CPU_ZERO(&m);
    sched_getaffinity(0, sizeof(cpu_set_t), &m);

    for (int i = 0; i < max_cpus; i++) {
      int c = (current_cpu + i + 1) % max_cpus;
      if (CPU_ISSET(c, &m)) {
        CPU_ZERO(&m);
        CPU_SET(c, &m);
        int ret;
        if ((ret = pthread_attr_setaffinity_np(&attr,
                                               sizeof(cpu_set_t), &m)))
          DIE("pthread_attr_setaffinity_np(%d) failed: %s",
              c, strerror(ret));
        current_cpu = c;
        break;
      }
    }
  }

  // create an epoll set
  td->efd = epoll_create1(0);
  if (pthread_create(&td->pt, NULL, thread_main, td))
    DIE("pthread_create() failed: %s", strerror(errno));
}

static void set_nonblocking(int fd) {
  int opts = fcntl(fd, F_GETFL);
  if (opts < 0) DIE("fcntl(F_GETFL): %s", strerror(errno));
  if (fcntl(fd, F_SETFL, opts | O_NONBLOCK) < 0)
    DIE("fcntl(F_SETFL): %s", strerror(errno));
}

int main(int argc, char **argv) {
  if (cmdline_parser(argc, argv, &args) != 0) DIE("cmdline_parser failed");

  for (unsigned int i = 0; i < args.verbose_given; i++)
    log_level = (log_level_t) ((int) log_level - 1);
  if (args.quiet_given) log_level = QUIET;

  V("%s v%s ready to roll",
    CMDLINE_PARSER_PACKAGE_NAME, CMDLINE_PARSER_VERSION);

  Thread td[args.threads_arg];

  for (int i = 0; i < args.threads_arg; i++)
    spawn_thread(&td[i]);

  int next_thread = 0;
  int listen_socket = open_listen_socket(args.port_arg);
  struct sockaddr sa;
  socklen_t sa_len;

  while (1) {
    sa_len = sizeof(sa);
    int newfd = accept(listen_socket, &sa, &sa_len);
    if (newfd < 0) DIE("accept() failed: %s", strerror(errno));

    int optval = 1;
    if (setsockopt(newfd, IPPROTO_TCP, TCP_NODELAY,
                   (void *) &optval, sizeof(optval)))
      DIE("setsockopt(TCP_NODELAY) failed: %s", strerror(errno));

    set_nonblocking(newfd);

    Connection* conn = new Connection(newfd);

    struct epoll_event ev;
    ev.events = EPOLLIN;
    ev.data.ptr = conn;

    if (epoll_ctl(td[next_thread].efd, EPOLL_CTL_ADD, newfd, &ev))
      DIE("epoll_ctl(%d, EPOLL_CTRL_ADD, %d) failed: %s",
          td[next_thread].efd, newfd, strerror(errno));

    next_thread = (next_thread + 1) % args.threads_arg;
  }  
}
