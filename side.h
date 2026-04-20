/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Definitions about sides need terrain.h, unit.h, map.h */

/* Modes governing the interpretation of input. */

#define WEDGED    0
#define MOVE      1
#define SURVEY    2

/* These are 7-char strings (except for the first) so hiliting works right. */

#define MODENAMES { "       ", "  Move ", " Survey" } 

/* Reasons for gains and losses of units. */

#define NUMREASONS 14

#define FIRSTUNIT   0
#define PRODUCED    1
#define CAPTURE     2
#define DUMMYREAS   3
#define COMBAT      4
#define PRISONER    5
#define GARRISON    6
#define DISASTER    7
#define STARVATION  8
#define DISBAND     9
#define SURRENDER  10
#define VICTOR     11
#define ENDOFWORLD 12
#define GIFT       13

#define REASONNAMES \
  { "Ini", "Pro", "Cap", "   ", "Cbt", "Pri", "Gar", "Acc", "Sup", "Dis", \
    "Sur", "Win", "End", "Gift" }

/* Input request results - tells what came back as answer to a request. */

#define GARBAGE  0
#define KEYBOARD 1
#define MAPPOS   2
#define UNITTYPE 3
#define SCROLLUP 4
#define SCROLLDOWN 5

/* Color display modes. */

/*** (HW) change -> ***/
#define NUMSHOWMODES 5
/*** was:
#define NUMSHOWMODES 4
*** <- change ***/

#define FULLHEX   0
#define BORDERHEX 1
#define TERRICONS 2
#define BOTHICONS 3
/*** (HW) insert -> ***/
#define ONEICON   4
/*** <- insert ***/

/*** (HW) insert -> ***/
#define NUMLINEMODES 4

#define NORMAL_COLS 0
#define INVERTED_COLS 1
#define ALWAYSWHITE 2
#define ALLINVERTED 3

#define NUMTHICKNESSMODES 4
/*** <- insert ***/

/* Attitude boundaries of one side with respect to another. */

#define ENEMY   (-100)
#define NEUTRAL     0
#define ALLY      100

/* Possible ideologies of a machine player - determines general behavior. */
/* Very crude - really more for color than anything else. */

#define EXPANSIONIST 0
#define ISOLATIONIST 1
#define FASCIST      2
#define FANATICAL    3
#define DEMOCRATIC   4

/* An area of control is a block that is treated as a single */
/* location. All information on the whole area is combined so that the */
/* machine can do some planning.  */

typedef struct a_control_area {
  short enemy_strength;   /* Estimate of enemy unit strength */
  short enemy_seen_recently; /* How many enemy have we seen in last */
			     /* turn. */
  short units_lost;       /* How many units have we lost here. */
			  /* Weighted average of last few turns. */
  short capturers_approaching; /* number of units comming to capture bases */
  short capture_time;     /* When do we expect to have capturers here */
  short safe_area;        /* do we really need to move units here */
} Control_area;
			     
typedef unsigned short viewdata;

/*** (UK) insert -> ***/
typedef struct a_save_record {
  bool used;
  int x;
  int y;
} SaveState;
/*** <- insert ***/

/* Each xconq player is a "side" - more or less one country.  A side may or */
/* may not be played by a person, and may or may not have a display attached */
/* to it.  Each side has a different view of the world.  Zillions of slots */
/* are needed because each side needs to keep its own interaction/display */
/* data - a side is almost like a full-blown process... */

typedef struct a_side {
    /* Level 1 detail */
    char *name;               /* name used for display */
    /* Level 2 detail */
    short ideology;           /* what kind of people are on this side */
    short attitude[MAXSIDES];   /* war/peace/ally status of other sides */
    short counts[MAXUTYPES];  /* array of numbers for identifying units */
    /* Level 3 detail */
    viewdata *view;      /* pointer to array of view info */
#ifdef LARGE
    short *viewtimestamp;     /* indicates when we last looked at this hex */
    viewdata *prevview;  /* what we saw before what we see now. */
#endif
    /* Level 4 detail */
    char *host;               /* which host is this side attached to */
    short humanp;             /* is this side played by a person? */
    short lost;               /* true if this side was knocked out */
    short cx, cy;             /* current center of player's focus */
    struct a_unit *markunit;  /* unit being remembered for later use */
    unsigned timeleft;           /* seconds left for this side to play */
    unsigned timetaken;          /* seconds played by this side */
    short timedout;           /* true when clock has run out for this side */
    short itertime;           /* length of order repetition */
    short graphical;          /* if true, use bar graphs for unit info */
    short bonw;               /* true if display is black-on-white */
    short showmode;           /* one of four color display modes */
    /* Statistics */
    short balance[MAXUTYPES][NUMREASONS];  /* what happened to units */
    short atkstats[MAXUTYPES][MAXUTYPES];  /* how many attacks */
    short hitstats[MAXUTYPES][MAXUTYPES];  /* how many hits */
    /* Never saved */
/*** (UK) insert -> ***/
    char *curgroup;           /* current group for defining standing orders */
/*** <- insert ***/
    struct a_unit *unithead;  /* all units on the side */
    short mode;               /* player's mode (move/survey) */
    short curx, cury;         /* current spot being looked at */
    struct a_unit *curunit;   /* unit under cursor */
    struct a_unit *movunit;   /* unit being moved currently */
    struct a_unit *last_unit; /* last unit moved */
    short more_units;		/* TRUE if more units to more this turn */
    short directorder;        /* true if order has just been given */
    short *coverage;          /* indicates how many looking at this hex */
    short units[MAXUTYPES];   /* cached count of units */
    short resources[MAXRTYPES];  /* cached count of resources */
    short building[MAXUTYPES];  /* cached count of units being built */
    long lasttime;            /* when clock started counting down again */
    short followaction;       /* move to where a change has occured */
    /* machine player */
    long plan;                /* all the machine's strategy and tactics */
    struct a_unit *unitlist[MAXUTYPES];   /* lists to help mplay efficiency */
    short unitlistcount[MAXUTYPES];  /* counts of above lists */
    int numunits;             /* number of units the side has */
    /* Higher level control information for machine */
    Control_area *areas;
    /* input hacking */
/*** (HW) insert -> ***/
    int helpstate;            /* current help page number */
    int curnote;              /* current note line number in help window */
    bool show_product;        /* show products of units instead of units */
    bool show_only_producers; /* show only producers instead of all units */
    bool show_no_movers;      /* show no movers */
    bool show_orders;         /* show orders of units */
    short showsorderutype;    /* show standing move orders for this utype */
    short showsorderlevel;    /* internal show order level to reduce redraw */
    bool showsorderflag;      /* flag showing a necessary redraw */
    bool notifyvisibility;    /* show unseen terrain in diff. colors */
    short linemode;           /* mode for drawing move sorder arrows */
    short thicknessmode;      /* move order/sorder lines thick or thin */
    bool distinctsotypes;     /* distinct p,e,f order types by dashing */
    int clip_xmin, clip_xmax, clip_ymin, clip_ymax;
			      /* clip area for line drawing */
    bool redraw;              /* flag for redrawing */
/*** <- insert ***/
    bool reqactive;           /* true if request made but yet unfulfilled */
    int (*reqhandler)();      /* function to call to process fulfilled req */
    int reqtype;              /* what sort of response was made to req */
    char reqch;               /* keyboard char */
    int reqx, reqy;           /* map coordinates */
    int requtype;             /* unit type number */
    /* Random input data slots - could be merged/simplified? */
    struct a_side *reqoside;
    struct a_unit *requnit;
    struct a_unit *tmpcurunit;  /* saved value of curunit */
    int reqvalue, reqvalue2;
/*** (UK) insert -> ***/
    int reqtvalue;             /* used by input system */
    int reqsvalue;             /* not used by input system (save) */
    int (*reqnexthandler)();   /* function to call for fulfilled ask req */
    bool workmode;	       /* workmode active? */
    bool w_show_product;       /* saved modes for working mode */
    bool w_show_only_producers;
    bool w_show_no_movers;
    bool w_show_orders;
    bool w_followaction;
    short w_showsorderutype;
    int  savmode;              /* saved info for seen enemy */
    bool savshow_product;
    bool savshow_only_producers;
    bool savshow_no_movers;
    bool savflag;	       /* values saved? */
    struct a_unit *savcurunit; /* saved value of curunit */
    int savcurx,savcury;       /* saved coordinates */
    bool enemyseen;	       /* enemy seen in current turn */
    int seenx, seeny;          /* coordinates of last seen enemy  */
    SaveState savstate[MAXSAVESTATE];        /* saved positions */
    short last_x,last_y;       /* last screen position */ 
    char *passwd;              /* password */
/*** <- insert ***/
    int reqstrbeg, reqcurstr;  /* data about string under construction */
    char *reqdeflt;           /* A default string */
    int tmpcurx, tmpcury;     /* saved values of curx,cury */
    int reqposx, reqposy;     /* accumulator for position reading */
    char ustr[MAXUTYPES];     /* used in composing unit type hints */
    int uvec[MAXUTYPES];      /* vector of allowed unit types to input */
    int bvec[MAXUTYPES];      /* bit vector of allowed unit types to input */
/*** (UK) insert -> ***/
    char tstr[MAXTTYPES];     /* used in composing terrain type hints */
    int tvec[MAXTTYPES];      /* vector of allowed terrain types to input */
    int tbvec[MAXTTYPES];      /* bit vector of allowed terrain types to input */
/*** <- insert ***/
    /* Machinery for standing orders */
    bool teach;               /* true when only setting a standing order */
/*** (UK) insert -> ***/
    int somode;		      /* standing order type (UK) */
    bool sosimple;	      /* simple standing order definition */
    char *sogroup;	      /* standing order group */
    struct a_scondition *socond;/* condition for standing order */
    char *curprompt;          /* current base prompt */
    bool compactmode;	      /* compact display of standing orders */
/*** <- insert ***/
    struct a_unit *sounit;    /* unit that stores standing order for occs */
    int soutype;              /* unit type that will get standing orders */
    struct a_order *tmporder; /* holding place for orders */
    /* Constructed during display init */
    short monochrome;         /* obvious */
/*** (HW) insert -> ***/
    int svw,svh;              /* viewport width and height in pixels */
    int sww, swh;             /* world width and heigth int pixels */
/*** <- insert ***/
    short nw, nh;             /* length and number of notice lines */
    short vw, vh;             /* viewport width and height in hexes */
    short vw2, vh2;           /* 1/2 (rounded down) of above values */
    short vw_odd;             /* 1 if vw is odd */
    short sw;                 /* number of chars in side listing */
    short fw, fh;             /* dimensions of text font (in pixels) */
    short hfw, hfh;           /* dimensions of help text font (in pixels) */
    short hw, hh;             /* dimensions of general icon font (in pixels) */
    short hch;                /* center-to-center distance between hexes */
    short uw, uh;             /* dimensions of unit font/bitmaps (in pixels) */
    short mm;                 /* magnification of world hexes (in pixels) */
    short bd, margin;         /* spacing around things */
    short tlh, blh, trh, brh; /* heights of top and bottom parts of display */
    short rw, lw;             /* widths of right and left parts of display */
    short mw, mh;             /* main window width and height */
/*** (UK) insert -> ***/
    short wscale;             /* basic world scale */
    short mwe, mhe;
    bool  world_right;
    bool  info_right;
/*** <- insert ***/
    /* Working variables for the display */
    short vcx, vcy;           /* center hex in the viewport */
    short lastvcx, lastvcy;   /* last center hex (-1,-1 initially) */
    short lastx, lasty;       /* last current x and y (-1,-1 initially) */
    char *noticebuf[MAXNOTES];  /* data for the notice area */
    char promptbuf[BUFSIZE];  /* current prompt + input str on display */
    short bottom_note;       /* bottom note being displayed. */
    short reqchange;          /* something has changed requiring us to update
				 our cursor. */
    short redisplay_ok;       /* Process redisplay events */
    /* Keep track of units that have died.  Flush them at the end of the */
    /* players turn.  Player is reponsible for keeping track of units he kills. */
    struct a_unit *deadunits; /* list of units killed in last turn. */
    struct a_unit *curdeadunit; /* Dead unit we are looking at. */
    /* All the colors used - filled in by display init */
    long bdcolor;             /* color for borders */
    long bgcolor;             /* background color */
    long fgcolor;             /* foreground (text) color */
    long owncolor;            /* color for us (usually black) */
    long altcolor;            /* another color for us (usually blue) */
    long enemycolor;          /* color for them (usually red) */
    long neutcolor;           /* color for fencesitters (usually gray) */
    long graycolor;           /* color for graying out (usually gray) */
    long diffcolor;           /* unusual/distinct color (usually maroon) */
/*** (HW) insert -> ***/
    long productcolor;        /* color used to show products (usually white) */
    long sleepcolor;          /* col used to show sleep units (us. yellow) */
    long ordercolor;          /* col used to show order units (us. gold) */
    long althexcolor[MAXTTYPES]; /* the color of currently unseen terrain */
/*** <- insert ***/
    long goodcolor;           /* color for OKness (usually green) */
    long badcolor;            /* color for none-OKness (usually red) */
    long hexcolor[MAXTTYPES]; /* the color of each terrain type */
    long main;                /* main window */
    long help;                /* help window */
    long msg;                 /* places for notices/warnings */
    long info;                /* details about a unit */
    long prompt;              /* where prompts and input are */
    long map;                 /* detailed map of vicinity */
    long sides;               /* list of sides and their status */
    long timemode;            /* turn number and mode */
    long clock;               /* chess clock display */
    long state;               /* status of units on side */
    long world;               /* display of entire world */
    long display;             /* side's specific display structure */
    long rmdatabase;          /* resource database */
    /* Filled during side creation */
    struct a_side *next;      /* pointer to next in list */
    int startbeeptime;        /* beep at start of turn if this many */
			      /* seconds past */
    int laststarttime;        /* The physical time when the turn last */
			      /* started a turn */ 
} Side;

/* Some convenient macros. */

#define humanside(x) ((x) != NULL && (x)->humanp)

/* Iteration over all sides. */

#define for_all_sides(v) for (v = sidelist; v != NULL; v = v->next)

/* Manipulation of bytes encoding views of things. */
/* Types 254, 255 reserved for "seen but unoccupied" and "unseen". */

#define buildview(s,t) (((s)<<8)|(t))


#define vside(v) ((short)(((v)>>8) & 0x0ff))
/* #define vside(v) ((v).side) */
#define vtype(v) ((short)((v) & 0xff))
/* #define vtype(v) ((v).utype) */
#define vside_neutral(v) ((vside(v) == vside(buildview(MAXSIDES,0))))


#define UNSEEN (MAXUTYPES)
#define EMPTY  (MAXUTYPES+1)

#define side_view(s,x,y) ( ((s)->view)[world.width*(y)+(x)] )

extern void set_side_view();

#define cover(s,x,y) (((s)->coverage)[world.width*(y)+(x)])

#define set_cover(s,x,y,v) (((s)->coverage)[world.width*(y)+(x)] = (v))

#define add_cover(s,x,y,v) (((s)->coverage)[world.width*(y)+(x)] += (v))

#ifdef PREVVIEW
#define side_view_timestamp(s,x,y) (((s)->viewtimestamp)[world.width*(y)+(x)])

#define side_view_age(s,x,y) \
            ((cover(s,x,y) > 0) ? 0 : \
	     (global.time - (((s)->viewtimestamp)[world.width*(y)+(x)])))

#define side_prevview(s,x,y) (((s)->prevview)[world.width*(y)+(x)])
#endif

#define area_index(x,y) (((y) / AREA_SIZE) * areas_wide + (x) / AREA_SIZE)


/* Side variables. */

extern Side neutral_placeholder;
extern Side *sidelist, *curside, *tmpside;
extern Side *create_side(), *side_n(), *read_basic_side();

extern int numgivens, numhumans, numsides;

extern char *hosts[];
extern char *random_side_name();

extern bool humans[];

extern short areas_wide;      /* number of areas in x direction */
extern short areas_high;      /* number of areas in y direction */

