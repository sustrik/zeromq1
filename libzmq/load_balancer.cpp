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

#include <assert.h>
#include <algorithm>
#include <zmq/load_balancer.hpp>

zmq::load_balancer_t::load_balancer_t () :
    current (0)
{
}

zmq::load_balancer_t::~load_balancer_t ()
{
}

void zmq::load_balancer_t::send_to (pipe_t *pipe_)
{
    //  Associate demux with a new pipe.
    pipes.push_back (pipe_);
}

bool zmq::load_balancer_t::write (message_t &msg_)
{
    //  Underlying layers work with raw_message_t, layers above use message_t.
    //  Demux is the component that translates between the two.
    raw_message_t *msg = (raw_message_t*) &msg_;

    //  For delimiters, the algorithm is straighforward.
    //  Send it to all pipes. We don't care about pipe limits.
    //  TODO: Delimiters are special, they should be passed via different
    //  codepath.
    if (msg->content == (message_content_t*) raw_message_t::delimiter_tag) {
        for (pipes_t::iterator it = pipes.begin (); it != pipes.end (); it ++)
            (*it)->write (msg);
        raw_message_init (msg, 0);
        return true;
    }

    if (pipes.size () == 0)
        return false;

    //  Find the first pipe that is ready to accept the message.
    bool found = false;
    for (pipes_t::size_type i = 0; !found && i < pipes.size (); i++) {
        if (pipes [current]->check_write ())
            found = true;
        else
            current = (current + 1) % pipes.size ();
    }

    //  Oops, no pipe is ready to accept he message.
    if (!found)
        return false;

    //  Send the message to the selected pipe.
    pipes [current]->write (msg);
    current = (current + 1) % pipes.size ();

    //  Detach the original message from the data buffer.
    raw_message_init (msg, 0);

    return true;
}

void zmq::load_balancer_t::flush ()
{
    //  Flush all the present messages to the pipes.
    for (pipes_t::iterator it = pipes.begin (); it != pipes.end (); it ++)
        (*it)->flush ();
}

void zmq::load_balancer_t::gap ()
{
    for (pipes_t::iterator it = pipes.begin (); it != pipes.end (); it ++)
        (*it)->gap ();
}

bool zmq::load_balancer_t::empty ()
{
    return pipes.empty ();
}

void zmq::load_balancer_t::release_pipe (pipe_t *pipe_)
{
    //  Find the pipe.
    pipes_t::iterator it = std::find (pipes.begin (), pipes.end (), pipe_);

    //  The given pipe must be present in our list.
    assert (it != pipes.end ());

    //  Remove the pipe from the list.
    pipes.erase (it);

    //  Fix the 'current' variable if it points beyond the end of the list.
    if (current == pipes.size ())
        current = 0;
}

void zmq::load_balancer_t::initialise_shutdown ()
{
    //  Broadcast 'terminate_writer' to all associated pipes.
    for (pipes_t::iterator it = pipes.begin (); it != pipes.end (); it ++)
        (*it)->terminate_writer ();
}
