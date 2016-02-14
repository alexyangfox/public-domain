/* $Header: /nw/tony/src/stevie/src/RCS/unix.c,v 1.8 89/08/06 09:51:13 tony Exp $
 *
 * System-dependent routines for UNIX System V or Berkeley.
 */

#include "stevie.h"
#ifdef BSD
#include <sgtty.h>
#else
#include <termio.h>
#endif
#include <signal.h>

/*
 * inchar() - get a character from the keyboard
 */
int
inchar()
{
	char	c;

	flushbuf();		/* flush any pending output */

	do {
		while (read(0, &c, 1) != 1)
			;
	} while (c == NUL);

	got_int = FALSE;
	return c;
}

#define	BSIZE	2048
static	char	outbuf[BSIZE];
static	int	bpos = 0;

void
flushbuf()
{
	if (bpos != 0)
		write(1, outbuf, bpos);
	bpos = 0;
}

/*
 * Macro to output a character. Used within this file for speed.
 */
#define	outone(c)	outbuf[bpos++] = c; if (bpos >= BSIZE) flushbuf()

/*
 * Function version for use outside this file.
 */
void
outchar(c)
char	c;
{
	outone(c);
}

void
outstr(s)
register char	*s;
{
	while (*s) {
		outone(*s++);
	}
}

void
beep()
{
	if ( P(P_VB) )
		vbeep ();
	else
		outone('\007');
}

/*
 * remove(file) - remove a file
 */
void
remove(file)
char *file;
{
	unlink(file);
}

/*
 * rename(of, nf) - rename existing file 'of' to 'nf'
 */
void
rename(of, nf)
char	*of, *nf;
{
	unlink(nf);
	link(of, nf);
	unlink(of);
}

void
pause()
{
	sleep (1);
}

#ifdef BSD
static	struct	sgttyb	ostate;
#else
static	struct	termio	ostate;
#endif

/*
 * Go into cbreak mode
 */
void
set_tty()
{
#ifdef BSD
	struct	sgttyb	nstate;

	ioctl(0, TIOCGETP, &ostate);
	nstate = ostate;
	nstate.sg_flags &= ~(XTABS|CRMOD|ECHO);
	nstate.sg_flags |= CBREAK;
	ioctl(0, TIOCSETN, &nstate);
#else
	struct	termio	nstate;

	ioctl(0, TCGETA, &ostate);
	nstate = ostate;
	nstate.c_lflag &= ~(ICANON|ECHO|ECHOE|ECHOK|ECHONL);
	nstate.c_cc[VMIN] = 1;
	nstate.c_cc[VTIME] = 0;
	ioctl(0, TCSETAW, &nstate);
#endif
}

/*
 * Restore original terminal modes
 */
void
reset_tty()
{
#ifdef BSD
	ioctl(0, TIOCSETP, &ostate);
#else
	ioctl(0, TCSETAW, &ostate);
#endif
}

void
sig()
{
	signal(SIGINT, sig);
	signal(SIGQUIT, sig);

	got_int = TRUE;
}

void
windinit()
{
#ifdef	TERMCAP
	if (t_init() != 1) {
		fprintf(stderr, "unknown terminal type\n");
		exit(1);
	}
#else
	Columns = 80;
	P(P_LI) = Rows = 24;
#endif

	/*
	 * The code here makes sure that there isn't a window during which
	 * we could get interrupted and exit with the tty in a weird state.
	 */
	signal(SIGINT, sig);
	signal(SIGQUIT, sig);

	set_tty();

	if (got_int)
		windexit(0);
}

void
windexit(r)
int r;
{
	reset_tty();
	exit(r);
}

void
windgoto(r, c)
register int	r, c;
{
#ifdef	TERMCAP
	char	*tgoto();
#else
	r += 1;
	c += 1;
#endif

	/*
	 * Check for overflow once, to save time.
	 */
	if (bpos + 8 >= BSIZE)
		flushbuf();

#ifdef	TERMCAP
	outstr(tgoto(T_CM, c, r));
#else
	outbuf[bpos++] = '\033';
	outbuf[bpos++] = '[';
	if (r >= 10)
		outbuf[bpos++] = r/10 + '0';
	outbuf[bpos++] = r%10 + '0';
	outbuf[bpos++] = ';';
	if (c >= 10)
		outbuf[bpos++] = c/10 + '0';
	outbuf[bpos++] = c%10 + '0';
	outbuf[bpos++] = 'H';
#endif
}

FILE *
fopenb(fname, mode)
char	*fname;
char	*mode;
{
	return fopen(fname, mode);
}

char *
fixname(s)
char	*s;
{
	return s;
}

/*
 * doshell() - run a command or an interactive shell
 */
void
doshell(cmd)
char	*cmd;
{
	char	*getenv();
	char	cline[128];

	outstr("\r\n");
	flushbuf();

	if (cmd == NULL) {
		if ((cmd = getenv("SHELL")) == NULL)
			cmd = "/bin/sh";
		sprintf(cline, "%s -i", cmd);
		cmd = cline;
	}

	reset_tty();
	system(cmd);
	set_tty();

	wait_return();
}


/*
 *	FILL IT IN, FOR YOUR SYSTEM, AND SHARE IT!
 *
 *	The next couple of functions do system-specific stuff.
 *	They currently do nothing; I'm not familiar enough with
 *	system-specific programming on this system.
 *	If you fill it in for your system, please post the results
 *	and share with the rest of us.
 */


setcolor (c)
/*
 * Set the color to c, using the local system convention for numbering
 * colors or video attributes.
 *
 * If you implement this, remember to note the original color in
 * windinit(), before you do any setcolor() commands, and
 * do a setcolor() back to the original as part of windexit().
 */
  int c;
{
	/* Dummy routine, just return 0 */
	return (0);
}


setrows (r)
/*
 * Set the number of lines to r, if possible.  Otherwise
 * "do the right thing".  Return the number of lines actually set.
 *
 * If you implement this, remember to note the original number of rows
 * in windinit(), before you do any setrows() commands, and
 * do a setrows() back to the original as part of windexit().
 */
  int r;
{
	/* Since we do nothing, just return the current number of lines */
	return ( P(P_LI) );
}


vbeep ()
/*
 * Do a "visual bell".  This generally consists of flashing the screen
 * once in inverse video.
 */
{
	int	color, revco;

	color = P( P_CO );		/* get current color */
	revco = reverse_color (color);	/* system-specific */
	setcolor (revco);
	flushbuf ();
	pause ();
	setcolor (color);
	windgoto (Cursrow, Curscol);
	flushbuf ();
}

reverse_color (co)
/*
 * Returns the inverse video attribute or color of co.
 * The existing code below is VERY simple-minded.
 * Replace it with proper code for your system.
 */
 int co;
{
	if (co)		return (0);
	else		return (1);
}


/********** End of do-it-yourself kit **********************/
