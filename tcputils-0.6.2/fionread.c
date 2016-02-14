#include <stdio.h>
#include <sys/filio.h>

int
main(int argc,
     char **argv)
{
    long unread = 0;
    int status = 0;

    status = ioctl(0, FIONREAD, &unread);
    printf("Status: %d    Unread: %ld\n", status, unread);

    return 0;
}
