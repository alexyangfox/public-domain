#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#if defined(__hpux__) || defined(__linux__)
#  include <sys/ioctl.h>
#else
#  include <sys/filio.h>
#endif
#include <sys/socket.h>

#include "relay.h"


#define Export


#define MIN(a,b)	((a) < (b) ? (a) : (b))
#define MAX(a,b)	((a) > (b) ? (a) : (b))


/*
 *  Estimate how many bytes are available for reading on a file descriptor.
 */
static	long
available_bytes(int fd)
{
    long	  nbytes;
    struct stat	  sb;
    off_t	  pos;

    /*
     *  Some Unices don't support the FIONREAD ioctl on regular files.
     *  They don't even return an error, but says there are zero bytes
     *  to be read.  We then try to fstat() the file descriptor to see
     *  how big the file is, and try to determine at what offset the
     *  file pointer is at now.  That doesn't work always either...
     */

    if (ioctl(fd, FIONREAD, &nbytes) < 0  ||  nbytes == 0)
    {
	if (fstat(fd, &sb) < 0)
	    return -1;
	errno = 0;
	pos = lseek(fd, 0, SEEK_CUR);
	if ((pos < 0  &&  errno != 0)  ||  !S_ISREG(sb.st_mode))
	    pos = 0;
	nbytes = sb.st_size - pos;
    }
    return nbytes;
}



/*
 *  Wait for data to arrive on any of the source file descriptors in
 *  RELAYS, read all available data, and write to respective dest
 *  file descriptors.  TIMEOUT is the maximum time to wait, or a nil
 *  pointer to never time out.  For each read() done, the function
 *  CALLBACK is called.  Note that CALLBACK may be called more than
 *  once for each file descriptor if much data is available.
 *  NRELAYS is the number of elements in RELAYS.
 *
 *  If the 'readerror' field is non-zero for a relay, no read will be
 *  attempted from the source file descriptor of that relay.
 *  If the 'writerror' field is non-zero for a relay, no write will be
 *  attempted to the dest file descriptor of that relay, but the source
 *  descriptor will still be drained from any available data.
 *
 *  If end-of-file is reached on the source of a relay, the corresponding
 *  destination will be shutdown() for writing, thus causing the other
 *  end to see a end-of-file.
 *  If a write error occurs on the destination of a relay, the source
 *  of that relay will be shutdown() for reading, causing the other end
 *  to get errors when trying to write more data to us.
 *  Note that the SIGPIPE signal should be ignored, or possibly caught,
 *  by the caller, or the process will die when a receiver closes its
 *  end for receiving.
 *
 *  Returns the number of file descriptor read from, or negative on error.
 */
Export	int
relay_once(struct relay		* relays,
	   int			  nrelays,
	   struct timeval	* timeout,
	   int (*callback)(struct relay*, char*, size_t)
    )
{
    int		maxfd		= -1;
    fd_set	readset;
    int		i;
    int		nfds		= 0;
    int		nerrors		= 0;

    FD_ZERO(&readset);
    maxfd = 0;
    for (i = nrelays - 1 ;  i >= 0 ;  i--) 
    {
	if (relays[i].buffer && relays[i].bufferlen - relays[i].bufferoff > 0)
	{
	    FD_SET(relays[i].source, &readset);
	    if (relays[i].source > maxfd)
		maxfd = relays[i].source;
	    nfds++;
	}
	else if (!relays[i].readerror)
	{
	    FD_SET(relays[i].source, &readset);
	    if (relays[i].source > maxfd)
		maxfd = relays[i].source;
	}
    }

    if (nfds == 0 && maxfd >= 0)
    {
	nfds = select(maxfd+1, &readset, (fd_set*)NULL, (fd_set*)NULL, timeout);
	if (nfds <= 0) {
	    if (errno != EINTR)
		return nfds;
	    else
		return 0;
	}
    }

    for (i = nrelays - 1 ;  i >= 0 ;  i--)
    {
	if (FD_ISSET(relays[i].source, &readset))
	{
	    long  unread;

	    if (relays[i].buffer
		&& relays[i].bufferlen - relays[i].bufferoff > 0)
		unread = relays[i].bufferlen;
	    else
		unread = available_bytes(relays[i].source);

	    do 
	    {
		char buffer[8192];
		int bytes_read;

		if (relays[i].bufferlen - relays[i].bufferoff > sizeof(buffer))
		{
		    bytes_read = sizeof(buffer);
		    memcpy(buffer,
			   relays[i].buffer + relays[i].bufferoff,
			   sizeof(buffer));
		    relays[i].bufferoff += sizeof(buffer);
		}
		else if (relays[i].bufferlen - relays[i].bufferoff > 0)
		{
		    bytes_read = relays[i].bufferlen - relays[i].bufferoff;
		    memcpy(buffer,
			   relays[i].buffer + relays[i].bufferoff,
			   relays[i].bufferlen - relays[i].bufferoff);
		    relays[i].bufferoff += relays[i].bufferlen;
		    free(relays[i].buffer);
		    relays[i].bufferlen = 0;
		    relays[i].bufferoff = 0;
		}
		else
		    bytes_read = read(relays[i].source, buffer, sizeof buffer);

		if (bytes_read < 0) {
		    relays[i].readerror = errno;
		    nerrors++;
		    break;
		}
		unread -= bytes_read;
		if (bytes_read == 0) {
		    relays[i].readerror = -1;
		    if (shutdown(relays[i].dest, 1) < 0 && errno == ENOTSOCK)
			close(relays[i].dest);
		    nerrors++;
		    if (callback)
			(*callback)(&relays[i], NULL, 0);
		}
		if (callback)
		    (*callback)(&relays[i], buffer, bytes_read);
		if (!relays[i].writeerror  &&  bytes_read > 0) {
		    int written = write(relays[i].dest, buffer, bytes_read);
		    if (written < 0) {
			relays[i].writeerror = errno;
			if (shutdown(relays[i].source, 0) < 0
			    && errno == ENOTSOCK)
			    close(relays[i].source);
			nerrors++;
			if (callback)
			    (*callback)(&relays[i], NULL, 0);
			break;
		    }
		}
	    } while (unread > 0);
	}
    }

    return nerrors ? -nerrors : nfds;
}



/*
 *  Call relay_once() until end-of-file has been reached on all sources.
 *  No time limit.
 */
Export	int
relay_all(struct relay	* relaylist,
	  int		  nrelays,
	  int (*callback)(struct relay*, char*, size_t))
{
    int  nclosed = 0;
    do
    {
	int  status = relay_once(relaylist, nrelays, NULL, callback);
	if (status < 0)
	    nclosed += -status;
    } while (nclosed < nrelays);

    return 0;
}
