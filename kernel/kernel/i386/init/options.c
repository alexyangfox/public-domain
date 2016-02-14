#include <kernel/types.h>
#include <kernel/proc.h>
#include <kernel/i386/i386.h>
#include <kernel/utility.h>
#include <kernel/i386/init.h>
#include <kernel/i386/multiboot.h>
#include <kernel/dbg.h>
#include <kernel/config.h>



/*
 *
 */

char kargstr[512];
char *kargv[512];
int kargc;
int	opterr = 1;
int	optind = 1;
int	optopt;
char *optarg;



void InitConfigOptions (void);
char *Tokenize (char *c);
int InitKargvArray (char *kargstr, char *endch, char **kargv);
int GetOpt(char **argv, int argc, char *opts);
char *StrChr (char *str, char ch);




/*
 *
 */

void InitConfigOptions (void)
{
	char *endch;
	char opt;

	
	StrLCpy (kargstr, (char*)mbi->cmdline, 512);
	endch = Tokenize (kargstr);
	kargc = InitKargvArray (kargstr, endch, kargv);

	while ((opt = GetOpt(kargv, kargc, "vd:pb:")) != -1)
	{
		switch (opt)
		{
			case 'v':		/* Verbose */
				cfg_boot_verbose = 1;
				break;
			
			case 'd':		/* debug level */
				break;
			
			case 'p':		/* Pause debug output */
				break;
			
			case 'b':		/* Boot device */
				cfg_boot_prefix = optarg;
				break;
		}
	}
}





/*
 *
 */

char *Tokenize (char *c)
{
	int isquote = 0;
	
	while (*c != '\0')
	{
		if (*c == '"')
		{
			isquote = !isquote;
			*c = '\0';
		}
		else if (*c == ' ')
		{
			if (isquote == 0)
				*c = '\0';
		}
		
		c++;
	}
	
	return c;
}




/*
 *
 */
 
int InitKargvArray (char *kargstr, char *endch, char **kargv)
{
	int i;
	char *ch;
	char prev_ch;
	

	i = 0;
	ch = kargstr;
	prev_ch = '\0';
	
	while (ch < endch && i < 512)
	{
		if (*ch != '\0' && prev_ch == '\0')
			kargv[i++] = ch;
		
		prev_ch = *ch++;
	}
	
	return i;
}







/*
**  This is the public-domain AT&T getopt(3) code.  I added the
**  #ifndef stuff because I include <stdio.h> for the program;
**  getopt, per se, doesn't need it.  I also added the INDEX/index
**  hack (the original used strchr, of course).  And, note that
**  technically the casts in the write(2) calls shouldn't be there.
*/

 
int GetOpt(char **argv, int argc, char *opts)
{
	static int sp = 1;
	int c;
	char *cp;


	if (sp == 1)
	{
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
		{
			return -1;
		}
		else if (StrCmp (argv[optind], "--") == 0)
		{
			optind++;
			return -1;
		}
	}

	optopt = c = argv[optind][sp];
	
	if (c == ':' || (cp = StrChr(opts, c)) == NULL)
	{
		if (argv[optind][++sp] == '\0')
		{
			optind++;
			sp = 1;
		}
		
		return '?';
	}
	
	if (*++cp == ':')
	{
		if (argv[optind][sp+1] != '\0')
		{
			optarg = &argv[optind++][sp+1];
		}
		else if (++optind >= argc)
		{
			sp = 1;
			
			return '?';
		}
		else
		{
			optarg = argv[optind++];
		}
		
		sp = 1;
	}
	else
	{
		if(argv[optind][++sp] == '\0')
		{
			sp = 1;
			optind++;
		}
		
		optarg = NULL;
	}
	
	return c;
}
