#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

int
main(int argc, char **argv)
{
    struct stat f;
    int status;

    status = fstat(0, &f);
    printf("Status: %d   errno: %d\n", status, errno);
    printf("st_mode: %o\n", f.st_mode);
    printf("st_ino: %d\n", f.st_ino);
    printf("st_dev: %d\n", f.st_dev);
    printf("st_rdev: %d\n", f.st_rdev);
    printf("st_nlink: %d\n", f.st_nlink);
    printf("st_uid: %d\n", f.st_uid);
    printf("st_gid: %d\n", f.st_gid);
    printf("st_size: %d\n", f.st_size);
    printf("st_atime: %d\n", f.st_atime);
    printf("st_mtime: %d\n", f.st_mtime);
    printf("st_ctime: %d\n", f.st_ctime);
    printf("st_blksize: %d\n", f.st_blksize);
    printf("st_blocks: %d\n", f.st_blocks);

    return 0;
}
