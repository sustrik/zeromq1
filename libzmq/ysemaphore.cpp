/*
    Copyright (c) 2007-2009 FastMQ Inc.

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

#include <zmq/ysemaphore.hpp>
#include <zmq/platform.hpp>

#if (defined ZMQ_HAVE_LINUX || defined ZMQ_HAVE_OSX || defined ZMQ_HAVE_OPENVMS)

void zmq::ysemaphore_t::signal (int signal_)
{
    assert (signal_ == 0);
    int rc = pthread_mutex_unlock (&mutex);
    errno_assert (rc == 0);
}

#elif defined ZMQ_HAVE_WINDOWS

void zmq::ysemaphore_t::signal (int signal_)
{
    assert (signal_ == 0);
    int rc = SetEvent (ev);
    win_assert (rc != 0);
}

#else

void zmq::ysemaphore_t::signal (int signal_)
{
    assert (signal_ == 0);
    int rc = sem_post (&sem);
    errno_assert (rc != -1);
}

#endif

