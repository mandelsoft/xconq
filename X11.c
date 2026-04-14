/* Copyright (c) 1987, 1988, 1991  Stanley T. Shebs. */
/* Copyright 1988 by Chris D. Peterson, MIT. */
/* Many improvements by Tim Moore, University of Utah. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact.  */

/* Interface implementations for the X11 version of xconq. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

/* careful of the path of the X header files. */

#ifdef UNIX
#include <X11/Xos.h>
#undef SIGCHLD
#include <signal.h>   /* needed for ^C disabling */
#include <errno.h>    /* needed to detect interrupted selects */
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/Xresource.h>
#include <X11/Intrinsic.h>
#include <ctype.h>
/*** (UK) insert -> ***/
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

static int sock=-1; /* server socket */
/*** <- insert ***/ 
#endif /* UNIX */

#ifndef XAPPLRESDIR
#define XAPPLRESDIR "/local/lib/x11r4/app-defaults"
#endif

/* various bitmap definitions. */

#include "xconqlogo.icon"

#define dots_width 16
#define dots_height 16
static char dots_bits[] = {
   0x01, 0x01,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x10, 0x10,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x01, 0x01,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x10, 0x10,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00};

#define mask_width 16
#define mask_height 16
static char mask_bits[] = {
   0xe0, 0x03,  0xd8, 0x0f,  0xb4, 0x19,  0x8a, 0x21, 
   0x86, 0x61,  0x85, 0x41,  0x83, 0xc1,  0xff, 0xff, 
   0xff, 0xff,  0x83, 0xc1,  0x82, 0xa1,  0x86, 0x61, 
   0x84, 0x51,  0x98, 0x2d,  0xf0, 0x1b,  0xc0, 0x07};

#define curs_width 16
#define curs_height 16
static char curs_bits[] = {
   0xe0, 0x03,  0x98, 0x0c,  0x84, 0x10,  0x82, 0x20, 
   0x82, 0x20,  0x81, 0x40,  0x81, 0x40,  0xff, 0x7f, 
   0x81, 0x40,  0x81, 0x40,  0x82, 0x20,  0x82, 0x20, 
   0x84, 0x10,  0x98, 0x0c,  0xe0, 0x03,  0x00, 0x00};

#define bomb1_width 32
#define bomb1_height 32
static char bomb1_bits[] = {
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0xc0,  0x01, 0x00,  0x00, 0xf0,  0x07, 0x00, 
   0x00, 0xf0,  0x07, 0x00,  0x00, 0xf8,  0x0f, 0x00, 
   0x00, 0xf8,  0x0f, 0x00,  0x00, 0xfc,  0x0f, 0x00, 
   0x00, 0xfe,  0x1f, 0x00,  0x00, 0xfe,  0x3f, 0x00, 
   0x00, 0xfe,  0x3f, 0x00,  0x00, 0xfc,  0x1f, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00};

#define bomb2_width 32
#define bomb2_height 32
static char bomb2_bits[] = {
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0xf8,  0x01, 0x00, 
   0x00, 0xf8,  0x07, 0x00,  0x00, 0xfc,  0x0f, 0x00, 
   0x00, 0xfc,  0x1f, 0x00,  0x00, 0xfe,  0x1f, 0x00, 
   0x00, 0xfe,  0x3f, 0x00,  0x00, 0xfc,  0x1f, 0x00, 
   0x00, 0xfc,  0x0f, 0x00,  0x00, 0xf0,  0x07, 0x00, 
   0x00, 0xf0,  0x03, 0x00,  0x00, 0xe0,  0x03, 0x00, 
   0x00, 0xf0,  0x07, 0x00,  0x00, 0xf0,  0x07, 0x00, 
   0x00, 0xf0,  0x0f, 0x00,  0x00, 0xf0,  0x0f, 0x00, 
   0x00, 0xf8,  0x0f, 0x00,  0x00, 0xf8,  0x1f, 0x00, 
   0x00, 0xf8,  0x1f, 0x00,  0x00, 0xfc,  0x1f, 0x00, 
   0x00, 0xff,  0x7f, 0x00,  0xc0, 0xff,  0xff, 0x03, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00};

#define bomb3_width 32
#define bomb3_height 32
static char bomb3_bits[] = {
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0xfe, 0xe2,  0xa3, 0x3f, 
   0x00, 0xfc,  0x1f, 0x00,  0x3c, 0xff,  0x7f, 0x7c, 
   0x80, 0xff,  0xff, 0x00,  0xc0, 0xff,  0xff, 0x01, 
   0xc0, 0xff,  0xff, 0x01,  0xe0, 0xff,  0xff, 0x03, 
   0xe0, 0xff,  0xff, 0x03,  0xe0, 0xff,  0xff, 0x03, 
   0xe0, 0xff,  0xff, 0x01,  0xe0, 0xff,  0xff, 0x49, 
   0x82, 0xff,  0xff, 0x34,  0x14, 0xfe,  0x3f, 0x42, 
   0xe2, 0xff,  0x9f, 0x34,  0x40, 0xfe,  0x3f, 0x41, 
   0xbe, 0xfd,  0xdf, 0x1e,  0x00, 0xf8,  0x1f, 0x01, 
   0xfe, 0xfd,  0xdf, 0x7f,  0x00, 0xfc,  0x1f, 0x00, 
   0x00, 0xfc,  0x1f, 0x00,  0x00, 0xfc,  0x3f, 0x00, 
   0x00, 0xfe,  0x1f, 0x00,  0x00, 0xfe,  0x3f, 0x00, 
   0x00, 0xff,  0x3f, 0x00,  0xc0, 0xff,  0xff, 0x00, 
   0xfc, 0xff,  0xff, 0x3f,  0xfe, 0xff,  0xff, 0x7f, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00};

#define bomb4_width 32
#define bomb4_height 32
static char bomb4_bits[] = {
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0xc0,  0x07, 0x00, 
   0x00, 0x78,  0x10, 0x00,  0x00, 0x0f,  0x46, 0x00, 
   0x80, 0x61,  0x81, 0x00,  0xc0, 0x1c,  0x00, 0x01, 
   0x40, 0x02,  0x00, 0x00,  0x20, 0x01,  0x00, 0x00, 
   0x20, 0x01,  0x00, 0x00,  0x20, 0x00,  0x00, 0x00, 
   0x20, 0x00,  0x00, 0x00,  0x40, 0x00,  0x00, 0x00, 
   0x80, 0x00,  0x00, 0x00,  0x00, 0x02,  0x00, 0x00, 
   0x00, 0x02,  0x00, 0x00,  0x00, 0x04,  0x00, 0x00, 
   0x00, 0x04,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x04,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x04,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x04,  0x00, 0x00,  0x00, 0x02,  0x00, 0x00, 
   0x00, 0x01,  0x00, 0x00,  0xc0, 0x00,  0x00, 0x00, 
   0x2c, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00, 
   0x00, 0x00,  0x00, 0x00,  0x00, 0x00,  0x00, 0x00};

/* This is the name of a family of programs, so argv[0] inadequate. */

#define PROGRAMNAME "xconq"
#define PROGRAMCLASS "XConq"

/* Name of default font - why is this wired in? */

#define STANDARD "standard"

/* Magic number meaning that this pixmap intentionally left blank. */

#define NOPIXMAP 17

#define INFOLINES 5

#define BH 32

/* Don't use all of really big displays. */

/*** (UK) change -> ***/
#define MAXDISPLAYWIDTH 950
#define MAXDISPLAYHEIGHT 850
/*** was:
#define MAXDISPLAYWIDTH 900
#define MAXDISPLAYHEIGHT 800
*** <- change ***/

/* Use with caution - variable name "side" embedded! */

#define sd() ((Screeno *) side->display)

#define sdd() (sd()->disp)

#define sdds() DefaultScreen(sdd())

typedef unsigned int unint;


/* GC rules are for different fonts etc, but not colors or bitmaps. */

typedef struct a_screen {
    Display *disp;
    GC gc;			/* a tmp graphics context for this display */
    GC flashgc;			/* The gc for drawing the flash lines. */
    GC textgc;			/* foreground on background text */
    GC helpgc;			/* foreground on background help text */
    GC icongc;			/* icon graphics context */
    GC invicongc;		/* icon gc inverted colors. */
    GC varicongc;		/* This is an icon gc with variable fgcolor. */
    GC unitgc;			/* unit bitmap printing gc. */
    GC unittextgc;		/* unit text printing gc. */
/*** (HW) insert -> ***/
    GC sordergc;                /* gc for drawing move sorder arrows */
/*** <- insert ***/
    GC cleargc;			/* The gc to use for clearing areas. */
    GC clearbitgc;              /* gc for clearing bitmaps */
    XFontStruct *textfont;	/* font for text display */
    XFontStruct *iconfont;	/* utility font with assorted icons */
    XFontStruct *unitfont;	/* font for unit characters */
    XFontStruct *helpfont;      /* font for help window */
    Pixmap bombpics[4];		/* mushroom clouds */
    Pixmap unitpics[MAXUTYPES];	/* used instead of font sometimes */
    Pixmap wbdots, bwdots;	/* blue border and dotted backgrounds */
    Pixmap logostipple,logopmap;/* the XConq logo bitmap and pixmaps */
    long logocolor;		/* the color for the logopixmap */
    Cursor curs;		/* the cursor object itself */
} Screeno;

/* resources to retrieve */

#define sideoffset(field) XtOffset(Side*, field)
#define displayoffset(field) XtOffset(Screeno*, field)


static XtResource SideRes[] = {
  {"map.%s.hexDisplayMode", "Map.Period.HexDisplayMode", "HexDisplayMode",
     TRUE, sideoffset(showmode), 0, "BorderHex"},
/*** (HW) insert -> ***/
  {"map.%s.lineDisplayMode", "Map.Period.LineDisplayMode", "LineDisplayMode",
     TRUE, sideoffset(linemode), 0, "NormalCols"},
  {"map.%s.lineThicknessMode", "Map.Period.LineThicknessMode", "LineThicknessMode",
     TRUE, sideoffset(thicknessmode), 0, "AllThin"},
  {"map.%s.showSOrders", "Map.Period.ShowSOrders", "UType",
     TRUE, sideoffset(showsorderutype), 0, "<none>"},
  {"map.%s.distinctSOTypes", "Map.Period.DistinctSOTypes", "Boolean",
     TRUE, sideoffset(distinctsotypes), 0, "off"},
  {"map.%s.notifyVisibility", "Map.Period.NotifyVisibility", "Boolean",
     TRUE, sideoffset(notifyvisibility), 0, "off"},
  {"map.%s.followAction", "Map.Period.FollowAction", "Boolean",
     TRUE, sideoffset(followaction), 0, "off"},
  {"map.%s.showProduct", "Map.Period.ShowProduct", "Boolean",
     TRUE, sideoffset(show_product), 0, "off"},
  {"map.%s.showOrders", "Map.Period.ShowOrders", "Boolean",
     TRUE, sideoffset(show_orders), 0, "off"},
  {"map.%s.graphical", "Map.Period.Graphical", "SBoolean",
     TRUE, sideoffset(graphical), 0, "off"},
/*** <- insert ***/
  {NULL}
},
ColorRes[] = {
  {"map.%s.ownColor", "Map.Period.Background", "Pixel",
     FALSE, sideoffset(owncolor), 0, "black"},
  {"map.%s.alternateColor", "Map.Period.Background", "Pixel",
     FALSE, sideoffset(altcolor), 0, "blue"},
  {"map.%s.differentColor", "Map.Period.Background", "Pixel",
     FALSE, sideoffset(diffcolor), 0, "maroon"},
/*** (HW) insert -> ***/
  {"map.%s.productColor", "Map.Period.Background", "Pixel",
     FALSE, sideoffset(productcolor), 0, "white"},
  {"map.%s.sleepColor", "Map.Period.Background", "Pixel",
     FALSE, sideoffset(sleepcolor), 0, "cyan"},
  {"map.%s.orderColor", "Map.Period.Background", "Pixel",
     FALSE, sideoffset(ordercolor), 0, "violet"},
/*** <- insert ***/
  {"map.%s.borderColor", "Map.Period.Foreground", "Pixel",
     FALSE, sideoffset(bdcolor), 0, "blue"},
  {"map.%s.grayColor", "Map.Period.Foreground", "Pixel",
     FALSE, sideoffset(graycolor), 0, "light gray"},
  {"map.%s.enemyColor", "Map.Period.Foreground", "Pixel",
     FALSE, sideoffset(enemycolor), 0, "red"},
  {"map.%s.neutralColor", "Map.Period.Foreground", "Pixel",
     FALSE, sideoffset(neutcolor), 0, "light gray"},
  {"map.%s.goodColor", "Map.Period.Foreground", "Pixel",
     FALSE, sideoffset(goodcolor), 0, "green"},
  {"map.%s.badColor", "Map.Period.Foreground", "Pixel",
     FALSE, sideoffset(badcolor), 0, "red"},
  {NULL}
},
DpyRes[] = {
  {"textFont", "Font", "Font", TRUE,
     displayoffset(textfont), 0, TEXTFONT},
  {"helpFont", "Font", "Font", TRUE,
     displayoffset(helpfont), 0, HELPFONT},
  {"iconFont", "Font", "Font", TRUE,
     displayoffset(iconfont), 0, ICONFONT},
  {NULL}
};

/* Random function declarations. */

XFontStruct * open_font();
Pixmap load_bitmap();
void get_font_size();
Cursor make_cursor();
void program_crash();

/* The array of screen objects. */

Screeno screens[MAXSIDES];      /* All the "screen objects" */

/* Values of parameters generally tied to fonts and the like. */

int hw = 20;
int hh = 22;
int hch = 17;
int margin = 2;
int bd = 1;

int helpwinlines = 1;           /* size of help window */

bool rootcursor;                /* true if using parent window's cursor */
bool windows_up = FALSE;         /* Try to handle initial exposes. */

char *routine_executing = "Program Start";

/* Put in a default player, probably the invoker of the program. */
/* An empty host name will confuse everybody. */

add_default_player()
{
#ifdef UNIX
    char * default_display = getenv("DISPLAY");

    if (default_display != NULL){
	add_player(TRUE, default_display);
    } else {
	printf("I don't know what display you're on!\n");
	exit(1);
    }
#endif /* UNIX */
}

#ifndef DEBUG

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
  if (already_been_here) exit(1);
  else {
    already_been_here = TRUE;
    write_savefile("emergency.save.xconq");
    if (sig != 2)
      abort(1);
    else exit(1);
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
    write_savefile(SAVEFILE2);
    exit(1);
  }
}
/* Handlers for X catastrophes attempt to do a save first. */
/* Could be more friendly and check success of save, but can't do anything */
/* about it anyway... */

static int
handle_x_error (disp, evt)
Display *disp;
XErrorEvent *evt;
{
    static int num_errors = 0;
    char buf[BUFSIZE];

    XGetErrorText(disp, evt->error_code, buf, BUFSIZE);
    printf("\nX error on display %s: %s\n", DisplayString(disp), buf);
    if (++num_errors >= 10) {
        printf("\nX error: trying emergency save!\n");
        write_savefile(SAVEFILE);
	exit(1);
    }
}

static int
handle_xio_error (disp)
Display *disp;
{
    printf("\nX IO error on display %s: trying emergency save!\n",
	 DisplayString(disp));
    write_savefile(SAVEFILE);
    exit(1);
}

/* Ignore ^C if humans in the game, do it otherwise, including when the */
/* last human player turns into a machine (this is called by option cmd). */
/* Attempts to be more clever seem to be bad news. */

init_sighandlers()
{
#ifdef UNIX
/*** (UK) insert -> ***/
    extern bool ServerFlag;
    if (ServerFlag) init_server();
/*** <- insert ***/ 
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
#ifndef DEBUG
    signal(SIGBUS, program_crash);
    signal(SIGSEGV, program_crash);
    signal(SIGFPE, program_crash);
    signal(SIGILL, program_crash);
    /*
    signal(SIGSYS, program_crash);
    */
    signal(SIGINT, program_crash);
    signal(SIGQUIT, program_crash);
    signal(SIGTERM, program_crash);
#endif
#endif /* UNIX */
    XSetIOErrorHandler(handle_xio_error);
    XSetErrorHandler(handle_x_error);
}

#ifdef XFONTDIR
static void append_font_path(side, path)
Side *side;
char *path;
{
    char        **font_path, **new_path;
    int         npaths, ii;

    font_path = XGetFontPath(sdd(), &npaths);

    for (ii = 0; ii < npaths; ii++) {
        if (!strcmp(font_path[ii], path)) break;
    }

    /* I know this looks weird but that's X. */
    if (ii == npaths) {
        new_path = (char**)malloc((npaths+1)*sizeof(char*));
        /*if (!new_path) *****************************************/
        for (ii=0; ii < npaths; ii++) {
            new_path[ii] = font_path[ii];
        }
        new_path[npaths] = path;
        XSetFontPath(sdd(), new_path, npaths+1);
        free(new_path);
    }
    XFreeFontPath(font_path);
    return;
}
#endif /* XFONTDIR */

/* Note that the open_display function syncronizes the X server when the */
/* Debug flag is set. */

open_display(side)
Side *side;
{
  side->display = (long) &(screens[side_number(side)]);
  sdd() = XOpenDisplay(side->host);
  if (sdd()==NULL) {
/*** (UK) insert -> ***/
    side->display=NULL;
    return 0;
/*** <- insert ***/ 
/*** (UK) remove -> ***
    fprintf(stderr,"Error opening display %s\n", side->host);
    exit(1);
*** <- remove ***/ 
  }

  if (Debug) {
    XSynchronize(sdd(), TRUE);
    printf("Synching the X server.\n");
  }

  {
    XrmDatabase	temp;
    char	*database;
    char	path[200];

    XrmInitialize();
    sprintf(path, "%s/%s", XAPPLRESDIR, PROGRAMCLASS);
    side->rmdatabase = (long)XrmGetFileDatabase(path);
    if (side->rmdatabase==NULL) {
      fprintf(stderr,
	      "Warning: couldn't open app-defaults file \"%s\", ignoring it\n",
	      path);
      side->rmdatabase = (long)XrmGetStringDatabase("");
    }

#ifdef XFONTDIR
    append_font_path(side, XFONTDIR);
#endif

    database = XResourceManagerString(sdd()); /* grab property off
						 root window */
    if (database==NULL) { /* if it's not there then go for $HOME/.Xdefaults */
      char	*home=getenv("HOME");
      if (home==NULL) {
	fprintf(stderr,"No HOME?  you lose all around buddy.\n");
      } else {
	sprintf(path,"%s/.Xdefaults", home);
	temp = XrmGetFileDatabase(path);
	if (temp==NULL) {
	  fprintf(stderr,
		  "Warning: couldn't open X defaults file \"%s\", ignoring it\n",
		  path);
	} else {
	  XrmMergeDatabases(temp,(XrmDatabase *)&side->rmdatabase);
	}
      }
    } else { /* otherwise use the property */
      XrmMergeDatabases(XrmGetStringDatabase(database),
			(XrmDatabase *)&side->rmdatabase);
    }
  }
/*** (HW) change -> ***/
  side->main = XCreateSimpleWindow(sdd(), DefaultRootWindow(sdd()),
				   50, 3,
				   display_width(side), display_height(side),
				   3, black_color(side), white_color(side));
/*** was:
  side->main = XCreateSimpleWindow(sdd(), DefaultRootWindow(sdd()),
				   50, 3,
				   display_width(side), display_height(side),
				   3, white_color(side), black_color(side));
*** <- change ***/

  return 1;
}

/* A predicate that tests whether our display can safely be written to. */

active_display(side)
Side *side;
{
    return (side && side->host && !side->lost && side->display);
}

display_width(side)
Side *side;
{
    return min(MAXDISPLAYWIDTH,
	       ((19 * XDisplayWidth(sdd(), sdds())) / 20));
}

display_height(side)
Side *side;
{
    return min(MAXDISPLAYHEIGHT,
	       ((19 * XDisplayHeight(sdd(), sdds())) / 20));
}

/* Most X displays have enough screen to do a world map. */

world_display(side) Side *side; {  return TRUE;  }

/* utterly frivolous procedure to create the logo stipple and pixmap */
static void
create_logo(side, gc)
     Side *side;
     GC gc;	/* so we don't have to create one */
{
    XColor	xc;
    sd()->logostipple = XCreateBitmapFromData(sdd(), side->main,
        xconqlogo_bits, xconqlogo_width, xconqlogo_height);
    XSetStipple(sdd(), gc, sd()->logostipple);
    XSetFillStyle(sdd(), gc, FillOpaqueStippled);
    xc.red = 0x4000;
    xc.blue = xc.green = 0x2800;
    xc.flags = DoRed|DoGreen|DoBlue;
    XAllocColor(sdd(), DefaultColormap(sdd(), sdds()), &xc);
    XSetForeground(sdd(), gc, xc.pixel);
    XSetBackground(sdd(), gc, side->bgcolor);
    sd()->logopmap = XCreatePixmap(sdd(), side->main, xconqlogo_width,
				   xconqlogo_height,
				   DefaultDepth(sdd(), sdds()));
    XFillRectangle(sdd(), sd()->logopmap, gc, 0, 0, xconqlogo_width,
				   xconqlogo_height);
}

/*** (UK) insert -> ***/
convert_bool(side,dest,value)
Side *side;
char *dest, *value;
{
    static char *boolmodes[] = {
      "off", "on", "Off", "On", "OFF", "ON", NULL };
    int	i;
    for (i=0; boolmodes[i] != NULL; i++) {
      if (0==strcasecmp(value, boolmodes[i]))
	break;
    }
    if (boolmodes[i]!=NULL) {
      *(bool*)dest = (i&1)!=0;
    }
    else {
      fprintf(stderr, "%s: invalid boolean spec %s\n", side->host, value);
    }
}
/*** <- insert ***/

/* this is our fake version of a resource conversion routine.
   it converts the String in <value> and turns it into an object of
   type <type> and places it in <dest>.  We assume <dest> points to
   enough space to hold the object, otherwise why give it to us?
   The currently supported types are:
     Pixel:	a string that is either a colorname or a #xxx code
		is allocated in the colormap and the pixel number
		is returned in dest.
     Font:	a font name is turned into a font struct and the
		pointer to the XFontStruct is returned in dest.
     HexDispMode: the string is compared to "fullhex"/"borderhex"/
		"terricons"/"bothicons" with total case insensitivity
		and converted to one of the corresponding integers
		from side.h

   If other types become necessary it will be trivial to add them.
   */

convert_thing(side, type, value, dest)
     Side	*side;
     char	*type, *value, *dest;
{
  if (Debug) {
    printf("converting %s into a %s for %s\n", value, type, side->name);
  }
  if (0==strcmp(type, "Pixel")) {
    XColor	xc;
    if (Debug) printf("Allocating %s\n", value);
    if (!XParseColor(sdd(), DefaultColormap(sdd(), sdds()), value, &xc)) {
      fprintf(stderr, "%s: invalid color spec %s\n", side->host, value);
      return;
    }
    if (!XAllocColor(sdd(), DefaultColormap(sdd(), sdds()), &xc)) {
      fprintf(stderr, "%s: unable to allocate color %s\n", side->host, value);
      return;
    }
    *(long*)dest = xc.pixel;
  } else if (0==strcmp(type, "Font")) {
    XFontStruct **font = (XFontStruct **)dest;
    *font = XLoadQueryFont(sdd(), value);
    if (*font == NULL) {
      fprintf(stderr, "%s: Can't open font %s, ", side->host, value);
      if (font == &(sd()->textfont)) {
	fprintf(stderr, "you're hosed\n");
	exit(1);
      } else {
	fprintf(stderr, "trying to substitute text font\n");
	*font = sd()->textfont;
      }
    }
    if (Debug) printf("Opened font \"%s\" ...\n", value);
  } else if (0==strcmp(type, "HexDisplayMode")) {
    static char *hexdispmodes[] = {
/*** (HW) change -> ***/
      "FullHex", "BorderHex", "TerrIcons", "BothIcons", "OneIcon", NULL };
/*** was:
      "FullHex", "BorderHex", "TerrIcons", "BothIcons", NULL };
*** <- change ***/
    int	i;
    for (i=0; hexdispmodes[i] != NULL; i++) {
      if (0==strcasecmp(value, hexdispmodes[i]))
	break;
    }
    if (hexdispmodes[i]!=NULL) {
      *(short*)dest = i;
    }
/*** (HW) insert -> ***/
    else {
      fprintf(stderr, "%s: invalid hex display mode  spec %s\n", side->host, value);
    }
  } else if (0==strcmp(type, "LineDisplayMode")) {
    static char *linedispmodes[] = {
      "NormalCols", "InvertedCols", "AlwaysWhite", "Special", NULL };
    int	i;
    for (i=0; linedispmodes[i] != NULL; i++) {
      if (0==strcasecmp(value, linedispmodes[i]))
	break;
    }
    if (linedispmodes[i]!=NULL) {
      *(short*)dest = i;
    }
    else {
      fprintf(stderr, "%s: invalid line display mode  spec %s\n", side->host, value);
    }
/*** <- insert ***/
/*** (UK) insert -> ***/
  } else if (0==strcmp(type, "LineThicknessMode")) {
    static char *linethicknessmodes[] = {
      "AllThin", "MoveOrderThick", "StandingOrderThick", "AllThick", NULL };
    int	i;
    for (i=0; linethicknessmodes[i] != NULL; i++) {
      if (0==strcasecmp(value, linethicknessmodes[i]))
	break;
    }
    if (linethicknessmodes[i]!=NULL) {
      *(short*)dest = i;
    }
    else {
      fprintf(stderr, "%s: invalid line thickness mode  spec %s\n", side->host, value);
    }
  } else if (0==strcmp(type, "UType")) {
    int	i;
    if (!strcasecmp(value,"<none>")) {
      *(short*)dest = NOTHING;
    }
    else
    if (!strcasecmp(value,"<all>")) {
      *(short*)dest = ALL;
    }
    else {
      for_all_unit_types(i) {
	if (0==strcasecmp(value, utypes[i].name)) {
	  *(short*)dest = i;
	  return;
	}
      }
      fprintf(stderr, "%s: unit type %s\n", side->host, value);
    }
  } else if (0==strcmp(type, "Boolean")) {
    convert_bool(side,dest,value);
  } else if (0==strcmp(type, "SBoolean")) {
    bool b;
    convert_bool(side,&b,value);
    *(short*)dest = b;
/*** <- insert ***/
  } else {
    fprintf(stderr, "What the heck kind of resource is %s?!\n", type);
    exit(1);
  }
}

/* src1 contains a prefix, usually the name or class (xconq or XConq).
   src2 contains a resource path like Map.Period.AltColor or map.%s.altColor.
   The %s allows us to insert the period name into a resource.
   dest must have enough space to hold the final string, however
   big that is >:) */
format_resource_string(src1, src2, dest)
     char	*src1, *src2, *dest;
{
  static char	buf[200],*p;
  sprintf(buf, "%s.%s", src1, src2);
  sprintf(dest, buf, period.name);
  for (p=dest; *p; p++)
    if (*p==' ')
      *p = '_';
}

/* this procedure gets resources from the display's resource database.
   It uses the toolkit's XtResource structure because it seems to have
   all the structure memebers we need and looks familiar, but we do
   use it slightly differently.
   We retrieve resources that go in the Side structure from the SideRes
   list.  We retrieve resources that go in the Screeno structure from
   the DpyRes list */
get_resources(side, resources, base)
     Side	*side;
     XtResource	*resources;
     char	*base;
{
  XtResource	*res;
  static char	rmName[200], rmClass[200];
  char	*rtype, *resval;
  XrmValue	rval;

  for (res = resources; res->resource_name!=NULL; res++) {
    if (side->monochrome && !res->resource_size) {
      /* goofy hack to avoid setting colors on monochrome */
      continue; 
    }
    format_resource_string(PROGRAMNAME, res->resource_name, rmName);
    format_resource_string(PROGRAMCLASS, res->resource_class, rmClass);
    XrmGetResource((XrmDatabase)side->rmdatabase, rmName,
		   rmClass, &rtype, &rval);
    /*
    printf("rmName=%s rtype=%s\n",rmName,rtype);
    */
    if (rtype==NULL) {
      resval = res->default_addr;
    } else if (0!=strcmp(rtype, "String")) {
      fprintf(stderr, "Whoa!, funky resource type %s for %s\n",
	      rtype, rmName);
      resval = res->default_addr;
    } else {
      resval = rval.addr;
    }
    convert_thing(side, res->resource_type, resval,
		  base + (res->resource_offset));
  }
}

get_color_resources(side)
     Side	*side;
{
  get_resources(side, ColorRes, side);
}

/* Could use handlers for X failures... */

/* Do misc setup thingies. */

/* Do the rigmarole to convert all those short arrays into X-approved */
/* Bitmaps.  Note that this has to be for *each* display separately (!) */
/* Also get the cursor shape.  If the hardware can't hack the desired */
/* cursor size, warn about it and just use the root window's cursor. */
/* 0x0 cursor specs seem to be don't cares - machine can handle any size */
/* X11 also needs gazillions of GCs, since we've got so many fonts and */
/* colors and bitmaps. */

init_misc(side)
Side *side;
{
    int u;
    unsigned int w, h;
    unsigned long mask;
    XGCValues values;
    Pixmap mmask, mcurs, dots;
    GC gc;
    char *name;
/*** (HW) insert -> ***/
    static char dash[] = {4,4};
/*** <- insert ***/

    side->margin = margin;
    side->bd = bd;
    side->hw = hw;
    side->hh = hh;
    side->hch = hch;
    /* get the name from the display unless the game is being restored from a 
       save file. */
    if (!midturnrestore &&
	(name = XGetDefault(sdd(), PROGRAMNAME, "SideName")) != NULL)
      side->name = name;
    mask = GCForeground | GCBackground;
    values.foreground = side->fgcolor;
    values.background = side->bgcolor;
    gc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->gc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->flashgc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->textgc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->helpgc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->icongc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->varicongc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->unitgc = XCreateGC(sdd(), side->main, mask, &values);
    sd()->unittextgc = XCreateGC(sdd(), side->main, mask, &values);
/*** (HW) insert -> ***/
    sd()->sordergc = XCreateGC(sdd(), side->main, mask, &values);
/*** <- insert ***/
    values.foreground = side->bgcolor;
    values.background = side->fgcolor;
    sd()->invicongc = XCreateGC(sdd(), side->main, mask, &values);
    values.function = GXclear;
    mask = GCFunction;
    sd()->cleargc = XCreateGC(sdd(), side->main, mask, &values);
#if 1
    get_resources(side, SideRes, side);
    get_resources(side, DpyRes, sd());
    /* ginit retrieves the colors */
#else
    sd()->textfont = open_font(side, TEXTFONT, "TextFont", NULL);
    sd()->helpfont = open_font(side, HELPFONT, "HelpFont", sd()->textfont);
    sd()->iconfont = open_font(side, ICONFONT, "IconFont", sd()->textfont);
#endif
    get_font_size(sd()->textfont, &(side->fw), &(side->fh) );
    get_font_size(sd()->helpfont, &(side->hfw), &(side->hfh) );
    get_font_size(sd()->iconfont, &(side->hw), &(side->hh) );
    if (period.fontname != NULL && strlen(period.fontname) > 0) {
	sd()->unitfont = 
	    open_font(side, period.fontname, "UnitFont", sd()->textfont);
	get_font_size(sd()->unitfont, &(side->uw), &(side->uh) );
    } else {
      	sd()->unitfont = 
	    open_font(side, STANDARD, "UnitFont", sd()->textfont);
	get_font_size(sd()->unitfont, &(side->uw), &(side->uh) );
    }
    if (Debug) printf("doing textgc...\n");
    XSetFont(sdd(), sd()->textgc, sd()->textfont->fid);
    if (Debug) printf("doing helpfont ...\n");
    XSetFont(sdd(), sd()->helpgc, sd()->helpfont->fid);
    if (Debug) printf("doing icongc ...\n");
    XSetFont(sdd(), sd()->icongc, sd()->iconfont->fid);
    if (Debug) printf("done fonts ...\n");
    mask = GCFillStyle | GCGraphicsExposures;
    values.fill_style = FillSolid;
    values.graphics_exposures = FALSE;
    XChangeGC(sdd(), sd()->unitgc, mask, &values);
/*** (HW) insert -> ***/
    XChangeGC(sdd(), sd()->sordergc, mask, &values);
    XSetDashes(sdd(),sd()->sordergc,0,dash,2);
    set_line_mode(side);
/*** <- insert ***/
    XSetFont(sdd(), sd()->unittextgc, sd()->unitfont->fid);
    XSetFont(sdd(), sd()->invicongc, sd()->iconfont->fid);
    XSetFont(sdd(), sd()->varicongc, sd()->iconfont->fid);
    XSetFunction(sdd(), sd()->flashgc, GXinvert);

    mmask = XCreateBitmapFromData(sdd(), side->main,
					mask_bits, mask_width, mask_height);
    mcurs = XCreateBitmapFromData(sdd(), side->main,
					curs_bits, curs_width, curs_height);

    dots  = XCreateBitmapFromData(sdd(), side->main, 
				  dots_bits, dots_width, dots_height);

    sd()->bombpics[0] = XCreateBitmapFromData(sdd(), side->main,
	bomb1_bits, bomb1_width, bomb1_height);
    sd()->bombpics[1] = XCreateBitmapFromData(sdd(), side->main,
        bomb2_bits, bomb2_width, bomb2_height);
    sd()->bombpics[2] = XCreateBitmapFromData(sdd(), side->main,
	bomb3_bits, bomb3_width, bomb3_height);
    sd()->bombpics[3] = XCreateBitmapFromData(sdd(), side->main,
	bomb4_bits, bomb4_width, bomb4_height);
    for_all_unit_types(u) {
	if (utypes[u].bitmapname && (strlen(utypes[u].bitmapname) > 0)) {
	    sd()->unitpics[u] = load_bitmap(side, utypes[u].bitmapname);
	} else {
	    utypes[u].bitmapname = NULL;
	}
    }
    if (Debug) printf("Bitmaps stored ...\n");

    sd()->wbdots = XCreatePixmap(sdd(), side->main, dots_width, dots_height,
				 DefaultDepth(sdd(), sdds()));
    sd()->bwdots = XCreatePixmap(sdd(), side->main, dots_width, dots_height,
				 DefaultDepth(sdd(), sdds()));

    /* Since clearbitgc has to have a depth of 1 we'll */
    /* hang it off bombpics[0] */
    mask = GCFunction;
    values.function = GXclear;
    sd()->clearbitgc = XCreateGC(sdd(), sd()->bombpics[0], mask, &values);

    XSetForeground(sdd(), gc, side->fgcolor);
    XSetBackground(sdd(), gc, side->bgcolor);
    XSetStipple(sdd(), gc, dots);
    XSetFillStyle(sdd(), gc, FillOpaqueStippled);
    XFillRectangle(sdd(), sd()->wbdots, gc, 0, 0, dots_width, dots_height);

    XSetForeground(sdd(), gc, side->bgcolor);
    XSetBackground(sdd(), gc, side->fgcolor);
    XFillRectangle(sdd(), sd()->bwdots, gc, 0, 0, dots_width, dots_height);

    create_logo(side,gc); /* this procedure changes the GC rather radically */
    if (Debug) printf("Tiles stored ...\n");

    rootcursor = FALSE;
    XQueryBestCursor(sdd(), side->main, curs_width, curs_height, &w, &h);
    if (Debug) printf("Best cursor size is %dx%d\n", w, h);
    if (w >= curs_width && h >= curs_height) {
        sd()->curs =
	  make_cursor(sdd(), mcurs, mmask, white_color(side),
		      black_color(side), 7, 7);
	   
    } else {
	fprintf(stderr, "Warning: Can't have %dx%d cursors on \"%s\"!\n",
		curs_width, curs_height, side->host);
        fprintf(stderr, "Using default cursor...\n");
        rootcursor = TRUE;
    }
    if (Debug) printf("Cursor stored ...\n");
    XFreeGC(sdd(), gc);
    XFreePixmap(sdd(), dots);
    XFreePixmap(sdd(), mmask);
    XFreePixmap(sdd(), mcurs);
}

/* Since XCreatePixmapCursor() takes XColors and not pixel values we */
/* have to look the colors associated with the foreground and */
/* background pixel values up in the color table and pass them to */
/* XCreatePixmapCursor(). */
   
Cursor make_cursor(dpy, curs, mask, foreground, background, x, y)
Display *dpy;
Pixmap curs, mask;
unsigned long foreground, background;
unsigned int x, y;
{
    XColor defs[2];

    defs[0].pixel = foreground;
    defs[1].pixel = background;
    XQueryColors(dpy, DefaultColormap(dpy, DefaultScreen(dpy)), defs, 2);
    return  XCreatePixmapCursor(dpy, curs, mask, &defs[0], &defs[1], x, y);
}

/* Open a font, possibly substituting another font if our desired one is */
/* missing for some reason. */

XFontStruct *
open_font(side, name, xdefault, altfont)
Side *side;
char *name, *xdefault;
XFontStruct * altfont;
{
  char *altname;
  XFontStruct * font;

  if ((altname = XGetDefault(sdd(), PROGRAMNAME, xdefault)) != NULL)
    name = altname;
  if ((font = XLoadQueryFont(sdd(), name)) == NULL) {
    fprintf(stderr, "Can't open font \"%s\" on \"%s\"\n", name, side->host);
    if (altfont != NULL) {
      /* ought to display the name... */
      fprintf(stderr, "Substituting another font...\n");
      return altfont;
    } else {
      fprintf(stderr, "No font to substitute!!\n");
      exit(1);
    }
  }
  if (Debug) printf("Opened font \"%s\" ...\n", name);
  return font;
}

/* Force X11 font fanciness into semblance of X10 font plainness. */

void
get_font_size(font, width, height)
XFontStruct * font;
short *width, *height;
{
    *width = font->max_bounds.width;
    *height = font->max_bounds.ascent + font->max_bounds.descent;
    if (Debug) {
	printf("returning font size %d %d\n", (int)*width, (int)*height);
    }
}

/* Try to load a bitmap of the given name, looking in both the current dir */
/* and the library dir. */

Pixmap
load_bitmap(side, name)
Side *side;
char *name;
{
    int hotx, hoty;
    unsigned int w, h, a;
    Pixmap rslt;

    make_pathname(NULL, name, "b", spbuf);
    a = XReadBitmapFile(sdd(), side->main, spbuf, &w, &h, &rslt, &hotx, &hoty);
    if (a == BitmapSuccess) return rslt;
    make_pathname(xconqlib, name, "b", spbuf);
    a = XReadBitmapFile(sdd(), side->main, spbuf, &w, &h, &rslt, &hotx, &hoty);
    if (a == BitmapSuccess) return rslt;
    fprintf(stderr, "Bitmap name \"%s\" not found here or in \"%s\"!\n",
	    name, xconqlib);
    return (-1);
}

/* This routine has to be able to cope with window managers constraining */
/* size.  Actually, the main window was already opened when the display */
/* was opened, so the name is not quite accurate! */

create_main_window(side)
Side *side;
{
    Pixmap dots;
    XSizeHints hints;
    XWMHints	wmhints;
		  
    XStoreName(sdd(), side->main, PROGRAMNAME);
    dots = (side->bonw ? sd()->bwdots : sd()->wbdots); 
    XSetWindowBackgroundPixmap(sdd(), side->main, dots);

    hints.width = side->mw;
    hints.height = side->mh;
    hints.min_width = side->mw/4;
    hints.min_height = side->mh/4;
    hints.flags = PSize|PMinSize;
    XSetNormalHints(sdd(), side->main, &hints);
    wmhints.flags = InputHint|IconPixmapHint|WindowGroupHint;
    wmhints.input = True;
    wmhints.icon_pixmap = sd()->logostipple;
    wmhints.window_group = side->main;
    XSetWMHints(sdd(), side->main, &wmhints);
}

/* Help window is not necessarily a subwindow, though it might be sometimes. */

create_help_window(side)
Side *side;
{
    XWMHints	wmhints;
    helpwinlines =
	max(56, (25 + period.numrtypes + period.numttypes + period.numutypes));

/*** (HW) change -> ***/
    side->help = XCreateSimpleWindow(sdd(), DefaultRootWindow(sdd()),
				     0, 0, 80*side->hfw+1,
				     helpwinlines*side->hfh+2,
				     1, side->fgcolor, side->bgcolor);
/*** was:
    side->help = XCreateSimpleWindow(sdd(), DefaultRootWindow(sdd()),
				     0, 0, 80*side->hfw+1,
				     helpwinlines*side->hfh+1,
				     1, side->fgcolor, side->bgcolor);
*** <- change ***/
#if 0
    /* This is fun but makes the help window hard to read, even in 
       muted color. */
    XSetWindowBackgroundPixmap(sdd(), side->help, sd()->logopmap);
#endif
    XStoreName(sdd(), side->help, "xconq-help");

    wmhints.flags = IconPixmapHint|WindowGroupHint;
    wmhints.icon_pixmap = sd()->logostipple;
    wmhints.window_group = side->main;
    XSetWMHints(sdd(), side->help, &wmhints);
}

/* Subwindow creator. */

create_window(side, x, y, w, h)
Side *side;
int x, y, w, h;
{
    return XCreateSimpleWindow(sdd(), side->main, x, y, w, h,
			       1, side->fgcolor, side->bgcolor);
}

/* force x to retain this window. */
set_retain(side, window)
Side *side;
long window;
{
  XSetWindowAttributes foo;
  char *retain;

  if ((retain = XGetDefault(sdd(), PROGRAMNAME, "Retain")) == NULL ||
      0!=strcasecmp(retain, "no")) {
    foo.backing_store = WhenMapped;
    XChangeWindowAttributes(sdd(), window, CWBackingStore, &foo);
  }
}


/* Do little things necesary to make it all go, in this case mapping all */
/* the windows (except help win). */

fixup_windows(side)
Side *side;
{
    XMapWindow(sdd(), side->main);
    XMapSubwindows(sdd(), side->main);
    /* if (!global.giventime) XUnmapWindow(sdd(), side->clock); */
    if (!rootcursor) XDefineCursor(sdd(), side->map, sd()->curs);
}

/* Specify the sorts of input that will be allowed - main window needs to */
/* see mouse buttons so unit type selection works right. */

enable_input(side)
Side *side;
{
    XSelectInput(sdd(), side->main, /* ExposureMask|*/
		 KeyPressMask|StructureNotifyMask);
    XSelectInput(sdd(), side->map, ButtonPressMask|ExposureMask);
    XSelectInput(sdd(), side->state, ButtonPressMask|ExposureMask);
    XSelectInput(sdd(), side->msg, ButtonPressMask|ExposureMask);
    XSelectInput(sdd(), side->help, KeyPressMask|ExposureMask);
    XSelectInput(sdd(), side->info, ExposureMask);
    XSelectInput(sdd(), side->prompt, ExposureMask);
    XSelectInput(sdd(), side->world, ButtonPressMask|ExposureMask);
    XSelectInput(sdd(), side->sides, ExposureMask);
    XSelectInput(sdd(), side->timemode, ExposureMask);
    XSelectInput(sdd(), side->clock, ExposureMask); 
}

/* Move windows and change their sizes to correspond with the new sizes of */
/* viewports, etc */

reset_misc(side)
Side *side;
{
    Pixmap dots;
    XSizeHints hints;
    XGCValues values;
    unsigned long gcmask;

    dots = (side->bonw ? sd()->bwdots : sd()->wbdots); 

    XResizeWindow(sdd(), side->main, side->mw, side->mh);
    hints.width = side->mw;  hints.height = side->mh;
    hints.min_width = side->mw/4;  hints.min_height = side->mh/4;
    hints.flags = PSize|PMinSize;
    XSetNormalHints(sdd(), side->main, &hints);
    if (Debug)
      printf("hinting for %dx%d window\n", side->mw, side->mh);

    XSetWindowBackgroundPixmap(sdd(), side->main, dots);
    XSetWindowBackground(sdd(), side->msg, side->bgcolor);
    XSetWindowBackground(sdd(), side->info, side->bgcolor); 
    XSetWindowBackground(sdd(), side->prompt, side->bgcolor); 
    XSetWindowBackground(sdd(), side->map, side->bgcolor);
    XSetWindowBackground(sdd(), side->timemode, side->bgcolor);
    XSetWindowBackground(sdd(), side->clock, side->bgcolor);
    XSetWindowBackground(sdd(), side->state, side->bgcolor);
    XSetWindowBackground(sdd(), side->help, side->bgcolor);
    XSetWindowBackground(sdd(), side->sides, side->bgcolor);
    XSetWindowBackground(sdd(), side->world, side->bgcolor);
    XSetWindowBorder(sdd(), side->msg, side->fgcolor);
    XSetWindowBorder(sdd(), side->info, side->fgcolor); 
    XSetWindowBorder(sdd(), side->prompt, side->fgcolor); 
    XSetWindowBorder(sdd(), side->map, side->fgcolor);
    XSetWindowBorder(sdd(), side->timemode, side->fgcolor);
    XSetWindowBorder(sdd(), side->clock, side->fgcolor);
    XSetWindowBorder(sdd(), side->state, side->fgcolor);
    XSetWindowBorder(sdd(), side->help, side->fgcolor);
    XSetWindowBorder(sdd(), side->sides, side->fgcolor);
    XSetWindowBorder(sdd(), side->world, side->fgcolor);

    gcmask = GCForeground | GCBackground;
    values.foreground = side->fgcolor;
    values.background = side->bgcolor;
    XChangeGC(sdd(), sd()->gc, gcmask, &values);
    XChangeGC(sdd(), sd()->flashgc, gcmask, &values);
    XChangeGC(sdd(), sd()->textgc, gcmask, &values);
    XChangeGC(sdd(), sd()->helpgc, gcmask, &values);
    XChangeGC(sdd(), sd()->icongc, gcmask, &values);
    XChangeGC(sdd(), sd()->unitgc, gcmask, &values);
/*** (HW) insert -> ***/
    XChangeGC(sdd(), sd()->sordergc, gcmask, &values);
/*** <- insert ***/
    values.foreground = side->bgcolor;
    values.background = side->fgcolor;
    XChangeGC(sdd(), sd()->invicongc, gcmask, &values);
}

/* Alter the size and position of a window. */

change_window(side, win, x, y, w, h)
Side *side;
Window win;
int x, y, w, h;
{
    unsigned int mask;
    XWindowChanges changes;

    if (active_display(side)) {
	if (x >= 0) {
	    if (w >= 0) {
		mask = CWX | CWY | CWWidth | CWHeight;
		changes.x = x;  changes.y = y;
		changes.width = w;  changes.height = h;
	    } else {
		mask = CWX | CWY;
		changes.x = x;  changes.y = y;
	    }
	} else {
	    mask = CWWidth | CWHeight;
	    changes.width = w;  changes.height = h;
	}
    }
    XConfigureWindow(sdd(), win, mask, &changes);
}

/* Return the number of colors - this is used to guess about monochromeness. */

display_colors(side)
Side *side;
{
    return XDisplayCells(sdd(), sdds());
}

white_color(side)
Side *side;
{
    return WhitePixel(sdd(), sdds());
}

black_color(side)
Side *side;
{
    return BlackPixel(sdd(), sdds());
}

/* Get a color set up and warn if not getting what was asked for.  We can */
/* tolerate being off somewhat.  (Note X rgb value range is 0-65535.) */

#define CLOSE_ENOUGH(X,Y) (abs(((int) X) - ((int) Y)) < 2000)

long
request_color(side, name, class, fallback)
Side *side;
char *name, *class, *fallback;
/*** (HW) insert -> ***/
{
  XColor xcol;

  request_xcolor(side, name, class, fallback, &xcol);
  return xcol.pixel;
}

long
request_xcolor(side, name, class, fallback, xcol)
Side *side;
char *name, *class, *fallback;
XColor *xcol;
/*** <- insert ***/
{
    XColor c, avail;
    static char	rmName[200], rmClass[200];
    char	*rtype;
    XrmValue	rval;
    char	*colorname;

    if (class!=NULL && name!=NULL) {
      sprintf(rmName, "%s.%s", PROGRAMNAME, name);
      sprintf(rmClass,"%s.%s", PROGRAMCLASS, class);
      XrmGetResource((XrmDatabase)side->rmdatabase, rmName,
		     rmClass, &rtype, &rval);
      if (rtype==NULL) {
	colorname = fallback;
      } else if (0!=strcmp(rtype, "String")) {
	fprintf(stderr, "Whoa!, funky resource type %s for %s\n",
		rtype, rmName);
	colorname = fallback;
      } else {
	colorname = rval.addr;
      }
    } else {
      colorname = fallback;
    }

    if (Debug) printf("Allocating %s\n", colorname);
    if (!XAllocNamedColor(sdd(), DefaultColormap(sdd(), sdds()),
			  colorname, &avail, &c)) {
      fprintf(stderr, "Error: Couldn't allocate color %s!", colorname);
      return white_color(side);
    }
    if (!(CLOSE_ENOUGH(c.red, avail.red) &&
	  CLOSE_ENOUGH(c.green, avail.green) &&
	  CLOSE_ENOUGH(c.blue, avail.blue))) {
	fprintf(stderr, "Warning: %s color is way off on \"%s\"! ",
		name, side->host);
	fprintf(stderr, "(%d %d %d instead of %d %d %d)\n",
		avail.red, avail.green, avail.blue, c.red, c.green, c.blue);
    }
/*** (HW) insert -> ***/
    *xcol = avail;
/*** <- insert ***/
    return avail.pixel;
}

/*** (HW) insert -> ***/
long brighten_color(side, name, class, fallback)
Side *side;
char *name, *class, *fallback;
{
  XColor xcol;
  unsigned short old, delta_gray, nhigh;
  unsigned sumdelta;
#define GRAY_VAL 10000
#define MAXPIXVAL 65535

  request_xcolor(side, name, class, fallback, &xcol);
  delta_gray = GRAY_VAL;
  nhigh = 0;
  if (xcol.red >= 65000) { delta_gray += 5000; nhigh++; }
  if (xcol.green >= 55000) { delta_gray += 24000; nhigh++; }
  if (xcol.blue >= 65000) { delta_gray += 5000; nhigh++; }
  if (nhigh > 1)
    if (delta_gray > MAXPIXVAL-4000) delta_gray = MAXPIXVAL;
    else delta_gray += 4000;
  sumdelta = delta_gray + xcol.green/5;
  if (sumdelta > MAXPIXVAL) sumdelta = MAXPIXVAL;

  old = xcol.red;
  xcol.red += sumdelta;
  if (xcol.red < old) xcol.red = MAXPIXVAL;
  if (xcol.red < 10000) xcol.red *= 2;

  old = xcol.blue;
  xcol.blue += sumdelta;
  if (xcol.blue < old) xcol.blue = MAXPIXVAL;
  if (xcol.blue < 10000) xcol.blue *= 2;

  old = xcol.green;
  xcol.green += sumdelta;
  if (xcol.green < old) xcol.green = MAXPIXVAL;
  if (xcol.green < 10000) xcol.green *= 2;

  if (!XAllocColor(sdd(), DefaultColormap(sdd(), sdds()), &xcol)) {
    return white_color(side);
  }
  return xcol.pixel;
}
/*** <- insert ***/

/* Main funnel for input returns both mouse and keyboard events, and maybe */
/* other kinds eventually.  Some events like window exposure are handled */
/* strictly locally. */
/*
  Timeout needed to make it play smooth while still being fair.
 */
/*struct timeval { 
	long	tv_sec;		/* seconds * /
	long	tv_usec;	/* and microseconds * /
}; */

struct timeval timeout = {0, 50000};

/*** (UK) insert -> ***/
test_input()
{   Side *side;
    int mask;
    for_all_sides(side) {
	if (active_display(side)) {
	    side->redisplay_ok = TRUE;
	    mask |= (1 << ConnectionNumber(sdd()));
	    while (XPending(sdd()) > 0) {
		process_events(side);
	    } 
	    
	}
    }
}
/*** <- insert ***/

get_input()
{
#ifdef SELECT2
    int i, mask;
    Side *side, *asides[32];
    bool waiters = FALSE;

/*** (UK) change -> ****/
    bool other_action=FALSE;

    if (sock>0) mask = 1<<sock;
    else mask = 0;
/*** was:
    mask = 0;
*** <- change ***/
    for_all_sides(side) {
	if (active_display(side)) {
	    side->redisplay_ok = TRUE;
	    mask |= (1 << ConnectionNumber(sdd()));
	    asides[ConnectionNumber(sdd())] = side;
	    while (XPending(sdd()) > 0) {
		process_events(side);
		waiters = TRUE;
	    } 
	    
	}
    }
    if (waiters) {
      if (Debug) printf("Handled pending events, so didn't need to wait\n");
      return;
    }
    if (Debug) {
	printf("Waiting for input from ");
	for_all_sides(side)
	    if (active_display(side)) printf("%s ", side->host);
	printf("\n");
    }
/*** (UK) insert -> ***/
    for_all_sides(side) {
      if ((humanside(side) && (active_display(side) ||
           (side->mode==MOVE && !side->reqactive))) ||
	  (!humanside(side) && side->more_units))
	other_action=TRUE;
    }
/*** <- insert ***/
/*** (UK) change -> ***/
    if (select(32, &mask, 0, 0, other_action?&timeout:0) < 0) {
/*** was:
    if (select(32, &mask, 0, 0, &timeout) < 0) {
*** <- change ***/
      if (errno!=EINTR) {
	fprintf(stderr, "error in select!\n");
	abort();
      }
    } else {
/*** (UK) insert -> ***/
        if (sock>0 && (mask&(1<<sock))) {
	  handle_connection();
	  mask&=~(1<<sock);
        }
/*** <- insert ***/ 
	for (i = 0; i < 32; ++i) {
	    if (mask & (1 << i)) {
		process_events(asides[i]);
	      }
	}
	for_all_sides(side) {
	  int delta_time = (time(0) - side->lasttime);

	  side->lasttime = time(0);
	  if (side->more_units) {
/*** (UK) change -> ***/
            if (!humanside(side) || active_display(side)) {
	      side->timeleft -= delta_time;
	      side->timetaken += delta_time;
	      if (delta_time) update_clock(side);
            }
/*** was:
	    side->timeleft -= delta_time;
	    side->timetaken += delta_time;
	    update_clock(side);
*** <- change ***/
	  }
	}
    }
#else
#if 0
#ifdef BOGUS
    /* this doesn't work anymore */
    extern Side *curside;

    /* No simultaneity, but there's no portable way to do it, sigh */
    if (active_display(curside) && humanside(curside)) {
	if (Debug) printf("Waiting for input from %s\n", curside->host);
	process_events(curside);
    }
#endif
#endif
#endif /* SELECT2 */
}

/* Look at a single event and fill the request structure appropriately. */

process_events(side)
Side *side;
{
    XEvent evt;
    char buf[BUFSIZE];
    int nchar, rawx, rawy;

    side->reqtype = GARBAGE;
    XNextEvent(sdd(), &evt);
    switch (evt.type) {
    case KeyPress:
	if (evt.xkey.window != side->main && evt.xkey.window != side->help) 
	    return;
	nchar = XLookupString(&(evt.xkey), buf, BUFSIZE, NULL, NULL);
	if (nchar > 0) {
	    side->reqtype = KEYBOARD;
	    side->reqch = *buf;
	    if (Debug) printf(isprint(toascii(side->reqch)) ?
			      "Host %s returns key '%c'\n" :
			      "Host %s returns key '\\%03o'\n" ,
			      side->host, side->reqch);
	    if (evt.xkey.window==side->help) {
	      x_help(side);
	      /* this little help handler might be a bad idea.
		 The entire xconq event loop is hacked.  That's
		 why Stan's redoing it */
	    } else if (side->reqch == '\014') { /* control l */
	      redraw(side);
	      side->reqtype = GARBAGE;
	    }
	  }
	break;
    case ButtonPress:
	if (evt.xbutton.window == side->map) {
	    rawx = evt.xbutton.x;  rawy = evt.xbutton.y; 
	    side->reqtype = MAPPOS;
	    side->reqch = (evt.xbutton.state & ShiftMask) ? 1 : 0;
	    /* the modifier is used by x_command */
	    deform(side, rawx, rawy, &(side->reqx), &(side->reqy));
	    if (Debug) printf("Host %s returns map %d %d\n",
			      side->host, side->reqx, side->reqy);
	} else if (evt.xbutton.window == side->world) {
	    rawx = evt.xbutton.x;  rawy = evt.xbutton.y; 
	    side->reqtype = MAPPOS;
	    w_deform(side, rawx, rawy, &(side->reqx), &(side->reqy));
	    if (Debug) printf("Host %s returns world %d %d\n",
			      side->host, side->reqx, side->reqy);
	} else if (evt.xbutton.window == side->state) {
	    rawx = evt.xbutton.x;  rawy = evt.xbutton.y; 
	    side->reqtype = UNITTYPE;
	    side->requtype = rawy / max(side->hh, side->fh);
	    if (Debug) printf("Host %s returns unit type %d\n",
			      side->host, side->requtype);
	} else if (evt.xbutton.window == side->msg) {
	  if (evt.xbutton.y < side->nh * side->fh / 2)
	    side->reqtype = SCROLLUP;
	  else side->reqtype = SCROLLDOWN;
	}
	break;
    /*case MapNotify:*/
    case Expose:
#if 0
	do {
#endif
	  /* Prevent multiple redisplays in exit robot check */
	  if (side->redisplay_ok
#if 0
	      && evt.xexpose.count == 0
#endif
	      ) {
	    int	count;
	    Region	reg;
	    reg = XCreateRegion();
	    count=0;
	    do { /* burn all expose events for this window.
		    ideally we'd collect a region and just redraw that
		    part, but we aren't that sophisticated yet */
	      XRectangle	rect;

	      /*count++;
	      printf("0x%x %dx%d+%d+%d", evt.xexpose.window,
		     evt.xexpose.width,evt.xexpose.width,
		     evt.xexpose.x,evt.xexpose.y); */
	      rect.x = evt.xexpose.x;
	      rect.y = evt.xexpose.y;
	      rect.width = evt.xexpose.width;
	      rect.height = evt.xexpose.height;
	      XUnionRectWithRegion(&rect,reg, reg);
	    } while (XCheckWindowEvent(sdd(), evt.xexpose.window,
				       ExposureMask, &evt));
	    XSetRegion(sdd(), sd()->textgc, reg);
	    /* printf("%d expose events for window 0x%x burned\n",count,
		   evt.xexpose.window); */
	    if (evt.xexpose.window == side->map) {
	      erase_cursor(side);
	      show_map(side);
	    } else if (evt.xexpose.window == side->msg) {
	      show_note(side,FALSE);
	    } else if (evt.xexpose.window == side->world) {
	      show_world(side);
	    } else if (evt.xexpose.window == side->info) {
	      show_info(side);
	    } else if (evt.xexpose.window == side->prompt) {
	      XSetClipMask(sdd(), sd()->textgc, None);
	      show_prompt(side);
	    } else if (evt.xexpose.window == side->sides) {
	      show_all_sides(side,FALSE);
	    } else if (evt.xexpose.window == side->timemode) {
	      show_timemode(side,FALSE);
	    } else if (evt.xexpose.window == side->clock) {
	      show_clock(side);
	    } else if (evt.xexpose.window == side->state) {
	      XSetRegion(sdd(), sd()->varicongc, reg);
	      XSetRegion(sdd(), sd()->unitgc, reg);
	      XSetRegion(sdd(), sd()->unittextgc, reg);
	      show_state(side,FALSE);
	      XSetClipMask(sdd(), sd()->varicongc, None);
	      XSetClipMask(sdd(), sd()->unitgc, None);
	      XSetClipMask(sdd(), sd()->unittextgc, None);
	    }  else if (evt.xexpose.window == side->help) {
	      side->reqtype = KEYBOARD; /* hoo boy :( */
	      side->reqch = '\014';
	      x_help(side);
	    } else if (evt.xexpose.window == side->main) {
	    } else {
	      printf("WARNING: Unknown window being redisplayed. \n");
	      redraw(side);
	    }
	    XSetClipMask(sdd(), sd()->textgc, None);
	    XDestroyRegion(reg);
	  }
#if 0
	} while (XCheckMaskEvent(sdd(), ExposureMask, &evt));
	side->redisplay_ok = FALSE;
	/* flush_input(side); */
	if (Debug) printf("Host %s exposes itself\n", side->host);
	if (!humanside(side))
	  while (XPending(sdd()) > 0) {
	    process_events(side);
/*** (UK) remove -> ***
#define ESCAPE '\033'
*** <- remove ***/
	    if (!humanside(side) && side->reqtype == KEYBOARD &&
		side->reqch == ESCAPE) {
	      side->humanp = TRUE;
	      numhumans++;
	      init_sighandlers();
	      return;
	    }
	  }
#endif
	break;
    case ConfigureNotify:
	if (Debug) {
	  printf("Want a %dx%d window\n", evt.xconfigure.width,
		 evt.xconfigure.height);
	}
	if (evt.xconfigure.width != side->mw ||
	    evt.xconfigure.height != side->mh) {
	  resize_display(side, evt.xconfigure.width, evt.xconfigure.height);
	  /* set_sizes has been called already */
	  reconfigure_display(side,False);
	} else if (Debug) {
	  printf(" already got one. (must be stacking or relocation event)\n");
	}
	break;
      case ClientMessage:
	break;
    default:
	/* shouldn't this be left in? */
#if 0
        case_panic("event type", evt.type);
#endif
	break;
    }
}

/*** (UK) remove -> ***
#define ESCAPE '\033'
*** <- remove ***/

/* Allow a player to exit robot mode, even if he is the only player. */
exit_robot_check()
{
  Side *side;
  
  for_all_sides(side) {
    if (active_display(side)) {
      side->redisplay_ok = TRUE;
      while (XPending(sdd()) > 0) {
	process_events(side);
	if (!humanside(side) && side->reqtype == KEYBOARD &&
	    side->reqch == ESCAPE) {
	  side->humanp = TRUE;
	  numhumans++;
	  init_sighandlers();
	  return;
	}
      } 
    }
  }
}

/* Freese everything until given side supplies input, use sparingly. */

freeze_wait(side)
Side *side;
{
    XEvent evt;
    char buf[BUFSIZE];
    int nchar;

    if (Debug) printf("Waiting for a %s event\n", side->host);
    flush_input(side);
    while(TRUE) {
	XNextEvent(sdd(), &evt);
	if (evt.type == KeyPress) {
	    nchar = XLookupString(&(evt.xkey), buf, BUFSIZE, NULL, NULL);
	    if (nchar > 0) 
		return *buf;
	    else
		return '\0';
	}
    }  
}

/* Get rid of extra key/mouse clicks *only*. */

flush_input(side)
Side *side;
{
    if (!windows_up) {
	windows_up = TRUE;
	get_input();
	/* fetch initial expose event */
    }
    /* mxconq: display is sometimes not active in single player mode! */
    if (humanside(side) && active_display(side) ) {
	XEvent evt;

	/* discard all pending mouse and keyboard events. */
	while (XCheckMaskEvent(sdd(), KeyPressMask|ButtonPressMask, &evt));
	side->reqtype = GARBAGE;
    }
    side->reqchange = TRUE;
}

/* Trivial abstraction - sometimes other routines like to ensure all output */
/* actually on the screen. */

flush_output(side) 
Side *side; 
{  
    /* mxconq: check for active_display first */
    if (humanside(side) && active_display(side) ) XFlush(sdd());  
    side->reqchange = TRUE;
}

/*** (UK) insert -> ***/
flush_output2(side) 
Side *side; 
{  
    /* mxconq: check for active_display first */
    if (humanside(side) && active_display(side) ) XFlush(sdd());  
}

/*** <- insert ***/

/* General window clearing. */

clear_window(side, win)
Side *side;
Window win;
{
    XClearArea(sdd(), win, 0,0, 0,0, False);
}

/* Draw a single horizontal constant-color bar on the world map.  If part */
/* would not be drawn because of the map's obliqueness, cut it in two and */
/* wrap one of the pieces around. */

draw_bar(side, x, y, len, color)
Side *side;
int x, y, len, color;
{
    int sx1, sx2, sy, sww;

    w_xform(side, x, y, &sx1, &sy);
    w_xform(side, x + len, y, &sx2, &sy);
    sww = side->mm * world.width;
    XSetFillStyle(sdd(), sd()->gc, FillSolid);
    XSetForeground(sdd(), sd()->gc, color);
    if (sx1 < sww && sx2 >= sww) {
	XFillRectangle(sdd(), side->world, sd()->gc,
		       sx1, sy, (unint) sww - sx1, (unint) side->mm);
	XFillRectangle(sdd(), side->world, sd()->gc,
		       0, sy, (unint) sx2 - sww, (unint) side->mm);
    } else {
	sx1 %= sww;
	sx2 %= sww;
	XFillRectangle(sdd(), side->world, sd()->gc,
		       sx1, sy, (unint) sx2 - sx1, (unint) side->mm);
    }
#ifdef STUPIDFLUSH
    XFlush(sdd());
#endif /* STUPIDFLUSH */
}

/* Invert the outline box on the world map.  This is a little tricky, */
/* because we want the lines to run through the middle of the world's */
/* hexes, and because the line drawn should not overlap (or the overlaps */
/* will be doubly inverted and look strange). */

invert_box(side, vcx, vcy)
Side *side;
int vcx, vcy;
{
    int x1, y1, x2, y2, sx1, sy1, sx2, sy2, mm2 = side->mm/2;

    x1 = vcx - side->vw2 + side->vh2/2;  y1 = vcy - side->vh2;
    x2 = vcx + side->vw2 - side->vh2/2;  y2 = vcy + side->vh2;
    w_xform(side, x1, y1, &sx1, &sy1);
    w_xform(side, x2, y2, &sx2, &sy2);
    sx1 += mm2;  sy1 -= mm2;  sx2 += mm2;  sy2 += mm2;
    XSetFunction(sdd(), sd()->gc, GXinvert);
    /* is this next call really necessary? */
    XSetLineAttributes(sdd(), sd()->gc, 1, LineSolid, CapButt, JoinMiter);
    XDrawLine(sdd(), side->world, sd()->gc, sx1, sy1, sx2, sy1);
    XDrawLine(sdd(), side->world, sd()->gc, sx2, sy1-1, sx2, sy2+1);
    XDrawLine(sdd(), side->world, sd()->gc, sx2, sy2, sx1, sy2);
    XDrawLine(sdd(), side->world, sd()->gc, sx1, sy2+1, sx1, sy1-1);
    XSetFunction(sdd(), sd()->gc, GXcopy);
}

/* This interfaces higher-level drawing decisions to the rendition of */
/* individual pieces of display. */

draw_terrain_row(side, sx, sy, buf, len, color)
Side *side;
int sx, sy, len, color;
char *buf;
{
    sy += sd()->iconfont->max_bounds.ascent;
    XSetForeground(sdd(), sd()->icongc, color);
    XDrawString(sdd(), side->map, sd()->icongc, sx, sy, buf, len);
#ifdef STUPIDFLUSH
    XFlush(sdd());
#endif /* STUPIDFLUSH */
}

/* Flash a pair of lines up, slow enough to draw the eye, but not so slow */
/* as to get in the way. */

flash_position(side, sx, sy, tm)
Side *side;
int sx, sy, tm;
{
    int sx1, sy1, sx2, sy2;

    if (tm > 0) {
	sx1 = sx - 50 + side->hw/2;  sy1 = sy + 50 + side->hch/2;
	sx2 = sx + 50 + side->hw/2;  sy2 = sy - 50 + side->hch/2;
	XDrawLine(sdd(), side->map, sd()->flashgc, sx1, sy1, sx2, sy2);
	XDrawLine(sdd(), side->map, sd()->flashgc, sx1, sy2, sx2, sy1);
	flush_output(side);
	nap(tm);
	XDrawLine(sdd(), side->map, sd()->flashgc, sx1, sy1, sx2, sy2);
	XDrawLine(sdd(), side->map, sd()->flashgc, sx1, sy2, sx2, sy1);
    }
}

/* Napping is like sleeping, but maybe shorter.  Arg is milliseconds. */

nap(t)
long t;
{
#ifdef UNIX
#ifdef SELECT2
    struct timeval naptime;

    naptime.tv_usec = (t % 1000) * 1000;
    naptime.tv_sec = t / 1000;
    select(0, NULL, NULL, NULL, &naptime);
#else
    if (t >= 1000) sleep(t/1000);
#endif
#endif /* UNIX */
}

/* The "cursor icon" is just a pair of special chars - nothing to do with */
/* X's notion of cursors. */

draw_cursor_icon(side, sx, sy)
Side *side;
int sx, sy;
{
    sy += sd()->iconfont->max_bounds.ascent;
    XDrawString(sdd(), side->map, sd()->invicongc, sx, sy, "[", 1);
    XSetForeground(sdd(), sd()->icongc, side->fgcolor);
    XDrawString(sdd(), side->map, sd()->icongc, sx, sy, "]", 1);
}

/* Draw the icon for a hex (given as a char). */

draw_hex_icon(side, win, sx, sy, color, ch)
Side *side;
Window win;
int sx, sy, color;
char ch;
{
    XSetForeground(sdd(), sd()->varicongc, color);
    sy += sd()->iconfont->max_bounds.ascent;
    XDrawString(sdd(), win, sd()->varicongc, sx, sy, &ch, 1);
}

/* Draw the number of an unfriendly side (never called for own units). */

draw_side_number(side, win, sx, sy, n, color)
Side *side;
Window win;
int sx, sy, n, color;
{
    char ch = n%22;

    ch += (ch<10)?'0':('A' - 10);

    if (n >= 0 && n!=MAXSIDES) {
	XSetForeground(sdd(), sd()->varicongc, color);
	sy += sd()->iconfont->max_bounds.ascent;
	XDrawString(sdd(), win, sd()->varicongc, sx, sy, &ch, 1);
    }
}

/*** (HW) insert -> ****/
draw_info_char(side, win, sx, sy, ch, color)
Side *side;
Window win;
int sx, sy;
char ch;
int color;
{
    sx -= 12; /* draw info char on the left side */
    XSetForeground(sdd(), sd()->varicongc, color);
    sy += sd()->iconfont->max_bounds.ascent-1;
    XDrawString(sdd(), win, sd()->varicongc, sx, sy, &ch, 1);
}
/*** <- insert ***/

draw_blast_icon(side, win, sx, sy, type, color)
Side *side;
Window win;
int sx, sy, color;
char type;
{
    char buf[1];

    XSetForeground(sdd(), sd()->varicongc, color);
    buf[0] = type;
    sy += sd()->iconfont->max_bounds.ascent;
    XDrawString(sdd(), win, sd()->varicongc, sx, sy, buf, 1);
    flush_output(side);
}

/* Flash the player's screen in an unmistakable way. */

invert_whole_map(side)
Side *side;
{
    int sw = side->vw * side->hw, sh = side->vh * side->hh;

    /* GC needs to be set for inverted drawing */
    XDrawRectangle(sdd(), side->map, sd()->gc, 0, 0, sw, sh);
    flush_output(side);
}

/* Draw just one of the mushroom cloud shapes. */

draw_mushroom(side, x, y, i)
Side *side;
int x, y, i;
{
    int sx, sy;
    int color;

    color = ((side->monochrome || i == 3) ? side->fgcolor : side->bgcolor);
    xform(side, unwrap(side, x, y), y, &sx, &sy);
    XSetForeground(sdd(), sd()->unitgc, color);
    XSetClipMask(sdd(), sd()->unitgc, sd()->bombpics[i]);
    XSetClipOrigin(sdd(), sd()->unitgc, sx-BH/4, sy-BH/2);
    XFillRectangle(sdd(), side->map, sd()->unitgc, sx-BH/4, sy-BH/2, BH, BH);
    flush_output(side);
}

/* Confirm that we can indeed do bar graph displays. */

bar_graphs(side) Side *side;  {  return TRUE;  }

/* Do yet another X-toolkit-type function.  This draws a bar graph. */
/*** (HW)
     The total amount will be also shown below the bar graph.
***/

draw_graph(side, number, amount, total, critical, title)
Side *side;
int number, amount, total, critical;
char *title;
{
/*** (UK) add -> ***/
    char buf[100];
/*** <- add ***/
    int boxwidth, boxheight, boxoffset, boxleft, barwidth, barheight;

    if (total > 0) {
/*** (HW) add -> ***/
	sprintf(buf,"%c%c(%d)",*title,*(title+1),amount);
/*** <- add ***/
	boxwidth = 8*side->fw;
	boxheight = (INFOLINES-1)*side->fh - 2*side->margin;
	boxoffset = side->margin;
	boxleft = 50*side->fw + number * boxwidth;
	barwidth = boxwidth / 3;
	barheight = (boxheight * amount) / total;
	XSetForeground(sdd(), sd()->gc, side->fgcolor);
	XFillRectangle(sdd(), side->info, sd()->gc,
		       boxleft + boxwidth/3 - 1, boxoffset - 1,
		       barwidth + 2, boxheight + 2);
	XSetForeground(sdd(), sd()->gc, side->bgcolor);
	XFillRectangle(sdd(), side->info, sd()->gc,
		       boxleft + boxwidth/3, boxoffset,
		       barwidth, boxheight);
	if ( amount > critical)
	  XSetForeground(sdd(), sd()->gc, side->goodcolor);
	else
	  XSetForeground(sdd(), sd()->gc, side->badcolor);
	XFillRectangle(sdd(), side->info, sd()->gc,
		       boxleft + boxwidth/3, boxoffset + boxheight - barheight,
		       barwidth, barheight);
/*** (HW) change -> ***/
	draw_text(side, side->info,
		  boxleft+(boxwidth-strlen(buf)*side->fw)/2,
		  (INFOLINES-1)*side->fh, buf, side->fgcolor);
/*** was:
	draw_text(side, side->info,
		  boxleft+(boxwidth-strlen(title)*side->fw)/2,
		  (INFOLINES-1)*side->fh, title, side->fgcolor);
*** <- change ***/
    }
}

/* Splash a unit image (either bitmap or font char) onto some window. */

draw_unit_icon(side, win, x, y, u, color)
Side *side;
Window win;
int x, y, u, color;
{
    char buf[1];
    y += 3;			/* fudge factor to make x11 look */
    x += 2;			/*  like X10 (ugh). */
    if (utypes[u].bitmapname != NULL ) {
      XSetForeground(sdd(), sd()->unitgc, color);
      XSetClipMask(sdd(), sd()->unitgc, sd()->unitpics[u]);
      XSetClipOrigin(sdd(), sd()->unitgc, x, y);
      XFillRectangle(sdd(), win, sd()->unitgc, x, y, side->uw, side->uh);
    } else {
        XSetForeground(sdd(), sd()->unittextgc, color);
	buf[0] = utypes[u].uchar;
	y += sd()->unitfont->max_bounds.ascent;
	XDrawString(sdd(), win, sd()->unittextgc, x, y, buf, 1);
    }
}

/* General text drawer. */

draw_text(side, win, x, y, str, color)
Side *side;
Window win;
int x, y, color;
char *str;
{
  if (win == side->help) draw_help_text(side, win, x, y, str, color);
  else {
    y += sd()->textfont->max_bounds.ascent;
    if (color != side->bgcolor) {
	XSetForeground(sdd(), sd()->textgc, color);
	XDrawImageString(sdd(), win, sd()->textgc, x, y, str, strlen(str));
    } else {
	XSetForeground(sdd(), sd()->textgc, side->bgcolor);
	XSetBackground(sdd(), sd()->textgc, side->fgcolor);
	XDrawImageString(sdd(), win, sd()->textgc, x, y, str, strlen(str));
	XSetBackground(sdd(), sd()->textgc, side->bgcolor);
    }}
#ifdef STUPIDFLUSH
    XFlush(sdd());
#endif /* STUPIDFLUSH */
}

/* General text drawer. */

draw_help_text(side, win, x, y, str, color)
Side *side;
Window win;
int x, y, color;
char *str;
{
    y += sd()->helpfont->max_bounds.ascent;
    if (color != side->bgcolor) {
	XSetForeground(sdd(), sd()->helpgc, color);
	XDrawString(sdd(), win, sd()->helpgc, x, y, str, strlen(str));
    } else {
	XSetForeground(sdd(), sd()->helpgc, side->bgcolor);
	XSetBackground(sdd(), sd()->helpgc, side->fgcolor);
	XDrawString(sdd(), win, sd()->helpgc, x, y, str, strlen(str));
	XSetBackground(sdd(), sd()->helpgc, side->bgcolor);
    }
}

/* Draw a line through some side's title. */

draw_scratchout(side, pos)
Side *side;
int pos;
{
    XSetForeground(sdd(), sd()->textgc, side->fgcolor);
    XDrawLine(sdd(), side->sides, sd()->textgc, 0, pos, 30*side->fw, pos);
}

/* Beep the beeper! */

beep(side)
Side *side;
{
    XBell(sdd(), sdds());
}

/* Little routines to pop up the help window and make it go away again */
/* They only get called when display is in use. */

reveal_help(side)
Side *side;
{
    XEvent evt;
    XWindowAttributes	xwa;

    XMapWindow(sdd(), side->help);
    XGetWindowAttributes(sdd(), side->help, &xwa);
/*** (HW) remove ->
    printf("window state: %d\n",xwa.map_state);
*** <- remove ***/
    if (xwa.map_state == IsUnmapped) {
      /* wait until this window is exposed to return. */
      XWindowEvent(sdd(), side->help, ExposureMask, &evt);
    }
    return TRUE;
}

conceal_help(side)
Side *side;
{
    XUnmapWindow(sdd(), side->help);
    flush_output(side);
}

dump_view(side)
Side *side;
{
  /* Implement when we figure out how to do full map dumps. */
}

/* Shut a single display down, but only if there's one to operate on. */
/* Hit the display slot, for safety. */

close_display(side)
Side *side;
{
    XrmDestroyDatabase((XrmDatabase) side->rmdatabase);
    XCloseDisplay(sdd());
    side->display = (long) NULL;
}

/*** (HW) insert -> ***/
clip_tohex(x1,y1,x2,y2,d)
int *x1,*y1,*x2,*y2,d;
{
  int dx = abs(*x2-*x1), dy = abs(*y2-*y1);
  int delx,dely;
  double sqrt();

  delx = (int)((double)d /
	       sqrt(1.0+(double)dy*(double)dy/((double)dx*(double)dx)));
  dely = (int)((double)(dy*delx)/(double)dx);

  if (x1 > x2) { *x1 += delx; *x2 -= delx; }
  else { *x1 -= delx; *x2 += delx; }
  if (y1 > y2) { *y1 += dely; *y2 -= dely; }
  else { *y1 -= dely; *y2 += dely; }
}

void DoArrowComputation(Ax,Ay,Bx,By,points)
int Ax,Ay,Bx,By;
XPoint *points;
{ double Vx, Vy, norm_fact;
  static int arrow_wid = 6, arrow_len = 6;
  double sqrt();

  if (Ax == Bx && Ay == By) norm_fact = 1.0;
  else norm_fact = sqrt((double)((Bx-Ax)*(Bx-Ax) + (By-Ay)*(By-Ay)));
  Vx = (double)(Bx-Ax)/norm_fact;
  Vy = (double)(By-Ay)/norm_fact;
  points[0].x = (Ax + arrow_len * Vx + (arrow_wid/2) * (-Vy));
  points[0].y = (Ay + arrow_len * Vy + (arrow_wid/2) * Vx);
  points[1].x = (points[0].x - arrow_wid * (-Vy));
  points[1].y = (points[0].y - arrow_wid * Vx);
  points[2].x = Ax; points[2].y = Ay;
}

set_line_mode(side)
Side *side;
{
  long col1,col2,fgcolor,bgcolor;
  int func;
  unsigned long valuemask;
  XGCValues values;
  
  switch (side->linemode) {
    case NORMAL_COLS:
      col1 = side->fgcolor;
      col2 = side->bgcolor;
      func = GXcopy;
      break;
    case INVERTED_COLS:
      col1 = side->bgcolor;
      col2 = side->fgcolor;
      func = GXcopy;
      break;
    case ALWAYSWHITE:
      col1 = col2 = white_color(side);
      func = GXcopy;
      break;
    case ALLINVERTED:
      col1 = side->bgcolor;
      col2 = side->fgcolor;
      func = GXinvert;
      break;
    default:
      col1 = side->fgcolor;
      col2 = side->bgcolor;
      func = GXcopy;
      break;
  }
  if (side->monochrome||side->showmode==TERRICONS|| side->showmode==ONEICON) {
    fgcolor = col1; bgcolor = (col1 == col2) ? black_color(side) : col2;
  }
  else {
    fgcolor = col2; bgcolor = (col1 == col2) ? black_color(side) : col1;
  }
  valuemask = GCForeground | GCBackground | GCFunction;
  values.foreground = fgcolor;
  values.background = bgcolor;
  values.function = func;
  XChangeGC(sdd(),sd()->sordergc,valuemask,&values);
}

draw_line(side,win,ordertype,x1,y1,x2,y2)
Side *side;
Window win;
char ordertype;
int x1,y1,x2,y2;
{
  int corrx = side->hw/2, corry = side->hch/2;
  int thickness, style;

  x1 += corrx; y1 += corry; x2 += corrx; y2 += corry;
  if (ordertype == 'm') {
    thickness = (side->thicknessmode & 0x1) ? 2 : 0;
  } else {
    thickness = (side->thicknessmode & 0x2) ? 2 : 0;
  }
  if (!side->distinctsotypes || ordertype == 'p' || ordertype == 'm') 
    style = LineSolid;
  else
    style = (ordertype == 'e' ? LineOnOffDash : LineDoubleDash);
  /* clip_tohex(&x1,&y1,&x2,&y2,corr/2); */
  XSetLineAttributes(sdd(),sd()->sordergc,thickness,
		     style,CapButt,JoinMiter);
  XDrawLine(sdd(),side->map,sd()->sordergc,x1,y1,x2,y2);
}

draw_arrow_line(side,win,ordertype,x1,y1,x2,y2)
Side *side;
Window win;
char ordertype;
int x1,y1,x2,y2;
{
  int corrx = side->hw/2, corry = side->hch/2;
  int thickness, style;
  XPoint points[3];

  x1 += corrx; y1 += corry; x2 += corrx; y2 += corry;
  if (ordertype == 'm') {
    thickness = (side->thicknessmode & 0x1) ? 2 : 0;
  } else {
    thickness = (side->thicknessmode & 0x2) ? 2 : 0;
  }
  if (!side->distinctsotypes || ordertype == 'p' || ordertype == 'm') 
    style = LineSolid;
  else
    style = (ordertype == 'e' ? LineOnOffDash : LineDoubleDash);
  /* clip_tohex(&x1,&y1,&x2,&y2,corr/2); */
  XSetLineAttributes(sdd(),sd()->sordergc,thickness,
		     style,CapButt,JoinMiter);
  XDrawLine(sdd(),side->map,sd()->sordergc,x1,y1,x2,y2);
  DoArrowComputation(x2,y2,x1,y1,points);
  XFillPolygon(sdd(),side->map,sd()->sordergc,points,3,Convex,CoordModeOrigin);
}
/*** <- insert ***/


/*** (UK) insert -> ***/
extern int server_port;

init_server()
{ struct sockaddr_in sin;
  int ret;

  bzero((char*)&sin,sizeof(sin));
  sin.sin_port=server_port;
  sock = socket(AF_INET, SOCK_STREAM, 0);
  ret=bind(sock, (struct sockaddr*)&sin, sizeof(sin));
  if (ret && (errno==EADDRINUSE || errno==EINTR)) {
    printf("address already in use: retrying");
    while ((ret=bind(sock, (struct sockaddr*)&sin, sizeof(sin))) < 0 &&
	    (errno==EADDRINUSE || errno==EINTR)) {
      printf(".");
      fflush(stdout);
      sleep(2);
    }
    printf("\n");
  }
  if (ret<0) {
    perror("bind");
    printf("ret=%d\n",errno);
    close(sock);
    sock=-1;
    return 1;
  }
  listen(sock, 5);
  return 0;
}

#include "request.h"

gwrite(g,s)
char *s;
{ write(g,s,strlen(s)+1);
}

handle_connection()
{ int g;
  struct sockaddr_in from;
  struct hostent *hp;
  XCRequest buf;
  Side *side;
  Side *side2;
  short s;
  short c;

  int len=sizeof(from);

  g = accept(sock, (struct sockaddr*)&from, &len);
  if (g<0) return;

  hp=gethostbyaddr((char*)&from.sin_addr,sizeof(from.sin_addr),from.sin_family);
  /*
  printf("request from %d.%d.%d.%d (%s)\n",
     from.sin_addr.S_un.S_un_b.s_b1,
     from.sin_addr.S_un.S_un_b.s_b2,
     from.sin_addr.S_un.S_un_b.s_b3,
     from.sin_addr.S_un.S_un_b.s_b4, (hp?hp->h_name:"unknown"));
  */
  if (read(g,&buf,sizeof(buf))!=sizeof(buf)) {
    printf("   illegal request\n");
  }
  else {
    printf("  side %d host %s ",ntohs(buf.side),buf.display);
    s=ntohs(buf.side);
    c=ntohs(buf.cmd);
    if (s>=0) side=side_n(ntohs(buf.side));
    else {
      side=0;
      for_all_sides(side2) {
/*	printf("'%s '%s'\n",side2->host,buf.display); */
	if (!strcmp(side2->host,buf.display)) side=side2;
      }
      if (side) printf("side number %d ",side_number(side));
    }
    if (side) {
      switch (c) {
	case XC_LOGIN:
	  printf("LOGIN\n");
	  if (active_display(side)) {
	    printf("side already active!\n");
	    gwrite(g,"Side is already active");
	  }
	  else {
	    if (side->passwd && (strcmp(side->passwd,buf.passwd))) {
	      printf("wrong password!\n");
	      gwrite(g,"Wrong password");
	    }
	    else {
	      if (side->host) free(side->host);
	      side->host=copy_string(buf.display);
	      if (init_display(side)) {
		Side *side2;
		init_curxy(side);
		notify_all("The %s have logged in!",plural_form(side->name));
		for_all_sides(side2) show_all_sides(side2,TRUE);
		gwrite(g,"You have logged in");
	      }
	      else gwrite(g,"cannot open display");
	    }
	  }
	  break;
	case XC_QUERY:
	  { char *m;
	    char buf[1024];
	    char other[100];

	    printf("QUERY\n");
	    if (side->more_units) sprintf(buf,"There are more units to move.");
	    else sprintf(buf,"There is nothing to do.");
	    for_all_sides(side2) {
	      if (side2->more_units || active_display(side2)) {
		sprintf(other,"\n Side %d (%s): ",
			      side_number(side2),side2->name);
		strcat(buf,other);
		if (active_display(side2)) {
		  strcat(buf,"logged in");
		  if (side2->more_units) strcat(buf,",");
		}
		if (side2->more_units) strcat(buf,"units to move");
	      }
	    }
	    gwrite(g,buf);
	  }
          break;
	case XC_SAVE:
	  printf("SAVE\n");
	  if (!write_savefile(SAVEFILE)) gwrite(g,"Cannot write savefile.");
	  gwrite(g,"Game saved.");
	  break;
	case XC_QUIT:
	  printf("QUIT\n");
	  if (side->passwd && (strcmp(side->passwd,buf.passwd))) {
	    printf("wrong password!\n");
	    gwrite(g,"Wrong password");
	  }
	  else {
	    gwrite(g,"Game will be quitted.");
	    y_exit(side,NULL,TRUE);
	  }
	  break;

	default:
	  printf("unknown command\n");
	  gwrite(g,"Unknown command");
	  break;
      }
    }
    else {
      printf("unknown side number!\n");
      gwrite(g,"Unknown side number");
    }
  }
  close(g);
  printf("request handled\n"); fflush(stdout);
}
/*** <- insert ***/ 
