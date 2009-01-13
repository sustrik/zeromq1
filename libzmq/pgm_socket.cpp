/*
    Copyright (c) 2007-2008 FastMQ Inc.

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

#include <zmq/platform.hpp>

#if ZMQ_HAVE_OPENPGM && defined ZMQ_HAVE_LINUX

#include <pgm/pgm.h>
#include <zmq/err.hpp>
#include <string>
#include <assert.h>

#include <zmq/pgm_socket.hpp>

#define PGM_SOCKET_DEBUG
#define PGM_SOCKET_DEBUG_LEVEL 4

// level 1 = key behaviour
// level 2 = processing flow
// level 4 = infos

#ifndef PGM_SOCKET_DEBUG
#   define zmq_log(n, ...)  while (0)
#else
#   define zmq_log(n, ...)    do { if ((n) <= PGM_SOCKET_DEBUG_LEVEL) { printf (__VA_ARGS__);}} while (0)
#endif

zmq::pgm_socket_t::pgm_socket_t (bool receiver_, const char *interface_, 
    size_t readbuf_size_) : 
    g_transport (NULL), pgm_msgv (NULL), pgm_msgv_len (-1)
{
    
    //  Init PGM transport.
    pgm_init ();

    pgm_gsi_t gsi;

    int rc = pgm_create_md5_gsi (&gsi);
	assert (rc == 0);

    struct group_source_req recv_gsr, send_gsr;
    int recv_len = 1;

    //  Parse port number from interface_ string.
    char *port_delim = strchr (interface_, ':');
    assert (port_delim);

    port_delim++;

    uint16_t port = atoi (port_delim);

    //  Strip port number from interface_ string.
    std::string interface (interface_);
    std::string::size_type pos = interface.find (":");

    assert (pos != std::string::npos);

    interface = interface.substr (0, pos);
   
    rc = pgm_if_parse_transport (interface.c_str (), AF_INET, &recv_gsr, 
        &recv_len, &send_gsr);
    assert (rc == 0);
    assert (recv_len == 1);

    rc = pgm_transport_create (&g_transport, &gsi, 0, port, &recv_gsr, 1, 
        &send_gsr);
    errno_assert (rc == 0);

    //  Size of window in sequence numbers. The same for transmit and receive
    //  window.
    int g_sqns = 100;

    //  The maximum transport protocol data unit (TPDU) size.
    int g_max_tpdu = 1500;

    //  Common parameters for receiver and sender.

    //  Set maximum transport data unit size.
    rc = pgm_transport_set_max_tpdu (g_transport, g_max_tpdu);
    assert (rc == 0);

    //  Set maximum number of network hops to cross.
    rc = pgm_transport_set_hops (g_transport, 16);
    assert (rc == 0);

    //  Receiver transport.
    if (receiver_) {
  
        //  Set transport->may_close_on_failure to true,
        //  after data los recvmsgv returns -1 errno set to ECONNRESET.
        rc = pgm_transport_set_close_on_failure (g_transport);
        assert (rc == 0);

        //  Set transport->can_send_data = FALSE.
        //  Note that NAKs are still generated by the transport.
        rc = pgm_transport_set_recv_only (g_transport, false);
        assert (rc == 0);

        //  Set NAK transmit back-off interval [us].
        rc = pgm_transport_set_nak_bo_ivl (g_transport, 50*1000);
        assert (rc ==0);
    
        //  Set timeout before repeating NAK [us].
        rc = pgm_transport_set_nak_rpt_ivl (g_transport, 200*1000);
        assert (rc == 0);

        //  Set timeout for receiving RDATA.
        rc = pgm_transport_set_nak_rdata_ivl (g_transport, 200*1000);
        assert (rc == 0);

        //  Set retries for NAK without NCF/DATA (NAK_DATA_RETRIES).
        rc = pgm_transport_set_nak_data_retries (g_transport, 5);
        assert (rc == 0);

        //  Set retries for NCF after NAK (NAK_NCF_RETRIES).
        rc = pgm_transport_set_nak_ncf_retries (g_transport, 5);
        assert (rc == 0);

        //  Set timeout for removing a dead peer [us].
        rc = pgm_transport_set_peer_expiry (g_transport, 5*8192*1000);
        assert (rc == 0);

        //  Set expiration time of SPM Requests [us].
        rc = pgm_transport_set_spmr_expiry (g_transport, 250*1000);
        assert (rc == 0);

        //  Set receive window size in sequence numbers.
        rc = pgm_transport_set_rxw_sqns (g_transport, g_sqns);
        assert (rc == 0);
        
    //  Sender transport.
    } else {

        //  Set transport->can_recv = FALSE, data packets will not be read.
        rc = pgm_transport_set_send_only (g_transport);
        assert (rc == 0);

        //  Set send window size in sequence numbers. 
	rc = pgm_transport_set_txw_sqns (g_transport, g_sqns);
        assert (rc == 0);

        //  Preallocate full (g_sqns) transmit window.
        rc = pgm_transport_set_txw_preallocate (g_transport, g_sqns);
        assert (rc == 0);

        //  Set interval of background SPM packets [us].
        rc = pgm_transport_set_ambient_spm (g_transport, 8192*1000);
        assert (rc == 0);

        //  Set intervals of data flushing SPM packets [us].
        guint spm_heartbeat[] = { 1*1000, 1*1000, 2*1000, 4*1000, 8*1000, 
            16*1000, 32*1000, 64*1000, 128*1000, 256*1000, 512*1000, 
            1024*1000, 2048*1000, 4096*1000, 8192*1000 };

	rc = pgm_transport_set_heartbeat_spm (g_transport, spm_heartbeat, 
            G_N_ELEMENTS(spm_heartbeat));
        assert (rc == 0);
    }
    
    //  Bind a transport to the specified network devices.
    rc = pgm_transport_bind (g_transport);
    assert (rc == 0);

    //  For receiver transport preallocate pgm_msgv array.
    if (receiver_) {
        pgm_msgv_len = get_max_apdu_at_once (readbuf_size_);
        pgm_msgv = new pgm_msgv_t [pgm_msgv_len];
    }

}

zmq::pgm_socket_t::~pgm_socket_t ()
{
    //  Celanup.
    if (pgm_msgv) {
        delete [] pgm_msgv;
    }

    pgm_transport_destroy (g_transport, TRUE);
}

int zmq::pgm_socket_t::get_receiver_fds (int *recv_fd_, 
    int *waiting_pipe_fd_)
{

    //  For POLLIN there are 2 pollfds in pgm_transport.
    int fds_array_size = pgm_receiver_fd_count;
    pollfd *fds = new pollfd [fds_array_size];
    memset (fds, '\0', fds_array_size * sizeof (fds));

    //  Retrieve pollfds from pgm_transport.
    int rc = pgm_transport_poll_info (g_transport, fds, &fds_array_size, POLLIN);

    //  pgm_transport_poll_info has to return 2 pollfds for POLLIN. 
    //  Note that fds_array_size parameter can be 
    //  changed inside pgm_transport_poll_info call.
    assert (rc == pgm_receiver_fd_count);
 
    //  Store pfds into user allocated space.
    *recv_fd_ = fds [0].fd;
    *waiting_pipe_fd_ = fds [1].fd;

    delete [] fds;

    return pgm_receiver_fd_count;
}

int zmq::pgm_socket_t::get_sender_fd (int *send_fd_)
{

    //  For POLLOUT there are 1 pollfds in pgm_transport.
    int fds_array_size = pgm_sender_fd_count;
    pollfd *fds = new pollfd [fds_array_size];
    memset (fds, '\0', fds_array_size * sizeof (fds));

    //  Retrieve pollfds from pgm_transport
    int rc = pgm_transport_poll_info (g_transport, fds, &fds_array_size, POLLOUT);

    //  pgm_transport_poll_info has to return 1 pollfds for POLLOUT. 
    //  Note that fds_array_size parameter can be 
    //  changed inside pgm_transport_poll_info call.
    assert (rc == pgm_sender_fd_count);
 
    //  Store pfds into user allocated space.
    *send_fd_ = fds [0].fd;

    delete [] fds;

    return pgm_sender_fd_count;
}

//  Send one APDU, transmit window owned memory.
size_t zmq::pgm_socket_t::write_one_pkt (unsigned char *data_, size_t data_len_)
{
    iovec iov = {data_,data_len_};

    ssize_t nbytes = pgm_transport_send_packetv (g_transport, &iov, 1, MSG_DONTWAIT | MSG_WAITALL, true);

    assert (nbytes != -EINVAL);

    if (nbytes == -1 && errno != EAGAIN) {
        errno_assert (0);
    }

    //  If nbytes is -1 and errno is EAGAIN means that we can not send data 
    //  now. We have to call write_one_pkt again.
    nbytes = nbytes == -1 ? 0 : nbytes;

    zmq_log (2, "wrote %iB, %s(%i)\n", (int)nbytes, __FILE__, __LINE__);
    
    // We have to write all data as one packet.
    if (nbytes > 0) {
        assert (nbytes = data_len_);
    }

    return nbytes;
}

//  Return max TSDU size from current PGM transport.
size_t zmq::pgm_socket_t::get_max_tsdu_size (bool can_fragment_)
{
    return (size_t)pgm_transport_max_tsdu (g_transport, can_fragment_);
}

//  Returns how many APDUs are needed to fill reading buffer.
size_t zmq::pgm_socket_t::get_max_apdu_at_once (size_t readbuf_size_)
{
    assert (readbuf_size_ > 0);

    //  Read max TSDU size without fragmentation.
    size_t max_tsdu_size = get_max_tsdu_size (false);

    //  Calculate number of APDUs needed to fill the reading buffer.
    size_t apdu_count = (int)readbuf_size_ / max_tsdu_size;

    if ((int) readbuf_size_ % max_tsdu_size)
        apdu_count ++;

    zmq_log (2, "readbuff size %i, maxapdu_at_once %i, max tsdu %i\n", 
        (int)readbuf_size_, (int)apdu_count, (int)get_max_tsdu_size (false));

    //  Have to have at least one APDU.
    assert (apdu_count);

    return apdu_count;
}

//  Allocate a packet from the transmit window, The memory buffer is owned 
//  by the transmit window and so must be returned to the window with content
//  via pgm_transport_send() calls or unused with pgm_packetv_free1(). 
unsigned char *zmq::pgm_socket_t::alloc_one_pkt (bool can_fragment_)
{
    return (unsigned char*) pgm_packetv_alloc (g_transport, can_fragment_);
}

//  Return an unused packet allocated from the transmit window 
//  via pgm_packetv_alloc(). 
void zmq::pgm_socket_t::free_one_pkt (unsigned char *data_, bool can_fragment_)
{
    pgm_packetv_free1 (g_transport, data_, can_fragment_);
}

void zmq::pgm_socket_t::drop_superuser ()
{
    pgm_drop_superuser ();
}

//  Receive a vector of Application Protocol Domain Unit's (APDUs) from the 
//  transport. Note that iov_len has to be equal to the pgm_msgv_len it means
//  that entire pgm_msgv structure returned by pgm_transport_recvmsgv will be 
//  returned in one read_pkt call.
size_t zmq::pgm_socket_t::read_pkt (iovec *iov_, size_t iov_len_)
{

    assert (iov_len_ == pgm_msgv_len);
    
    //  Receive a vector of Application Protocol Domain Unit's (APDUs) 
    //  from the transport.
    ssize_t nbytes = pgm_transport_recvmsgv (g_transport, pgm_msgv, 
        pgm_msgv_len, MSG_DONTWAIT);
   
    //  In a case when not ODATA/RDATA fired POLLIN event (SPM...)
    //  pgm_transport_recvmsg returns -1 with errno == EAGAIN.
    //  For data loss nbytes == -1 errno == ECONNRESET.
    if (nbytes == -1 && errno != EAGAIN) {
        errno_assert (false); 
    }

    //  Tanslate from pgm_msgv_t into iovec structures. Note that pgm_msgv_t 
    //  and therefore iov buffers are directly from the receive window. Memory 
    //  is returned on the next call or transport destruction.
    
    //  How mant bytes we already translated.
    int translated = 0;

    //  Pointer to received pgm_msgv_t structure.
    pgm_msgv_t *msgv = pgm_msgv;

    //  Pointer to destination iovec structures.
    iovec *iov = iov_;
    iovec *iov_last = iov + iov_len_;

    while (translated < (int) nbytes) {

        assert (iov <= iov_last);

        iov->iov_base = (msgv->msgv_iov)->iov_base;
        iov->iov_len = (msgv->msgv_iov)->iov_len;
        
        // Only one APDU per pgm_msgv_t structure is allowed. 
        assert (msgv->msgv_iovlen == 1);
        
        translated += iov->iov_len;

        msgv++;
        iov++;
    }

    //  In a case if no RDATA/ODATA and no data loss caused POLLIN 0 is 
    //  returned.
    nbytes = nbytes == -1 ? 0 : nbytes;
    
    if (nbytes)
        zmq_log (2, "received %i bytes, ", (int)nbytes);

    assert (nbytes == translated);

    return (size_t) nbytes;
}

#endif
