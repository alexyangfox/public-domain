#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/filio.h>
#include <sys/socket.h>


#define	MAX(a,b)	((a) > (b) ? (a) : (b))
#define	MIN(a,b)	((a) < (b) ? (a) : (b))


int
relay(int fd1,
      int fd2,
      int (*callback)(char prefix, char *data, size_t nbytes))
{
    fd_set readset;
    int nfds, len;
    char buffer[8192];
    int fd1_eof = 0;
    int fd2_eof = 0;

    while (!fd1_eof  ||  !fd2_eof)
    {
	/* Setup file descriptor set */
	FD_ZERO(&readset);
	if (!fd1_eof)
	    FD_SET(fd1, &readset);
	if (!fd2_eof)
	    FD_SET(fd2, &readset);

	nfds = select(MAX(fd1, fd2)+1,
		      &readset, NULL, NULL, NULL);

	/* error? */
	if (nfds < 0)
	{
	    if (errno == EINTR)
		continue;
	    return -1;
	}

	/* timeout? */
	if (nfds == 0)
	    continue;

	if (FD_ISSET(fd1, &readset))
	{
	    long  unread;
	    ioctl(fd1, FIONREAD, &unread);
	    do
	    {
		if ((len = read(fd1, buffer, sizeof buffer)) < 0)
		{
		    return -1;
		}
		unread -= len;

		/* remote server close? */
		if (len == 0)
		{
		    shutdown(fd2, 1);
		    fd1_eof = 1;
		}
		callback('>', buffer, len);
		write(fd2, buffer, len);
	    } while (unread > 0);
	}

	if (FD_ISSET(fd2, &readset))
	{
	    long unread;
	    ioctl(fd2, FIONREAD, &unread);
	    do
	    {
		if ((len = read(fd2, buffer, sizeof buffer)) < 0)
		{
		    return -1;
		}
		unread -= len;

		/* remote client close? */
		if (len == 0)
		{
		    shutdown(fd1, 1);
		    fd2_eof = 1;
		}
		callback('<', buffer, len);
		write(fd1, buffer, len);
	    } while (unread > 0);
	}

    }

    return 0;
}
