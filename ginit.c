/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Initialization and random routines for the display part of xconq. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

/*** (UK) insert -> ***/

#define W_SIDES_WIN (side->sw * side->fw)
#define H_SIDES_WIN (numsides * side->fh)

#define X_SIDES_WIN (side->lw + side->bd + (side->info_right?W_STATE_WIN+side->bd:0))
#define Y_SIDES_WIN (0)

#define W_MODE_WIN  (2 * side->margin + 14 * side->fw)
#define H_MODE_WIN  (4 * side->fh)

#define X_MODE_WIN  (side->lw + side->fw + (side->info_right?W_STATE_WIN+side->bd:0))
#define Y_MODE_WIN  (side->trh - H_MODE_WIN - side->fh)


#define W_CLOCK_WIN (9 * side->fw)
#define H_CLOCK_WIN (side->fh)

#define X_CLOCK_WIN (side->lw + 20 * side->fw + (side->info_right?W_STATE_WIN+side->bd:0))
#define Y_CLOCK_WIN (side->trh - 2 * side->fh)


#define W_STATE_WIN (29 * side->fw)
#define H_STATE_WIN (period.numutypes*max(side->hh, side->fh))

#define X_STATE_WIN (side->lw + side->bd)
#define Y_STATE_WIN (side->world_right?side->mh-H_STATE_WIN-2*side->bd:(side->info_right?0:side->trh))

#define W_WORLD_WIN (world.width * side->mm)
#define H_WORLD_WIN (world.height * side->mm)

#define Y_WORLD_WIN (side->mh-H_WORLD_WIN-2*side->bd)
#define X_WORLD_WIN (side->lw + side->bd + (side->world_right?W_STATE_WIN+side->bd:0))

/*
#define X_WORLD_WIN (side->lw+side->bd*2+W_STATE_WIN)
*/

/*** <- insert ***/

/* Always use four lines for the unit info display, even on small screens. */

#define INFOLINES 5

int maxnamelen = 0;		/* length of longest side name+host */

/* mxconq: defined in xconq.c */
extern bool singlePlayerMode;

/* mxconq: returns a side given a host_name. */
Side *get_side(host_name)
char *			host_name;
{
Side *			return_value;

    return_value = sidelist;
    while (return_value && (strcmp(return_value->host, host_name) != 0))
	    return_value = return_value->next;

    if (return_value) printf("name is %s.\n", return_value->host);

    return(return_value);

}

/* mxconq: Have user select a new side */

Side *get_new_side(host_name)
char *			host_name;
{
Side *			return_value;
int			num_sides = 0;
int			index;
char			reply[2];

    return_value = NULL;

    while (!return_value)
      {
      printf("Nonexistent side: %s.\n", host_name);
      printf("These are valid sides:\n");
      for_all_sides(return_value)
        {
        num_sides++;
        printf("  %d: %s on host %s\n",
	       num_sides, return_value->name,
	       (return_value->humanp ? return_value->host : "*") );
        }
      printf("Type number of player to use (0 to exit): ");
      reply[0] = getchar(); reply[1] = 0;
      index = atoi(reply);
      if (!index) exit(1);
      if (1 <= index <= num_sides)
        {
	return_value = sidelist;
        while(return_value && index > 1)
          {
          index--;
	  return_value = return_value->next;
          }
        }
      }
    return_value->host = copy_string(host_name);
    return(return_value);
}

/* Find a unit and put it at center (or close to it) of first view.  I guess */
/* can't use put_on_screen because not enough stuff is inited yet. */

init_curxy(side)
Side *side;
{
    Unit *unit;
    Side *loop_side;

    for_all_units(loop_side, unit) {
	if (unit->side == side) {
	    side->cx = unit->x;  side->cy = unit->y;
	    side->vcx = unit->x;
	    side->vcy = max(unit->y, side->vw2+1);
	    side->vcy = min(side->vcy, (world.height-1) - side->vh2);
	    return;
	}
    }
    side->vcx = side->vw2;  side->vcy = side->vh2;
    side->cx = side->vcx;  side->cy = side->vcy;
}

/* open displays early so that the first redraw will find the displays already up. */

/* mxconq: altered to only init players side if singlePlayerMode is TRUE */
/* open displays early so that the first redraw will find the displays already up. */

init_displays1()
{
    int len;
    Side *side;
    char *playerHost;
    /* mxconq: defined in xconq.c */
    extern bool singlePlayerMode;

    for_all_sides(side) {
	len = 8 + strlen(side->name) + 1;
	maxnamelen = max(maxnamelen,
			 len + (side->host ? hostlen(side->host) + 3 : 0));
    }

    if (singlePlayerMode) {
	playerHost = getenv("DISPLAY");
	side = get_side(playerHost);
	if (!side)
	    side = get_new_side(playerHost);

	if (side)  {
	    printf("Player side is: %s.\n", side->name);
	    init_display(side);
	}
	else  {
	    printf("Can't find player side: %s.\n", playerHost);
	    exit_xconq();
	}
    }
    else {
	for_all_sides(side) {
	    if (side->host != NULL) {
/*** (UK) insert -> ***/
                if (side_number(side)<numgivens && hosts[side_number(side)]) 
/*** <- insert ***/ 
		init_display(side);
	    }
	}
    }
}


/* The very first step in using X is to open up all the desired displays. */
/* In our case, there are many displays each with many windows.  If the */
/* display opens successfully, do an initial "redraw" to set the screen up. */

init_displays2()
{
    Side *side;

    for_all_sides(side) {
	if (side->host != NULL) {
	    if (active_display(side)) {
		init_curxy(side);
#if 0
		redraw(side);
#endif
	    }
	}
    }
}

/* The length of the printed host name is the length till the first */
 /* period or the total length */

int hostlen(str)
char str[];
{
  int len = 0;

  while ((str[len] != '.') && (str[len] != '\0')) len++;
  return len;
}

/* Open display, create all the windows we'll need, do misc setup things, */
/* and initialize some globals to out-of-range values for recognition later. */

init_display(side)
Side *side;
{
    int i;

    if (Debug) printf("Will try to open display \"%s\" ...\n", side->host);

    if (!open_display(side)) {
/*** (UK) insert -> ***/
	extern bool PartialGame;
	if (PartialGame) return 0;
/*** <- insert ***/ 
	fprintf(stderr, "Display \"%s\" could not be opened!\n", side->host);
	exit_xconq();
    }
    active_display(side);   /* done for side effect */
    init_colors(side);
    init_misc(side);
    init_sizes(side);
    create_main_window(side);
    side->msg =
	create_window(side, 0, 0,
		      side->lw, side->nh * side->fh);
    side->info =
	create_window(side, 0, side->nh * side->fh + side->bd,
		      side->lw, INFOLINES * side->fh);
    side->prompt =
	create_window(side, 0, (side->nh + INFOLINES) * side->fh + 2*side->bd,
		      side->lw, side->fh);
    side->map =
	create_window(side, 0, side->tlh,
		      side->vw * side->hw,
		      side->vh * side->hch + (side->hh - side->hch));
    set_retain(side, side->map);
    side->sides =
/*** (UK) change -> ***/
        create_window(side, X_SIDES_WIN, Y_SIDES_WIN,
			    W_SIDES_WIN, H_SIDES_WIN);
/*** was:
	create_window(side, side->lw + side->bd + 1, 0,
		      side->sw * side->fw, numsides * side->fh);
*** <- change ***/
    side->timemode =
/*** (UK) change -> ***/
	create_window(side, X_MODE_WIN, Y_MODE_WIN,
		            W_MODE_WIN, H_MODE_WIN);
/*** was:
	create_window(side, side->lw + side->fw, side->trh - 4 * side->fh,
		      2 * side->margin + 8 * side->fw, 2 * side->fh);
*** <- change ***/
    side->clock =
/*** (UK) change -> ***/
	create_window(side, X_CLOCK_WIN, Y_CLOCK_WIN,
			    W_CLOCK_WIN, H_CLOCK_WIN);
/*** was:
	create_window(side, side->lw + 13 * side->fw, side->trh - 2 * side->fh,
		      8 * side->fw, side->fh);
*** <- change ***/
    side->state =
/*** (UK) change -> ***/
	create_window(side, X_STATE_WIN, Y_STATE_WIN,
		      W_STATE_WIN,H_STATE_WIN);
/*** was:
	create_window(side, side->lw + 1, side->trh,
		      29 * side->fw, period.numutypes*max(side->hh, side->fh));
*** <- change ***/
    if (world_display(side)) {
	side->world =
	    create_window(side,
                          X_WORLD_WIN, Y_WORLD_WIN,
			  W_WORLD_WIN, H_WORLD_WIN);
/*** (UK) change -> ***/
/*** was:
			  side->lw+side->bd, side->mh-side->mm*world.height,
			  world.width * side->mm, world.height * side->mm);
*** <- change ***/
	set_retain(side, side->world);
    }
    create_help_window(side);
    fixup_windows(side);
    enable_input(side);
    for (i = 0; i < MAXNOTES; ++i) side->noticebuf[i] = " ";
    for (i = 0; i < MAXUTYPES; ++i) side->bvec[i] = 0;
    /* Flag some values as uninitialized */
    side->vcx = side->vcy = -1;
    side->lastvcx = side->lastvcy = -1;
    side->lastx = side->lasty = -1;
    if (Debug) printf("Successfully opened \"%s\"!\n", side->host);
/*** (UK) insert -> ***/
    update_clock(side);
    return 1;
/*** <- insert ***/ 
}

/* Initial sizing tries to take as much screen as possible. */

init_sizes(side)
Side *side;
{
  if (!resize_display(side, display_width(side), display_height(side))) {
    fprintf(stderr, "Display \"%s\" is too small!\n", side->host);
    exit_xconq();
  }
}

/*** (UK) insert -> ***/
sides_width()
{ Side *side;
  int l=0;
  int t;
  char *s;

  for_all_sides(side) {
    t=0;
    if (s=side->host) {
      t=3;
      while (*s && *s!='.') s++,t++;
    }
    t+=strlen(side->name);
    if (t>l) l=t;
  }
  return l+12;
}

config_rpart(side)
Side *side;
{
    if (side->mhe<=0) side->mhe=display_height(side);
    if (H_SIDES_WIN+H_MODE_WIN+H_STATE_WIN+H_WORLD_WIN+2*side->fh+6*side->bd
	>side->mhe) { /* not all in one column */
      if (H_STATE_WIN+H_WORLD_WIN+3*side->bd>side->mhe) {
	/* world right */
	/*
	printf("world right\n");
	*/
	side->rw=W_STATE_WIN+W_WORLD_WIN+3*side->bd;
	side->brh=H_STATE_WIN+2*side->bd;
	side->world_right=TRUE;
	side->info_right=FALSE;
	if (H_SIDES_WIN+H_MODE_WIN+H_STATE_WIN+2*side->fh+4*side->bd>side->mhe) 
	  return FALSE;
      }
      else { /* info right */
	/*
	printf("info right\n");
	*/
	side->rw=max(W_SIDES_WIN+W_STATE_WIN+3*side->bd,
		     W_WORLD_WIN+2*side->bd);
	side->brh=H_STATE_WIN+H_WORLD_WIN+3*side->bd;
	side->world_right=FALSE;
	side->info_right=TRUE;
      }
    }
    else {
      side->rw=max(W_SIDES_WIN,W_WORLD_WIN);
      side->rw=max(side->rw,W_STATE_WIN)+2*side->bd;
      side->brh=H_STATE_WIN+H_WORLD_WIN+3*side->bd;
      side->world_right=FALSE;
      side->info_right=FALSE;
    }
    side->trh=H_SIDES_WIN+H_MODE_WIN+2*side->fh+3*side->bd;
    /*
    printf("H_STATE_WIN=%d H_WORLD_WIN=%d\n",H_STATE_WIN,W_WORLD_WIN);
    printf("rw=%d trh=%d brh=%d\n",side->rw, side->trh, side->brh);
    */
    return TRUE;
}

config_lpart(side)
Side *side;
{   short abh;
    short alw=side->mwe-side->rw-2*side->bd;

    side->vw = min(world.width, alw / side->hw);
    side->nw = min(BUFSIZE, alw / side->fw);

    abh = side->mhe / 3;
    side->nh = max(1, min((abh/side->fh) - INFOLINES - 1, MAXNOTES));
    abh = side->mhe - ((side->nh+INFOLINES+1)*side->fh+3*side->bd);
    side->vh = min(world.height, abh / side->hch);
}

/*** <- insert ***/

/* Decide/compute all the sizes of things.  Our main problem here is that */
/* the display might be smaller than we really desire, so things have to */
/* be adjusted to fit. */

resize_display(side, mw, mh)
Side *side;
int mw, mh;
{
  /* if you adjust this procedure, please make a corresponding
     adjustment to resize_display2() */
    int alw, abh;

/*** (UK) change -> ***/
    int arh;

/*
    printf("resize %d x %d\n",mw,mh);
*/
    side->mwe=mw;
    side->mhe=mh;
    side->sw = sides_width();

    side->mm=1;
    if (!config_rpart(side)) return FALSE;
    arh=mh-(side->world_right?2*side->bd:
	    (side->info_right?H_STATE_WIN:side->trh+H_STATE_WIN)+3*side->bd);

    config_lpart(side);

    alw=max(side->vw*side->hw,side->nw*side->fw)+3*side->bd;
    if (side->world_right) alw+=W_STATE_WIN+2*side->bd;
    alw=mw-alw;
    side->mm = min(arh / world.height, alw / world.width);
    /*
    printf("vh=%d vw=%d\n",side->vh,side->vw);
    printf("arh=%d arw=%d\n",arh,alw);
    */
    if (side->mm<=0) return FALSE;
/*** was:
    alw = max(mw/4, world.width);
    side->sw = min(maxnamelen+7, alw / side->fw);

    alw = mw - max((side->sw * side->fw), world.width) - side->bd;
    alh = mh - (side->world_right?0:side->brh)-2*side->bd;
    side->vw = min(world.width, alw / side->hw);
    side->nw = min(BUFSIZE, alw / side->fw);

    abh = (2 * mh) / 3;
    side->vh = min(world.height, abh / side->hch);
    side->nh = max(1, min((abh/side->fh)/2 - 5, MAXNOTES));

    side->mm = min(5, (max(world.width, side->fw*side->sw) / world.width));
*** <- change ***/
    set_sizes(side);
/*** (UK) remove -> ***
    side->mw = mw;
    side->mh = mh;
*** <- remove ***/
    return (side->vw >= MINWIDTH && side->vh >= MINHEIGHT);
}

/* this version doesn't adjust the +side->bdmap magnification. */
resize_display2(side, mw, mh)
Side *side;
int mw, mh;
{
  /* if you adjust this procedure, please make a corresponding
     adjustment to resize_display() */
    int alw, abh;

/*** (UK) change -> ***/
    side->mwe=mw;
    side->mhe=mh;
    side->sw = sides_width();

    if (!config_rpart(side)) return FALSE;
    config_lpart(side);
/*** was:
    alw = max(mw/4, side->mm*world.width);
    side->sw = min(maxnamelen+5, alw / side->fw);

    alw = mw - max((side->sw * side->fw), side->mm*world.width) - side->bd;
    side->vw = min(world.width, alw / side->hw);
    side->nw = min(BUFSIZE, alw / side->fw);

    abh = (2 * mh) / 3;
    side->vh = min(world.height, abh / side->hch);
    side->nh = max(1, min((abh/side->fh)/2 - 5, MAXNOTES));
*** <- change ***/
    set_sizes(side);
/*** (UK) remove -> ***
    side->mw = mw;
    side->mh = mh;
*** <- remove ***/
    return (side->vw >= MINWIDTH && side->vh >= MINHEIGHT);
}

/* This fn is a "ten-line horror"; effectively a mini tiled window manager */
/* that keeps the subwindows from overlapping no matter what the display and */
/* requested sizes are.  Siemens eat your heart out... */

set_sizes(side)
Side *side;
{
    int ulhgt, llhgt, urhgt, lrhgt;
/*** (UK) insert -> ***/
    int trh;
/*** <- insert ***/

    /* Make sure map window dimensions are OK */
    side->vw = min(world.width, side->vw);
    side->vh = min(world.height, side->vh);
/*** (HW) insert -> ***/
    side->svw = side->vw * side->hw;
    side->svh = side->vh * side->hch;
    side->sww = world.width * side->hw;
    side->swh = world.height * side->hch;
/*** <- insert ***/
    side->vw2 = side->vw / 2;  side->vh2 = side->vh / 2;
    side->vw_odd = side->vw % 2;
    /* Compute subregion sizes (left/right upper/lower width/height) */
/*** (UK) change -> ***/
    config_rpart(side);
    side->mhe=-1;

    side->lw = max(side->nw * side->fw, side->hw * side->vw) + 2*side->bd;

    trh=side->trh;
    if (side->info_right) trh=0;

    side->tlh = side->fh * (side->nh + INFOLINES + 1) + 3 * side->bd;
/*** (UK) change -> ***/
    side->blh = side->hch * side->vh + (side->hh - side->hch);
/*** was:
    side->blh = side->hch * side->vh + (side->hh - side->hch);
*** <- change ***/

    if (side->brh+trh > side->blh+side->tlh)
      side->blh = side->brh+trh - side->tlh;

    if (trh<side->tlh) {
	if (side->brh+trh<side->blh+side->tlh) {
	  trh+=min(side->tlh-trh,
		   side->blh+side->tlh-side->brh-trh);
	  if (!side->info_right) side->trh=trh;
	}
    }
    side->brh = side->tlh+side->blh - trh;

    side->mw = side->lw + side->bd + side->rw;
    side->mh = side->tlh + 2*side->bd + side->blh;
/*** was:
    side->lw = max(side->nw * side->fw, side->hw * side->vw);
    side->rw = max((world_display(side) ? world.width * side->mm : 0),
		   side->sw * side->fw);

    side->trh = (numsides + 4) * side->fh + side->bd;
    side->tlh = side->fh * (side->nh + INFOLINES + 1) + 3 * side->bd;

    side->brh = period.numutypes * max(side->hh, side->fh);
    if (world_display(side))
      side->brh += side->mm * world.height + 2*side->bd;
    side->blh = side->hch * side->vh + (side->hh - side->hch);

    if (side->brh+side->trh > side->blh+side->tlh)
      side->blh = side->brh+side->trh - side->tlh;

    if (side->trh > side->tlh) {
      side->brh = side->tlh+side->blh - side->trh;
    } else if (side->brh > side->blh) {
      side->trh = side->tlh+side->blh - side->brh;
    } else {
      side->brh = side->blh;
      side->trh = side->tlh;
    }

    side->mw = side->lw + side->bd + side->rw;
    side->mh = side->tlh + 3*side->bd + side->blh;
*** <- change ***/
    /* Only vcy needs adjustment, since vcx can never be close to an edge */
    side->vcy = min(max(side->vh2, side->vcy), (world.height-1)-side->vh2);
}

/* Acquire a set of colors.  There are too many to make it feasible to */
/* customize them via .Xdefaults, so we don't even try.  If there aren't */
/* enough colors, drop into monochrome mode.  This doesn't take the window */
/* manager into account - it may have grabbed some color space. */

init_colors(side)
Side *side;
{
    if (Debug) printf("%d colors available ...\n", display_colors(side));
    side->monochrome = (display_colors(side) <= 2);
/*** (UK) change -> ***/
    side->bonw = FALSE;
/*** was:
    side->bonw = (side->monochrome ? BLACKONWHITE : FALSE);
*** <- change ***/
    set_colors(side);
}

/* This will set up the correct set of colors at any point in the game. */
/* Note that terrain colors get NO value if in monochrome mode. */
/* If the colors requested are not available, it is up to the graphics */
/* interface to supply a substitute. */


long
get_map_color(side, name, class, fallback)
     Side	*side;
     char	*name, *class, *fallback;
{
  static char	rmName[200], rmClass[200];
  sprintf(rmName, "map.%s.%s", period.name, name);
  sprintf(rmClass, "map.Period.%s", class);
  return request_color(side, rmName, rmClass, fallback);
}

set_colors(side)
Side *side;
{
  int	t;
  long fg, bg;
  
  fg = (side->bonw ? black_color(side) : white_color(side));
  bg = (side->bonw ? white_color(side) : black_color(side));
/*** (HW) change -> ***/
  side->bgcolor = side->owncolor = side->altcolor = side->diffcolor =
    side->productcolor = side->sleepcolor = side->ordercolor = bg;
/*** was:
  side->bgcolor = side->owncolor = side->altcolor = side->diffcolor = bg;
*** <- change ***/
  side->fgcolor = side->bdcolor = side->graycolor = side->enemycolor = fg;
  side->neutcolor = side->goodcolor = side->badcolor = fg;
  if (!side->monochrome) {
    static char	*FG="Foreground", *BG="Background";
    for_all_terrain_types(t) {
      side->hexcolor[t] = request_color(side, NULL, NULL, ttypes[t].color);
/*** (HW) insert -> ***/
      side->althexcolor[t] = brighten_color(side, NULL,NULL, ttypes[t].color);
/*** <- insert ***/
    }
#if 1
    get_color_resources(side);
#else
    side->owncolor = get_map_color(side,"ownColor",BG,"black");
    side->altcolor = get_map_color(side,"alternateColor",BG,"blue");
    side->diffcolor = get_map_color(side,"differentColor",BG,"maroon");
    side->bdcolor = get_map_color(side,"borderColor",FG,"blue");
    side->graycolor = get_map_color(side,"grayColor",FG,"light gray");
    side->enemycolor = get_map_color(side,"enemyColor",FG,"red");
    side->neutcolor = get_map_color(side,"neutralColor",FG,"light gray");
    side->goodcolor = get_map_color(side,"goodColor",FG,"green");
    side->badcolor = get_map_color(side,"badColor",FG,"red");
#endif
  }
}

/* Move windows and change their sizes to correspond with the new sizes of */
/* viewports, etc. */

reconfigure_display(side, rzmain)
Side *side;
int rzmain;
{
  int sy, sdy;
  
  if (active_display(side)) {
    if (rzmain)
      set_colors(side);
    reset_misc(side);
    sy = 0;  sdy = side->nh * side->fh;
    change_window(side, side->msg, 0, sy, side->lw, sdy);
    sy += sdy + side->bd;  sdy = INFOLINES * side->fh;
    change_window(side, side->info, 0, sy, side->lw, sdy);
    sy += sdy + side->bd;  sdy = 1 * side->fh;
    change_window(side, side->prompt, 0, sy, side->lw, sdy);
    change_window(side, side->map,
		  0, side->tlh,
		  side->vw * side->hw,
		  side->vh * side->hch + (side->hh - side->hch));
    change_window(side, side->sides,
/*** (UK) change -> ***/
                  X_SIDES_WIN, Y_SIDES_WIN, -1, -1);
/*** was:
		  side->lw + side->bd, 0, -1, -1);
*** <- change ***/
    change_window(side, side->timemode,
/*** (UK) change -> ***/
		  X_MODE_WIN, Y_MODE_WIN, -1, -1);
/*** was:
		  side->lw + side->fw, side->trh - 4 * side->fh, -1, -1);
*** <- change ***/
    change_window(side, side->clock,
/*** (UK) change -> ***/
		  X_CLOCK_WIN, Y_CLOCK_WIN, -1, -1);
/*** was:
		  side->lw + 13 * side->fw, side->trh - side->fh, -1, -1);
*** <- change ***/
/*** (UK) change -> ***/
    change_window(side, side->state,
		  X_STATE_WIN, Y_STATE_WIN, -1, -1);
/*** was:
    change_window(side, side->state,
		  side->lw + side->bd, side->trh + side->bd, -1, -1);
*** <- change ***/
    if (world_display(side)) {
/*** (UK) change -> ***/
      change_window(side, side->world,
		    X_WORLD_WIN, Y_WORLD_WIN,
		    W_WORLD_WIN, H_WORLD_WIN);
/*** was:
      change_window(side, side->world,
		    side->lw+side->bd, side->mh - world.height*side->mm,
		    world.width * side->mm, world.height * side->mm);
*** <- change ***/
    }
    if (rzmain) {
      change_window(side, side->main, -1, -1, side->mw, side->mh);
      printf("main window configured to %dx%d\n", side->mw, side->mh);
    }
#if 0
    redraw(side);
#endif
/*** (UK) insert -> ***/
    clear_window(side,side->main);
/*** <- insert ***/
  }
}


