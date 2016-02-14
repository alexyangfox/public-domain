#include <sys/wait.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/ksyscalls.h>
#include <errno.h>
#include <curses.h>
#include <sys/stat.h>
#include <string.h>
#include <signal.h>


void GetUsername (char *buf);
void print_martian (void);


char username[100+1] = "";
char *args[4+1];

int res;
int r;
int t;
int pid;
int rc;
int ec;


#define MAX_CONSOLE 12
#define CON_PATH_SZ 20

int console_pid[MAX_CONSOLE];
char con_path[CON_PATH_SZ];
char con_nn[10];

struct sigaction act;


void sigint_handler (int signal);






int main (int argc, char *argv[])
{	
	char *filename;
	int term;
	int fd;
	int rc;
	
	
	act.sa_flags = 0;
	act.sa_mask = 0;
	act.sa_handler = sigint_handler;
		
	sigaction (SIGINT, &act, NULL);

	kos_set_assign ("bin", "/sys/bin");
	kos_set_assign ("etc", "/sys/etc");
	
	
	setenv ("PATH", "/usr/local/bin:/usr/bin:/bin", 1);
	
	
	pid = fork();
	
	if (pid == 0)
	{
		rc = execlp("sh", "sh", "-c", "/sys/boot/startup.sh", (char *)0);
		
		printf ("getenv() PATH = %s\n", getenv("PATH"));
		printf ("execlp() rc = %d\n, error = %s\n", rc, strerror (errno));
	}
	else if (pid > 0)
	{
		waitpid (pid, NULL, 0);
	}
	else
	{
	}	
	
	/*
	system ("/sys/boot/startup.sh");
	*/


	for (t=0; t< MAX_CONSOLE; t++)
	{
		pid = fork();
		
		if (pid == 0)
		{
			term = t;
			break;
		}
		else if (pid > 0)
		{
			term = -1;
			console_pid[t] = pid;
		}
		else
		{
			printf ("Init failed to fork\n");
		
			while(1)
			{
				sigsuspend(0);
			}
		}
	}
	

	while (1)
	{	
		if (pid == 0)
		{
			setpgrp();
		
			strlcpy(con_path, "/con/", CON_PATH_SZ);
			
			snprintf (con_nn, 10, "%d", term);
			
			strlcat(con_path, con_nn, CON_PATH_SZ);
			
			fd = open (con_path, O_RDWR);
			
			if (fd < 0)
				exit (-1);
				
			dup2 (0, 1);
			dup2 (1, 2);
			
			
			print_martian();
						
			printf ("Console : /con/%d\n", term);
			GetUsername (&username[0]);
				
			filename = "/sys/bin/ksh";
				
			args[0] = filename;
			args[1] = "-l";
			args[2] = NULL;
		
			setenv ("HOME", "/sys/home", 1);
			setenv ("TERM", "kielder", 1);
			setenv ("TERMCAP", "/sys/etc/termcap", 1);
			setenv ("USER", username, 1);
			chdir ("/sys/home");
			
			execvp (filename, args);
			exit (0);
		}
		else
		{			
			pid = waitpid (-1, NULL, 0);
			
			for (t=0; t< MAX_CONSOLE; t++)
			{
				if (console_pid[t] == pid)
					break;
			}
			
			pid = fork();
		
			if (pid == 0)
			{
				term = t;
			}
			else
			{
				term = -1;
				console_pid[t] = pid;
			}
		}
	}
	
	return 0;
}




/*
 *
 */

void GetUsername (char *buf)
{
	ec = 0;
			
	while (ec == 0)
	{
		printf ("Login: ");
		fflush (stdout);
		
		
		/* Perhaps use something else, as scanf() bloats executable */
		*buf ='\0';			
		rc = scanf("%100[^\n]%*[^\n]", buf); 
		
		if (rc >= 0)
		{
			getchar();
		
			if (*buf != '\0')
				ec = 1;
		}
		else
		{
			clearerr(stdin);
		}
	}
}

		







/*
 *
 */

void print_martian (void)
{
	return 0;

	printf("\033[0;0H\033[0J");	
	printf("            \033[33mxodxxxxxooolooxkxxdcx\033[m\n");
	printf("              \033[32mkkkxo::do:cdxkkox\033[m\n");
	printf("            \033[32mXl;:cc::::::::cc;cOKWW\033[m\n");
	printf("          \033[32mKkoc:c:::::::::::::::::oxO\033[m\n");
	printf("        \033[32mOo:::;;;:ccccccccccccc:;;;;;cxK\033[m\n");
	printf("      \033[32mOc;;:::cccccccccccccccccccccc;;,,d\033[m\n");
	printf("     \033[32mk;,:ccccccccccccccccccccccccccccc:':X\033[m\n");
	printf("    \033[32md,:ccccccccccccccccccccccccccccccccc:c0\033[m\n");
	printf("   \033[32mo:ccccccccccc:c:;::;;;;:;:::ccccccccccccx\033[m   Where's the kaboom?\n");
	printf("  \033[32mdclccccc:;;:::;;;;:;,,,,::;;;:::;;:cccccc:k\033[m\n");
	printf("  \033[32mccccc:;;;,,'\033[m;okkOKNd....kNKOkxc'\033[32m;;;:;:ccccl\033[m  There was supposed to be an\n");
	printf("  \033[32mccc:;;;,.   \033[mcWMMMMMX'..:NMMMMMk.\033[32m ..,;;,;ccl\033[m  Earth-shattering kaboom!\n");
	printf("  \033[32mlc;;;,.     \033[mcWMMMMMW:..oMMMMWWO..\033[32m   ..;;;:l\033[m\n");
	printf("  \033[32mo;;:':'     \033[m:NMMMMX0l..xN0WMMMO..\033[32m    ..,:,o\033[m\n");
	printf("   \033[32m,c,;,'    \033[m..0MMMWl'c .dk'KMWMx  \033[32m    ..'::x\033[m\n");
	printf("    \033[32mlc;'..    \033[m.oWMMMX0c .cXKWMMWc. \033[32m    ..:ox\033[m\n");
	printf("       \033[32m0c.    \033[m..dWMMMN,...kMMMWx.  \033[32m   .,cm\033[m\n");
	printf("       \033[32mO:,.    \033[m..xWWWd....,KWKc.  \033[32m   .':;O\033[m\n");
	printf("      \033[32mO;,:c;.\033[m    .':,...   ... \033[32m    .,cc;;c\033[m\n");
	printf("    \033[32md:,:ccccc,.... ..      .......;cllcc,;;d\033[m\n");
	printf("    \033[32mKc:,:lcccc:codl;,......,:lxOXKlcllc;:c0\033[m\n");
	printf("     \033[32mXo:;:cccd0     \033[31mWK,.''cXW\033[32m      kc:;:oK\033[m\n");
	printf("      \033[32mWxlcoO      \033[31mW0c,'','',:xN\033[32m     0dxK\033[m\n");
	printf("                 \033[31mOl,',,,,,,,,':O\033[m\n");
}




void sigint_handler (int sig)
{
}
