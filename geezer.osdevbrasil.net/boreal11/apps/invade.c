/*----------------------------------------------------------------------------
ANSI SPACE INVADERS

EXPORTS:
int main(void)

To do:
Need to reduce width of fleet when complete columns of ships
on the left and right sides of the fleet are destroyed
-- makes the fleet take longer to move left and right

Need to reduce height of fleet when complete rows of ships
on the bottom are destroyed
-- makes the fleet take longer to land

Need to have invaders shooting at the defender
(multiple missiles? try to implement something like sprites)

Defender needs bunkers to hide under
----------------------------------------------------------------------------*/
#include <string.h> /* strlen() */
#include <stdio.h> /* stdout, fflush(), fputs(), printf(), putchar() */
#include <conio.h> /* KEY_nnn */
#include <os.h> /* read(), select() */

#define	FLEET_WD		8
#define	FLEET_HT		4

#define	INVADER_WD		5
#define	INVADER_HT		2
#define	TOTAL_INVADER_WD	(INVADER_WD + 3)
#define	TOTAL_INVADER_HT	(INVADER_HT + 2)

#define	SCORE_X			60

#define	FLEET_Y_GAME_OVER	8
#define	DEFENDER_Y		23
#define	STATUS_Y		24

/* 10s of milliseconds between fleet movements */
#define	DELAY			25

#define	COLOR_BLACK		0
#define	COLOR_RED		1
#define	COLOR_GREEN		2
#define	COLOR_BROWN		3
#define	COLOR_BLUE		4
#define	COLOR_MAGENTA		5
#define	COLOR_CYAN		6
#define	COLOR_LIGHT_GRAY	7
#define	COLOR_DARK_GRAY		8
#define	COLOR_PINK		9
#define	COLOR_LIGHT_GREEN	10
#define	COLOR_YELLOW		11
#define	COLOR_LIGHT_BLUE	12
#define	COLOR_LIGHT_MAGENTA	13
#define	COLOR_LIGHT_CYAN	14
#define	COLOR_WHITE		15

static char g_destroyed[FLEET_HT][FLEET_WD];
static unsigned g_fleet_x, g_fleet_y, g_score;
static unsigned g_missile_x, g_missile_y, g_defender_x;
/* maybe I need a syscall or ioctl or something for this... */
static const int g_scn_wd = 80, g_scn_ht = 25;
/*****************************************************************************
*****************************************************************************/
static void move_csr(unsigned x, unsigned y)
{
	printf("\x1B[%u;%uH", y, x);
}
/*****************************************************************************
*****************************************************************************/
static void set_fore_color(unsigned color)
{
	if(color >= 8)
		printf("\x1B[%u;1m", 30 + (color & 7));
	else
		printf("\x1B[%u;0m", 30 + (color & 7));
}
/*****************************************************************************
*****************************************************************************/
static void set_back_color(unsigned color)
{
	printf("\x1B[%um", 40 + (color & 7));
}
/*****************************************************************************
*****************************************************************************/
static void clear_screen(void)
{
	printf("\x1B[2J");
}
/*****************************************************************************
*****************************************************************************/
static void draw_invaders(void)
{
	static const char *invader_a[INVADER_HT] =
	{
		" @@@ ",
		"@   @"
	};
	static const char *invader_b[INVADER_HT] =
	{
		" @@@ ",
		" @ @ "
	};
/**/
	unsigned ypos, fx, fy, iy;
	const char **invader;

	for(fy = 0; fy < FLEET_HT; fy++)
	{
/* set color */
		set_fore_color(fy + 9);
/* blank line to prevent stuff left on screen when moving invaders down */
		ypos = g_fleet_y + fy * TOTAL_INVADER_HT + 0;
		move_csr(g_fleet_x, ypos);
/* errrr, my printf() doesn't handle this yet */
//		printf("%-*.*s", g_scn_wd, g_scn_wd, "");
printf("                                                               ");
		for(iy = 0; iy < INVADER_HT; iy++)
		{
/* move cursor */
			ypos = g_fleet_y + fy * TOTAL_INVADER_HT + (iy + 1);
			move_csr(g_fleet_x, ypos);
/* blank char to prevent stuff left on screen when moving invaders right */
			putchar(' ');
			for(fx = 0; fx < FLEET_WD; fx++)
			{
/* set invader shape */
				if((fx ^ g_fleet_x ^ fy ^ g_fleet_y) & 1)
						invader = invader_a;
				else
					invader = invader_b;
/* draw one row */
				if(!g_destroyed[fy][fx])
//					printf("%-*.*s", TOTAL_INVADER_WD,
//						TOTAL_INVADER_WD,
//						invader[iy]);
					printf("%s   ", invader[iy]);
				else
//					printf("%-*.*s", TOTAL_INVADER_WD,
//						TOTAL_INVADER_WD, "");
					printf("        ");
			}
/* blank char to prevent stuff left on screen when moving invaders left */
			putchar(' ');
		}
	}
	fflush(stdout);
}
/*****************************************************************************
*****************************************************************************/
static void move_fleet(void)
{
	static int delta_x = +1;
/**/

	if(delta_x > 0)
	{
		if(g_fleet_x > 10)
		{
			delta_x = -delta_x;
				g_fleet_y++;
		}
		else
			g_fleet_x += delta_x;
	}
	else if(delta_x < 0)
	{
		if(g_fleet_x <= 0)
		{
			delta_x = -delta_x;
			g_fleet_y++;
		}
		else
			g_fleet_x += delta_x;
	}
}
/*****************************************************************************
*****************************************************************************/
static int move_missile(void)
{
	int got_one = 0;
	unsigned y, x;

	if(g_missile_y == 0)
		return got_one;
	set_fore_color(COLOR_WHITE);
/* erase old missile */
	move_csr(g_missile_x, g_missile_y);
	putchar(' ');
/* move missile */
	g_missile_y--;
/* gone off top of screen? */
	if(g_missile_y != 0)
	{
/* see if it hit anyone */
		y = (g_missile_y - g_fleet_y) / TOTAL_INVADER_HT;
		x = (g_missile_x - g_fleet_x) / TOTAL_INVADER_WD;
		if(x < FLEET_WD && y < FLEET_HT && !g_destroyed[y][x])
		{
			g_destroyed[y][x] = 1;
			g_missile_y = 0;
			got_one = 1;
			g_score++;
			move_csr(SCORE_X, STATUS_Y);
			printf("Score: %u    ", g_score);
		}
/* draw new missile */
		else
		{
			move_csr(g_missile_x, g_missile_y);
			putchar('*');
		}
	}
	fflush(stdout);
	return got_one;
}
/*****************************************************************************
*****************************************************************************/
static void draw_defender(int delta_x)
{
	g_defender_x += delta_x;
/* -1 so we can erase left side of defender if moving right */
	move_csr(g_defender_x - 1, DEFENDER_Y);
	set_fore_color(COLOR_WHITE);
/* spaces on the left and right to erase old defender */
	fputs(" # ", stdout);
	fflush(stdout);
}
/*****************************************************************************
display string centered on the screen
*****************************************************************************/
static void center_string(unsigned y, const char *str)
{
	unsigned x;

	x = (g_scn_wd - strlen(str)) / 2;
	move_csr(x, y);
	printf("%s", str);
}
/*****************************************************************************
*****************************************************************************/
static int kbhit(void)
{
/* 0=stdin file handle, 1=check for input, return immediately if no data */
	return select(0, 1, NULL);
}
/*****************************************************************************
*****************************************************************************/
static int getch(void)
{
	unsigned char rv;

	(void)read(0, &rv, 1);
	return rv;
}
/*****************************************************************************
*****************************************************************************/
static void delay(unsigned milliseconds)
{
/* 0=stdin file handle (not used),
0=do not check if I/O ready (timeout only) */
	(void)select(0, 0, &milliseconds);
}
/*****************************************************************************
the blocks below are '\xDB' characters. If they don't look right,
you should use the PC character set, rather than Latin-1 or whatever.
*****************************************************************************/
static void banner(void)
{
	static const char *l33t_banner[] = /* well, it IS... */
	{
	"ллл  л   л  ллл  л      л  л   л  л   л  ллл  лл   ллл  ллл   ллл\n",
	"л л  лл  л  л    л      л  лл  л  л   л  л л  л л  л    л  л  л  \n",
	"ллл  л л л  ллл  л      л  л л л   л л   ллл  л л  ллл  ллл   ллл\n",
	"л л  л  лл    л  л      л  л  лл   л л   л л  л л  л    л  л    л\n",
	"л л  л   л  ллл  л      л  л   л    л    л л  лл   ллл  л  л  ллл\n",
		"\n",
		"Press a key to begin\n"
	};
/**/
	unsigned n, i;

	set_back_color(4);
	clear_screen();
	n = sizeof(l33t_banner) / sizeof(l33t_banner[0]);
	for(i = 0; i < n; i++)
	{
		set_fore_color((COLOR_WHITE + 1) - n + i);
		center_string(i + 1, l33t_banner[i]);
	}
	fflush(stdout);
	getch();
	g_score = 0;
	g_defender_x = g_scn_wd / 2;
}
/*****************************************************************************
*****************************************************************************/
int main(void)
{
	unsigned timeout, key, targets;

/* turn off keyboard input buffering */
//	(void)ioctl(0, 0, 1);
BEGIN_1:
	banner();
BEGIN_2:
/* clear screen and draw status line */
	clear_screen();
	move_csr(0, STATUS_Y);
	printf("Esc=quit  Space=fire  Left/Right Arrow=move");
	draw_defender(0);
	g_missile_y = 0;
/* (re)create fleet */
	memset(g_destroyed, 0, sizeof(g_destroyed));
	g_fleet_x = g_fleet_y = 0;
/* when this wave is entirely destroyed, start over with a fresh wave */
	targets = FLEET_WD * FLEET_HT;
/* until invader fleet lands... */
	while(g_fleet_y < FLEET_Y_GAME_OVER)
	{
/* move fleet every 100 milliseconds */
		for(timeout = DELAY; timeout != 0; timeout--)
		{
			delay(10);
/* move missile, if any */
			if(move_missile())
			{
				targets--;
				if(targets == 0)
					goto BEGIN_2;
			}
/* was a key pressed? */
			if(!kbhit())
				continue;
			key = getch();
/* Esc quits */
			if(key == 27)
			{
				set_back_color(COLOR_BLACK);
				set_fore_color(COLOR_WHITE);
				center_string(g_scn_ht / 2,
					" GO ON, HUMAN -- ");
				center_string(g_scn_ht / 2 + 1,
					" CRAWL BACK INTO YOUR CAVE ");
				goto END;
			}
/* left and right arrow keys move defender */
			else if(key == KEY_LEFT && (g_defender_x > 1))
				draw_defender(-1);
			else if(key == KEY_RIGHT && (g_defender_x < 78))
				draw_defender(+1);
/* spacebar fires missile, but only if no other missile in flight
(as indicated by g_missile_y == 0) */
			else if(key == ' ' && g_missile_y == 0)
			{
				g_missile_x = g_defender_x;
				g_missile_y = DEFENDER_Y - 1;
			}
		}
		move_fleet();
		draw_invaders();
	}
	set_back_color(COLOR_BLACK);
	set_fore_color(COLOR_WHITE);
	center_string(g_scn_ht / 2, " EARTH VANQUISHED! ");
	center_string(g_scn_ht / 2 + 1, " YOU LOSE,HUMAN! ");
END:
	fflush(stdout);
#if 1
	getch();
	goto BEGIN_1;
#endif
	return 0;
}
