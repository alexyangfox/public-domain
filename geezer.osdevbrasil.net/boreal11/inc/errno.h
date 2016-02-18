#ifndef __TL_ERRNO_H
#define	__TL_ERRNO_H

#ifdef __cplusplus
extern "C"
{
#endif

#define	ERR_NO_MEM	2	/* [k][m|c]alloc() failed */
#define	ERR_NO_FILES	3	/* no file_t pointers left in task_t */
#define	ERR_NOSUCH_FILE	4	/* can't open file/directory/device */
#define	ERR_BAD_ARG	5	/* invalid argument to function */
#define	ERR_NOT_OPEN	6	/* attempted read(), etc. on closed file */
#define	ERR_RELOC	7	/* error relocating/linking kernel module */
#define	ERR_EXEC	8	/* error loading executable file */

#ifdef __cplusplus
}
#endif

#endif
