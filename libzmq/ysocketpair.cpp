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

#include <zmq/ysocketpair.hpp>

#ifdef ZMQ_HAVE_WINDOWS

void zmq::ysocketpair_t::signal (int signal_)
{
    assert (signal_ >= 0 && signal_ < 31);
    char c = (char) signal_;
    int rc = send (w, &c, 1, 0);
    win_assert (rc != SOCKET_ERROR);
}

#else

void zmq::ysocketpair_t::signal (int signal_)
{
    assert (signal_ >= 0 && signal_ < 31);
    unsigned char c = (unsigned char) signal_;
    ssize_t nbytes = send (w, &c, 1, 0);
    errno_assert (nbytes == 1);
}

#endif