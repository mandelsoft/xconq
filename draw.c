/* Copyright (c) 1987, 1988, 1991  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Geometry is somewhat tricky because our viewports are supposed */
/* to wrap around a cylinder transparently.  The general idea is that */
/* if modulo opns map x coordinates onto a cylinder, adding and subtracting */
/* the diameter of the cylinder "un-modulos" things.  If this doesn't make */
/* any sense to you, then be careful about fiddling with the code! */

/* If that wasn't bad enough, the hexes constitute an oblique coordinate */
/* where the axes form a 60 degree angle to each other.  Fortunately, no */
/* trig is necessary - to convert to/from rectangular, add/subtract 1/2 of */
/* the y coordinate to x, and leave the y coordinate alone. */

/* The graphical code uses mostly text drawing functions, which are more */
/* likely to be efficient than is random bitblting.  (The interface may */
/* implement the operations as random blitting, but that's OK.) */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
/*** (HW) insert -> ***/
#include "global.h"
#include "limits.h"
/*** <- insert ***/

extern bool populations;        /* used to decide about running pop display */

char rowbuf[BUFSIZE];           /* buffer for terrain row drawing */


/* Completely redo a screen, making no assumptions about appearance. */
/* This one is used frequently, especially when a window is exposed. */

redraw(side)
Side *side;
{
    if (active_display(side)) {
	erase_cursor(side);
#if 0
	clear_window(side, side->main);
#endif
#if 1
	show_note(side,TRUE);
	show_info(side);
	show_prompt(side);
	show_all_sides(side,TRUE);
	show_timemode(side,TRUE);
	show_clock(side);
	show_state(side,TRUE);
	show_map(side);
	show_world(side);
#endif
	flush_output(side);
	flush_input(side);
    }
}

/* Put the point x, y in the center of the screen.  Limit to make sure */
 /* it keeps the screen properly aligned.  */

void recenter(side, x, y)
Side *side;
int x, y;
{
  int oldx, oldy;

  oldx = side->vcx;
  oldy = side->vcy;
  side->vcx = wrap(x);
  side->vcy = min(max(side->vh2-(1-(side->vh&1)), y),
		  (world.height-1)-side->vh2);
  if ((side->vcx == oldx) && (side->vcy == oldy)) ;
  else {
    if (side->lastvcx >= 0) undraw_box(side);
    draw_box(side);
    show_map(side);
    flush_output(side);
    flush_input(side);
  }
}
/* Ensure that given location is visible.  We also flush the input because */
/* any input relating to a different screen is probably worthless. */

put_on_screen(side, x, y)
Side *side;
int x, y;
{

    /* Ugly hack to prevent extra boxes being drawn during init - don't ask!*/
    if (x == 0 && y == 0) return;
    if (active_display(side))
	if (!in_middle(side, x, y)) 
	  recenter(side, x, y);
}

/* Undo the wrapping effect, relative to viewport location. */
/* Note that both conditions cannot both be true at the same time, */
/* since viewport is smaller than map. */

/* This function has been updated to calculate the exact locations of */
 /* the hex in the viewport, even if the viewport is small. */

int unwrap(side, x, y)
Side *side;
int x, y;
{
    int vcx = side->vcx, vw2 = side->vw2, odd = side->vw_odd;
    int vcy = side->vcy;
    int left_hex, right_hex;
    
    left_hex = vcx - vw2 - ((y - vcy) / 2) - (y > vcy);
    right_hex = vcx + vw2 + odd - ((y - vcy) / 2) + ((y < vcy) && odd)
      - ((y > vcy) && !odd);

    if ((left_hex < 0) && (x > right_hex)) x -= world.width;
    else if ((right_hex >= world.width) && (x < left_hex)) x += world.width;
    return x;
}

/* Decide whether given location is not too close to edge of screen. */
/* We do this because it's a pain to move units when half the adjacent */
/* places aren't even visible.  This routine effectively places a lower */
/* limit of 5x5 for the map window. (I think) */

in_middle(side, x, y)
Side *side;
int x, y;
{
    int vcx = side->vcx, vw2 = side->vw2, odd = side->vw_odd;
    int vcy = side->vcy, vh2 = side->vh2;
    int left_hex, right_hex;

    if(Debug>1) printf("In_middle: %d, %d, %d, %d\n", x, y, vcx, vcy);
	
    /* changed so that units on top and bottom lines do the right thing. */
    if (!between(vcy-vh2+2, y, vcy+vh2-1) && between(1, y, world.height-2))
	return FALSE;

    left_hex = vcx - vw2 - ((y - vcy) / 2) - (y > vcy) + 3;
    right_hex = vcx + vw2 + odd - ((y - vcy) / 2) + ((y < vcy) && odd)
      - ((y > vcy) && !odd) - 3;

    if (left_hex < 0) return !between(right_hex, x, left_hex + world.width);
    else if (right_hex >= world.width)
      return !between(right_hex - world.width, x, left_hex);
    return between(left_hex, x, right_hex);
}

/* Transform map coordinates into screen coordinates, relative to the given */
/* side.  Allow for cylindricalness and number of pixels in a hex. */

xform(side, x, y, sxp, syp)
Side *side;
int x, y, *sxp, *syp;
{
  
    *sxp = ((side->hw * (x - (side->vcx - side->vw2))) +
	    (side->hw * (y - side->vcy)) / 2);
    *syp = side->hch * ((side->vcy + side->vh2) - y);
}

/* Un-transform screen coordinates (as supplied by mouse perhaps) into */
/* map coordinates.  This doesn't actually account for the details of */
/* hexagonal boundaries, and actually discriminates box-shaped areas. */

deform(side, sx, sy, xp, yp)
Side *side;
int sx, sy, *xp, *yp;
{
    int vcx = side->vcx, vcy = side->vcy, adjust;

    *yp = (vcy + side->vh2) - (sy / side->hch);
    adjust = (((*yp - vcy) & 1) ? ((side->hw/2) * (*yp >= vcy ? 1 : -1)) : 0);
    *xp = wrap(((sx - adjust) /	side->hw) - (*yp - vcy) / 2 +
	       (vcx - side->vw2));
}

/* Transform coordinates in the world display.  Simpler, since no moving */
/* viewport nonsense. */

w_xform(side, x, y, sxp, syp)
Side *side;
int x, y, *sxp, *syp;
{
/*** (UK) change -> ***/
    *sxp = side->mm * ( x+ wrap(y/2));
/*** was:
    *sxp = side->mm * x + (side->mm * y) / 2;
*** <- change ***/
    *syp = side->mm * (world.height - 1 - y);
}

/* Translate Cartesian pixels back to map coords. */

w_deform(side, sx, sy, xp, yp)
Side *side;
int sx, sy, *xp, *yp;
{
    *yp = world.height - 1 - sy / side->mm;
    *xp = sx / side->mm - (*yp) / 2;
    *xp = wrap(*xp);
}

/* Redraw the map of the whole world.  We use square blobs instead of icons, */
/* since individual "hexes" may be as little as 1x1 pixels in size, and */
/* there are lots of them.  Algorithm uses run-length encoding to find and */
/* draw bars of constant color.  Monochrome just draws units - no good */
/* choices for terrain display. */

show_world(side)
Side *side;
{
    int x, y, color, barcolor, x1;

    if (active_display(side) && world_display(side)) {
	clear_window(side, side->world);
	for (y = world.height-1; y >= 0; --y) {
	    x1 = 0;
	    barcolor = world_color(side, x1, y);
	    for (x = 0; x < world.width; ++x) {
		color = world_color(side, x, y);
		if (color != barcolor) {
		    draw_bar(side, x1, y, x - x1, barcolor);
		    x1 = x;
		    barcolor = color;
		}
	    }
	    draw_bar(side, x1, y, world.width - x1, barcolor);
	}
	if (side->vcy >= 0) draw_box(side);
    }
}

/* Compute the color representing a hex from the given side's point of view. */

world_color(side, x, y)
Side *side;
int x, y;
{
    viewdata view = side_view(side, x, y);
    Side *side2;

    if (side->monochrome) {
/*** (UK) change -> ***/
	return ((view == EMPTY) ? side->fgcolor : side->bgcolor);
/*** was:
	return ((view == EMPTY) ? side->bgcolor : side->fgcolor);
*** <- change ***/
    } else {
	if (view == UNSEEN) {
	    return (side->bgcolor);
	} else if (view == EMPTY) {
	    return (side->hexcolor[terrain_at(x, y)]);
	} else {
	    side2 = side_n(vside(view));
	    return ((side2 == NULL) ? side->neutcolor :
		    (allied_side(side2, side) ? side->altcolor :
		     side->enemycolor));
	}
    }
}

/* Draw an outline box on the world map.  Since we adopt the dubious trick */
/* of inverting through all planes, must be careful to undo before moving; */
/* also, draw/undraw shiftedly so both boxes appear on both sides of world. */

draw_box(side)
Side *side;
{
    invert_box(side, side->vcx, side->vcy);
    invert_box(side, side->vcx - world.width, side->vcy);
    side->lastvcx = side->vcx;  side->lastvcy = side->vcy;
}

undraw_box(side)
Side *side;
{
    invert_box(side, side->lastvcx, side->lastvcy);
    invert_box(side, side->lastvcx - world.width, side->lastvcy);
}

/*** (HW) insert -> ***/
char hexbuf[BUFSIZE];

InitHexBuf()
{
  int i;

  for(i=0;i<BUFSIZE;i++) hexbuf[i] = HEX;
}

RedrawHexArray(side,x1,y1,x2,y2)
Side *side;
int x1,y1,x2,y2;
{
  int ax,ay,bx,by,y,len;
  int uwx1,uwx2,uwax,uwbx;
  int xs,xe, sx,sy;
  static bool hexbufinit = FALSE;

  if (x1 < 0 || y1 < 0 || x2 < 0 || y2 < 0 || !active_display(side)) return;
  uwx1 = unwrap(side,x1,y1); uwx2 = unwrap(side,x2,y2);
  if (uwx1 < uwx2) { ax = x1; bx = x2; uwax = uwx1; uwbx = uwx2; }
  else { ax = x2; bx = x1; uwax = uwx2; uwbx = x1; }
  if (y1 < y2) { ay = y1; by = y2; }
  else { ay = y2; by = y1; }
  len = wrap(uwbx - uwax + 1);
/*
printf("RedrawHex: (x1=%d,x2=%d) (y1=%d,y2=%d) (uwax=%d,uwbx=%d)\n",
       x1,x2,y1,y2,uwax,uwbx);
       */
  xform(side,uwax,by,&sx,&sy);
  for (y = by; y >= ay; y--) {
    if (side->monochrome || side->showmode == TERRICONS ||
	side->showmode == ONEICON) {
      if (!hexbufinit) {
	InitHexBuf();
	hexbufinit = TRUE;
      }
/*
printf("      (sx=%d,sy=%d)\n",sx,sy);
*/
      draw_terrain_row(side,sx,sy,hexbuf,len,black_color(side));
      sy += side->hch;
      sx -= side->hw/2;
    }
/*
printf("  draw_row: (%d,%d,%d)\n",uwax, y, len);
*/
    draw_row(side, uwax, y, len);
  }
  UpdateClipArea(side,ax,ay);
  UpdateClipArea(side,(bx+(by-ay+1))%world.width,by);
}
/*** <- insert ***/

/* Draw immediate area in more detail.  We make a little effort to avoid */
/* drawing hexes off the visible part of the screen, but are still somewhat */
/* conservative, so as not to get holes in the display.  Implication is that */
/* some lower level of routines has to be able to clip the map window. */

show_map(side)
Side *side;
{
    int y1, y2, y, x1, x2, adj;

    if (active_display(side)) {
	clear_window(side, side->map);
	y1 = side->vcy + side->vh2;
	y2 = side->vcy - side->vh2 + 1 - (side->vh & 1);
	for (y = y1; y >= y2; --y) {
	    adj = (y - side->vcy) / 2;
	    x1 = side->vcx - side->vw2 - adj - 1;
	    x2 = side->vcx + side->vw2 - adj + 1 + (side->vw & 1);
	    draw_row(side, x1, y, x2 - x1);
	}
	draw_cursor(side);
	flush_output(side);
#if 0
	draw_box(side);  /* must be after flush */
#endif
    }
/*** (HW) insert -> ***/
    draw_smoveorders(side,-1,-1);
    draw_moveorders(side,-1,-1);
/*** <- insert ***/
}

/* Draw an individual detailed hex, as a row of one. */
/* This routine may be called in cases where the hex is not on the main */
/* screen;  if so, then the world map but not the local map is drawn on. */
/* (should the display be shifted to make visible?) */

draw_hex(side, x, y, flushit)
Side *side;
int x, y;
bool flushit;
{
    int sx, sy;

    if (active_display(side)) {
	if (side->monochrome || side->showmode == TERRICONS ||
	    side->showmode == ONEICON) {
	    xform(side, unwrap(side, x, y), y, &sx, &sy);
	    draw_hex_icon(side, side->map, sx, sy, side->bgcolor, HEX);
	}
	draw_row(side, unwrap(side, x, y), y, 1);
	draw_bar(side, x, y, 1, world_color(side, x, y));
/*** (HW) insert -> ***/
	draw_smoveorders(side,x,y);
	draw_moveorders(side,x,y);
/*** <- insert ***/
	if (flushit) flush_output(side);
    }
}

/* Return the color of the hex. (for color displays only) */

/*** (HW) change -> ***/
long hex_color(side, x, y)
Side *side;
int x, y;
{
  if (side_view(side, wrap(x), y) == UNSEEN) return side->bgcolor;
  if (side->notifyvisibility && cover(side,wrap(x),y) <= 0) {
    return side->althexcolor[terrain_at(wrap(x), y)];
  }
  return side->hexcolor[terrain_at(wrap(x), y)];
}

/*** was:
long hex_color(side, x, y)
Side *side;
int x, y;
{
  long color;

  color = ((side_view(side, wrap(x), y) == UNSEEN) ? side->bgcolor :
	    side->hexcolor[terrain_at(wrap(x), y)]);
  return (color);
}
*** <- change ***/

/* The basic map drawing routine does an entire row at a time, which yields */
/* order-of-magnitude speedups (!).  This routine is complicated by several */
/* tricks:  1) in monochrome, the entire line can be drawn at once; 2) in */
/* color, run-length encoding maximizes the length of constant-color strings */
/* and 3) anything which is in the background color need not be drawn. */
/* In general, this routine dominates the map viewing process, so efficiency */
/* here is very important. */

draw_row(side, x0, y0, len)
Side *side;
int x0, y0, len;
{
    bool empty = TRUE;
    char ch;
    int i = 0, x, x1, sx, sy;
    long color, segcolor;

    if (side->monochrome) {
	xform(side, x0, y0, &sx, &sy);
	for (x = x0; x < x0 + len; ++x) {
	    if (side_view(side, wrap(x), y0) == EMPTY) {
		rowbuf[i++] = ttypes[terrain_at(wrap(x), y0)].tchar;
		empty = FALSE;
	    } else {
		rowbuf[i++] = ' ';
	    }
	}
	if (!empty) draw_terrain_row(side, sx, sy, rowbuf, i, side->fgcolor);
    } else {
	x1 = x0;
	segcolor = hex_color(side, x0, y0);
	for (x = x0; x < x0 + len; ++x) {
	    color = hex_color(side, x, y0);
	    sx = side->bgcolor;
	    if (color != segcolor) {
		if (segcolor != side->bgcolor) {
		    xform(side, x1, y0, &sx, &sy);
		    draw_terrain_row(side, sx, sy, rowbuf, i, segcolor);
		}
		i = 0;
		x1 = x;
		segcolor = color;
	    }
	    switch(side->showmode) {
	    case FULLHEX:
	    case BOTHICONS:
		ch = HEX;
		break;
	    case BORDERHEX:
		ch = OHEX;
		break;
	    case TERRICONS:
/*** (HW) insert -> ***/
	    case ONEICON:
/*** <- insert ***/
		ch = ttypes[terrain_at(wrap(x), y0)].tchar;
		break;
	    }
	    rowbuf[i++] = ch;
	}
	if (len == 1) i = 1;
	xform(side, x1, y0, &sx, &sy);
	draw_terrain_row(side, sx, sy, rowbuf, i, segcolor);
	if (side->showmode == BOTHICONS) {
	    i = 0;
	    x1 = x0;
	    segcolor = terricon_color(side, x0, y0);
	    for (x = x0; x < x0 + len; ++x) {
		color = terricon_color(side, x, y0);
		if (color != segcolor) {
		    xform(side, x1, y0, &sx, &sy);
		    draw_terrain_row(side, sx, sy, rowbuf, i, segcolor);
		    i = 0;
		    x1 = x;
		    segcolor = color;
		}
		rowbuf[i++] = ttypes[terrain_at(wrap(x), y0)].tchar;
	    }
	    if (len == 1) i = 1;
	    xform(side, x1, y0, &sx, &sy);
	    draw_terrain_row(side, sx, sy, rowbuf, i, segcolor);
	}
    }
    /* Units are much harder to optimize - fortunately they're sparse */
    for (x = x0; x < x0 + len; ++x) {
	draw_unit(side, x, y0);
    }
}

/* Return the color of a terrain icon overlaying a colored hex. */

terricon_color(side, x, y)
Side *side;
int x, y;
{
    return ((side_view(side, wrap(x), y) == UNSEEN) ? side->bgcolor :
	    (ttypes[terrain_at(wrap(x), y)].dark ? side->fgcolor :
	     side->bgcolor));
}

/*** (HW) insert -> ***/
bool do_clip;

#define clip(ax,bx,ay,by) (do_clip && ((ax) > side->clip_xmax || \
                                       (bx) < side->clip_xmin \
				     || (ay) > side->clip_ymax || \
					(by) < side->clip_ymin))

set_clipcoords(side)
Side *side;
{ int h;

  if (side->clip_xmax < 0) {
    do_clip = FALSE;
    return;
  }
  do_clip = TRUE;

  side->clip_xmax += side->hw;
  side->clip_ymax += side->hch;
}

/*** (UK) insert -> ***/
handle_showsmorders(side,oe,x1,y1,sotype)
Side *side;
StandingOrderEntry *oe;
{ StandingConditionEntry *ce;
  while (oe) {
    ce=oe->cond;
    while (ce) {
      handle_showmorder(side,ce->order,x1,y1,sotype);
      ce=ce->next;
    }
    oe=oe->next;
  }
}
/*** <- insert ***/ 

handle_showmorder(side,order,x1,y1,sotype)
Side *side;
Order *order;
int x1,y1;
char sotype;
{
  int sx1,sy1,sx2,sy2, minx,miny,maxx,maxy;
  bool out1,out2, sx1_lt_sx2, transpose = FALSE;
  
  if (!order || order->type != MOVETO) return;

  /* we want easy to handle coordinates */
  xform(side,unwrap(side,x1,y1),y1,&sx1,&sy1);
  xform(side,unwrap(side,order->p.pt[0].x,order->p.pt[0].y),
	order->p.pt[0].y,&sx2,&sy2);

  out1 = (sx1 < 0 || sx1 > side->svw || sy1 < 0 || sy1 > side->svh);
  out2 = (sx2 < 0 || sx2 > side->svw || sy2 < 0 || sy2 > side->svh);
  if (out1 && out2) return; /* don't draw lines if both endpoints invisible */
  if (sx1 < sx2) {
    sx1_lt_sx2 = TRUE;
    minx = sx1; maxx = sx2;
  } else {
    sx1_lt_sx2 = FALSE;
    minx = sx2; maxx = sx1;
  }
  if (sy1 < sy2) {
    miny = sy1; maxy = sy2;
  } else {
    miny = sy2; maxy = sy1;
  }
  
  /* now we have to check the drawing direction */
  if (abs(sx1-sx2)*2 > side->sww) transpose = TRUE;
/*
printf("(x1=%d,y1=%d)(x2=%d,y2=%d)  (sx1=%d,sy1=%d)(sx2=%d,sy2=%d)\n",
       x1,y1,order->p.pt[0].x,order->p.pt[0].y,sx1,sy1,sx2,sy2);
printf("minx=%d miny=%d maxx=%d maxy=%d out1=%d out2=%d transpose=%d\n",
       minx,miny,maxx,maxy,out1,out2,transpose);
       */

  if (!out1) {
    if (!transpose) {
      if (clip(minx,maxx,miny,maxy)) return;
      if (!out2) { /* both inside no transpose */
	draw_arrow_line(side,side->map,sotype,sx1,sy1,sx2,sy2);
      } else { /* out2 outside no transpose */
	draw_line(side,side->map,sotype,sx1,sy1,sx2,sy2);
      }
    } else { /* transpose */
      if (sx1_lt_sx2) { /* transpose p2 to left */
	if (clip(sx2-side->sww,minx,miny,maxy)) return;
	draw_line(side,side->map,sotype,sx1,sy1,sx2-side->sww,sy2);
	if (!out2) { /* draw second part of arrow with p1 transposed */
	  if (clip(sx2,sx1+side->sww,miny,maxy)) return;
	  draw_arrow_line(side,side->map,sotype,sx1+side->sww,sy1,sx2,sy2);
	}
      } else { /* transpose p2 to right */
	if (clip(sx2+side->sww,minx,miny,maxy)) return;
	draw_line(side,side->map,sotype,sx1,sy1,sx2+side->sww,sy2);
	if (!out2) { /* draw second part of arrow with p1 transposed */
	  if (clip(sx1-side->sww,sx2,miny,maxy)) return;
	  draw_arrow_line(side,side->map,sotype,sx1-side->sww,sy1,sx2,sy2);
	}
      }
    }
  } else { /* p1 outside, p2 must be inside now (tested earlier) */
    if (!transpose) {
      if (clip(minx,maxx,miny,maxy)) return;
      draw_arrow_line(side,side->map,sotype,sx1,sy1,sx2,sy2);
    } else { /* transpose */
      if (sx1_lt_sx2) { /* transpose p1 to right */
	if (clip(sx2,sx1+side->sww,miny,maxy)) return;
	draw_arrow_line(side,side->map,sotype,sx1+side->sww,sy1,sx2,sy2);
      } else { /* transpose p1 to left */
	if (clip(sx1-side->sww,sx2,miny,maxy)) return;
	draw_arrow_line(side,side->map,sotype,sx1-side->sww,sy1,sx2,sy2);
      }
    }
  }
}

UpdateClipArea(side,x,y)
Side *side;
int x,y;
{
  int sx,sy;

  xform(side,unwrap(side,x,y),y,&sx,&sy);
  if (sx < 0) sx = 0;
  else if (sx > side->svw) sx = side->svw;
  if (sy < 0) sy = 0;
  else if (sy > side->svh) sy = side->svh;
  if (sx < side->clip_xmin) side->clip_xmin = sx;
  if (sx > side->clip_xmax) side->clip_xmax = sx;
  if (sy < side->clip_ymin) side->clip_ymin = sy;
  if (sy > side->clip_ymax) side->clip_ymax = sy;
}

draw_moveorders(side,x,y)
Side *side;
int x,y;
{
  Unit *unit;
  
  if (!side->show_orders) return;

  if (x >= 0 && side->showsorderutype == NOTHING) {
    UpdateClipArea(side,x,y);
  }

  if (side->showsorderlevel != 0) {
    side->showsorderflag = TRUE; 
    return;
  }
  
  set_clipcoords(side);
/*
printf("draw_move: clip=(%d,%d),(%d,%d)\n",side->clip_xmin,side->clip_xmax,
       side->clip_ymin,side->clip_ymax);
       */

  for_all_side_units(side,unit) {
    if (unit->orders.type == MOVETO)
      handle_showmorder(side,&unit->orders,unit->x,unit->y,'m');
  }    
  side->clip_xmin = side->clip_ymin = INT_MAX;
  side->clip_xmax = side->clip_ymax = -1;
}

draw_smoveorders(side,x,y)
Side *side;
int x,y;
{
  Unit *unit;
  
  if (side->showsorderutype == NOTHING) return;

  if (x >= 0) {
    UpdateClipArea(side,x,y);
  }

  if (side->showsorderlevel != 0) {
    side->showsorderflag = TRUE; 
    return;
  }

  set_clipcoords(side);
/*
printf("draw_smove: clip=(%d,%d),(%d,%d)\n",side->clip_xmin,side->clip_ymin,
       side->clip_xmax,side->clip_ymax);
       */

  for_all_side_units(side,unit) {
    if (unit->standing) {
/*** (UK) changed showmorder to showsmorders ***/
      if (side->showsorderutype==ALL) {
	int u;
	for_all_unit_types(u) {
	  handle_showsmorders(side,
	     unit->standing->p_orders[u],unit->x,unit->y,'p');
	  handle_showsmorders(side,
	     unit->standing->e_orders[u],unit->x,unit->y,'e');
	  handle_showsmorders(side,
	     unit->standing->f_orders[u],unit->x,unit->y,'f');
	}
      }
      else {
	handle_showsmorders(side,
	   unit->standing->p_orders[side->showsorderutype],unit->x,unit->y,'p');
	handle_showsmorders(side,
	   unit->standing->e_orders[side->showsorderutype],unit->x,unit->y,'e');
	handle_showsmorders(side,
	   unit->standing->f_orders[side->showsorderutype],unit->x,unit->y,'f');
      }
    }
  }
  if (!side->show_orders) {
    side->clip_xmin = side->clip_ymin = INT_MAX;
    side->clip_xmax = side->clip_ymax = -1;
  }
}

IncrementSORedrawLevel(side)
Side *side;
{
  side->showsorderlevel++;
}

DecrementSORedrawLevel(side)
Side *side;
{
  side->showsorderlevel--;
  if (side->showsorderlevel == 0 && side->showsorderflag) {
    if (side->showsorderutype != NOTHING) draw_smoveorders(side,-1,-1);
    if (side->show_orders) draw_moveorders(side,-1,-1);
    side->showsorderflag = FALSE;
  }
}
/*** <- insert ***/

/* Draw a single unit icon as appropriate.  This *also* has a bunch of */
/* details to worry about: centering of icon in hex, clearing a rectangular */
/* area for the icon, picking a color for the unit, using either a bitmap */
/* or font char, and adding a side number for many-player games. */
/* Must also be careful not to draw black-on-black for units in space. */
/* This routine has also been drafted into drawing populace side numbers */
/* for otherwise empty hexes. */

draw_unit(side, x, y)
Side *side;
int x, y;
{
    viewdata view = side_view(side, wrap(x), y);
    int sx, sy, ucolor, hcolor, n;
    int terr = terrain_at(wrap(x), y);
    Side *side2;
/*** (HW) insert -> ***/
    int utype;
    Unit *unit;
    bool show_product = FALSE;
    long col;
    short ordertype = AWAKE;
    char orderchar;
/*** <- insert ***/

    if (view != UNSEEN) {
	if (view == EMPTY) {
/*	    if (populations) {
		pop = people_at(wrap(x), y);
		if (pop != NOBODY) {
		    side2 = side_n(pop-8);
		    pcolor = (allied_side(side, side2) ? side->owncolor :
			      (enemy_side(side, side2) ? side->enemycolor :
			       side->neutcolor));
		    if (pcolor == side->owncolor && 
			(ttypes[terr].dark ||
			 side->monochrome ||
			 (side->showmode == TERRICONS)))
			pcolor = side->fgcolor;
		    xform(side, x, y, &sx, &sy);
		    draw_side_number(side, side->map, sx, sy, pop-8, pcolor);
		}
	    } */
	} else {
	    xform(side, x, y, &sx, &sy);
	    side2 = side_n(vside(view));
	    ucolor = (allied_side(side, side2) ? side->owncolor :
		      (enemy_side(side, side2) ? side->enemycolor :
		       side->neutcolor));
	    if (ucolor == side->owncolor && 
		(ttypes[terr].dark ||
		 side->monochrome ||
		 (side->showmode == TERRICONS || side->showmode == ONEICON)))
		ucolor = side->fgcolor;
	    if (side->monochrome && side != side2)
		ucolor = side->bgcolor;
	    hcolor = (side == side2 ? side->bgcolor : side->fgcolor);
	    if (side->monochrome || side->showmode == ONEICON) {
		/* erasing background */
		draw_hex_icon(side, side->map, sx, sy, hcolor, HEX);
	    } else if (side->showmode != TERRICONS) {
		draw_hex_icon(side, side->map, sx, sy, hex_color(side, x, y),
			      ((side->showmode == BORDERHEX) ? OHEX : HEX));
	    }
/*** (HW) change -> ***/
	    utype = vtype(view);
	    unit = unit_at(wrap(x),y);
	    if (side->show_product) {
	      if (unit && side == unit->side && producing(unit)) {
		utype = unit->product;
		if (!side->monochrome) ucolor = side->productcolor;
		show_product = TRUE;
	      }
	    }
	    if (!show_product && !side->show_only_producers &&
		unit && side == unit->side && side->show_orders &&
	        !is_awake(unit)) {
	      ordertype = unit->orders.type;
	      switch(ordertype) {
		case SENTRY:
		  if (!side->monochrome) ucolor = side->sleepcolor;
		  orderchar = 'D';
		  break;
		default:
		  if (!side->monochrome) ucolor = side->ordercolor;
		  orderchar = 'C';
		  break;
	      }
	    }
	    if (side != side2 || 
                ((!side->show_only_producers || (unit && producing(unit))) &&
		 (!side->show_no_movers || (unit && !utypes[unit->type].speed)))
	       ) {
	      draw_unit_icon(side, side->map, sx, sy, utype, ucolor);
	      n = side_number(side2);
	      if ((numsides>2 || side->monochrome) && n != side_number(side)) {
		  draw_side_number(side, side->map, sx, sy, n, ucolor);
	      }
	      if (show_product)
		draw_info_char(side, side->map, sx, sy, 'B', ucolor);
	      else
		if (ordertype != AWAKE)
		  draw_info_char(side, side->map, sx, sy, orderchar, ucolor);
	    }
	    else {
	      col = (side->monochrome ? side->fgcolor :
		     (side->showmode == BOTHICONS) ?
		     terricon_color(side,wrap(x),y) :
		     hex_color(side,wrap(x),y));
	      if (!side->monochrome &&
		  side->showmode != TERRICONS && side->showmode != ONEICON)
		draw_hex_icon(side, side->map, sx, sy, hex_color(side, x, y),
			      ((side->showmode == BORDERHEX) ? OHEX : HEX));
	      if (side->monochrome ||
		  side->showmode == TERRICONS ||
		  side->showmode == BOTHICONS ||
		  side->showmode == ONEICON) {
		draw_terrain_row(side, sx, sy,
				 &ttypes[terrain_at(wrap(x),y)].tchar, 1, col);
	      }
	    }
/*** was:
	    draw_unit_icon(side, side->map, sx, sy, vtype(view), ucolor);
	    n = side_number(side2);
	    if ((numsides > 2 || side->monochrome) && n != side_number(side)) {
		draw_side_number(side, side->map, sx, sy, n, ucolor);
	    }
*** <- change ***/
	}
    }
}

/* Cursor drawing also draws the unit in some other color if it's not the */
/* "top-level" unit in a hex, as well as getting the player's attention */
/* if the new location is sufficiently far from the last. */

draw_cursor(side)
Side *side;
{
    int sx, sy;

    if (active_display(side)) {
	/* ugly hack to prevent extra cursor draw */
	if (side->cury == 0) return;
	xform(side, unwrap(side, side->curx, side->cury), side->cury, &sx, &sy);
	if (side->curunit != NULL && side->curunit->transport != NULL) {
	    if (side->monochrome) {
		draw_hex_icon(side, side->map, sx, sy, side->bgcolor, HEX);
		draw_unit_icon(side, side->map, sx, sy,
			       side->curunit->type, side->fgcolor);
	    } else {
/*** (UK) insert -> ***/
		if (side->showmode == ONEICON) 
		  draw_hex_icon(side, side->map, sx, sy, side->bgcolor, HEX);
/*** <- insert ***/
		draw_unit_icon(side, side->map, sx, sy,
			       side->curunit->type, side->diffcolor);
	    }
	}
	/* Flash something to draw the eye a long ways */
	if (humanside(side)) {
	    if (distance(side->curx, side->cury, side->lastx, side->lasty) > 3)
		flash_position(side, sx, sy, 100);
	    draw_cursor_icon(side, sx, sy);
	    side->lastx = side->curx;  side->lasty = side->cury;
	}
    }
}

/* Get rid of cursor by redrawing the hex. */

erase_cursor(side)
Side *side;
{
    if (side->lasty > 0) draw_hex(side, side->lastx, side->lasty, TRUE);
}

/* Draw a splat visible to both sides at a given location.  Several splats */
/* available, depending on the seriousness of the hit.  Make an extra-flashy */
/* display when The Bomb goes off.  Because of the time delays involved, we */
/* have to update both sides' displays more or less simultaneously. Would be */
/* better to exhibit to all sides maybe, but I'm not going to bother! */

draw_blast(unit, es, hit)
Unit *unit;
Side *es;
int hit;
{
    char ch;
    int ux = unit->x, uy = unit->y, sx, sy, i;
    Side *us = unit->side;

    if (hit >= period.nukehit) {
	if (active_display(us)) invert_whole_map(us);
	if (active_display(es)) invert_whole_map(es);
	/* may need a time delay if X is too speedy */
	if (active_display(us)) invert_whole_map(us);
	if (active_display(es)) invert_whole_map(es);
	for (i = 0; i < 4; ++i) {
	    if (active_display(us)) draw_mushroom(us, ux, uy, i);
	    if (active_display(es)) draw_mushroom(es, ux, uy, i);
	    if (i != 2 && (active_display(us) || active_display(es))) sleep(1);
	}
	if (active_display(us) || active_display(es)) sleep(1);
    } else {
	ch = ((hit >= unit->hp) ? 'd' : ((hit > 0) ? 'c' : 'b'));
	if (active_display(us)) {
/*** (UK) insert -> ***/
            if (cover(us,wrap(ux),uy)>0) {
/*** <- insert ***/
	    xform(us, unwrap(us, ux, uy), uy, &sx, &sy);
	    draw_blast_icon(us, us->map, sx, sy, ch, us->enemycolor);
	    flush_output(us);
/*** (UK) insert -> ***/
            }
/*** <- insert ***/
	}
	if (active_display(es)) {
/*** (UK) insert -> ***/
            if (cover(es,wrap(ux),uy)>0) {
/*** <- insert ***/
	    xform(es, unwrap(es, ux, uy), uy, &sx, &sy);
	    draw_blast_icon(es, es->map, sx, sy, ch, es->owncolor);
	    flush_output(es);
/*** (UK) insert -> ***/
            }
/*** <- insert ***/
	}
    }
}

/* Draw all the units in a column.  They should be spaced so they don't */
/* overlap or get misaligned with text, and can be inverted if desired. */

draw_unit_list(side, hilite)
Side *side;
bool hilite[];
{
    int u, pos = 0, spacing = max(side->hh, side->fh);
    
    if (active_display(side)) {
	for_all_unit_types(u) {
	    draw_hex_icon(side, side->state, side->margin, pos,
			  (hilite[u] ? side->fgcolor : side->bgcolor), OHEX);
	    draw_unit_icon(side, side->state, side->margin, pos, u,
			   (hilite[u] ? side->bgcolor : side->fgcolor));
	    pos += spacing;
	}
    }
}
