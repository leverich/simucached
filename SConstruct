#!/usr/bin/python
import os

env = Environment(ENV = os.environ)

#env.Append(CPPPATH = ['/usr/local/include', '/opt/local/include'])
#env.Append(LIBPATH = ['/opt/local/lib'])

env.Append(CCFLAGS   = '-std=c++0x -D_GNU_SOURCE -O3 -g')

conf = env.Configure(config_h = "config.h")
conf.Define("__STDC_FORMAT_MACROS")

if not conf.CheckCXX():
    print "A compiler with C++11 support is required."
    Exit(1)

conf.CheckLib("rt", "clock_gettime", language="C++")

print "Checking for gengetopt...",
if env.Execute("@which gengetopt &> /dev/null"):
    print "not found (required)"
    Exit(1)
else: print "found"

#if not conf.CheckLibWithHeader("event", "event2/event.h", "C++"):
#    print "libevent required"
#    Exit(1)
#if not conf.CheckLibWithHeader("event_pthreads", "event2/event.h", "C++"):
#    print "libevent required"
#    Exit(1)

if not conf.CheckLibWithHeader("pthread", "pthread.h", "C++"):
    print "pthread required"
    Exit(1)

env = conf.Finish()

env.Append(CFLAGS = ' -O3 -Wall -g')

env.Command(['cmdline.cc', 'cmdline.h'], 'cmdline.ggo', 'gengetopt < $SOURCE')

src = Split("""simucached.cc cmdline.cc log.cc thread.cc work.cc""")

env.Program(target='simucached', source=src)

