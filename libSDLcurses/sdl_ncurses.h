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


#ifndef SDL_NCURSES_H_DEFINED
#define SDL_NCURSES_H_DEFINED

#include <stdarg.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <SDL_image.h>

/* XSI and SVr4 specify that curses implements 'bool'.  However, C++ may also
 * implement it.  If so, we must use the C++ compiler's type to avoid conflict
 * with other interfaces.
 *
 * A further complication is that <stdbool.h> may declare 'bool' to be a
 * different type, such as an enum which is not necessarily compatible with
 * C++.  If we have <stdbool.h>, make 'bool' a macro, so users may #undef it.
 * Otherwise, let it remain a typedef to avoid conflicts with other #define's.
 * In either case, make a typedef for NCURSES_BOOL which can be used if needed
 * from either C or C++.
 */

#undef TRUE
#define TRUE    1

#undef FALSE
#define FALSE   0

typedef unsigned char NCURSES_BOOL;

#if defined(__cplusplus)	/* __cplusplus, etc. */

/* use the C++ compiler's bool type */
#define NCURSES_BOOL bool

#else			/* c89, c99, etc. */

#if 1
#include <stdbool.h>
/* use whatever the C compiler decides bool really is */
#define NCURSES_BOOL bool
#else
/* there is no predefined bool - use our own */
#undef bool
#define bool NCURSES_BOOL
#endif

#endif /* !__cplusplus, etc. */

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

/**
 * these defines are set up for a 80x25 text terminal  window 
 * the adjust window size to accomodate the font size.
 */

#define SCREEN_CHAR_WIDTH (80)
#define SCREEN_CHAR_HEIGHT (25)



#undef	ERR
#define ERR     (-1)

#undef	OK
#define OK      (0)

/*
  The following video attributes, defined in <curses.h>, can be
  passed to the routines attron, attroff, and attrset, or OR'ed
  with the characters passed to addch.
*/

#define A_NORMAL     0   /*     Normal display (no highlight)  */
#define COLOR_PAIR(n)  ((n) << 8)
#define PAIR_NUMBER(n) ((n >> 8) &0xFF)
#define COLOR_PAIRS (128)
#define COLORS (256)

#define COLOR_BLACK 0
#define COLOR_RED 1
#define COLOR_GREEN 2
#define COLOR_YELLOW 3
#define COLOR_BLUE 4
#define COLOR_MAGENTA 5
#define COLOR_CYAN 6
#define COLOR_WHITE 7




/** maps from a colour pair index to a foreground colour
 ** USAGE FG(PAIR_NUMBER(c)) */

#define FG(n) (color_pairs[( n & (COLOR_PAIRS-1) ) * 2 + 1])
/** maps from a colour pair index to a background colour */
#define BG(n) (color_pairs[( n & (COLOR_PAIRS-1) ) * 2])

/* not going to support all of these */
#define A_BLINK      (1 << 24)
#define A_PROTECT    (2 << 24)
#define A_INVIS      (3 << 24)
#define A_ALTCHARSET (4 << 24)
#define A_CHARTEXT   (5 << 24)
#define A_BOLD       (6 << 24)
#define A_STANDOUT   (7 << 24)   /*    Best highlighting mode of the terminal. */
#define A_UNDERLINE  (8 << 24)
#define A_REVERSE    (9 << 24)
#define A_DIM        (10 << 24)

#define UNDERLINE(n) ((n) & A_UNDERLINE)
#define REVERSE(n) ((n) & A_REVERSE)
#define DIM(n) ((n) & A_DIM)
#define STANDOUT(n) ((n) & A_STANDOUT)
#define BOLD(n) ((n) & A_BOLD)

#define KEY_DOWN	SDLK_DOWN	/* down-arrow key */
#define KEY_UP		SDLK_UP		/* up-arrow key */
#define KEY_LEFT	SDLK_LEFT	/* left-arrow key */
#define KEY_RIGHT	SDLK_RIGHT	/* right-arrow key */
#define KEY_HOME	SDLK_HOME	/* home key */
#define KEY_F0		((SDLK_F1)-1)	/* Function keys.  Space for 64 */
#define KEY_F(n)	(KEY_F0+(n))	/* Value of function key n */
#define KEY_NPAGE	SDLK_PAGEDOWN	/* next-page key */
#define KEY_PPAGE	SDLK_PAGEUP	/* previous-page key */
#define KEY_LL		SDLK_KP1	/* lower-left key (home down) */
#define KEY_A1		SDLK_KP7	/* upper left of keypad */
#define KEY_A2          SDLK_KP8
#define KEY_A3		SDLK_KP9	/* upper right of keypad */
#define KEY_B1          SDLK_KP4
#define KEY_B2		SDLK_KP5	/* center of keypad */
#define KEY_B3          SDLK_KP6
#define KEY_C1		SDLK_KP1	/* lower left of keypad */
#define KEY_C2          SDLK_KP2
#define KEY_C3		SDLK_KP3	/* lower right of keypad */
#define KEY_END		SDLK_END	/* end key */
#define KEY_IC         SDLK_INSERT
#define KEY_EIC        SDLK_DELETE

#define MAX_INPUT_PENDING (256)
  
/* type defs */

   typedef unsigned int chtype;

   typedef unsigned int attr_t;		

   typedef struct s_Window 
   {
	 int x,y, width,height;
	 int cx,cy;
	 char *text;
	 attr_t *attrib;
	 bool delay;
	 bool keypad_on;
	 attr_t attributes;

   } WINDOW;


/*
 *  globals
 */
   extern WINDOW *stdscr;
   extern WINDOW *curscr;
   extern unsigned short LINES;
   extern unsigned short COLS;
   extern bool echo_on;
   extern bool cbreak_on;
   extern int pop_index;
   extern int push_index;
   extern char inbuffer[MAX_INPUT_PENDING];
   extern unsigned int screen_width;
   extern unsigned int screen_height;
   extern unsigned int display_char_height;
   extern unsigned int display_char_width;

  
   /*
     initscr is normally the first curses routine to call when
     initializing a program.
   */
   WINDOW *initscr(void);
  
   /*
     A program should always call endwin before exiting or escaping from
     curses mode temporarily.  This routine restores tty modes, moves the
     cursor to the lower left-hand corner of the screen and resets the
     ter- minal into the proper non-visual mode.  Calling refresh or
     doupdate after a temporary escape causes the program to resume
     visual mode.
   */

   int endwin(void);

/*
  The getyx macro places the current cursor position of the given
  window in the two integer variables y and x.
*/
#define getyx(w,y,x) { (x) = (w)->cx; (y) = (w)->cy; }

/*
  Like  getyx,  the getbegyx and getmaxyx macros store the current begin-
  ning coordinates and size of the specified window.
*/
#define getbegyx(w,y,x) { (x) = (w)->x; (y) = (w)->y; }
#define getmaxyx(w,y,x) { (x) = (w)->width; (y) = (w)->height; }

/*
  The touchwin and touchline routines throw away all optimization
  infor- mation about which parts of the window have been touched, by
  pretending that the entire window has been drawn on.
*/

   int touchwin(WINDOW *win);

/*
  Calling newwin creates and returns a pointer to a new window with
  the given number of lines and columns.  The upper left-hand corner
  of the window is at line begin_y, column begin_x.  If either nlines
  or ncols is zero, they default to LINES - begin_y and COLS -
  begin_x.  A new full-screen window is created by calling
  newwin(0,0,0,0).
*/
   WINDOW *newwin(int nlines, int ncols, int begin_y,  int begin_x);

/*
  Calling delwin deletes the named window, freeing all memory
  associated with it (it does not actually erase the window's
  screen image).  Sub- windows must be deleted before the main
  window can be deleted.
*/
   int delwin(WINDOW *win);

/*
  A program should always call endwin before exiting or escaping
  from curses mode temporarily.  This routine restores tty modes,
  moves the cursor to the lower left-hand corner of the screen
  and resets the terminal into the proper non-visual mode.
  Calling refresh or doupdate after a temporary escape causes the
  program to resume visual mode.
*/

   int endwin(void);

/*
  Calling mvwin moves the window so that the upper left-hand corner is
  at position (x, y).  If the move would cause the window to be off
  the screen, it is an error and the window is not moved.  Moving
  subwindows is allowed, but should be avoided.
*/
   int mvwin(WINDOW *win, int y, int x);

/*
  The refresh and wrefresh routines (or wnoutrefresh and doupdate)
  must be called to get actual output to the terminal, as other
  routines merely manipulate data structures.  The routine wrefresh
  copies the named window to the physical terminal screen, taking into
  account what is already there to do optimizations.  The refresh
  routine is the same, using stdscr as the default window.
*/
   int refresh(void);
   int wrefresh(WINDOW *win);

/*
  These routines move the cursor associated with the window to line y
  and column x.  This routine does not move the physical cursor of the
  termi- nal until refresh is called.  The position specified is
  relative to the upper left-hand corner of the window, which is
  (0,0).
*/
   int move(int y, int x);
   int wmove(WINDOW *win, int y, int x);


/*
  The getch, wgetch, mvgetch and mvwgetch, routines read a character
  from the window.  In no-delay mode, if no input is waiting, the
  value ERR is returned.  In delay mode, the program waits until the
  system passes text through to the program.  Depending on the
  setting of cbreak, this is after one character (cbreak mode), or
  after the first newline (nocbreak mode).  In half-delay mode, the
  program waits until a charac- ter is typed or the specified
  timeout has been reached.
*/
   int getch(void);
   int wgetch(WINDOW *win);
   int mvgetch(int y, int x);
   int mvwgetch(WINDOW *win, int y, int x);

/*
  The addch, waddch, mvaddch and mvwaddch routines put the character
  ch into the given window at its current window position, which is
  then advanced.
*/

   int addch(const chtype ch);
   int waddch(WINDOW *win, chtype ch);
   int mvaddch(int y, int x, chtype ch);
   int mvwaddch(WINDOW *win, int y, int x, chtype ch);
/*
  These  routines write the characters of the (null-terminated) character
  string str on the given window.  It is similar to calling  waddch  once
  for each character in the string.  The four routines with n as the last
  argument write at most n characters.  If  n  is  -1,  then  the  entire
  string  will be added, up to the maximum number of characters that will
  fit on the line, or until a terminating null is reached.
*/
   int addstr(const char *str);
   int addnstr(const char *str, int n);
   int waddstr(WINDOW *win, const char *str);
   int waddnstr(WINDOW *win, const char *str, int n);
   int mvaddstr(int y, int x, const char *str);
   int mvaddnstr(int y, int x, const char *str, int n);
   int mvwaddstr(WINDOW *win, int y, int x, const char *str);
   int mvwaddnstr(WINDOW *win, int y, int x, const char *str, int n);


/*
  The printw, wprintw, mvprintw and mvwprintw routines are  analogous  to
  printf  [see printf(3)].  In effect, the string that would be output by
  printf is output instead as though waddstr were used on the given  win-
  dow.
*/

   int printw(const char *fmt, ...);
   int wprintw(WINDOW *win, const char *fmt, ...);
   int mvprintw(int y, int x, const char *fmt, ...);
   int mvwprintw(WINDOW *win, int y, int x, const char *fmt, ...);
   int vwprintw(WINDOW *win, const char *fmt, va_list varglist);
   int vw_printw(WINDOW *win, const char *fmt, va_list varglist);


   int cbreak(void);
   int crmode(void);
   int nocbreak(void);

/*
  The echo and noecho routines control whether characters  typed  by  the
  user  are echoed by getch as they are typed.  Echoing by the tty driver
  is always disabled, but initially getch is in echo mode, so  characters
  typed  are  echoed. 
*/
   int echo(void);
   int noecho(void);
   int nodelay(WINDOW *win, bool bf);

   int standend(void);
   int wstandend(WINDOW *win);
   int standout(void);
   int wstandout(WINDOW *win);

/*
  The erase and werase routines copy blanks to every position in
  the win- dow, clearing the screen.
  
  The clear and wclear routines are like erase and werase, but
  they also call clearok, so that the screen is cleared
  completely on the next call to wrefresh for that window and
  repainted from scratch.
*/
   int erase(void);
   int werase(WINDOW *win);
   int clear(void);
   int wclear(WINDOW *win);
   int clearok(WINDOW *win, bool bf);


/*
  To use these routines start_color must  be  called
*/
   int start_color(void);

/*
  A programmer initializes a color-pair with the routine init_pair.
  After it has been initialized, COLOR_PAIR(n), a macro defined in
  <curses.h>, can be used as a new video attribute.
*/
   int init_pair(short pair, short f, short b);

/*
  If a terminal is capable of redefining colors, the programmer  can  use
  the  routine  init_color to change the definition of a color.
*/
   int init_color(short color, short r, short g, short b);

/*
  The routines has_colors and can_change_color return TRUE or FALSE,
  depending on whether the terminal has color capabilities and whether
  the program mer can change the colors. */

   bool has_colors(void);
   bool can_change_color(void);

/*
  The routine color_content allows a programmer to extract the amounts
  of red, green, and blue components in an initialized color.
*/
   int color_content(short color, short *r, short *g, short *b);

/*
  The routine pair_content allows a programmer to find out how a given
  color-pair is currently defined.
*/
   int pair_content(short pair, short *f, short *b);


/*
  The routine attron turns on
  the named attributes without affecting any others. */
   int attroff(int attrs);
   int wattroff(WINDOW *win, int attrs);
/*
  The routine attroff turns  off  the  named  attributes  without
  turning  any  other  attributes on or off.
*/
   int attron(int attrs);
   int wattron(WINDOW *win, int attrs);
/*
  The routine attrset sets the current attributes of the given
  window to attrs. */
   int attrset(int attrs);
   int wattrset(WINDOW *win, int attrs);

/*
  The routine color_set sets the current color of the given window to the
  foreground/background combination described by  the  color_pair_number.
  The parameter opts is reserved for future use, applications must supply
  a null pointer.
*/
   int color_set(short color_pair_number, void* opts);
   int wcolor_set(WINDOW *win, short color_pair_number,  void* opts);

/* The routine standend is the    same as attrset(A_NORMAL) or attrset(0), that  is,  it  turns  off  all     attributes. */
   int standend(void);
   int wstandend(WINDOW *win);

/*  The routine  stand  out  is  the  same  as attron(A_STANDOUT). */
   int standout(void);
   int wstandout(WINDOW *win);



#ifdef __cplusplus
}
#endif

#endif
