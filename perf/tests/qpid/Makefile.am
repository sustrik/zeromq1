INCLUDES = -I$(top_builddir)/libzmq -I$(top_srcdir)/libzmq

noinst_PROGRAMS = local_lat remote_lat local_thr remote_thr

local_lat_SOURCES = local_lat.cpp ../../transports/qpid_transport.hpp  \
../../transports/i_transport.hpp ../scenarios/lat.hpp
local_lat_CXXFLAGS = -Wall -pedantic -Werror
local_lat_LDADD = -lqpidclient

remote_lat_SOURCES = remote_lat.cpp ../../transports/qpid_transport.hpp \
../../transports/i_transport.hpp ../scenarios/lat.hpp
remote_lat_CXXFLAGS = -Wall -pedantic -Werror
remote_lat_LDADD = -lqpidclient

local_thr_SOURCES = local_thr.cpp ../../transports/qpid_transport.hpp \
../../transports/i_transport.hpp ../scenarios/thr.hpp
local_thr_CXXFLAGS = -Wall -pedantic -Werror
local_thr_LDADD = -lqpidclient $(top_builddir)/libzmq/libzmq.la

remote_thr_SOURCES = remote_thr.cpp ../../transports/qpid_transport.hpp \
../../transports/i_transport.hpp ../scenarios/thr.hpp
remote_thr_CXXFLAGS = -Wall -pedantic -Werror
remote_thr_LDADD = -lqpidclient $(top_builddir)/libzmq/libzmq.la

