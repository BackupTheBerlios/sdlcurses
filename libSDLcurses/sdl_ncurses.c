/*

libSDLCurses - a curses compatible API that writes to an SDL
framebuffer 
Copyright (C) 2006 John Connors

This library is free software; you can redistribute it and/or modify
it under the terms of the GNU Lesser General Public License as
published by the Free Software Foundation; either version 2.1 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful, but
WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
USA

*/

#include <signal.h>
#include <malloc.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>
#include "sdl_ncurses.h"



/*
 * globals
 */
SDL_Surface *screen;
TTF_Font *g_term_font;
WINDOW *stdscr = NULL;
WINDOW *curscr = NULL;
bool echo_on = TRUE;
bool cbreak_on = FALSE;
int pop_index = 0;
int push_index = 0;
char inbuffer[MAX_INPUT_PENDING];
unsigned int screen_width;
unsigned int screen_height;
unsigned int display_char_height;
unsigned int display_char_width;

 
unsigned short LINES = SCREEN_CHAR_HEIGHT;
unsigned short COLS = SCREEN_CHAR_WIDTH;

SDL_Color color_pots[ COLORS ] = {
   {0, 0, 0, 0},    // black 0 
   {128, 0, 0, 0},    // red 1
   {0, 128, 0, 0},    // green 2
   {128, 128, 0, 0},    // yellow (or brown?) 3
   {0, 0, 128, 0},    // blue 4
   {128, 0, 128, 0},    // magenta 5
   {0, 128, 128, 0},    // cyan 6
   {160, 160, 160, 0},    // white - light grey 7
   {80, 80, 80, 0},    // dark grey 8
   {255, 0, 0, 0},     // bright red 9
   {0, 255, 0, 0},    // bright green 10
   {255, 255, 0, 0},    // bright yellow 11 
   {0, 0, 255, 0},    // bright blu 12
   {255, 0, 255, 0},    // bright magenta 13
   {0, 255, 255, 0},    // bright cyan 14
   {255, 255, 255, 0} // bright white 15
};

short color_pairs[COLOR_PAIRS * 2];


/***********************************
 *** Window Manipulation Routines ***
 ***********************************/

/*
  Calling newwin creates and returns a pointer to a new window
  with the given number of lines and columns.  The upper
  left-hand corner of the window is at line begin_y, column
  begin_x.  If either nlines or ncols is zero, they default to
  LINES - begin_y and COLS - begin_x.  A new full-screen window
  is created by calling newwin(0,0,0,0).  * Create a new window
  of the given dimensions at a given position
*/

WINDOW *newwin( int height, int  width, int ypos, int xpos )
{
   WINDOW * newwinptr;

   /* allocate buffer to hold window */
   newwinptr = malloc( sizeof( WINDOW ) );

   if ( newwinptr == NULL ) {
      return ( NULL );
   }

   /* set up data members of window */
   newwinptr->x = xpos;
   newwinptr->y = ypos;
   newwinptr->cx = 0;
   newwinptr->cy = 0;
   newwinptr->width = width;
   newwinptr->height = height;
   newwinptr->delay = TRUE;
   newwinptr->keypad_on = FALSE;

   /* allocate buffer to hold window text */
   newwinptr->text = malloc( width * height * sizeof(char) );
   if (newwinptr->text == NULL) {
      return NULL;
   }
   memset( newwinptr->text, 32, width * height * sizeof(char)  );

   /* allocate buffer to hold window attribures */
   newwinptr->attrib = malloc( width * height * sizeof(attr_t) );
   if (newwinptr->attrib == NULL)
   {
      free( newwinptr->text );
      return NULL;
   }	
   memset( newwinptr->attrib, 0, width * height * sizeof(attr_t) );
   newwinptr->attributes = 0;
   return newwinptr;
}

/**
   initscr is normally the first curses routine to call when
   initializing a program.  A few special routines sometimes need
   to be called before it; these are slk_init, filter, ripoffline,
   use_env. We do not implement these ;-)

*/
WINDOW *initscr(void)
{

   int minx, maxx;
   int miny, maxy;
   int advance;
   int x,y;

   SDL_Init( SDL_INIT_VIDEO );
   SDL_EnableKeyRepeat( 100, 10 );
   SDL_EnableUNICODE(1);
   TTF_Init();

   /* man is the measure of all things, and this is a monospace font,
    * so we dimension around '@' */
   g_term_font = TTF_OpenFont( "ASCII.fon", 12 );
   /* if this fires, you are probably missing a font file */
   assert( g_term_font != NULL );

   TTF_SizeText( g_term_font, "@", &display_char_width, &display_char_height);

   /* work out window size needed */
   screen_width = SCREEN_CHAR_WIDTH * display_char_width;
   screen_height = SCREEN_CHAR_HEIGHT * display_char_height;

   /* set it up */
   screen = SDL_SetVideoMode( screen_width, screen_height, 0, 0 );
   assert(screen != NULL);

   SDL_WM_SetCaption("Omega", NULL);

   
   /* ignore events we can't possibly use */
   SDL_EventState( SDL_IGNORE, SDL_MOUSEMOTION );
   SDL_EventState( SDL_IGNORE, SDL_JOYBALLMOTION );
   SDL_EventState( SDL_IGNORE, SDL_JOYHATMOTION );
   
   atexit( SDL_Quit );
   stdscr = newwin(screen_height, screen_width, 0, 0);
   curscr = stdscr;
   echo_on = TRUE;
   cbreak_on = FALSE;
   pop_index = 0;
   push_index = 0;
   memset(inbuffer, 0, MAX_INPUT_PENDING * sizeof(char));

   return stdscr;
}

/*
  Calling delwin deletes the named window, freeing all memory
  associated with it (it does not actually erase the window's
  screen image).  Subwindows must be deleted before the main
  window can be deleted.
*/

int delwin(WINDOW *win)
{
   if (win == NULL)
      return ERR;
   free( win->attrib );
   win->attrib = NULL;
   free( win->text );
   win->text = NULL;
   free( win );
   return OK;
}

/*
  A program should always call endwin before exiting or escaping
  from curses mode temporarily.  This routine restores tty modes,
  moves the cursor to the lower left-hand corner of the screen
  and resets the terminal into the proper non-visual mode.
  Calling refresh or doupdate after a temporary escape causes the
  program to resume visual mode.
*/

int endwin(void)
{
   /* we don't have any sensible way of doing this, so it always fails */
   return ERR;
}

/*
  Calling mvwin moves the window so that the upper left-hand
  corner is at position (x, y).  If the move would cause the
  window to be off the screen, it is an error and the window is
  not moved.  Moving subwindows is allowed, but should be
  avoided.
*/
int mvwin(WINDOW *win, int y, int x)
{
   win->x = x;
   win->y = y;
   return OK;
}

/*
  The refresh and wrefresh routines (or wnoutrefresh and
  doupdate) must be called to get actual output to the terminal,
  as other routines merely manipulate data structures.  The
  routine wrefresh copies the named window to the physical
  terminal screen, taking into account what is already there to
  do optimizations.  The refresh routine is the same, using
  stdscr as the default window.  Unless leaveok has been enabled,
  the physical cursor of the terminal is left at the location of
  the cursor for that window.
*/
int wrefresh( WINDOW *win )
{
   // actually draw
   int xat, yat;
   int xscreen, yscreen;
   char string[ 1024 ];

   SDL_Color fg;
   SDL_Color bg;
   SDL_Rect rect;
   SDL_Surface *temp;
   attr_t last_attrib;
   attr_t attrib;
   int style;
   int string_index;
   int w,h;

   /* if COLS > 1023 then there is the possibility of overflow */
   assert(COLS <= 1023 );


   for ( yat = 0; yat < win->height; yat++ ) {

      yscreen = ( yat + win->y ) * display_char_height;
      rect.x = win->x * display_char_width;
      rect.y = yscreen;
      rect.w = 0;
      rect.h = display_char_height;
      xat = 0;


      while ( xat < win->width ) {

	 string_index = 0;
	 memset(string, 0, 1024);


	 do {
	    string[ string_index ] = *( win->text + xat + ( yat * win->width ) );
	    last_attrib =  *( win->attrib + xat + ( yat * win->width ) ); 
	    rect.w += display_char_width;
	    string_index++;
	    xat++;
	 } while (( xat < win->width ) && 
		  ( last_attrib == *( win->attrib + xat + ( yat * win->width ) ) )); 

/*	 string[ string_index ] = *( win->text + xat + ( yat * win->width ) );
	 last_attrib =  *( win->attrib + xat + ( yat * win->width ) ); 
	 rect.w = display_char_width;
*/
	 /* if our attributes change, or we get to the end of the line, render what we have */
	 if (!REVERSE( last_attrib )) { 
	 
	    
	    fg = color_pots[ FG( PAIR_NUMBER( last_attrib ) ) ];
	    bg = color_pots[ BG( PAIR_NUMBER( last_attrib ) ) ]; 
	    
	 } else {
	    
	    bg = color_pots[ FG( PAIR_NUMBER( last_attrib ) ) ];
	    fg = color_pots[ BG( PAIR_NUMBER( last_attrib ) ) ]; 
	 }
	 
	 
	 if (DIM(last_attrib)) {
	    fg.r = fg.r >> 1;	 fg.b = fg.b >> 1;	 fg.g = fg.g >> 1;
	    bg.r = bg.r >> 1;	 bg.b = bg.b >> 1;	 bg.g = bg.g >> 1;
	 }
	 
	 if (STANDOUT(last_attrib)) {
	    fg.r = fg.r << 1;	    fg.b = fg.b << 1;	    fg.g = fg.g << 1;
	    bg.r = bg.r << 1;	    bg.b = bg.b << 1;       bg.g = bg.g << 1;
	 }
	 
	 if (BOLD(last_attrib)) {
	    fg.r = (fg.r << 1) | 15;	    fg.b = (fg.b << 1) | 15;	    fg.g = (fg.g << 1) | 15;
	 }

	 style = TTF_STYLE_NORMAL;
	 style |= UNDERLINE(last_attrib) ? TTF_STYLE_UNDERLINE : 0;
	 TTF_SetFontStyle( g_term_font, TTF_STYLE_NORMAL );
	 

/*	 if ((string[0] != '\0') && (string[0] != ' '))
	 fprintf(stderr,"%c %x %x\t", string[0], string[0], PAIR_NUMBER( last_attrib ));*/

	 temp = TTF_RenderText_Shaded( g_term_font, string, fg, bg );
	 if (temp == NULL)
	 {
	    return ERR;
	 }

	 SDL_BlitSurface( temp, NULL, screen, &rect );
	 SDL_FreeSurface( temp );
	 
	 rect.x += rect.w; /* display_char_width; */
	 rect.w = 0;
	 /* xat++; */
      }
   }


   SDL_UpdateRect(screen, 0, 0, 0, 0); 
   return OK;
}

int wmove( WINDOW *win, int y, int x )
{
   if ((x < 0) || (y < 0) || (y >= win->width) || (x >= win->width))
      return ERR;
   win->cx = x;
   win->cy = y;
   return OK;
}

/*
  The addch, waddch, mvaddch and mvwaddch routines put the
  character ch into the given window at its current window
  position, which is then advanced.  They are analogous to
  putchar in stdio(3).  If the advance is at the right margin,
  the cursor automatically wraps to the beginning of the next
  line.  At the bottom of the current scrolling region, if
  scrollok is enabled, the scrolling region is scrolled up one
  line.

  If ch is a tab, newline, or backspace, the cursor is moved
  appropriately within the window.  Backspace moves the cursor
  one character left; at the left edge of a window it does
  nothing.  Newline does a clrtoeol, then moves the cursor to the
  window left margin on the next line, scrolling the window if on
  the last line.  Tabs are considered to be at every eighth
  column.  The tab interval may be altered by setting the TABSIZE
  variable.

*/

int waddch( WINDOW *win, chtype c )
{
   attr_t attrs = c & ~0xFF;

   if ( c == '\t' ) {
      win->cx += 4 - ( win->cx % 4 );
   }

   else {
      if ( c == '\r' ) {
	 win->cx = 0;
      }

      else {
	 if ( c == '\n' ) {
	    win->cx = 0;
	    win->cy++;
	 }

	 else {
	    if ( ( win->cx < win->width ) && ( win->cy < win->height ) ) {
	       *( win->text + win->cx + win->cy * win->width ) = c;
	       if (attrs == 0U)
		  *( win->attrib + win->cx + win->cy * win->width ) = win->attributes;
	       else
		  *( win->attrib + win->cx + win->cy * win->width ) = attrs;
	    }

	    ( win->cx ) ++;

	 }
      }
   }
   return OK;
}

int mvwaddch(WINDOW *win, int y, int x, const chtype ch)
{
   if (wmove(win,y,x) != OK)
      return ERR;
   return waddch(win, ch);
}


int waddnstr( WINDOW *win, const char *string, int n)
{
   const char * stringg;
   int count;

   if ((string == NULL) || (n == 0))
      return OK;

   for ( stringg = string, count = 0; 
	 ((*stringg != '\0') && (count != n)); 
	 stringg++, count++ ) {
      if (waddch(win, *stringg) == ERR)
	 return ERR;
   }
   return OK;
}


/*
  These routines write the characters of the (null-terminated)
  character string str on the given window.  It is similar to
  calling wad- dch once for each character in the string.  The
  four routines with n as the last argument write at most n
  characters.  If n is -1, then the entire string will be added,
  up to the maximum number of characters that will fit on the
  line, or until a terminating null is reached.
*/

int waddstr( WINDOW *win, const char *str)
{
   return waddnstr( win, str, -1 );
}

int mvwaddstr(WINDOW *win, int y, int x, const char *str)
{
   if (wmove(win, y, x) != OK)
      return ERR;
   return waddstr(win, str);
}
	
int mvwaddnstr(WINDOW *win, int y, int x, const char *str, int n)
{
   if (wmove(win, y, x) != OK)
      return ERR;
   return waddnstr(win, str, n);
}

int vw_printw(WINDOW *win,  const char *fmt, va_list arglist)
{
   static char buffer[2048];

   memset(buffer, 0, 2048 * sizeof(char));
   vsnprintf(buffer, 2047, fmt, arglist);
   waddstr(win, buffer);
   return OK;
}

int wprintw(WINDOW *win, const char *fmt, ...)
{
   va_list params;
   int result;

   va_start(params, fmt);
   result = vw_printw(win, fmt, params);
   va_end(params);
   return result;
}

int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...)
{
   va_list params;
   int result;

   if (wmove(win, y, x) != OK)
      return ERR;
   va_start(params, fmt);
   result = vw_printw(win, fmt, params);
   va_end(params);
   return result;

}

/*
  The getch, wgetch, mvgetch and mvwgetch, routines read a
  character from the window.  In no-delay mode, if no input is
  waiting, the value ERR is returned.  In delay mode, the program
  waits until the system passes text through to the program.
  Depending on the setting of cbreak, this is after one
  character (cbreak mode), or after the first newline (nocbreak
  mode).  In half-delay mode, the pro- gram waits until a
  character is typed or the specified timeout has been reached.
*/


int wgetch(WINDOW *win) 
{

   SDL_Event event;
   int retchar = ERR;
   int key_pressed = FALSE;
   static SDL_keysym last_keysym;
   

   /* fprintf(stderr, "wgetchar  delay %x cbreak %x pop %x push %x \n", win->delay, cbreak_on, pop_index, push_index); */
   if ((pop_index != push_index) && (cbreak_on == FALSE))
   {
      retchar = inbuffer[pop_index];
      pop_index = ( pop_index + 1 ) % MAX_INPUT_PENDING;
      key_pressed = TRUE;
   } else {
      pop_index = push_index;
   }


   while ( !key_pressed ) {


      if ((win->delay) || (cbreak == FALSE))
      {
	 while ( SDL_WaitEvent( &event ) == 0 ) {
	    SDL_PumpEvents();
	 };
      } else {
	 if (!SDL_PollEvent( &event ))
	    return ERR;
      }

      if ( ( event.type == SDL_KEYUP ) && ( event.key.keysym.scancode == last_keysym.scancode ) )
	 last_keysym.scancode = 0;

      if ( event.type == SDL_KEYDOWN )
      {
	 last_keysym = event.key.keysym;

	 /* fprintf(stderr, "wgetch event.key.keysym.sym %x\n", event.key.keysym.sym ); */
	 switch ( event.key.keysym.sym ) 
	 {
	    

	    case SDLK_UP: 
	    case SDLK_DOWN: 
	    case SDLK_LEFT: 
	    case SDLK_RIGHT: 
	    case SDLK_PAGEDOWN: 
	    case SDLK_PAGEUP: 
	    case SDLK_HOME: 
	    case SDLK_END: 
	    case SDLK_F1:
	    case SDLK_F2:
	    case SDLK_F3:
	    case SDLK_F4:
	    case SDLK_F5:
	    case SDLK_F6:
	    case SDLK_F7:
	    case SDLK_F8:
	    case SDLK_F9:
	    case SDLK_F11:
	    case SDLK_F12:
	    case SDLK_F13:
	    case SDLK_F14:
	    case SDLK_F15:
	    {
	       retchar = event.key.keysym.sym;
	       break;
	    }

	    case SDLK_KP0:
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '0';
	       }
	       break;

	    case SDLK_KP1:
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '1';
	       }
	       break;

	    case SDLK_KP3:
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '3';
	       }
	       break;

	    case SDLK_KP5:
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '5';
	       }
	       break;

	    case SDLK_KP7:
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '7';
	       }
	       break;

	    case SDLK_KP9:
	    {
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '9';
	       }
	       break;
	    }
	    
	    case SDLK_KP2:
	    {
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '2';
		  break;
	       }
	       if (win->keypad_on)
		  retchar = SDLK_DOWN;
	       break;
	    }
	    case SDLK_KP6:
	    {
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '6';
		  break;
	       } 
	       if (win->keypad_on) {
		  retchar = SDLK_RIGHT;
	       }
	       break;
	    }
	    case SDLK_KP4:
	    {
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '4'; 
		  break;
	       }
	       if (win->keypad_on)
		  retchar = SDLK_LEFT;
	       break;
	    }

	    case SDLK_KP8:
	    {
	       retchar = event.key.keysym.sym;
	       if (event.key.keysym.mod & KMOD_NUM) {
		  retchar = '8';
		  break;
	       }
	       if (win->keypad_on)
		  retchar = SDLK_UP;
	       break;
	    }

	    default:
	       if ((echo_on) && (event.key.keysym.unicode)) {
		  waddch(win, event.key.keysym.unicode);
		  wrefresh(win);
		  SDL_PumpEvents();
	       }
	       retchar = event.key.keysym.unicode;
	       break;

	 } /* switch */

	 if (cbreak_on == FALSE) {
	    if  (retchar != '\r') {
	       if (((push_index + 1) % MAX_INPUT_PENDING) != pop_index) {
		  inbuffer[push_index] = retchar;
		  push_index = ( push_index + 1 ) % MAX_INPUT_PENDING;
	       }		     
	    }  else {
	       key_pressed = TRUE;
	    }
	 } else {
	    key_pressed = TRUE;
	 }
	 break;
	 
      }

      if ( event.type == SDL_QUIT ) {
	 TTF_Quit();
	 SDL_Quit();
	 exit( 0 );
      }

      if (( !key_pressed ) && ( win->delay ))
	 SDL_PumpEvents();

      if ( !win->delay )
	 break;
   } /* while !key_pressed */

   /* bodge.. */
   if (retchar == '\r')
      retchar = '\n';

   /* fprintf(stderr, "wgetchar returning %x in delay %x cbreak %x\n", retchar, win->delay, cbreak_on); */
   return ( retchar );
}

int cbreak(void)
{
   cbreak_on = TRUE;
   return OK;
}

int nocbreak(void)
{
   cbreak_on = FALSE;
   return OK;
}

int crmode(void)
{
   return cbreak();
}

int nodelay(WINDOW *win, bool bf)
{
   win->delay = bf;
   return OK;
}

int keypad(WINDOW *win, bool bf)
{
   win->keypad_on = bf;
   return OK;
}

int mvwgetch(WINDOW *win, int y, int x)
{
   if (wmove(win, y, x) != OK)
      return ERR;
   return wgetch(win);
}

int refresh( void )
{
   return wrefresh(stdscr);
}

int move( int y, int x )
{
   return wmove(stdscr, y, x);
}

int getch(void)
{
   return wgetch(stdscr);
}

int mvgetch(int y, int x)
{
   if(move(x,y) != OK)
      return ERR;
   return getch();
}

int addch(const chtype ch)
{
   return waddch(stdscr, ch);
}

int mvaddch(int y, int x, const chtype ch)
{
   if (move(y,x) != OK)
      return ERR;
   return addch(ch);
}

int addstr(const char *str)
{
   return waddstr(stdscr, str);
}

int addnstr(const char *str, int n)
{
   return waddnstr(stdscr, str, n);
}

int mvaddstr(int y, int x, const char *str)
{
   return mvwaddstr(stdscr, y, x, str);
}

int mvaddnstr(int y, int x, const char *str, int n)
{
   return mvwaddnstr(stdscr, y, x, str, n);
}

int printw(const char *fmt, ...)
{
   va_list params;
   int result;

   va_start(params, fmt);
   result = vw_printw(stdscr, fmt, params);
   va_end(params);
   return result;

}

int mvprintw(int y, int x, const char *fmt, ...)
{
   va_list params;
   int result;

   if (wmove(stdscr, y, x) != OK)
      return ERR;
   va_start(params, fmt);
   result = vw_printw(stdscr, fmt, params);
   va_end(params);
   return result;

}

/*
       The start_color routine requires no arguments.  It must be
       called if the programmer wants to use colors, and before any
       other color manipulation routine is called.  It is good
       practice to call this routine right after initscr.  start_color
       initializes eight basic colors (black, red, green, yellow,
       blue, magenta, cyan, and white), and two global variables,
       COLORS and COLOR_PAIRS (respectively defining the maximum
       number of colors and color-pairs the terminal can support).  It
       also restores the colors on the terminal to the values they had
       when the terminal was just turned on.

       The init_pair routine changes the definition of a color-pair.
       It takes three arguments: the number of the color-pair to be
       changed, the foreground color number, and the background color
       number.  For portable applications:

       - The value of the first argument must be between 1 and
            COLOR_PAIRS-1.

       - The value of the second and third arguments must be between 0
            and COLORS (the 0 color pair is wired to white on black
            and cannot be changed).

       If the color-pair was previously initialized, the screen is
       refreshed and all occurrences of that color-pair are changed to
       the new definition.

       As an extension, ncurses allows you to set color pair 0 via the
       assume_default_colors routine, or to specify the use of default
       colors (color number -1) if you first invoke the
       use_default_colors routine.

       The init_color routine changes the definition of a color.  It
       takes four arguments: the number of the color to be changed
       followed by three RGB values (for the amounts of red, green,
       and blue components).  The value of the first argument must be
       between 0 and COLORS.  (See the section Colors for the default
       color index.)  Each of the last three arguments must be a value
       between 0 and 1000.  When init_color is used, all occurrences
       of that color on the screen immediately change to the new
       definition.

       The has_colors routine requires no arguments.  It returns TRUE
       if the terminal can manipulate colors; otherwise, it returns
       FALSE.  This routine facilitates writing terminal-independent
       programs.  For example, a programmer can use it to decide
       whether to use color or some other video attribute.

       The can_change_color routine requires no arguments.  It returns
       TRUE if the terminal supports colors and can change their
       definitions; other, it returns FALSE.  This routine facilitates
       writing terminal-independent programs.

       The color_content routine gives programmers a way to find the
       intensity of the red, green, and blue (RGB) components in a
       color.  It requires four arguments: the color number, and three
       addresses of shorts for storing the information about the
       amounts of red, green, and blue components in the given color.
       The value of the first argument must be between 0 and COLORS.
       The values that are stored at the addresses pointed to by the
       last three arguments are between 0 (no component) and 1000
       (maximum amount of component).

       The pair_content routine allows programmers to find out what
       colors a given color-pair consists of.  It requires three
       arguments: the color-pair number, and two addresses of shorts
       for storing the foreground and the background color numbers.
       The value of the first argument must be between 1 and
       COLOR_PAIRS-1.  The values that are stored at the addresses
       pointed to by the second and third argu- ments are between 0
       and COLORS.

*/

int start_color(void)
{
   color_pairs[0] = 0;  /* black */
   color_pairs[1] = 7;  /* white */
   color_pairs[2] = 0;  /* black */
   color_pairs[3] = 7;  /* white */

   return OK;
}

bool has_colors(void)
{
   return TRUE;
}

bool can_change_color(void)
{
   return TRUE;
}

int init_pair(short pair, short f, short b)
{
/*   fprintf(stderr, "Set pair %d to %x %x \n\r", pair, f, b); */
   if ((pair <= 0) || (pair >= COLOR_PAIRS))
      return ERR;
   color_pairs[pair * 2] = b;
   color_pairs[pair * 2 + 1] = f;
   return OK;
}

int pair_content(short pair, short *f, short *b)
{
   if ((pair <= 0) || (pair >= COLOR_PAIRS))
      return ERR;
   *f = color_pairs[pair * 2 + 1];
   *b = color_pairs[pair * 2];
   return OK;
}


int init_color(short color, short r, short g, short b)
{
   if ((color < 0) || (color >= COLORS))
      return ERR;
   color_pots[color].r = (r * 255) / 1000;
   color_pots[color].g = (g * 255) / 1000;
   color_pots[color].b = (b * 255) / 1000;
   return OK;
}

int color_content(short color, short *r, short *g, short *b)
{
   if ((color < 0) || (color >= COLORS))
      return ERR;
   *r = ((color_pots[color].r * 1000) / 255);
   *g = ((color_pots[color].g * 1000) / 255);
   *b = ((color_pots[color].b * 1000) / 255);
   return OK;
}

/*
       These routines manipulate the current attributes of the named
       window.  The current attributes of a window apply to all
       characters that are written into the window with waddch,
       waddstr and wprintw.  Attributes are a property of the
       character, and move with the character through any scrolling
       and insert/delete line/character operations.  To the extent
       possible, they are displayed as appropriate modifi- cations to
       the graphic rendition of characters put on the screen.

       The routine attrset sets the current attributes of the given
       window to attrs.  The routine attroff turns off the named
       attributes without turning any other attributes on or off.  The
       routine attron turns on the named attributes without affecting
       any others.  The routine standout is the same as
       attron(A_STANDOUT).  The routine standend is the same as
       attrset(A_NORMAL) or attrset(0), that is, it turns off all
       attributes.

       The attrset and related routines do not affect the attributes
       used when erasing portions of the window.  See curs_bkgd(3X)
       for func- tions which modify the attributes used for erasing
       and clearing.

       The routine color_set sets the current color of the given
       window to the foreground/background combination described by
       the color_pair_number. The parameter opts is reserved for
       future use, applications must supply a null pointer.

*/
int wattrset(WINDOW *win, int attrs)
{
   if (attrs < 0)
      return ERR;
   win->attributes = attrs;
   return OK;
}

int attrset(int attrs)
{
   return wattrset(stdscr, attrs);
}

int wattroff(WINDOW *win, int attrs)
{
   if (attrs < 0)
      return ERR;
   win->attributes = win->attributes & ~(attrs & 0xFFFF0000UL);
   return OK;	
}

int attroff(int attrs)
{
   return wattron(stdscr, attrs);
}

int wattron(WINDOW *win, int attrs)
{
   if (attrs < 0)
      return ERR;
   win->attributes = win->attributes | (attrs & 0xFFFF0000UL);
   return OK;	
}

int attron(int attrs)
{
   return wattron(stdscr, attrs);
}

int wstandend(WINDOW *win)
{
   return wattroff(win, A_STANDOUT);
}

int standend(void)
{
   return wstandend(stdscr);
}

int wstandout(WINDOW *win)
{
   return wattron(win, A_STANDOUT);
}

int standout(void)
{
   return wstandout(stdscr);
}

int wcolor_set(WINDOW *win, short color_pair_number, void *opts)
{
   win->attributes = (win->attributes & 0xFFFF00FFUL) | COLOR_PAIR(color_pair_number);
   return color_pair_number;
}

int color_set(short color_pair_number, void *opts)
{
   wcolor_set(stdscr, color_pair_number, NULL);
   return color_pair_number;
}

/**
 * misc 
 */
int touchwin(WINDOW *win)
{
   return OK;
}

int echo(void)
{
   echo_on = TRUE;
   return OK;
}


int noecho(void)
{
   echo_on = FALSE;
   return OK;
}

int clearok(WINDOW *win, bool bf)
{
   return OK;
}

int werase(WINDOW *win)
{
   win->cx = 0;
   win->cy = 0;
   memset( win->text, 32, win->width * win->height * sizeof(char) );
   memset( win->attrib, 0, win->width * win->height * sizeof(attr_t) );
   return OK;
}

int erase(void)
{
   return werase(stdscr);
}

int wclear(WINDOW *win)
{
   win->cx = 0;
   win->cy = 0;
   memset( win->text, 32, win->width * win->height * sizeof(char) );
   memset( win->attrib, 0, win->width * win->height * sizeof(attr_t) );
   return OK;
}

int clear()
{
   return wclear(stdscr);
}

int scrollok(WINDOW *win, bool bf)
{
   /* stub that does nothing */
   if (bf == FALSE)
      return OK;
   else
      return ERR; /* not yet implemented */
}
