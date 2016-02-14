#include <kernel/types.h>
#include <kernel/i386/i386.h>
#include <kernel/device.h>
#include <kernel/sync.h>
#include <kernel/proc.h>
#include <kernel/msg.h>
#include <kernel/vm.h>
#include <kernel/timer.h>
#include <kernel/dbg.h>
#include <kernel/error.h>
#include <kernel/kmalloc.h>
#include "console.h"




/*		
	Esc[n;n;nm   \033[n;n;nm
			
		1    Bold
		5    Blink
		7    Reverse Video
		
    Foreground colors
       30    Black
       31    Red
       32    Green
       33    Yellow
       34    Blue
       35    Magenta
       36    Cyan
       37    White
 
    Background colors
       40    Black
       41    Red
       42    Green
       43    Yellow
       44    Blue
       45    Magenta
       46    Cyan
       47    White
*/

uint8  ansi_color_conv [8] = 
	{ 0, 4, 2, 6, 1, 5, 3, 7}; 
	





/* Output a character, one at a time to the console display and
 * handle any special characters or escape sequences.
 */

void ConOutChar (struct Console *con, char ch)
{	
	int t, tab_spaces;

	if (con->esc_state != 0)
	{
		ParseEscape (con, ch);
		return;
	}
	

	switch (ch)
	{
		case 000:		/* NULL */
			return;
		
		case 007:		/* BELL */
			ConBell();
			return;
		
		case '\t':
			
			tab_spaces = (((con->column + 8) / 8) * 8) - con->column;
			for (t=0; t<tab_spaces; t++)
				ConPrintChar (con, ' ');
			return;
		
		case 013:		/* CTRL-K */
			ConMoveTo (con, con->column, con->row - 1);
			return;

		case 014:		/* CTRL-L */
			ConMoveTo (con, con->column + 1, con->row);
			return;

		case 016:		/* CTRL-N */
			ConMoveTo (con, con->column + 1, con->row);
			return;

		case '\b':		/* BACKSPACE */
			if (con->column == 0)
			{
				con->column = con->width -1;
				
				if (con->row > 0)
					con->row --;
				else
				{
					ConScrollScreen (con, SCROLL_DIR_DOWN);
				}

				ConMoveTo (con, con->column, con->row);
			}
			else
			{		
				ConMoveTo (con, con->column - 1, con->row);
			}

			return;


		case '\n':		/* LINE FEED */
			/* if (con->lf_to_crlf == TRUE) */
			
			ConMoveTo (con, 0, con->row);
				
			if (con->row == con->height-1)
				ConScrollScreen (con, SCROLL_DIR_UP);
			else
				con->row++;
	
			ConMoveTo (con, con->column, con->row);
			return;

		case '\r':		/* CARRIAGE RETURN */
			ConMoveTo (con, 0, con->row);
			return;
		
		case 033:		/* ESCAPE */
			con->esc_state = 1;
			return;
			
		default:		/* Printable char */
			ConPrintChar (con, ch);
			return;
	}
}




/* Determine first character in sequence, store numerical
 * parameters in array in console structure.  Call DoEscape()
 * when alphabetical command character found.
 *
 * The following ANSI escape sequences are currently supported.
 * If n and/or m are omitted, they default to 1.
 *   ESC [nA moves up n lines
 *   ESC [nB moves down n lines
 *   ESC [nC moves right n spaces
 *   ESC [nD moves left n spaces
 *   ESC [m;nH moves cursor to (m,n)
 *   ESC [J clears screen from cursor
 *   ESC [K clears line from cursor
 *   ESC [nL inserts n lines at cursor
 *   ESC [nM deletes n lines at cursor
 *   ESC [nP deletes n chars at cursor
 *   ESC [n@ inserts n chars at cursor
 *   ESC [nm enables rendition n (0=normal, 4=bold, 5=blinking, 7=reverse)
 *   ESC M scrolls the screen backwards if the cursor is on the top line
 */
 
void ParseEscape (struct Console *con, char ch)
{
	int t;
	
	switch (con->esc_state)
	{
		case 1: /* ESC seen */
		{
			con->esc_intro = '\0';
			con->esc_parm_idx = 0;
			
			for (t=0; t < MAX_ESC_PARMS; t++)
				con->esc_parmv[t] = 0;
				
			switch (ch)
			{
				case '[': /* Control Sequence Introducer */
				{
					con->esc_intro = '[';
					con->esc_state = 2; 
					return;
				}
				
				case 'M': /* Reverse Index */
				{
					DoEscape (con, ch);
					return;
				}	
				default: 
				{
					con->esc_state = 0;
					return;
				}
			}
		
			return;
		}
		
		case 2: /* ESC [ seen */
		{
			if (ch >= '0' && ch <= '9')
			{
				if (con->esc_parm_idx < MAX_ESC_PARMS)
					con->esc_parmv[con->esc_parm_idx] = con->esc_parmv[con->esc_parm_idx] * 10 + (ch - '0');
				return;
			}
			else if (ch == ';')
			{
				if (++con->esc_parm_idx < MAX_ESC_PARMS)
					con->esc_parmv[con->esc_parm_idx] = 0;
				return;
			}
			else
			{
				DoEscape (con, ch);
				return;
			}
		
			return;
		}
		
		default: /* illegal state */
		{
			con->esc_state = 0;
			return;
		}
	}
}




/* Check if first character in escape sequence was '[' or not.
 * Perform action of given command character.
 *
 *
 *
 *
 */

void DoEscape (struct Console *con, char ch)
{
	int n;
	int x,y;
	int t;
	
	if (con->esc_intro == '\0')
	{
		switch (ch)
		{
			case 'M':		/* Reverse Index */
			{
				if (con->row == 0)
					ConScrollScreen (con, SCROLL_DIR_DOWN);
				else
					con->row--;
		
				ConMoveTo (con, con->column, con->row);
				break;
			}
			
			default:
			{
				break;
			}
		}
	}
	else if (con->esc_intro == '[')
	{
		switch (ch)
		{
			/* Should these be bounded to the screen, not overflow ? */
		
		    case 'A': 		/* ESC [nA moves up n lines */
			{
				if (con->esc_parmv[0] == 0)
					con->esc_parmv[0] = 1;
				
				ConMoveTo (con, con->column, con->row - con->esc_parmv[0]);
				break;
			}
			
		   	case 'B':		/* ESC [nB moves down n lines */
			{
				if (con->esc_parmv[0] == 0)
					con->esc_parmv[0] = 1;

				ConMoveTo (con, con->column, con->row + con->esc_parmv[0]);
				break;
			}
			
		    case 'C':		/* ESC [nC moves right n spaces */
			{
				if (con->esc_parmv[0] == 0)
					con->esc_parmv[0] = 1;

				ConMoveTo (con, con->column + con->esc_parmv[0], con->row);
				break;
			}
			
		    case 'D':		/* ESC [nD moves left n spaces */
			{
				if (con->esc_parmv[0] == 0)
					con->esc_parmv[0] = 1;

				ConMoveTo (con, con->column - con->esc_parmv[0], con->row);
				break;
			}
			
		    case 'H':		/* ESC [m;nH" moves cursor to (m,n) */
			{
				x = con->esc_parmv[1] -1;
				y = con->esc_parmv[0] -1;
				
				if (x<0) x=0;
				if (x>79) x=79;
				
				if (y < 0) y=0;
				if (y > 24)	y=24;
				
				ConMoveTo (con, x, y);
				break;
			}
			
			
			
			/*
			 * In following cases, column (and row) may go beyond width or height.
			 * We need to use the minimum value of column or con->width-1;
			 *
			 * start/end x = con->column % con->width;
			 *
			 * or add if column < width tests
			 *
			 * Or always wrap line (but without line-feed)
			 */
			
			
			
			
		    case 'J':		/* ESC [nJ clears screen */
			{
				if (con->esc_parmv[0] == 0) /* Cursor to bottom-right */
				{
					int clr_cnt = 0;
									
					for (x = con->column; x < con->width; x++)
					{
						con->display_buffer[x][con->row] = ' ';
						con->attr_buffer[x][con->row] = con->current_attr;
						
						clr_cnt ++;
					}

											
					for (y = con->row+1; y < con->height; y++)
					{
						for (x = 0; x < con->width; x++)
						{
							con->display_buffer[x][y] = ' ';
							con->attr_buffer[x][y] = con->current_attr;
							
							clr_cnt ++;
						}
					}
					
					RefreshDisplay (con, 0, 0, con->width, con->height);
				}
				else if (con->esc_parmv[0] == 1) /* Top-Left to Cursor */
				{
					for (y=0; y < con->row; y++)
					{
						for (x = 0; x < con->width; x++)
						{
							con->display_buffer[x][y] = ' ';
							con->attr_buffer[x][y] = con->current_attr;
						}
					}
					
					for (x = 0; x < con->column; x++)
					{
						con->display_buffer[x][con->row] = ' ';
						con->attr_buffer[x][con->row] = con->current_attr;
					}
					
					RefreshDisplay (con, 0,0, con->column+1, con->row+1);
				}
				else if (con->esc_parmv[0] == 2) /* Entire screen */
				{
					for (y = 0; y < con->height; y++)
					{
						for (x = 0; x < con->width; x++)
						{
							con->display_buffer[x][con->row] = ' ';
							con->attr_buffer[x][con->row] = con->current_attr;
						}
					}
					
					RefreshDisplay (con, 0, 0, con->width, con->height);
				}
				
				break;
			}
			
		    case 'K':		/* ESC [K clear line from cursor */
			{
				if (con->esc_parmv[0] == 0)
				{
					for (x = con->column; x < con->width; x++)
					{
						con->display_buffer[x][con->row] = ' ';
						con->attr_buffer[x][con->row] = 0x07;
					}
					
					RefreshDisplay (con, 0, con->row, con->width, 1);
				}
				break;
			}
			
		    case 'L':		/* ESC [nL insert n lines at cursor */
			{
				n = con->esc_parmv[0];
				
				if (n < 1)
					 n = 1;
				
				if (n > (con->height - con->row)) 
					n = con->height - con->row;
				
				for (y = con->height-1; y >= con->row + n; y--)
				{
					for (x=0; x < con->width; x++)
					{
						con->display_buffer[x][y] = con->display_buffer[x][y-n];
						con->attr_buffer[x][y] = con->attr_buffer[x][y-n];
					}
				}
				
				for (y = con->row; y < con->row + n; y++)
				{
					for (x=0; x < con->width; x++)
					{
						con->display_buffer[x][y] = ' ';
						con->attr_buffer[x][y] = con->current_attr;
					}
				}

				RefreshDisplay (con, 0, 0, con->width, con->height);

				break;
			}
			
		    case 'M':		/* ESC [nM delete n lines at cursor */
			{
				n = con->esc_parmv[0];
				
				if (n < 1)
					 n = 1;
				
				if (n > (con->height-1 - con->row)) 
					n = con->height-1 - con->row;
				
				for (y=con->row; y < con->height - n; y++)
				{
					for (x=0; x < con->width; x++)
					{
						con->display_buffer[x][y] = con->display_buffer[x][y+n];
						con->attr_buffer[x][y] =  con->attr_buffer[x][y+n];
					}
				}
				
				for (y = con->height - n - 1; y < con->height; y++)
				{
					for (x=0; x < con->width; x++)
					{
						con->display_buffer[x][y] = ' ';
						con->attr_buffer[x][y] = con->current_attr;
					}
				}
				
				RefreshDisplay (con, 0, 0, con->width, con->height);
				
				break;
			}
			
			
			
		    case 'P':		/* ESC [nP delete n chars at cursor */
			{
				n = con->esc_parmv[0];

				if (n < 1)
					 n = 1;
				
				if (n > (con->width - con->column)) 
					n = con->width - con->column;
				
				if (n == 0 || n > con->width)
					break;

				
				for (x = con->column; x < con->width; x++)
				{
					con->display_buffer[x][con->row] = con->display_buffer[x+n][con->row];
					con->attr_buffer[x][con->row] = con->attr_buffer[x+n][con->row];
				}
				
				for (x=con->width - n; x < con->width; x++)
				{
					con->display_buffer[x][con->row] = ' ';
					con->attr_buffer[x][con->row] =  0x07;
				}
				
				
				RefreshDisplay (con, 0, con->row, con->width, 1);
				break;
			}
			
		    case '@':  		/* ESC [n@ insert n chars at cursor */
			{
				n = con->esc_parmv[0];
				if (n < 1)
					n = 1;

				if (n > (con->width - con->column)) 
					n = con->width - con->column;
				
				if (n == 0 || n > con->width)
					break;

				
				for (x = con->width -1; x >= con->column + n; x--)
				{
					con->display_buffer[x][con->row] = con->display_buffer[x-n][con->row];
					con->attr_buffer[x][con->row] = con->attr_buffer[x-n][con->row];
				}
				
					
				for (x = con->column; x < con->column + n; x++)
				{
					con->display_buffer[x][con->row] = ' ';
					con->attr_buffer[x][con->row] =  con->current_attr;
				}
				
				
				RefreshDisplay (con, 0, con->row, con->width, 1);
				break;
			}
			
			
			
	   	    case 'm':		/* ESC [n;n;nm enables rendition n */
		 	{
				for (t=0; t<= con->esc_parm_idx; t++)
				{
					if (con->esc_parmv[t] == 0)  /* Attrs off */
					{
						con->bold = FALSE;
						con->blink = FALSE;
						con->reverse = FALSE;
						con->fg_color = 7;
						con->bg_color = 0;
					}
					else if (con->esc_parmv[t] == 1)  /* Bold */  
						con->bold = TRUE;
					else if (con->esc_parmv[t] == 5)  /* Blink */
						con->blink = TRUE;
					else if (con->esc_parmv[t] == 7)  /* Reverse Video */
						con->reverse = TRUE;
					else if (con->esc_parmv[t] >= 30 && con->esc_parmv[t] <= 37)
						con->fg_color = con->esc_parmv[t] - 30;
					else if (con->esc_parmv[t] >= 40 && con->esc_parmv[t] <= 47)
						con->bg_color = con->esc_parmv[t] - 40;
				}
				
				
				if (con->reverse == FALSE)
					con->current_attr = ansi_color_conv[con->fg_color]
										 + (ansi_color_conv[con->bg_color] << 4);
				else
					con->current_attr = ansi_color_conv[con->bg_color]
										 + (ansi_color_conv[con->fg_color] << 4);
					
				if (con->bold == TRUE)
					con->current_attr |= (1<<3);
				
				if (con->blink == TRUE)
					con->current_attr |= (1<<7);

			 	break;
			}
			
	   	    default:
			{
				KPRINTF ("Esc[ unknown");
				break;
			}
		}
	}
	
	con->esc_state = 0;
}




/* Change current cursor position.
 */

void ConMoveTo (struct Console *con, int x, int y)
{
	if (x < 0)
		x = 0;
	
	if (x > con->width)
		con->column = con->width;
	else
		con->column = x;
	
	if (y < 0)
		y = 0;
	
	if (y > con->height)
		con->row = con->height;
	else
		con->row = y;

	RefreshCursor(con);
}




/* Scroll the console display, only scroll one line at a time.
 * Hmmm,  maybe we should have a linear buffer instead.
 *
 * Can 
 */

void ConScrollScreen (struct Console *con, int direction)
{
	int x,y;
	
	if (direction == SCROLL_DIR_DOWN)
	{
		for (y = con->height-1; y > 0; y--)
		{
			for (x = con->width-1; x >= 0; x--)
			{
				con->display_buffer[x][y] = con->display_buffer[x][y-1];
				con->attr_buffer[x][y] = con->attr_buffer[x][y-1];
			}
		}
		
		for (x=0; x < con->width; x++)
		{
			con->display_buffer[x][0] = ' ';
			con->attr_buffer[x][0] = 0x07;
		}
	}
	else if (direction == SCROLL_DIR_UP)
	{
		for (y = 0; y < con->height-1; y++)
		{
			for (x = 0; x < con->width; x++)
			{
				con->display_buffer[x][y] = con->display_buffer[x][y+1];
				con->attr_buffer[x][y] = con->attr_buffer[x][y+1];
			}
		}
		
		for (x=0; x < con->width; x++)
		{
			con->display_buffer[x][con->height-1] = ' ';
			con->attr_buffer[x][con->height-1] = 0x07;
		}
	}
	
	RefreshDisplay (con, 0,0, con->width, con->height);
}




/*
 *
 */

void ConBell (void)
{
}




/*
 * Need to refresh display
 */

void ConPrintChar (struct Console *con, char ch)
{	
	if (con->column < con->width && con->row < con->height)
	{
		con->display_buffer[con->column][con->row] = ch;
		con->attr_buffer[con->column][con->row] = con->current_attr;

		RefreshChar (con, con->column, con->row);
		con->column++;
	}
	
	
	if (con->column >= con->width)
	{
		con->column = 0;
		
		if (con->row == con->height-1)
			ConScrollScreen (con, SCROLL_DIR_UP);
		else
		{
			con->row ++;
		}
	}
	
	RefreshCursor(con);
}




/*
 *
 */
 
void RefreshDisplay (struct Console *con, int ox, int oy, int width, int height)
{
	int x, y;
	char *gfx = (char *)0xb8000;
	
	if (con == current_console)
	{
		for (y = oy; y < oy + height; y ++)
		{
			for (x = ox; x < ox + width; x++)
			{
				*(gfx + x*2 + y*160) = con->display_buffer[x][y];
				*(gfx + x*2 + y*160 +1) = con->attr_buffer[x][y];
			}
		}
	}
}


void RefreshChar (struct Console *con, int ox, int oy)
{
	char *gfx = (char *)0xb8000;

		
	if (con == current_console)
	{
		*(gfx + ox*2 + oy*160) = con->display_buffer[ox][oy];
		*(gfx + ox*2 + oy*160 +1) = con->attr_buffer[ox][oy];
	}
}




/*
 *
 */

void RefreshCursor (struct Console *con)
{
	uint32 location;
	
	if (con == current_console)
	{
		location = 80 * con->row + con->column;
		
		OutByte (0x3d4, 0x0e);
		OutByte (0x3d5, (uint8)(location>>8));
		
		OutByte (0x3d4, 0x0f);
		OutByte (0x3d5, (uint8)(location));	
	}
}



