/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Interface implementations for the curses version of xconq. */

/* This file is rather simple, since there is no support for multiple */
/* displays, no flashy graphics, and very little optimization. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "global.h"
#include "side.h"
#include "map.h"
#undef bool          /* ugh */
#define beep beepGURK /* the sgi-machines (and AMIX) seem to need this gec*/
#include <curses.h>
#undef beep


#ifdef UNIX
#include <signal.h>   /* needed for ^C handling */
#endif /* UNIX */

#define INFOLINES 5

int helpwinlines = 1;
char *routine_executing = "Program Start";

/* Positions and sizes of the windows. */

int wx[20], wy[20], ww[20], wh[20];
int nextwin = 0;                /* number of next window to open */

bool alreadyopen = FALSE;       /* flag to prevent double opening */

/* Put in a default player, probably the invoker of the program. */
/* Nonempty host name not actually used, but needed to keep things */
/* straight. */

add_default_player()
{
    add_player(TRUE, "curses");
}

/* Decide what to do about ^C interrupts. */

#ifdef UNIX

stop_handler()
{
    if (numhumans == 0 || Debug) {
	close_displays();
	exit(1);
    }
}

#endif /* UNIX */

#if 0

/* Function to print debugging info when a segmentation violation or */
/* bus error occurs. */

void program_crash(sig, code, scp, addr)
int sig, code;
struct sigcontext *scp;
char *addr;     
{
  int i;
  static bool already_been_here = FALSE;

close_displays();  
  printf("Fatal error encountered. Signal %d code %d\n", sig, code);
  printf("Routine executing %s\n", routine_executing);
  printf("Procedure stack (incomplete): \n");
  for (i = 0; i <= procedure_stack_ptr; i++)
    printf(" %s\n", procedure_executing[i]);
  if (already_been_here) exit(0);
  else {
    already_been_here = TRUE;
    write_savefile(SAVEFILE2);
    exit(1);
  }
}

#endif

void hang_up(sig, code, scp, addr)
int sig, code;
struct sigcontext *scp;
char *addr;     
{
  static bool already_been_here = FALSE;

  close_displays();  
  if (already_been_here) exit(1);
  else {
    already_been_here = TRUE;
    write_savefile("emergency.save.xconq");
    exit(1);
  }
}

/* Ignore ^C if humans in the game, do it otherwise, including when the */
/* last human player turns into a machine (this is called by option cmd). */
/* Attempts to be more clever seem to be bad news. */

init_sighandlers()
{
#ifdef UNIX
    if (numhumans > 0 && !Debug) {
	signal(SIGINT, SIG_IGN);
    }
#if 0
    else {
/*	signal(SIGINT, SIG_DFL);*/
      signal(SIGINT, program_crash);
    }
    signal(1, program_crash);
    signal(SIGINT, program_crash);
#endif
    signal(SIGHUP, hang_up);
#if 0
    signal(SIGBUS, program_crash);
    signal(SIGSEGV, program_crash);
    signal(SIGFPE, program_crash);
    signal(SIGILL, program_crash);
    signal(SIGSYS, program_crash);
    signal(SIGINT, program_crash);
    signal(SIGQUIT, program_crash);
    signal(SIGTERM, program_crash);
#endif
#endif /* UNIX */
}

/* "Opening" a curses "display" just involves a standard sequence of calls. */
/* Unfortunately, the "standard sequence" varies from system to system... */

open_display(side)
Side *side;
{
    if (!alreadyopen) {
	initscr();
	nonl();
	noecho();
#ifdef USECRMODE
	crmode();
#else
	cbreak();
#endif /* USECRMODE */
	clear();
	side->display = 1L;
	alreadyopen = TRUE;
	return TRUE;
    } else {
	/* if we got here, we're in big trouble, so clean up quickly */
	clear();
	refresh();
	endwin();
        printf("Can't open a second display!\n");
	return FALSE;
    }
}

/* A predicate that tests whether our display can safely be written to. */
/* This is permitted to do side-effects, although curses doesn't need */
/* anything special. */

active_display(side)
Side *side;
{
    return (side != NULL && side->host && !side->lost && side->display);
}

/* Display will use every character position we can get our hands on. */

display_width(side) Side *side; {  return COLS;  }

display_height(side) Side *side; {  return LINES;  }

/* Displaying the world map usually requires a large bitmap display. */

world_display(side) Side *side; {  return FALSE;  }

/* Most sizes of things are 1 (i.e. one character cell). */

init_misc(side)
Side *side;
{
    side->fw = side->fh = side->hh = side->hch = side->uw = side->uh = 1;
    side->hw = 2;
    side->bd = side->margin = 0;
}

/* The "root window" covers our entire display.  In theory, the size of the */
/* screen could exceed what xconq needs, but this is wildly improbable. */

create_main_window(side)
Side *side;
{
    create_window(side, 0, 0, COLS, LINES);
}

/* Subwindow creator. */

create_window(side, x, y, w, h)
Side *side;
int x, y, w, h;
{
    if (x + w > COLS) w = COLS - x;
    if (y + h > LINES) h = LINES - y;
    wx[nextwin] = x;  wy[nextwin] = y;
    ww[nextwin] = w;  wh[nextwin] = h;
    return (nextwin++);
}

/* Help window has to be larger than most terminals allow, so blow it off. */

create_help_window(side) Side *side; {}

/* No special fixups to do. */

fixup_windows(side) Side *side; {}

enable_input(side) Side *side; {}

reset_misc(side) Side *side; {}

/* Moving a window just involves plugging in new values. */

change_window(side, win, x, y, w, h)
Side *side;
int win, x, y, w, h;
{
    if (x + w > COLS) w = COLS - x;
    if (y + h > LINES) h = LINES - y;
    if (x >= 0) {
	wx[win] = x;  wy[win] = y;
    }
    if (w >= 0) {
	ww[win] = w;  wh[win] = h;
    }
}

/* Actually, terminals are really "one-color", but don't take a chance on */
/* confusing the main program. */

display_colors(side) Side *side; {  return 2;  }

white_color(side) Side *side; {  return 1;  }

black_color(side) Side *side; {  return 0;  }

/* Can't actually honor any color requests... */

get_color_resources(side) Side *side; {return;}

request_color(side, name, class, fallback)
     Side *side;
     char *name, *class, *fallback;
{  return 0;  }

/* Only kind of input is keystrokes, and from only one "display" at that. */

get_input()
{
    char ch;
    Side *side, *curside;

/* quick hack to make this work in the multiplayer game. */
    for_all_sides(side)
      if (side->reqactive)
	curside = side;

    if (active_display(curside)) {
	draw_cursor(curside);
	ch = getch() & 0177;
	curside->reqtype = KEYBOARD;
	curside->reqch = ch;
	/* Program will not work without following due to compiler bug. */
	if (Debug)
	  printf("key is %c", ch); 
	  /*	notify(curside, "key is %c", ch);  */
	return TRUE;
    }
}

/* Can implement this, but need to look at input without forcing a 
   wait for characters. */
exit_robot_check()
{}

/* Wait for any input, don't care what it is. */

freeze_wait(side)
Side *side;
{
    refresh();
    getch();
}

/* Actually would be nice to do something reasonable here. */

flush_input(side) Side *side;  {
/*  flushinp(); */
  side->reqchange = TRUE;
}

/* Trivial abstraction - sometimes other routines like to ensure all output */
/* actually on the screen. */

flush_output(side) Side *side;  {  refresh();
				 side->reqchange = TRUE;}

/* General window clearing. */

clear_window(side, win)
Side *side;
int win;
{
    int i;

    if (wx[win] == 0 && wy[win] == 0 && ww[win] == COLS && wh[win] == LINES) {
	clear();
    } else {
	for (i = 0; i < ww[win]; ++i) tmpbuf[i] = ' ';
	tmpbuf[ww[win]] = '\0';
	for (i = 0; i < wh[win]; ++i) mvaddstr(wy[win]+i, wx[win], tmpbuf);
    }
}

/* No world display for curses. */

draw_bar(side, x, y, len, color) Side *side; int x, y, len, color; {}

invert_box(side, vcx, vcy) Side *side; int vcx, vcy; {}

/* This interfaces higher-level drawing decisions to the rendition of */
/* individual pieces of display.  Note that a display mode determines */
/* whether one or two terrain characters get drawn. */

draw_terrain_row(side, x, y, buf, len, color)
Side *side;
int x, y, len, color;
char *buf;
{
    int i, xi = x;

    for (i = 0; i < len; ++i) {
	if (xi >= wx[side->map] + ww[side->map] - 2) break;
	if (cur_at(side->map, xi, y)) {
	    addch(buf[i]);
	    addch((side->showmode == BORDERHEX ? ' ' : buf[i]));
	}
	xi += 2;
      }
}

/* Don't just position the cursor, but also clip and return the decision. */

cur_at(win, x, y)
int win, x, y;
{
    if (x < 0 || x >= ww[win] || y < 0 || y >= wh[win]) {
	return FALSE;
    } else {
	move(wy[win] + y, wx[win] + x);
	return TRUE;
    }
}

/* Curses is never flashy... */

flash_position(side, x, y, tm) Side *side; int x, y, tm; {}

/* Curses cursor drawing is quite easy! (but ineffective? - see input fns) */

draw_cursor_icon(side, x, y)
Side *side;
int x, y;
{
    if (humanside(side) || (global.time % 4) == 1) {
      cur_at(side->map, x, y);
      refresh();
    }
}

/* Doesn't seem to be necessary. */

draw_hex_icon(side, win, x, y, color, ch)
Side *side;
int win, x, y, color;
char ch;
{
}

/* Splash a unit character onto some window. */

draw_unit_icon(side, win, x, y, u, color)
Side *side;
int win;
int x, y, u, color;
{
    if (humanside(side) || (global.time % 4) == 1) {
      if (cur_at(win, x, y)) {
	addch(utypes[u].uchar);
	addch(' ');   /* inefficient, sigh */
      }
    }
}

/* Use the second position in a "hex" for identification of enemies. */

draw_side_number(side, win, x, y, n, color)
Side *side;
int win, x, y, n, color;
{
  if (humanside(side) || (global.time % 4) == 1) {
    if (cur_at(win, x+1, y)) addch((n == -1 || n == MAXSIDES) ? '`' : n + '0');
  }
}

/* Curses has enough trouble splatting stuff on the screen without doing */
/* little flashes too... */

draw_blast_icon(side, win, x, y, type, color)
Side *side;
int win, x, y, type, color;
{
}

/* Unfortunately, terminals usually can't flash their screens. */

invert_whole_map(side) Side *side; {}

/* Mushroom clouds don't come out real well either. */
/* This could be a little more elaborate. */

draw_mushroom(side, x, y, i)
Side *side;
int x, y, i;
{
    int sx, sy;

    xform(side, unwrap(side, x), y, &sx, &sy);
    if (cur_at(side->map, sx, sy)) {
	addstr("##");
	flush_output(side);
	if (i > 0) {
	    if (cur_at(side->map, sx-1, sy+1)) addstr("####");
	    if (cur_at(side->map, sx-2, sy)) addstr("######");
	    if (cur_at(side->map, sx-1, sy-1)) addstr("####");
	    flush_output(side);
	}
    }
}

/* Indicate that bar graphs are out of the question. */

bar_graphs(side) Side *side;  {  return FALSE;  }

/* Thus this routine can be empty. */

draw_graph(side, number, amount, total, title)
Side *side;
int number, amount, total;
{
}

/* Drawing text is easy, as long as we can ignore the color of it. */
/* Need to do manual clipping though. */

draw_text(side, win, x, y, str, color)
Side *side;
int win;
int x, y, color;
char *str;
{
    int i;

    if (cur_at(win, x, y)) {
	for (i = 0; i < strlen(str); ++i) {
	    if (x + i >= ww[win]) return;
	    addch(str[i]);
	}
    }
}

/* Can't draw lines, but this will substitute, sort of. */

draw_scratchout(side, pos)
Side *side;
int pos;
{
    int i;

    for (i = 0; i < 20; i += 2) {
	if (cur_at(side->sides, i, pos)) addch('-');
    }
}

/* Beep the beeper! */

beep(side)
Side *side;
{
    putchar('\007');
}

/* Most help info is printable also. */

reveal_help(side)
Side *side;
{
    notify(side, "Screen too small - writing into files instead.");
    do_printables(side, 0);
    return FALSE;
}

conceal_help(side) Side *side; {}

/* Shut a single display down - presumably only called once. */

close_display(side)
Side *side;
{
    clear();
    refresh();
    endwin();
}

set_retain(side, window)
Side *side;
long window;
{
/* Used only for X */
}


/* Put the current view into a file. */

dump_view(side)
Side *side;
{
    char ch1, ch2;
    int x, y, i, vs;
    viewdata view;
    Side *side2;
    FILE *fp;

    if ((fp = fopen(VIEWFILE, "w")) != NULL) {
	for (y = world.height-1; y >= 0; --y) {
	    for (i = 0; i < y; ++i) fputc(' ', fp);
	    for (x = 0; x < world.width; ++x) {
		view = side_view(side, x, y);
		if (view == UNSEEN) {
		    ch1 = ch2 = ' ';
		} else if (view == EMPTY) {
		    ch1 = ttypes[terrain_at(x, y)].tchar;
		    ch2 = (side->showmode == BORDERHEX ? ' ' : ch1);
		} else {
		    ch1 = utypes[vtype(view)].uchar;
		    vs = vside(view);
		    side2 = side_n(vside(view));
		    ch2 = (side2 ? ((side == side2) ? ' ' : vs + '0') : '`');
		}
		fputc(ch1, fp);
		fputc(ch2, fp);
	    }
	    fprintf(fp, "\n");
	}
	fclose(fp);
	notify(side, "Dumped current view to \"%s\".", VIEWFILE);
    } else {
	notify(side, "Can't open \"%s\"!!", VIEWFILE);
    }
}
