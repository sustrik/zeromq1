/*
    Copyright (c) 2007-2008 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify it under
    the terms of the Lesser GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    Lesser GNU General Public License for more details.

    You should have received a copy of the Lesser GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_IO_THREAD_HPP_INCLUDED__
#define __ZMQ_IO_THREAD_HPP_INCLUDED__

#include "platform.hpp"

#include "poll_thread.hpp"
#include "select_thread.hpp"
#include "epoll_thread.hpp"
#include "devpoll_thread.hpp"
#include "kqueue_thread.hpp"

namespace zmq
{

#if defined (ZMQ_HAVE_LINUX)
    typedef epoll_thread_t io_thread_t;

#elif defined (ZMQ_HAVE_FREEBSD)
    typedef kqueue_thread_t io_thread_t;

#elif defined(ZMQ_HAVE_OPENBSD)
    typedef kqueue_thread_t io_thread_t;

#elif defined (ZMQ_HAVE_SOLARIS)
    typedef devpoll_thread_t io_thread_t;

#elif defined (ZMQ_HAVE_OSX)
    typedef kqueue_thread_t io_thread_t;

#elif defined (ZMQ_HAVE_QNXNTO)
    typedef poll_thread_t io_thread_t;

#else
    typedef select_thread_t io_thread_t;

#endif

}

#endif