Simucached
==========

Simucached is a memcached simulator.  It is event-based and
multi-threaded, and can execute GET and SET commands sent over the
memcached ASCII protocol.  It does not, however, do anything useful.
SET commands are shoveled into /dev/null, and GET commands receive
fake replies.

This program exists merely to ask the question, "How fast COULD
memcached be?"

Requirements
============

0. Linux (for epoll)
1. A C++0x compiler
2. scons
3. libevent
4. gengetopt

Building
========

    apt-get install scons libevent-dev gengetopt
    scons

Usage
=====

    $ ./simucached -h
    simucached 0.2
    
    Usage: simucached [options]
    
    bleh
    
      -h, --help            Print help and exit
          --version         Print version and exit
      -v, --verbose         Verbosity. Repeat for more verbose.
          --quiet           Disable log messages.
      -t, --threads=INT     Number of threads to spawn.  (default=`1')
      -p, --port=INT        What port to listen on.  (default=`11211')
      -T, --affinity        Set distinct CPU affinity for threads, round-robin
      -V, --value_size=INT  Size of memcached values to return.  (default=`200')
      -N, --no_parse        Don't parse memcached protocol. Reply with GET replies.
