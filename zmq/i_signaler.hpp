/*
    Copyright (c) 2007 FastMQ Inc.

    This file is part of 0MQ.

    0MQ is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    0MQ is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ZMQ_I_SIGNALER_HPP_INCLUDED__
#define __ZMQ_I_SIGNALER_HPP_INCLUDED__

namespace zmq
{

    struct i_signaler
    {
        //  The destructor shouldn't be virtual, however, not defining it as
        //  such results in compiler warnings.
        virtual ~i_signaler () {};
        virtual void signal (int index) = 0;
    };

}

#endif
