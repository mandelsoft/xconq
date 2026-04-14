/* Copyright (c) 1987, 1988, 1991  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */


#include <stdio.h>
#include <ctype.h>

/* If you don't have "strcasecmp", but do have "stricmp", use this
   cheap substitute.  If you don't have either see the BSD_O line in the
   Imakefile */

/* #define strcasecmp stricmp  */


/* This is where predefined maps/scenarios/periods/fonts can be found. */

#ifndef XCONQLIB
#define XCONQLIB "lib"
#endif  XCONQLIB



/* The newsfile always lives in the lib directory. */

#define NEWSFILE "xconq.news"

/* The name of the savefile.  It will be put in the current directory. */
/* If SAVEFILE cannot be written or in case of error, write SAVEFILE2. */

#define SAVEFILE "save.xconq"
#define SAVEFILE2 "/tmp/emergency.save.xconq"

/* This file gets the parameter listing for the period in use. */
/* It will also be created in the current directory. */

#define PARMSFILE "parms.xconq"

/* This file gets the game statistics when it's all over. */
/* It will also be created in the current directory. */

#define STATSFILE "stats.xconq"

/* This file gets a printout of the side's view. */

#define VIEWFILE "view.xconq"

/* This file gets a list of commands as they appear in the help window. */

#define CMDFILE "cmds.xconq"

/* Default random map sizes.  Adjust these to taste - 60x60 is a moderate */
/* length game, 30x30 is short, 360x120 is L-O-N-G ! */

#define DEFAULTWIDTH 60
#define DEFAULTHEIGHT 30

/* Absolute maximum number of sides that can play.  This limit can only be */
/* extended by changing the representation of views of players from bytes */
/* to something larger, thereby doubling (at least) space requirements. */

#define MAXSIDES 31

/* Absolute maximum number of kinds of units. (same restriction as above) */

#define MAXUTYPES 70

/* Maximum number of types of natural resources.  This number can be set */
/* higher, in fact I think the only limitation is that there won't be */
/* enough distinct chars, but more rtypes means larger unit objects. */

#define MAXRTYPES 6

/* Maximum number of terrain types.  Must be fewer than 256, but also */
/* limited by display capabilities. */

#define MAXTTYPES 20

/* Maximum number of random side names that can be defined. */

#define MAXSNAMES 200

/* Maximum number of random unit names that can be defined. */

#define MAXUNAMES 1000

/* The maximum number of mapfiles that can be in menus. (Not a limit on the */
/* total number of files that can exist, however.) */

#define MAXMAPMENU 100

/* The maximum number of mapfiles that can be loaded into a game (recursive */
/* loads are not performed and not counted). */

#define MAXLOADED 16

/*** (UK) insert -> ***/
/* number of save records */

#define MAXSAVESTATE 20	

/* default port of server */

#define SERVER_PORT 4712
/*** <- insert ***/

/* Default game length in turns. */

#define DEFAULTTURNS 1000

/* Number of messages displayed at one time. No upper limit I believe, */
/* but too many won't fit on the screen.  The actual numbers of lines */
/* displayed can be changed by the player, subject to limitations on the */
/* screen space available. */

#define MAXNOTES 45

/* Default color of text and icons - 0 is for white on black, 1 */
/* is for black on white.  Should be set appropriately for the most */
/* common monochrome display (color displays always do white on black). */
/* This is also settable by the player, so the default is just for */
/* convenience of the majority. */

#define BLACKONWHITE 1

/* When true, displays will use more graphics and less text.  This can */
/* also be toggled by players individually. */

#define GRAPHICAL 0

/* The default fonts can be altered by users, so these are just hints. */
/* These options do not necessarily apply to non-X versions. */

#define TEXTFONT "fixed"
#define HELPFONT "fixed"
#define ICONFONT "xconq"

/* All names, phrases, and messages must be able to fit in statically */
/* allocated buffers of this size. */

#define BUFSIZE 300
#define NOTIFY_SPACE  MAXNOTES * 60 * MAXSIDES + BUFSIZE

/* If defined, a statistics file is written at the end of the game. */
/* The numbers therein are only for serious gamers, and the files can */
/* be embarassing clutter in your directory! */

#define STATISTICS

/* Initial limit on units and cities that can be active at one time.  If */
/* growable option is enabled, will try to grow the array to hold more. */

#define INITMAXUNITS  500
#define GROWABLE

/* When this is enabled, machine players will be able to examine humans' */
/* units rather more closely than is possible in reverse.  In particular, */
/* a machine will know just where the human units are, as well as their */
/* current attributes (like hit points). */

/* #define CHEAT */

/* Some X11 servers die if too much is written between output flushes. */
/* This can be used with any graphics system with a similar problem; */
/* this option does not affect correctness, but may impact performance. */

#define STUPIDFLUSH

/* Beep if this much time has elapsed between turns. */

#define DEFAULT_STARTBEEPTIME 10

/* Unset this to compile on small or slow machines.  The machine */
/* player will be adversely affected, but the interactive game should */
/* be the same. */

#define LARGE

#ifdef LARGE

/* Save previous views or info about when we saw a hex.  Costs 3 bytes / hex */

#define PREVVIEW

/* Compute regions that units can move in.  Helps machine player plan */
/* moves and productions.  Costs 2 bytes / hex / unit type plus a fair */
/* amount of startup time for large maps. */

#define REGIONS

#endif /* LARGE */

/* This defines some extra commands.  Eliminating these commands will */
/* not seriously affect the capabilities of the game, but will reduce */
/* the size of the program. */

#define EXTRA_COMMANDS

/* Determine how many hexes across one area of control is for the machine.  */
#define AREA_SIZE 10
