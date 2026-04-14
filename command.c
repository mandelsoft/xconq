/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file contains almost all command functions. */
/* Help commands are in a separate file. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"
#include "mplay.h"

char *grok_string();

bool reconfig;          /* true when an option needs screen reconfigured */
bool remap;             /* true when only main map needs redrawing */
bool reinfo;            /* true when only info display needs redrawing */
bool tmparea = TRUE;    /* true for area painting, false for bar painting */

/* Things will be totally scrogged if two human players in build mode... */

int tmpterr = 0;	/* temporary terrain type for area operation */
int tmpdist = 0;        /* temporary argument for painting */
short tmpwaketype;      /* temporary short for unit type to wake up... (gec)*/

/* Move in given direction a given distance - used for both single and */
/* automatic multiple moves. */

do_dir(side, dir, n)
Side *side;
int dir, n;
{

  enter_procedure("do_dir");
    if (side->teach) {
	cache_movedir(side, dir, n);
    } else {
	switch (side->mode) {
	case MOVE:
	    if (side->curunit != NULL) order_movedir(side->curunit, dir, n);
	    break;
	case SURVEY:
	    move_survey(side,
			wrap(side->curx + n*dirx[dir]),
			side->cury + n*diry[dir]);
	    break;
	default:
	    case_panic("mode", side->mode);
	}
    }
  exit_procedure();
}

/* Wake *everything* (that's ours) within the given radius.  Two commands */
/* actually; lowercase is transports only, uppercase is everybody. */

do_wakeup(side, n)
Side *side;
int n;
{

  enter_procedure("do_wakeup");
    if (side->teach) {
	cache_awake(side);
    } else {
	wakeup_area(side, n, -1);
    }
  exit_procedure();
}


do_wakespec(side, n)  /* gec 15.7.92 */
Side *side;
int n;
{

  enter_procedure("do_wakespec");
    if (side->teach) {
	cache_awake(side);
    } else {
	side->reqvalue2=n;
        request_wakespec(side);
    }
  exit_procedure();
}
actually_wakespec(side,waketype)
Side *side;
short waketype;
{
    if (Debug) printf("Waking up all %d in area of %d.\n",waketype,
				side->reqvalue2);
    if (waketype != NOTHING) wakeup_area(side,side->reqvalue2, waketype);
}

/* Wake up only the main/current unit in a hex. */

do_wakemain(side, n)
Side *side;
int n;
{

  enter_procedure("do_wakemain");
    if (side->teach) {
	cache_awake(side);
    } else {
        if (side->curunit != NULL)
	  wake_unit(side->curunit, NOTHING, WAKEFULL, (Unit *) NULL);
	wakeup_area(side, n, NOTHING);
    }
  exit_procedure();
}

/* The area wakeup. */

wake_at(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);

    enter_procedure("wake_at");
    if (unit != NULL && (unit->side == tmpside || Build))
	wake_unit(unit, tmpwaketype, WAKEFULL, (Unit *) NULL);
    exit_procedure();
}

wakeup_area(side, n, waketype)
Side *side;
int n;
short waketype;
{
    tmpside = side;
    tmpwaketype=waketype;
    apply_to_area(side->curx, side->cury, n, wake_at);
}

/* Put unit to sleep for a while.  If we sleep it next to something that */
/* might wake it up, then adjust flags so it won't wake up on next turn. */

do_sentry(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  enter_procedure("do_sentry");
    if (side->teach) {
	cache_sentry(side, n);
    } else {
	order_sentry(unit, n);
	if (n > 1 && adj_enemy(unit))
	    unit->orders.flags &= ~(ENEMYWAKE|NEUTRALWAKE);
    }
  exit_procedure();
}

/* Don't move for remainder of turn, but be awake next turn.  This also */
/* hooks into terrain painting, since the space bar is big and convenient. */

do_sit(side, n)
Side *side;
int n;
{
    if (side->mode == SURVEY && Build) {
	paint_terrain(side);
    } else if (side->curunit != NULL) {
	do_sentry(side, side->curunit, 1);
    } else {
	cmd_error(side, "No unit to operate on here!");
    }
}

/* Set unit to move to a given location.  */

y_moveto(side,unit,x,y)
Side *side;
Unit *unit;
int x,y;
{
  enter_procedure("x_moveto");
	if (side->teach) {
	    cache_moveto(side, x, y);
	} else if (Build) {
	    leave_hex(unit);
	    occupy_hex(unit, x, y);
	    make_current(side, unit);
	    all_see_hex(side->curx, side->cury);
	} else {
	  order_moveto(unit, x, y);
	}
	restore_cur(side);
  exit_procedure();
}

/* The command proper. */

do_moveto(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    ask_position(side, unit, y_moveto, "Move to where?");
}

/* order a unit to move toward another unit */

y_movetounit(side,unit,n)
Side *side;
Unit *unit;
bool n;
{
  if (n) {
    if (side->teach) {
      cache_movetounit(side, side->markunit, side->reqsvalue);
    } else {
      order_movetounit(side->tmpcurunit, side->markunit, side->reqsvalue);
    }
  }
}

do_movetounit(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  static char buf[200];
  if (side->markunit==NULL) {
    cmd_error(side, "No marked unit.  Use 'x' to mark the destination");
    return;
  } else {
    sprintf(buf, "Move toward %s?", unit_handle(side, side->markunit));
  }
  move_survey(side, side->markunit->x, side->markunit->y);
  side->tmpcurunit = unit;
  side->reqsvalue=n;
  ask_bool(side, unit, y_movetounit,buf, FALSE);
}

/* Determine how far away another point is.  */

y_distance(side,unit,x,y)
Side *side;
Unit *unit;
int x,y;
{

  enter_procedure("x_distance");
      restore_cur(side);
      notify(side, "Distance from (%d, %d) to (%d, %d) is %d",
	     side->curx, side->cury, x, y,
	     distance(side->curx, side->cury, x, y));
  exit_procedure();
}

/* The command proper. */

do_distance(side, n)
Side *side;
int n;
{
    ask_position(side, NULL, y_distance, "Distance to where?");
}

/* Search for a friendly refueler within range and set course for it.  */
/* Warn player and refuse to move if nothing close enough. */

do_return(side, unit, n)
Side *side;
Unit *unit;
int n;
{
#if 1
  if (side->teach) {
    cache_return(side, n);
  } else {
    order_return(unit, n);
    unit->orders.flags &= ~(SUPPLYWAKE|ENEMYWAKE|NEUTRALWAKE);
  }
#else
    int x, y, u = unit->type, r, range = max(world.width, world.height);

    enter_procedure("do_return");
    for_all_resource_types(r) {
	if (utypes[u].tomove[r] > 0) {
	    range = min(range, unit->supply[r] / utypes[u].tomove[r]);
	}
    }
    tmpside = side;
    tmpunit = unit;
    if (refuel_here(unit->x, unit->y)) {
      cmd_error(side, "Already at a resupply point!");
    }
    else if (search_area(unit->x, unit->y, range, refuel_here, &x, &y, 1)) {
	    order_moveto(unit, x, y);
	    unit->orders.flags = SHORTESTPATH;
	  } else {
	    cmd_error(side, "No resupply point in range!");
	  }
    exit_procedure();
#endif
}

/* Set unit to attempt to follow a coast.  Needs a starting direction, */
/* which can be computed from a position. */

y_coast(side,unit,x,y)
Side *side;
Unit *unit;
int x,y;
{
    int dir;
    int	curx,cury;

    enter_procedure("x_coast");
      curx = unit->x;
      cury = unit->y;
      if (curx != x || cury != y) {
	int	ccw = 0, count=0;

	dir = find_dir(x - curx, y - cury);
	if (!could_move_in_dir(unit, dir)) {
	  cmd_error(side, "%s can't move there!",
		    unit_handle(side,unit));
	} else {
	  if (could_move_in_dir(unit, left_of(dir))) {	  
	    ccw++; count++;
	  }
	  if (could_move_in_dir(unit, right_of(dir))) {
	    ccw--; count++;
	  }
	  if (count>0 && ccw==0) {
	    cmd_error(side, "Those hexes don't share an edge!");
	  } else {
	    notify(side, "%s will follow %s%s",
		   unit_handle(side,side->requnit),
		   (ccw==0)?"isthsmus":"edge ",
		   (ccw==0)?"":(ccw>0)?"right":"left");
	    
	    if (side->teach) {
	      cache_edge(side, dir, ccw, side->reqvalue2);
	    } else {
	      order_edge(side->requnit, dir, ccw, side->reqvalue2);
	    }
	  }
	}
	restore_cur(side);
      } else {
	cmd_error(side, "No particular direction at own hex??");
      }
    exit_procedure();
}

/* The command proper just sets up the interaction. */

do_coast(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    side->reqvalue2 = n;
    ask_position(side, unit, y_coast, "Move in which direction?");
}

/* find the nearest filling transport and order unit towards it */

do_movetotransport(side, unit, n)
     Side	*side;
     Unit	*unit;
     int	n;
{
  enter_procedure("do_movetotransport");
  
  if (side->teach) {
    cache_movetotransport(side, n);
  } else {
    order_movetotransport(unit, n);
  }

  exit_procedure();
}

/* Set orders to follow a leader unit. */

y_follow(side,unit,x,y)
Side *side;
Unit *unit;
int x,y;
{
    Unit *leader;

    enter_procedure("x_follow");
	if ((leader = unit_at(x, y)) != NULL &&
	    leader->side == side) {
	    if (leader != unit) {
		if (side->teach) {
		    cache_follow(side, leader, side->reqvalue2);
		} else {
		    order_follow(unit, leader, side->reqvalue2);
		}
	    } else {
		cmd_error(side, "Unit can't follow itself!");
	    }
	} else {
	    cmd_error(side, "No unit to follow!");
	}
	restore_cur(side);
    exit_procedure();
}

/* The command proper. */

do_follow(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    side->reqvalue2 = n;
    ask_position(side, unit, y_follow, "Which unit to follow?");
}

/* Patrolling goes back and forth between two points.  First point is the */
/* current position. */

y_patrol(side,unit,x,y)
Side *side;
Unit *unit;
int x,y;
{

  enter_procedure("x_patrol");
	if (side->teach) {
	    cache_patrol(side, side->sounit->x, side->sounit->y,
			 x, y, side->reqvalue2);
	} else {
	    order_patrol(unit, unit->x, unit->y, x, y, side->reqvalue2);
	}
	restore_cur(side);
  exit_procedure();
}

/* The command proper. */

do_patrol(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    side->reqvalue2 = n;
    ask_position(side, unit, y_patrol, "What other endpoint for patrol?");
}

/* Delay a unit's move until a later time.  The set flag will be recognized */
/* by the movement loops, when deciding which unit to move next. */

do_delay(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  if (side->mode == MOVE) {
    delete_unit(unit);
    insert_unit_tail(side->unithead, unit);
    unit->orders.type = DELAY;
    side->movunit = NULL;
    notify(side, "Delaying moving %s.", unit_handle(side, unit));
  }
}

/*** (UK) insert -> ***/
do_afteroccupants(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  if (side->mode == MOVE) {
    delete_unit(unit);
    insert_unit_tail(side->unithead, unit);
    side->movunit = NULL;
  }
}
/*** <- insert ***/

/* Get rid of unit.  Some units cannot be disbanded, but if they can, the */
/* resources go to a transport if one is there.  Disbanding a transport */
/* also disbands all the occupants - oh well. */

do_disband(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    int u = unit->type;

    enter_procedure("do_disband");
    if ((utypes[u].disband && ((!No_give_units) || (unit->side == side)))
	|| Build) { /* XXX */
	notify(side, "%s goes home.", unit_handle(side, unit));
	if (unit->transport != NULL) recycle_unit(unit, unit->transport);
	kill_unit(unit, DISBAND);
	make_current(side, unit_at(side->curx, side->cury));
    } else {
	cmd_error(side, "You can't just get rid of the %s!", utypes[u].name);
    }
    exit_procedure();
}

/* Reclaim both the unit's supplies and anything used in its making, but */
/* only let a maker of the unit reclaim its ingredients. */

recycle_unit(unit, unit2)
Unit *unit, *unit2;
{
    int u = unit->type, u2 = unit2->type, r, scrap;

    for_all_resource_types(r) {
	transfer_supply(unit, unit2, r, unit->supply[r]);
	if (could_make(u2, u) > 0) {
	    scrap = utypes[u].tomake[r];
	    unit2->supply[r] += (scrap * period.efficiency) / 100;
	}
    }
}

/* Give a unit to another side (possibly to neutrals).  Units that won't */
/* change their sides when captured won't change voluntarily either. */

do_give_unit(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    int u = unit->type;

    if ((utypes[u].changeside && ((!No_give_units) || (unit->side == side)))
	|| Build) { /* XXX */
	if (side_n(n) == NULL) { /* XXX */
		cmd_error(side, "To which side???");
		return;
	}
	unit_changes_side(unit, side_n(n), CAPTURE, GIFT);
	all_see_hex(unit->x, unit->y);
	if (global.setproduct) {
	  set_product(unit, NOTHING);
	  unit->schedule = 0;
	} 
   } else {
	cmd_error(side, "You can't just give away the %s!", utypes[u].name);
    }
}

/* Marking is for the purpose of rearranging units within a hex. */

do_mark_unit(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    side->markunit = unit;
    notify(side, "%s has been marked.", unit_handle(side, unit));
}

/* This is a clever (if I do say so myself) command to examine all occupants */
/* and suboccupants, in a preorder fashion. */

do_occupant(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    Unit *nextup;

    switch (side->mode) {
    case MOVE:
	cmd_error(side, "Can only look at occupants when in survey mode!");
	break;
    case SURVEY:
	if (unit->occupant != NULL) {
	    make_current(side, unit->occupant);
	} else if (unit->nexthere != NULL) {
	    make_current(side, unit->nexthere);
	} else {
	    nextup = unit->transport;
	    if (nextup != NULL) {
		while (nextup->transport != NULL && nextup->nexthere == NULL) {
		    nextup = nextup->transport;
		}
		if (nextup->nexthere != NULL)
		    make_current(side, nextup->nexthere);
		if (nextup->transport == NULL)
		    make_current(side, nextup);
	    }
	}
	break;
    default:
	case_panic("mode", side->mode);
	break;
    }
}

/* This can actually do general rearrangement, but defaults to putting the */
/* unit on the first available transport in the hex.  */
/* What about trying to embark a unit on itself or on its previous transp? */

do_embark(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  Unit *mainunit = unit_at(unit->x, unit->y);
  Unit *transport = NULL;
  
  enter_procedure("do_embark");
  if (side->teach) {
    cache_embark(side, n);
  } else {
    if (mainunit != unit) {
      if (side->markunit == NULL ||
	  side->markunit->x != unit->x || side->markunit->y != unit->y) {
	for_all_occupants(mainunit, transport) {
	  if (can_carry(transport, unit)) break;
	}
      } else if (can_carry(side->markunit, unit)) { /* thx peterw@reed */
	transport = side->markunit;
      }
      if (transport != NULL) {
	if (can_carry(transport, unit)) {
	  leave_hex(unit);
	  occupy_unit(unit, transport);
/*** (HW) change -> ***/
	  order_sentry(unit,100);
/*** was:
	  if (n > 0) order_sentry(unit, n);
*** <- change ***/
	  routine(" done embark");
	  
	  exit_procedure();
	  return TRUE;
	}
	else {
	  if (n > 0) order_embark(unit, n);
	  else cmd_error(side, "Marked unit already full.");
	}
      } else {
	if (n > 0) order_embark(unit, n);
	else cmd_error(side, "No plausible transport!");
      }
    } else {
      cmd_error(side, "Nothing for this unit to get into!");
    }
    exit_procedure();
    return FALSE;
  }
}

/*** (UK) insert -> ***/
try_unit_pickup(side,unit,n)
Side *side;
Unit *unit;
{ Unit *u;
  int amount=0;
  Unit *transport=unit->transport;

  while (transport) {
    do {
      for_all_occupants(transport,u) {
	if (can_carry(unit, u)) break;
      }
      if (u) {
	  leave_hex(u);
	  occupy_unit(u, unit);
	  if (is_awake(u)) 
/*
	  if (transport->standing == NULL ||
	      transport->standing->e_orders[u->type] == NULL)
*/
	    order_sentry(u,100);
	  amount++;
	  if (amount>=n) return amount;
      }
    } while (u);
    transport=transport->transport;
  }
  return(amount);
}

try_pickup(side,unit,n)
Side *side;
Unit *unit;
{ int amount=0;
  Unit *occ;
  int found;
  int i;

  do {
    found=0;
    for_all_occupants(unit,occ) {
      i=try_pickup(side,occ,n-amount);
      amount+=i;
      found+=i;
      if (amount>=n) return amount>=0;
    }
    amount+=try_unit_pickup(side,unit,n-amount);
  } while (amount<n && found);
  return(amount>0);
}

do_pickup(side, unit, n)
Side *side;
Unit *unit;
int n;
{ 
  if (side->teach) {
      cache_pickup(side, n);
  } else {
    if (n>1) order_pickup(unit,n);
    else if (!try_pickup(side,unit,100))
      cmd_error(side,"Nothing found to pickup!");
  }
}

handle_pickup_amount(side,unit,a,n)
Side *side;
Unit *unit;
int a,n;
{
  if (side->teach) {
      cache_pickup_amount(side,a,n);
  } else {
    if (n>1) order_pickup_amount(unit,a,n);
    else if (!try_unit_pickup(side,unit,a))
      cmd_error(side,"Nothing found to pickup!");
  }
}

y_pickup_amount(side,unit,n)
Side *side;
Unit *unit;
int n;
{
  if (n<1) ask_number(side,unit,y_pickup_amount,"How many units (>0) ?");
  else handle_pickup_amount(side,side->requnit,n,side->reqsvalue);
}

do_pickup_amount(side,unit,n)
Side *side;
Unit *unit;
int n;
{
  if (n<0) {
    side->reqsvalue=-n;
    ask_number(side,unit,y_pickup_amount,"How many units?");
  }
  else {
    handle_pickup_amount(side,unit,n,1);
  }
}

/************/
try_leave(side,unit)
Side *side;
Unit *unit;
{
    Unit *mainunit = unit_at(side->curx, side->cury);
    Unit *transport = NULL;

    if (mainunit != unit) {
	if (side->markunit == NULL ||
	    side->markunit->x != unit->x || side->markunit->y != unit->y) {

            if (transport=unit->transport) 
	      while (transport=transport->transport)
		if (can_carry(transport, unit)) break;

	} else {
	    if (side->markunit &&
		can_carry(side->markunit,unit)) transport = side->markunit;
	    else transport=0;
	}
	if (transport != NULL) {
	    leave_hex(unit);
	    occupy_unit(unit, transport);
	    return TRUE;
	} 
    }
    return FALSE;
}

do_leave(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    Unit *mainunit = unit_at(side->curx, side->cury);

    if (side->teach) {
	cache_leave(side, n);
    } else {
      if (mainunit != unit) {
	  if (n>1) order_leave(unit,n);
	  else if (!try_leave(side,unit)) {
	      cmd_error(side, "No plausible transport!");
	  }
      } else {
	  cmd_error(side, "Nothing for this unit to get into!");
      }
    }
}

try_unit_unload(side,unit)
Side *side;
Unit *unit;
{ Unit *u;
  int success=FALSE;
  int local;
  do {
    local=FALSE;
    for_all_occupants(unit,u) {
      if (local=try_leave(side,u)) break;
    }
    success|=local;
  } while (local);
  return(success);
}

try_unload(side,unit)
Side *side;
Unit *unit;
{ bool success=FALSE;
  Unit *occ;
  for_all_occupants(unit,occ) success|=try_unload(side,occ);
  success|=try_unit_unload(side,unit);
  return(success);
}

do_unload(side, unit, n)
Side *side;
Unit *unit;
int n;
{ 
  if (side->teach) {
      cache_unload(side, n);
  } else {
    if (!try_unload(side,unit)) cmd_error(side,"Nothing found to unload!");
  }
}

/*** <- insert ***/

#ifndef NO_DISEMBARK
/* Disembark is intended to complement embarking... */
/*  This should, theoretically, be easier to implement than do_embark */

do_disembark(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  Unit *mainunit = unit_at(unit->x, unit->y);
  
  enter_procedure("do_disembark");
  if (side->teach) { /* setting a standing order? */
    cache_disembark(side, n);
  } else {
    /* find a unit to occupy. In this case, the 'mainunit' */
    if (mainunit != unit) {
      if(can_carry(mainunit, unit)){
	leave_hex(unit);
	occupy_hex(unit, mainunit->x, mainunit->y);
	routine(" done disembark");

	exit_procedure();
	return TRUE;
      } else {
        cmd_error(side, "Can't disembark onto the unit here!!");

        exit_procedure();
        return FALSE;
      }
    }  else {
      cmd_error(side, "That unit can't exit itself!");
      exit_procedure();
      return FALSE;
    }
    /* control should not reach here... */
  }
  exit_procedure();
  return FALSE; /* ?? */
}
#endif

do_fill(side, unit, n)
     Side *side;
     Unit *unit;
     int n;
{
  enter_procedure("do_fill");
  if (side->teach)
    cache_fill(side,n);
  else {
    order_fill(unit, n);
    wake_if_full(unit);
  }
  exit_procedure();
}

/* Give supplies to a transport.  The argument tells how many to give. */

do_give(side, unit, n)
Side *side;
Unit *unit;
int n;
/*** (UK) insert -> ***/
{
  if (side->teach) cache_give(side,n);
  else handle_order_give(side,unit,n,TRUE);
}


handle_order_give(side,unit,n,nflag)
Side *side;
Unit *unit;
/*** <- insert ***/
{
    bool something = FALSE;
    int u = unit->type, m, r, gift, actual;
    Unit *main = unit->transport;

#if 0 
    /* can't set give/take as standing orders! */
    if(side->teach){
	cache_give(side, n);
    }else 
#endif
    if (main != NULL) {
	sprintf(spbuf, "");
	m = main->type;
	for_all_resource_types(r) {
	    gift = (n < 0 ? utypes[m].storage[r] - main->supply[r] : n);
	    if (gift > 0) {
		something = TRUE;
		/* Be stingy if we're low */
		if (2 * unit->supply[r] < utypes[u].storage[r])
		    gift = max(1, gift/2);
		actual = transfer_supply(unit, main, r, gift);
		sprintf(tmpbuf, " %d %s", actual, rtypes[r].name);
		strcat(spbuf, tmpbuf);
	    }
	}
/*** (UK) insert -> ***/
        if (nflag)
/*** <- insert ***/
	if (something) {
	    notify(side, "%s gave%s.", unit_handle(side, unit), spbuf);
	} else {
	    notify(side, "%s gave nothing.", unit_handle(side, unit));
	}
    } else {
	cmd_error(side, "Can't transfer supplies here!");
    }
}

/* Take supplies from transport.  Both the transport must have something */
/* left. */

do_take(side, unit, n)
Side *side;
Unit *unit;
int n;
/*** (UK) insert -> ***/
{
  if (side->teach) cache_take(side,n);
  else handle_order_take(side,unit,n,TRUE);
}


handle_order_take(side,unit,n,nflag)
Side *side;
Unit *unit;
/*** <- insert ***/
{
    bool something = FALSE;
    int u = unit->type, m, r, need, actual;
    Unit *main = unit->transport;

#if 0
    /* can't set give/take as standing orders! */
    if(side->teach){
	cache_take(side, n);
    } else 
#endif
    if (main != NULL) {
	sprintf(spbuf, "");
	m = main->type;
	for_all_resource_types(r) {
	    need = (n < 0 ? utypes[u].storage[r] - unit->supply[r] : n);
	    if (need > 0) {
		something = TRUE;
		/* Be stingy if we're low */
		if (2 * main->supply[r] < utypes[m].storage[r])
		    need = max(1, need/2);
		actual = transfer_supply(main, unit, r, need);
		sprintf(tmpbuf, " %d %s", actual, rtypes[r].name);
		strcat(spbuf, tmpbuf);
	    }
	}
/*** (UK) insert -> ***/
        if (nflag)
/*** <- insert ***/
	if (something) {
	    notify(side, "%s got%s.", unit_handle(side, unit), spbuf);
	} else {
	    notify(side, "%s needed nothing.", unit_handle(side, unit));
	}
    } else {
	cmd_error(side, "Can't transfer supplies here!");
    }
}

/* Take the current player out of the game while letting everybody else */
/* continue on. */

y_resign(side,unit,b)
Side *side;
Unit *unit;
bool b;
{
    if (b) resign_game(side, side->reqoside);
}

do_resign(side, n)
Side *side;
int n;
{
  Side	*other;
  other = side->reqoside = side_n(n);
  if (other == side || (other != NULL && other->lost)) {
    cmd_error(side, "You have to give units to an active side.");
    return;
  }
  ask_bool(side, NULL, y_resign, "Do you really want to give up?", FALSE);
}

/* Unconditional resignation - usable by everybody. */

resign_game(side, side2)
Side *side, *side2;
{
    notify_all("Those wimpy %s have given up!", plural_form(side->name));
    if (side2 != NULL) {
	notify_all("... and they gave all their stuff to the %s!",
		   plural_form(side2->name));
    }
    side_loses(side, side2);
}

/* Leave quickly when the boss walks by.  One person can kill a multi-player */
/* game, which isn't too great, but the alternatives are complicated. */
/* The stats file will be left behind, to foment argument about who would */
/* have won... This routine also includes a trapdoor for freezing/unfreezing */
/* machine players when building - mode display will invert to confirm this. */

y_exit(side,u,b)
Side *side;
Unit *u;
bool b;
{
    Unit *unit;

    if (b) {
	Side *s;
	notify_all("Game will be quitted now!");
	for_all_sides(s) flush_output(s);
	if (!write_savefile(SAVEFILE)) /* XXX */
	    cmd_error(side, "Can't open file \"%s\"!", SAVEFILE);
	close_displays();
	printf("\nThe outcome remains undecided");
	if (numhumans == 1 && side->humanp) {
	    printf(", but you're probably the loser!\n\n", side->host);
	} else {
	    printf("...\n\n");
	}
	/* Need to kill off all units to finish up statistics */
	for_all_side_units(side,unit)
	  if (alive(unit)) kill_unit(unit, ENDOFWORLD);
	print_statistics();
	exit(0);
    }
    if (Build) {
	Freeze = !Freeze;
	show_timemode(side,TRUE);
    }
}

do_exit(side, n)
Side *side;
int n;
{
    ask_bool(side, NULL, y_exit, 
	      "Do you REALLY want to end the game for EVERYBODY?", FALSE);
}

/* Stuff game state into a file.  By default, it goes into the current */
/* directory.  If building a scenario, will ask about each section, values */
/* of globals, and dest file before actually writing anything. */
/* No capability to write out period at present... */

y_save_1(side,unit,sects)
Side *side;
Unit *unit;
char *sects;
{
    char *fname;
    int sdetail = 1, udetail = 1;

/*** (HW) insert -> ***/
	if (!strcmp("SAVE",sects)) {
	  if (!write_savefile(SAVEFILE)) { /* XXX */
	    cmd_error(side, "Can't open file \"%s\"!", SAVEFILE);
	  }
	  notify_all("Changes saved to \"%s\" ...", SAVEFILE);
	  return;
	}
/*** <- insert ***/
	fname = "random.scn";
	sprintf(spbuf, "------");
	if (iindex('v', sects) >= 0) spbuf[0] = '+';
	if (iindex('p', sects) >= 0) spbuf[1] = '+';
	if (iindex('m', sects) >= 0) spbuf[2] = '+';
	if (iindex('g', sects) >= 0) spbuf[3] = '+';
	if (iindex('s', sects) >= 0) spbuf[4] = '+';
	if (iindex('u', sects) >= 0) spbuf[5] = '+';
	if (iindex('s', sects) >= 0) {
	    sdetail = 1;
	    if (isdigit(sects[iindex('s', sects)+1]))
		sdetail = sects[iindex('s', sects)+1] - '0';
	}
	if (iindex('u', sects) >= 0) {
	    udetail = 1;
	    if (isdigit(sects[iindex('u', sects)+1]))
		udetail = sects[iindex('u', sects)+1] - '0';
	}
	notify(side, "Mapfile with sections %s will be saved to \"%s\" ...",
	       spbuf, fname);
	if (write_scenario(fname, spbuf, sdetail, udetail)) {
	    notify(side, "Done writing to \"%s\".", fname);
	} else {
	    cmd_error(side, "Can't open file \"%s\"!", fname);
	}
/*** (UK) insert -> ***/
        free(sects);
/*** <- insert ***/
}

/* Make a header appropriate to a save file, write the file, and leave. */

y_save_2(side,unit,b)
Side *side;
Unit *unit;
bool b;
{ 
if (b) {
	Side *s;
	notify_all("Game will be saved to \"%s\" ...", SAVEFILE);
	for_all_sides(s) flush_output(s);
	if (!write_savefile(SAVEFILE)) { /* XXX */
	    cmd_error(side, "Can't open file \"%s\"!", SAVEFILE);
	}
    }
}

/* The command proper just sets up different handlers, depending on */
/* whether we're building (and therefore saving a scenario/fragment), or */
/* saving as much game state as possible, for resumption later. */

do_save(side, n)
Side *side;
int n;
{
    if (Build) 
	ask_string(side, NULL, y_save_1, "Sections to write?", "ms1u1");
    else 
	ask_bool(side, NULL, y_save_2, "You really want to save?", FALSE);
}

/* Redraw everything using the same code as when windows need a redraw. */

do_redraw(side, n)
Side *side;
int n;
{
  int x, y, ts;

#ifdef PREVVIEW
    if (n > 0) {  /* remove the views of units likely to have moved. */
      for_all_hexes(x, y)
	  if (side_view_age(side, x, y) > n) {
	    int viewunit = vtype(side_view(side, x, y));

	    if (viewunit != EMPTY && viewunit != UNSEEN
		&& mobile(viewunit)) { 
	      ts = side_view_timestamp(side, x, y);
	      set_side_view(side, x, y, EMPTY);
	      side_view_timestamp(side, x, y) = ts;
	    }
	  }
    }
#endif
    redraw(side);
}

/* Flicker on the current position, in case it's not easily visible. */

do_flash(side, n)
Side *side;
int n;
{
    int sx, sy;

    xform(side, unwrap(side, side->curx, side->cury), side->cury, &sx, &sy);
    flash_position(side, sx, sy, 1000);
}

/* Name or rename the current unit or a given side.  We make a copy of the */
/* string after it's been successfully read, just in case. */

y_name(side,unit,name)
Side *side;
Unit *unit;
char *name;
{ Side *side2;

    if (strlen(name) == 0) {
        free(name);
	notify(side, "Name not changed.");
    } else if (unit != NULL) {
	if (unit->name) free(unit->name);
	unit->name = name;
    } else if (side->reqoside != NULL) {
	if (side->reqoside->name) free(side->reqoside->name);
	side->reqoside->name = name;
	for_all_sides(side2) show_all_sides(side2,TRUE);
    } else {
	cmd_error(side, "Nothing to name!");
    }
}

/* The command proper decides between unit and side naming. */

do_name(side, n)
Side *side;
int n;
{
    if (side->curunit != NULL) {
	ask_string(side, side->curunit, y_name, "New name for unit:", side->curunit->name);
    } else {
	ask_string(side, NULL, y_name, "New name for yourself:", side->name);
	side->reqoside = side;
    }
}

/*** (UK) insert -> ****/
y_group(side,unit, name)
Side *side;
Unit *unit;
char *name;
{
  if (!strcmp(name,"*") || !strcmp(name," ")) {
    cmd_error(side,"illegal group name!");
  } else if (side->teach) {
    cache_group(side,name);
    free(name);
  } else if (unit != NULL) {
    if (unit->group) free(unit->group);
    if (strlen(name)==0) {
      unit->group = NULL;
      free(name);
    }
    else unit->group = name;
  } else {
    if (side->curgroup) free(side->curgroup);
    if (strlen(name)==0) {
      side->curgroup = NULL;
      free(name);
    }
    else side->curgroup=name;
    show_timemode(side);
  }
}

do_group(side, n)
Side *side;
int n;
{ Unit *unit;
  if (side->teach || (unit=side->curunit)) {
    if (side->teach)
      ask_string(side, unit, y_group, "Setting group to:", side->curgroup);
    else if (unit->group) 
      ask_string(side, unit, y_group, "New group for unit:", side->curgroup);
    else
      ask_string(side, unit, y_group, "Group for unit:", side->curgroup);
  }
  else {
    ask_string(side, NULL, y_group, "Current group:", NULL);
  }
}


y_close(side,u,b)
Side *side;
Unit *u;
bool b;
{  Side  *side2;
   if (b) {
      extern bool PartialGame;
      cancel_request(side);
      side->movunit=NULL;
      wait_to_exit(side);
      clear_prompt(side);
      close_display(side);
      for_all_sides(side2) show_all_sides(side2,TRUE);
      notify_all("Display closed for the %s.",plural_form(side->name));
      if (PartialGame) printf("side %d closes display\n",side_number(side));
      write_savefile(SAVEFILE);
   }
}

do_close(side,n)
Side *side;
{   Side *side2;

    extern bool ServerFlag;

    if (active_display(side)) {
	for_all_sides(side2) {
	  if ((side!=side2 || ServerFlag) && active_display(side2)) {
	    ask_bool(side, NULL, y_close, 
		      "Do you really want to close your display?", FALSE);
	    return;
	  }
	}
	do_exit(side,0);
    }
}

do_open(side,n)
{  Side *side2;
   extern bool singlePlayerMode;

   if (singlePlayerMode) cmd_error(side,"Not in single player mode!");
   else {
     if (!(side2=side_n(n))) cmd_error(side,"Illegal side number to open!");
     else {
       if (!side2->host) cmd_error(side,"no display for this side!");
       else {
	 if (active_display(side2)) cmd_error(side,"side already active!");
	 else {
	   if (!init_display(side2)) cmd_error(side,"could not open display!");
	   else {
	     init_curxy(side2);
	     notify_all("Display opened for the %s.",plural_form(side2->name));
	     for_all_sides(side2) show_all_sides(side2,FALSE);
	   }
	 }
       }
     }
   }
}

y_passwd(side,u,s)
Side *side;
Unit *u;
char *s;
{ 

  if (!strcmp(s,"*") || !strcmp(s," ")) {
    free(s);
    cmd_error(side,"illegal passwd!");
  }
  else {
    if (side->passwd) free(side->passwd);
    side->passwd=s;
  }
}


do_passwd(side,n)
Side *side;
{  Side *side2;

       if (!active_display(side)) cmd_error(side,"side is not active!");
       else {
	    ask_string(side, NULL, y_passwd, 
		      "New password?", side->passwd);
       }
}
/*** <- insert ***/

/* Designate the current location as the center of action and sort all */
/* of our own units relative to it. */

do_center(side, n)
Side *side;
int n;
{
    side->cx = side->curx;  side->cy = side->cury;
/*** (HW) change -> ****/
    sort_side_units(side);
/*** was:
    sort_units(TRUE);
*** <- change ***/
    set_side_movunit(side);
    notify(side, "Units reordered.");
}

/* Center the screen on the current location. */

do_recenter(side, n)
Side *side;
int n;
{

    recenter(side, side->curx,side->cury);
    if (Debug) notify(side, "Current location %d %d ",  side->curx,side->cury);
}

/*** (HW) insert -> ***/
y_showsorder_utype(side,unit,u)
Side *side;
Unit *unit;
int u;
{
  int oldu=side->showsorderutype;

    if (u != oldu) {
      side->showsorderutype = u;
      update_state(side,oldu);
      update_state(side,u);
      show_map(side);
      show_timemode(side);
    }
}
/*** <- insert ***/

/* Hook command to set miscellaneous options.  Can't do from command line */
/* because each display may want different behavior.  This routine can */
/* change the display dramatically, but it should only redraw if a change */
/* has actually been made. */

/* Conversion to machine player is irreversible, so we confirm it first. */

y_options_2(side,u,b)
Side *side;
Unit *u;
bool b;
{
  Unit *unit;

    if (b) {
	side->humanp = !side->humanp;
	for_all_side_units(side, unit) 
	  unit->area = area_index(unit->x, unit->y);
	numhumans--;
	init_sighandlers();
    }
    else {
      notify(side,"This is a wise choice!");
    }
}

y_options(side,unit,opt)
Side *side;
Unit *unit;
char opt;
{
    int n = side->reqvalue2;
/*** (UK) insert -> ***/
    bool tm_redraw=FALSE;
/*** <- insert ***/

	switch (opt) {
/*** (UK) insert -> ***/
        case 'l':
	    do_look(side,-1);
	    tm_redraw=TRUE;
	    break;
/*** <- insert ***/
	case '?':
  notify(side, "Change window (h)eight, window (w)idth, world map (m)agnification,");
  notify(side, "  (n)otice buffer height, start (b)eep time,");
  notify(side, "Toggle (d)isplay mode, (g)raph mode , (i)nverse video, (2)-color,"); 
  notify(side, "  (a)rrow mode, (t)hin/thick lines, (e)distinct sorders");
  notify(side, "  (l)ook mode, (v)isibility mode,");
  notify(side, "Show/Hide (p)roducts, (P)roducers, (o)rders, (s)tanding orders,");
  notify(side, "  (M)overs");
  notify(side, "Become (r)obot, (z)enter working mode, (R)eset display");
/*** (UK) insert -> ***/
	    do_options(side,side->reqvalue2);
/*** <- insert ***/ 
	    break;
	case '2':
	    reconfig = TRUE;
	    side->monochrome = !side->monochrome;
	    side->bonw = FALSE;
/*** (HW) insert -> ***/
	    remap = TRUE;
	    show_world(side);
/*** <- insert ***/
	    break;
/*** (HW) insert -> ***/
	case 'a':
	    side->linemode = (side->linemode + 1) % NUMLINEMODES;
	    if (side->showsorderutype != NOTHING || side->show_orders)
	      remap = TRUE;
	    set_line_mode(side);
	    break;
/*** <- insert ***/
	case 'b':
	    side->startbeeptime = n;
	    break;
/*** (UK) insert -> ***/
        case 'c':
	    side->compactmode=!side->compactmode;
	    break;
/*** <- insert ***/
	case 'd':
	    remap = TRUE;
	    side->showmode = (side->showmode + 1) % NUMSHOWMODES;
	    set_line_mode(side);
	    break;
/*** (HW) insert -> ***/
	case 'e':
	    side->distinctsotypes = !side->distinctsotypes;
	    if (side->showsorderutype != NOTHING || side->show_orders)
	      remap = TRUE;
	    set_line_mode(side);
	    break;
/*** <- insert ***/
	case 'g':
	    reinfo = TRUE;
	    side->graphical = !side->graphical;
	    break;
	case 'h':
	    if (n < 5 || n > world.height) {
		cmd_error(side, "Bad height %d!", n);
	    } else {
		if (n != side->vh) reconfig = TRUE;
		side->vh = n;
	    }
	    break;
	case 'i':
	    if (side->monochrome) {
		reconfig = TRUE;
		side->bonw = !side->bonw;
	    } else {
		cmd_error(side, "Inverse video is only for monochrome!");
	    }
	    break;
	case 'm':
	    if (n<1 || n>5) {
	      cmd_error(side, "Bad magnification %d", n);
	    } else if (n!=side->mm) {
	      side->mm = n;
	      /* we don't reconfig=TRUE because map magnification
		 is normally a function of other things outside
		 easy control. */
	      if (active_display(side))
		resize_display2(side, side->mw, side->mh);
	      reconfigure_display(side,FALSE);
	    }
	    break;
	case 'n':
	    if (n < 1 || n > MAXNOTES) {
		cmd_error(side, "Bad number of notes %d!", n);
	    } else {
		if (n != side->nh) reconfig = TRUE;
		side->nh = n;
	    }
	    break;
/*** (HW) insert -> ***/
	case 'M':
	    side->show_no_movers = !side->show_no_movers;
	    remap = TRUE;
	    tm_redraw=TRUE;
	    break;
	case 'o':
	    side->show_orders = !side->show_orders;
	    remap = TRUE;
	    tm_redraw=TRUE;
	    break;
	case 'p':
	    side->show_product = !side->show_product;
	    remap = TRUE;
	    tm_redraw=TRUE;
	    break;
	case 'P':
	    side->show_only_producers = !side->show_only_producers;
	    remap = TRUE;
	    tm_redraw=TRUE;
	    break;
/*** <- insert ***/
/*** (UK) insert -> ***/
        case 'R':
	    resize_display(side, display_width(side), display_height(side));
	    reconfig=TRUE;
            break;
/*** <- insert ***/
	case 'r':	
	    if (side->mode == MOVE) {
		ask_bool(side,NULL,y_options_2,
			 "Do you really want to become a machine?", FALSE);
		return;
	    } else {
		cmd_error(side, "Must be in move mode!");
	    }
	    break;
/*** (HW) insert -> ***/
	case 's':
	    ask_unit_type(side, side->requnit, y_showsorder_utype, "show standing move orders for", NULL);
	    break;
/*** (UK) insert -> ***/
        case 'S': 
	    if (side->showsorderutype==ALL)
	      y_showsorder_utype(side,side->requnit,NOTHING);
	    else
	      y_showsorder_utype(side,side->requnit,ALL);
            break;
/*** <- insert ***/
	case 't':
	    side->thicknessmode = (side->thicknessmode+1) % NUMTHICKNESSMODES;
	    if (side->showsorderutype != NOTHING || side->show_orders)
	      remap = TRUE;
	    break;
	case 'v':
	    side->notifyvisibility = !side->notifyvisibility;
	    remap = TRUE;
	    tm_redraw=TRUE;
	    break;
/*** <- insert ***/
	case 'w':
	    if (n < 5 || n > world.width || n > BUFSIZE) {
		cmd_error(side, "Bad width %d!", n);
	    } else {
		if (n != side->vw) reconfig = TRUE;
		side->vw = n;
	    }
	    break;
/*** (UK) insert -> ***/
        case 'z':
	    side->savflag=FALSE;
	    tm_redraw=TRUE;
	    if (side->workmode) {
	      if (side->show_orders!=side->w_show_orders) remap=TRUE;
	      if (side->show_product!=side->w_show_product) remap=TRUE;
	      if (side->show_only_producers!=side->w_show_only_producers) 
		remap=TRUE;
	      if (side->show_no_movers!=side->w_show_no_movers) remap=TRUE;
	      if (side->showsorderutype!=side->w_showsorderutype) remap=FALSE;

	      side->show_orders=        side->w_show_orders;
              side->followaction=       side->w_followaction;
	      side->show_product=       side->w_show_product;
	      side->show_only_producers=side->w_show_only_producers;
	      side->show_no_movers=     side->w_show_no_movers;

	      side->     workmode=FALSE;

	      y_showsorder_utype(side,side->requnit,side->w_showsorderutype);
	    }
	    else {
	      side->w_show_orders=        side->show_orders;
              side->w_followaction=       side->followaction;
	      side->w_show_product=       side->show_product;
	      side->w_show_only_producers=side->show_only_producers;
	      side->w_show_no_movers=     side->show_no_movers;
	      side->w_showsorderutype=    side->showsorderutype;

	      if (side->show_orders!=FALSE) remap=TRUE;
	      if (side->show_product!=TRUE) remap=TRUE;
	      if (side->show_only_producers!=FALSE) remap=TRUE;
	      if (side->show_no_movers!=TRUE) remap=TRUE;
	      if (side->showsorderutype!=ALL) remap=FALSE;

	      side->savmode=SURVEY;
	      if (side->mode==MOVE) survey_mode(side);

	      side->show_orders=FALSE;
	      side->followaction=FALSE;
	      side->show_product=TRUE;
	      side->show_only_producers=FALSE;
	      side->show_no_movers=TRUE;

	      side->workmode=TRUE;

	      y_showsorder_utype(side,side->requnit,ALL);
	    }
	    break;
/*** <- insert ***/
	default:
	    cmd_error(side, "unrecognized option '%c'", opt);
	    break;
	}
	if (remap) {
	    /* undraw_box(side); */
	    show_map(side);
	}
/*** (UK) insert -> ***/
        if (tm_redraw) show_timemode(side);
/*** <- insert ***/
	if (reinfo) show_info(side);
	if (reconfig) {
	  if (active_display(side))
	    set_sizes(side);
	  reconfigure_display(side,TRUE);
/*** (UK) insert -> ***/
          redraw(side);
/*** <- insert ***/
	}
}

/* The command proper. */

do_options(side, n)
Side *side;
int n;
{
    reinfo = remap = reconfig = FALSE;
    side->reqvalue2 = n;
    ask_char(side, NULL, y_options, "Options:", "?2abcdeghilmMnopPrRsStvwz");
}



/* Show standing orders for a unit */

do_show_standing(side, unit, n)
Side *side;
Unit *unit;
int n;
{
   show_standing_orders(side,unit);
}


/*** (UK) insert -> ***/

StandingOrderEntry **get_current_sorder_array(side)
Side *side;
{
    if (side && side->sounit && side->sounit->standing)
      switch (side->somode) {
	case 1: return side->sounit->standing->p_orders;
	case 2: return side->sounit->standing->e_orders;
	case 3: return side->sounit->standing->f_orders;
      }
    return 0;
}

extern y_standing_remove_2();

standing_handle_group(side,s)
Side *side;
char *s;
{
  if (side->sogroup) free(side->sogroup);
  if (s) {
    if (!strcmp(s,"*") || !strcmp(s," ") || strlen(s)==0) {
      free(s);
      s=0;
    }
    else if (strlen(s)>=MAXGROUP) s[MAXGROUP-1]='\0';
  }
  side->sogroup=s;
}

y_standing_remove_4(side,unit,s)
Side *side;
Unit *unit;
char *s;
{
  standing_handle_group(side,s);
  ask_condition(side,unit,y_standing_remove_2,FALSE);
}

y_standing_remove_3(side,unit,n)
Side *side;
Unit *unit;
int n;
{ 
  if (n<0 || n>100) ask_number(side,unit,y_standing_remove_3,
				"What priority (0<=n<=100) ?");
  else {
    side->socond->priority=n;
    ask_condition(side,unit,y_standing_remove_2,FALSE);
  }
}

extern y_standing_remove_01();
extern y_standing_remove_1();

y_standing_remove_2(side,unit,cond)
Side *side;
Unit *unit;
int cond;
{   int u;

  switch (cond) {
    case -1: negate_condition(side->socond);
	     break;
    case -2: remove_standing_order(side, side->soutype);
	     return;
    case -3: ask_number(side,unit,y_standing_remove_3,"What priority?");
	     return;
    case -4: ask_string(side,unit,y_standing_remove_4,"What group?",NULL);
	     return;
    case -5: ask_unit_type(side,unit,y_standing_remove_1,"New unit type",
		    utypes[side->sounit->type].capacity);
             return;
    case -6: ask_somode(side,unit,y_standing_remove_01);
	     return;

    default: side->socond->type=cond;
	     break;
  }
  ask_condition(side,unit,y_standing_remove_2,FALSE);
}

y_standing_remove_1(side,unit,type)
Side *side;
{
   if (type != NOTHING) {
     side->soutype=type;
     if (side->sosimple) 
       remove_standing_order(side, side->soutype);
     else
       ask_condition(side,unit,y_standing_remove_2,FALSE);
   }
}

y_standing_remove_01(side,unit,type)
Side *side;
{
  side->somode=type;
  ask_condition(side,unit,y_standing_remove_2,FALSE);
}

y_standing_remove_0(side,unit,m)
Side *side;
Unit *unit;
int m;
{ int u;

  if (!m) {
    remove_group_orders(side,side->sogroup);
  }
  else if (m==-1) {
    delete_standing_orders(unit);   
  }
  else {
    side->somode=m;
    sprintf(tmpbuf,"Type of occupant to remove standing %s orders",
			sorder_types[side->somode]);
    if (!ask_unit_type(side, unit, y_standing_remove_1, tmpbuf,
		      utypes[unit->type].capacity)) 
	cmd_error(side, "This unit never has occupants to give orders to!");
  }
}
/*** <- insert ***/

do_remove_standing (side,unit, n)
Side *side;
Unit *unit;
int n;
{
    int u;

    if (side->teach == TRUE)
      cmd_error (side, "Teach mode active!");
    side->sounit = unit;
    show_standing_orders(side, unit);
/*** (UK) change -> ***/
    side->socond->type=C_NONE;
    side->socond->priority=0;
    side->sosimple=TRUE;
    if (side->sogroup) free(side->sogroup);
    if (side->curgroup) side->sogroup=copy_string(side->curgroup);
    else side->sogroup=NULL;
    ask_somode2(side,unit,y_standing_remove_0,side->sogroup);
/*** was:
    HEINZ!!!
*** <- change ***/
}

remove_standing_order(side, type)
Side *side;
int type;
{
    int u;
    StandingOrderEntry **oetab, *oe;
    StandingConditionEntry *ce;

    if (side->sounit->standing == NULL) 
      return;

    oetab=get_current_sorder_array(side);
    if (!oetab) {
      return;
    }

    oe=search_order_entry(oetab[type],side->sogroup);
    if (!oe) {
      notify(side,"no such standing order defined.");
    }
    else {
      ce=search_condition_entry(oe->cond,side->socond,TRUE);
      if (!side->socond->priority && !ce) {
	ce=search_condition_entry(oe->cond,side->socond,FALSE);
      }
      if (!ce) {
	notify(side,"no such condition defined.");
      }
      else {
	if (ce->order) {
	   UpdateSOMoveLineRedraw(side,side->sounit,ce->order);
	   if (side->sogroup) 
	     notify(side,"Canceling %s (%s) order for %s of group %s in %s.",
	       sorder_types[side->somode], cond_spec(&ce->cond),
	       utypes[type].name,side->sogroup,
	       unit_handle(side, side->sounit));
	   else
	     notify(side, "Canceling %s (%s) order for %s in %s.",
	       sorder_types[side->somode], cond_spec(&ce->cond),
	       utypes[type].name,unit_handle(side, side->sounit));

	}
	else {
	  notify(side,"no such standing order defined.");
	}
	delete_condition_entry(side->sounit,&oetab[type],oe,ce);
      }
    }
}

remove_group_orders(side, group)
Side *side;
char *group;
{
  int u;
  StandingOrderEntry **oetab, *oe;
  bool found=FALSE;

  if (side->sounit->standing == NULL) 
    return;

  for_all_unit_types(u) {
    for (side->somode=1; side->somode<4; side->somode++) {
      oetab=get_current_sorder_array(side);
      if (oetab) {
	oe=search_order_entry(oetab[u],group);
	if (oe) {
	  found=TRUE;
	  delete_condition_entries(side->sounit,oe);
	  cleanup_order_entry(side->sounit,&oetab[u],oe);
	}
      }
    }
  }
  side->somode=0;
  if (!found) {
    notify(side,"no such standing order defined.");
  }
}


/* The command proper starts the ball rolling by prompting for the type */
/* of unit that will get standing orders.  Of course, if a unit is not of */
/* a type that has occupants, standing orders are pretty useless.  Also, */
/* if only one type of occupant is possible, then no need to ask. */

/* Set standing orders for a unit of a given type that enters a given city. */
/* Space for standing orders is dynamically allocated the first time we */
/* request some orders. */


/*** (UK) insert -> ***/
extern y_standing_2();

y_standing_6(side,unit,u,n)
Side *side;
Unit *unit;
int u;
int n;
{
  if (u==NOTHING) ask_condition(side,unit,y_standing_2,TRUE);
  else {
    side->socond->d.utypes[u]=1;
    if (n>1) ask_for_unitlist(side,unit);
    else ask_condition(side,unit,y_standing_2,TRUE);
  }
}

y_standing_5(side,unit,n)
Side *side;
Unit *unit;
int n;
{ 
  if (n<1) ask_number(side,unit,y_standing_5,"What amount (>0) ?");
  else {
    side->socond->d.amount=n;
    ask_condition(side,unit,y_standing_2,TRUE);
  }
}

y_standing_4(side,unit,s)
Side *side;
Unit *unit;
char *s;
{
  standing_handle_group(side,s);
  ask_condition(side,unit,y_standing_2,TRUE);
}

y_standing_3(side,unit,n)
Side *side;
Unit *unit;
int n;
{ 
  if (n<0 || n>100) ask_number(side,unit,y_standing_3,"What priority (0<=n<=100) ?");
  else {
    side->socond->priority=n;
    ask_condition(side,unit,y_standing_2,TRUE);
  }
}

ask_for_unitlist(side,unit)
Side *side;
Unit *unit;
{ int u;
  char *s;

  strcpy(tmpbuf2,"Of what units ");
  add_utypes(tmpbuf2,side->socond->d.utypes);
  if (!ask_unit_type(side,unit,y_standing_6,tmpbuf2,
		  utypes[side->soutype].capacity)) {
    notify(side,"This unit type cannot carry anything!");
    ask_condition(side,unit,y_standing_2,TRUE);
  }
}

extern y_standing_01();
extern y_standing_1();

y_standing_2(side,unit,cond)
Side *side;
Unit *unit;
int cond;
{   int u;

  switch (cond) {
    case -1: negate_condition(side->socond);
	     break;
    case -2: get_standing_order(side, side->soutype);
	     return;
    case -3: ask_number(side,unit,y_standing_3,"What priority?");
	     return;
    case -4: ask_string(side,unit,y_standing_4,"What group?",NULL);
	     return;
    case -5: ask_unit_type(side,unit,y_standing_1,"New unit type",
		    utypes[side->sounit->type].capacity);
             return;
    case -6: ask_somode(side,unit,y_standing_01);
	     return;

    default: side->socond->type=cond;
	     switch (cond) {
	      case C_FULL:
	      case C_NFULL:
		 for_all_unit_types(u) {
		   side->socond->d.utypes[u]=0;
		 }
		 ask_for_unitlist(side,unit);
		 return;
	      case C_GT:
	      case C_LE:
		ask_number(side,unit,y_standing_5,"What amount?");
		return;
	    }
	    break;
  }
  ask_condition(side,unit,y_standing_2,TRUE);
}

y_standing_1(side,unit,type)
Side *side;
{
   if (type != NOTHING) {
     side->soutype=type;
     if (side->sosimple) 
       get_standing_order(side, side->soutype);
     else
     ask_condition(side,unit,y_standing_2,TRUE);
   }
}

y_standing_01(side,unit,type)
Side *side;
{
  side->somode=type;
  ask_condition(side,unit,y_standing_2,TRUE);
}

y_standing_0(side,unit,m)
Side *side;
Unit *unit;
int m;
{ int u;

  side->somode=m;
  sprintf(tmpbuf,"Type of occupant to get standing %s orders",
		      sorder_types[side->somode]);
  if (!ask_unit_type(side, unit, y_standing_1, tmpbuf,
		    utypes[unit->type].capacity)) 
      cmd_error(side, "This unit never has occupants to give orders to!");
}
/*** <- insert ***/

/* The command proper starts the ball rolling by prompting for the type */
/* of unit that will get standing orders.  Of course, if a unit is not of */
/* a type that has occupants, standing orders are pretty useless.  Also, */
/* if only one type of occupant is possible, then no need to ask. */

/*** (UK) change -> ***/
do_standing(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    int u;

    if (side->teach == TRUE)
      cmd_error (side, "Teach mode already active!");
    show_standing_orders(side, unit);
    side->socond->type=C_NONE;
    side->socond->priority=0;
    side->sounit = unit;
    side->sosimple=TRUE;
    if (side->sogroup) free(side->sogroup);
    if (side->curgroup) side->sogroup=copy_string(side->curgroup);
    else side->sogroup=NULL;
    ask_somode(side,unit,y_standing_0);
}
/*** was:
do_standing(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    int u;

    
    if (Debug) printf("doing standing order\n");
    u = ask_unit_type(side, "Type of occupant to get standing orders",
		      utypes[unit->type].capacity);
    if (Debug) printf("u is %d\n", u);
    if (u < 0) {
	show_standing_orders(side, unit);
	if (Debug) printf("Input being requested\n", u);
	request_input(side, unit, x_standing_1);
	side->sounit = unit;
    } else if (u == NOTHING) {
	cmd_error(side, "This unit never has occupants to give orders to!");
    } else {
	show_standing_orders(side, unit);
	get_standing_order(side, u);
    }
}
*** <- change ***/

/*** (UK) insert -> ***/
do_condition(side, n)
Side *side;
{
  if (side->teach) {
    ask_condition(side,side->sounit,y_standing_2,TRUE);
  }
  else 
    cmd_error(side,"Only in teach mode!");
}
/*** <- insert ***/

/* A standing order is acquired by snarfing the next order and saving it */
/* rather than applying it to some unit. */

get_standing_order(side, type)
Side *side;
int type;
{
  int i;
  
    if (side->requnit->standing == NULL) {
	side->requnit->standing =
	    (StandingOrder *) malloc(sizeof(StandingOrder));
	for_all_unit_types(i) {
/*** (UK) change -> ***/
	   side->requnit->standing->p_orders[i] = NULL;
	   side->requnit->standing->e_orders[i] = NULL;
	   side->requnit->standing->f_orders[i] = NULL;
/*** was:
	  side->requnit->standing->orders[i] = NULL;
*** <- change ***/
	}
    }
    side->teach = TRUE;
    side->soutype = type;
/*** (UK) insert -> ***/
    if (!side->tmporder)
/*** <- insert ***/
    side->tmporder = (Order *) malloc(sizeof(Order));
    side->tmporder->morder = FALSE;
    side->tmporder->flags = NORMAL;
    
/*** (UK) change -> ***/
    notify(side, "Next input order will become the standing order (DEL to delete, ESC to cancel).");
/*** was:
    notify(side, "Next input order will become the standing order (DEL to delete).");
*** <- change ***/
    show_timemode(side,TRUE);
    request_command(side);
}

/*** (UK) insert -> ***/
do_cancel(side, n)
     Side	*side;
     int	n;
{
  if (side->teach) {
    notify(side,"input of standing order canceled!");
    free(side->tmporder);
    side->tmporder=NULL;
    side->teach=FALSE;
    side->somode=0;
    show_timemode(side,TRUE);
  }
  else {
    side->savflag=FALSE;
  }
}

do_seenenemy(side,n)
Side *side;
{ if (side->teach) {
    cmd_error(side,"not while teaching!");
    return;
  }

  if (!side->enemyseen)  {
    cmd_error(side,"no enemy seen in this turn!");
    return;
  }

  if (!side->savflag || side->mode==MOVE) {
    if (side->workmode) {
      side->savshow_product=side->show_product;
      side->savshow_only_producers=side->show_only_producers;
      side->savshow_no_movers=side->show_no_movers;
      if (side->show_product || side->show_only_producers ||
	  side->show_no_movers) {
	side->show_product=FALSE;
	side->show_only_producers=FALSE;
	side->show_no_movers=FALSE;
	show_timemode(side);
	show_map(side);
      }
    }
    side->savmode=side->mode;
    side->savmode=SURVEY;
    side->savcurx=side->curx;
    side->savcury=side->cury;
    side->savcurunit=side->curunit;
    side->savflag=TRUE;
    if (side->mode==MOVE) survey_mode(side);
  }
  move_survey(side,side->seenx,side->seeny);
  recenter(side,side->seenx,side->seeny);
}

do_returnenemy(side,n)
Side *side;
{ if (side->teach) {
    cmd_error(side,"not while teaching!");
    return;
  }

  if (n>0) {
    do_returnposition(side,n);
    return;
  }

  if (!side->savflag)  {
    cmd_error(side,"nothing to return to!");
    return;
  }

  side->curx=side->savcurx;
  side->cury=side->savcury;
  side->curunit=side->savcurunit;
  side->savflag=FALSE;
  recenter(side,side->curx,side->cury);
  make_current(side,side->savcurunit);
  if (side->workmode) {
    if (side->savshow_only_producers || side->savshow_product ||
	side->savshow_no_movers) {
      side->show_product=side->savshow_product;
      side->show_only_producers=side->savshow_only_producers;
      side->show_no_movers=side->savshow_no_movers;
      show_timemode(side);
      show_map(side);
    }
  }
  if (side->mode!=side->savmode) {
    if (side->savmode==MOVE) {
      if (can_move_unit(side,side->curunit))
	side->movunit = side->curunit;
      move_mode(side);
    }
    else survey_mode(side);
  }
}

do_returnposition(side,n)
Side *side;
{ 
  if (n<1 || n>MAXSAVESTATE) {
    cmd_error(side,"illegal save slot number!");
  }
  n--;
  if (side->savstate[n].used) {
    side->curx=side->savstate[n].x;
    side->cury=side->savstate[n].y;
    survey_mode(side);
    recenter(side,side->savstate[n].x,side->savstate[n].y);
  }
  else {
    cmd_error(side,"save slot not used!");
  }
}

do_backposition(side,n)
Side *side;
{ int x,y;

  if (side->last_x>=0) {
    x=side->curx; side->curx=side->last_x; 
    y=side->cury; side->cury=side->last_y; 
    survey_mode(side);
    recenter(side,side->curx,side->cury);
    side->last_x=x;
    side->last_y=y;
  }
}

do_saveposition(side,n)
Side *side;
{ 
  if (n<1 || n>MAXSAVESTATE) {
    cmd_error(side,"illegal save slot number!");
  }
  else {
    n--;
    side->savstate[n].used=TRUE;
    side->savstate[n].x=side->curx;
    side->savstate[n].y=side->cury;
  }
}

/*** <- insert ***/

/* this command does nothing normally, but in teach mode it deletes the
   standing order */

do_nothing(side, n)
     Side	*side;
     int	n;
{
  if (side->teach) {
    free(side->tmporder);
    side->tmporder=NULL;
    finish_teach(side);
  }
}

/* Survey mode allows player to look around (and change things) by moving */
/* cursor.  The same command toggles in and out, so need a case statement. */
/* Players waiting their turn will be in survey mode, but can't get out. */

do_survey_mode(side, n)
Side *side;
int n;
{
/*** (UK) insert -> ***/
    if (side->workmode) {
      cmd_error(side,"not in working mode!");
      return;
    }
/*** <- insert ***/
    flush_input(side); /* don't want commands from one mode in other */
    switch (side->mode) {
    case MOVE:
	survey_mode(side);
	break;
    case SURVEY:
	  if (can_move_unit(side,side->curunit))
/*** (UK) insert -> ***/
            { Unit *u;
/*** <- insert ***/
	    side->movunit = side->curunit;
/*** (UK) insert -> ***/
	      side->last_unit = side->curunit;
	      u=side->movunit->transport;
	      while (u && u->side==side) {
		side->movunit->last_unit=u;
		u=u->transport;
	      }
	    }
/*** <- insert ***/
	  move_mode(side);
	break;
    default:
	case_panic("mode", side->mode);
    }
}

/* Change what a unit is producing. */

do_product(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    if (!global.setproduct) {
	cmd_error(side, "No construction changes allowed in this game!");
    } else {
	if (!can_produce(unit)) {
	    cmd_error(side, "This unit can't build anything!");
	} else if (utypes[unit->type].occproduce || utypes[unit->type].maker
		  || (unit->transport == NULL)){
	    if (!utypes[unit->type].maker) 
		wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
	    request_new_product(unit);
	} else 
	    cmd_error(side, "This unit unable to produce inside other units.");
    }
}
/* Setting what a unit will produce next. */

do_next_product(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    if (!global.setproduct) {
	cmd_error(side, "No construction changes allowed in this game!");
    } else {
	if (!can_produce(unit)) {
	    cmd_error(side, "This unit can't build anything!");
	} else if (utypes[unit->type].occproduce || (unit->transport == NULL)){
	    if (!utypes[unit->type].maker) 
		wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
	    request_next_product(unit);
	  }
          else cmd_error(side, "This unable to produce inside other units.");
    }
}

/* Set a unit to not produce anything (yes, this really is useful). */

do_idle(side, unit, n)
Side *side;
Unit *unit;
int n;
{
    if (!global.setproduct) {
	cmd_error(side, "No production changes allowed in this scenario!");
    } else {
	set_product(unit, NOTHING);
	unit->schedule = n;
	show_info(side);
    }
}

/* Send a short (1 line) message to another player.  Some messages are */
/* recognized specially, causing various actions. */

y_message(side,unit,msg)
Side *side;
Unit *unit;
char *msg;
{
    Side *side3;

	if (side->reqoside == NULL) {
	    if (msg != NULL && strlen(msg) > 0) {
		notify_all("The %s announce: %s",
			   plural_form(side->name), msg);
	    }
	} else if (strcmp(msg, "briefing") == 0) {
	    notify(side->reqoside, "Receiving a briefing from the %s...",
		   plural_form(side->name));
	    reveal_side(side, side->reqoside, 100);
	    notify(side, "You just briefed the %s on your position.",
		   plural_form(side->reqoside->name));
	} else if (strcmp(msg, "alliance") == 0) {
	    notify(side, "You propose a formal alliance with the %s.",
		   plural_form(side->reqoside->name));
	    side->attitude[side_number(side->reqoside)] = ALLY;
	    if (side->reqoside->attitude[side_number(side)] >= ALLY) {
		declare_alliance(side, side->reqoside);
		for_all_sides(side3) redraw(side3);
	    } else {
		notify(side->reqoside, "The %s propose a formal alliance.",
		       plural_form(side->name));
	    }
	} else if (strcmp(msg, "neutral") == 0) {
	    notify(side, "You propose neutrality with the %s.",
		   plural_form(side->reqoside->name));
	    side->attitude[side_number(side->reqoside)] = NEUTRAL;
	    if (side->reqoside->attitude[side_number(side)] == NEUTRAL) {
		declare_neutrality(side, side->reqoside);
		for_all_sides(side3) redraw(side3);
	    } else {
		notify(side->reqoside, "The %s propose neutrality.",
		       plural_form(side->name));
	    }
	} else if (strcmp(msg, "war") == 0) {
	    notify(side, "You declare war on the %s!",
		   plural_form(side->reqoside->name));
	    declare_war(side, side->reqoside);
	    for_all_sides(side3) redraw(side3);
	} else if (strlen(msg) > 0) {
	    notify(side->reqoside, "The %s say to you: %s",
		   plural_form(side->name), msg);
	} else {
	    notify(side, "You keep your mouth shut.");
	}
/*** (UK) insert -> ***/
        free(msg);
/*** <- insert ***/
}

/* The command proper. */

do_message(side, n)
Side *side;
int n;
{
    char prompt[BUFSIZE];
    Side *side2;

    side2 = side_n(n);
    side->reqoside = side2;
    if (side != side2) {
	if (side2) {
	    sprintf(prompt, "Say to the %s: ", plural_form(side2->name));
	} else {
	    sprintf(prompt, "Broadcast: ");
	}
	ask_string(side, NULL, y_message, prompt, (char *) NULL);
    } else {
	cmd_error(side, "You mumble to yourself.");
    }
}


/* toggle followaction flag */

do_look(side, n)
Side *side;
int n;
{

/*** (UK) insert -> ***/
  if (side->workmode) {
    cmd_error(side,"look mode not possible in work mode!");
    return;
  }
/*** <- insert ***/
  if (n == -1)
    side->followaction = !side->followaction;
  else side->followaction = n;
/*** (HW) insert -> ***/
  show_timemode(side,TRUE);
/*** <- insert ***/
}


/* Add a new player to the game. */
/* Should use arg to decide whether to convert machine player (or just */
/* use if available?)  Also needs to decide if new player is human and */
/* which host to open, then go through side's startup seq and open disp. */
/* Issues of cached values (?) and war/alliance setup and uses of numsides. */

y_add_player(side,u,dname)
     Side	*side;
     Unit	*u;
     char	*dname;
{
  Side	*zombie;
  Unit	*unit;

  zombie = side->reqoside;

/*** (UK) change -> ***/
  zombie->host = dname; /* already copied by grok_string */
/*** was:
  zombie->host = copy_string(dname);
*** <- change ***/

  zombie->humanp = TRUE;
  zombie->lost = FALSE;
  zombie->markunit = NULL;
  init_display(zombie);
  unit = side->markunit;
  unit_changes_side(unit, zombie, CAPTURE, GIFT);
  zombie->curunit = zombie->movunit = zombie->last_unit = unit;
  zombie->cx = unit->x;
  zombie->cy = unit->y;
}

do_add_player(side, n)
Side *side;
int n;
{
  Side	*zombie;

  cmd_error(side, "Sorry, can't add new players yet!");
  for_all_sides(zombie) {
    if (!zombie->humanp && zombie->lost &&
	zombie->unithead->next == zombie->unithead)
      break;
  }
  if (zombie!=NULL) {
    notify(side, "side %d is candidate", side_number(zombie));
    side->reqoside = zombie;
    side->markunit = side->curunit;
    ask_string(side, NULL, y_add_player, "display: ", (char*) NULL);
  } else
    cmd_error(side, "no corpses yet, can't resurrect");
}

/* Command to display the program version.  Looks wired in, but of course */
/* this is not something that we want to be easily changeable! */
/* This will also show data about other sides. */

do_version(side, n)
Side *side;
int n;
{
  
   notify(side, " ");
   notify(side,
	  "XCONQ version %s", version);
   notify(side,
	  "(c) Copyright 1987, 1988, 1991  Stanley T. Shebs");
   notify(side, " ");
   if (Debug || Build)
     reveal_side((Side *) NULL, side, 100);

   /* Now check that a lot of numbers are ok for the units. */
   {
     Unit *unit;
     Side *loop_side;
     
     for_all_units(loop_side, unit) {
       if (unit->x < 0 || unit->x >= world.width ||
	   unit->y <= 0 || unit->y >= (world.height - 1) ||
	   unit->hp <= 0)
	 notify(side, "Unit off map %s id %d (%d, %d) hp %d",
		unit_handle((Side *) NULL, unit),
		unit->id, unit->x, unit->y, unit->hp);
     }
   }
}

/* Set the unit to automatic control.  */

do_auto(side, unit, n)
Side *side;
Unit *unit;
int n;
{
  enter_procedure("do_auto");
    if (side->teach) {
	cache_auto(side);
    } else {
        if (side->curunit != NULL)
	  unit->orders.morder = TRUE;
    }
  exit_procedure();
}


/* Create any unit anywhere.  It gets the usual initial supply, and its */
/* current side is also its true side (i.e. it will never revolt). */

y_unit(side,x,u)
Side *side;
Unit *x;
int u;
{
    Unit *unit;

	if (u != NOTHING) {
	    unit = create_unit(u, (char *) NULL);
	    occupy_hex(unit, side->curx, side->cury);
	    init_supply(unit);
	    unit_changes_side(unit, side->reqoside, -1, -1);
	    unit->trueside = unit->side;
	    make_current(side, unit);
	    all_see_hex(side->curx, side->cury);
	}
}

/* The command function proper, which only works in Build mode. */

do_unit(side, n)
Side *side;
int n;
{
    if (Build) {
	if (unit_at(side->curx, side->cury) == NULL) {
	    side->reqoside = side_n(n);
	    ask_unit_type(side, NULL, y_unit, "Type of unit to create?", (short *) NULL);
	} else {
	    cmd_error(side, "Unit already here!");
	}
    } else {
	cmd_error(side, "Not building a mapfile!");
    }
}

/* Terrain editing alters a hexagonal area of given radius.  If only one */
/* hex changed (the default), just update that alone; otherwise, go ahead */
/* and redraw everything. */

/* The command itself just sets up what will be drawn. */

do_terrain(side, terr, n)
Side *side;
int terr, n;
{
    tmpterr = terr;
    tmpdist = min(abs(n), world.width);
    tmparea = (n >= 0);
    notify(side, "Will now paint %d hex %s of %s.",
	   tmpdist, (tmparea ? "radius area" : "bars"), ttypes[tmpterr].name);
}

/* Function to change just one hex and to echo that change. */
/* Don't need to make it show instantly, can wait. */

set_one_hex(x, y)
int x, y;
{
    set_terrain_at(x, y, tmpterr);
    see_exact(tmpside, x, y);
    draw_hex(tmpside, x, y, FALSE);
}

/* Painting operation is activated by the "sit" command. */

paint_terrain(side)
Side *side;
{
    int i;

    tmpside = side;
    if (tmparea) {
	apply_to_area(side->curx, side->cury, tmpdist, set_one_hex);
    } else {
	for (i = 0; i < tmpdist; ++i)
	    set_one_hex(wrap(side->curx + i), side->cury);
    }
}

/* Generic command error routine - beeps display etc. */

/*VARARGS*/
cmd_error(side, control, a1, a2, a3, a4, a5, a6)
Side *side;
char *control, *a1, *a2, *a3, *a4, *a5, *a6;
{
    notify(side, control, a1, a2, a3, a4, a5, a6);
    if (active_display(side) && humanside(side) &&
	(side->curunit == NULL || !side->curunit->orders.morder))
      beep(side);
}

/*** (HW/UK) reinsert and change -> ***/
y_type(side,unit,u)
Side *side;
Unit *unit;
int u;
{
    int i;

    if (u != NOTHING) {
      show_production(side,u);
    }
}

/* The command proper just prompts and issues the request. */

do_show_production(side, n)
Side *side;
int n;
{
    ask_unit_type(side, side->requnit, y_type, "Production of which unit type?", NULL);
}



/* iteratate type specific producer */

y_type2(side,unit,type)
Side *side;
Unit *unit;
int type;
{   Unit *u=side->unithead->next;
    int i;

	if (type != NOTHING) {
	  while (u!=side->unithead && (u->side!=side || !alive(u) ||
	       !producing(u) || u->product!=type)) u=u->next;
	  if (u) {
	    move_survey(side,u->x,u->y);
	    make_current(side, u);
	    show_info(side);
	  }
	  else cmd_error(side,"You produce no %s!",utypes[type].name);
	}
}

/* The command proper just prompts and issues the request. */

do_show_producer(side, n)
Side *side;
int n;
{
    if (side->mode!=SURVEY) {
      cmd_error(side, "Can only look at producer when in survey or watch mode!");
    }
    else {
      ask_unit_type(side, side->requnit, y_type2,"Producer of which unit type?", NULL);
    }
}

do_next_producer(side, unit, n)
Side *side;
Unit *unit;
int n;
{   int p=unit->product;
    Unit *u=unit;

    if (side->mode!=SURVEY) {
      cmd_error(side, "Can only look at next producer when in survey or watch mode!");
    }
    else if (producing(unit)) {
      if (unit->side!=side) {
	cmd_error(side,"Not your unit!");
      }
      else {
	while ((u=(u->next==side->unithead)?side->unithead->next:u->next) &&
	       (u->side!=side || !alive(u) ||
		!producing(u) || u->product!=p) && u != unit);
	if (u) {
	  move_survey(side,u->x,u->y);
	  make_current(side, u);
	  show_info(side);
	}
      }
    }
    else cmd_error(side,"Unit does not produce anything");
}

/* iteratate type specific units */

y_type3(side,unit,type)
Side *side;
Unit *unit;
int type;
{   Unit *u=side->unithead->next;
    int i;

	if (type != NOTHING) {
	  while (u!=side->unithead &&(u->side!=side || !alive(u) ||
	       u->type!=type)) u=u->next;
	  if (u) {
	    move_survey(side,u->x,u->y);
	    make_current(side, u);
	    show_info(side);
	  }
	  else cmd_error(side,"You own no %s!",utypes[type].name);
	}
}

/* The command proper just prompts and issues the request. */

do_show_unit(side, n)
Side *side;
int n;
{
    if (side->mode!=SURVEY) {
      cmd_error(side, "Can only look at unit when in survey or watch mode!");
    }
    else {
      ask_unit_type(side, side->requnit, y_type3, "Which unit type?", NULL);
    }
}

do_next_unit(side, unit, n)
Side *side;
Unit *unit;
int n;
{   int type=unit->type;
    Unit *u=unit;

    if (side->mode!=SURVEY) {
      cmd_error(side, "Can only look at next unit when in survey or watch mode!");
    }
    else if (unit->side!=side) {
	cmd_error(side,"Not your unit!");
    }
    else {
      while ((u=(u->next==side->unithead)?side->unithead->next:u->next) &&
	     (u->side!=side || !alive(u) ||
	   u->type!=type));
      if (u) {
	move_survey(side,u->x,u->y);
	make_current(side, u);
	show_info(side);
      }
    }
}

sentry_occupants(unit,n)
Unit *unit;
{ Unit *occ;

  for_all_occupants(unit,occ) {
    sentry_occupants(occ,n);
    order_sentry(occ,n);
  }
}

do_sentry_occupants(side,unit,n)
Side *side;
Unit *unit;
{
  sentry_occupants(unit_at(unit->x,unit->y),n);
}

int trans[100];

handle_take(unit)
Unit *unit;
{
    int u = unit->type;
    int m, r, need, actual;
    Unit *main = unit->transport;
    int sum=0;
    int n=-1;

    if (main != NULL) {
	m = main->type;
	for_all_resource_types(r) {
	    need = (n < 0 ? utypes[u].storage[r] - unit->supply[r] : n);
	    if (need > 0) {
		/* Be stingy if we're low */
		if (2 * main->supply[r] < utypes[m].storage[r])
		    need = max(1, need/2);
		actual = transfer_supply(main, unit, r, need);
		trans[r]+=actual;
		sum+=actual;
	    }
	}
    }
    return(sum);
}

take_unit(unit)
Unit *unit;
{ Unit *occ;
  int sum=0;

  handle_take(unit);
  for_all_occupants(unit,occ) sum+=take_unit(occ);
  return(sum);
}

do_take_all(side, unit, n)
Side *side;
Unit *unit;
int n;
{ Unit occ;
  int r;

  for_all_resource_types(r) trans[r]=0;

  while (n-->0) if (!take_unit(unit)) break;

  strcpy(spbuf,"");
  for_all_resource_types(r) {
    if (trans[r]) {
      sprintf(tmpbuf, " %d %s", trans[r], rtypes[r].name);
      strcat(spbuf,tmpbuf);
    }
  }
  if (*spbuf) notify(side, "%s transferred.", spbuf);
  else notify(side, "nothing transferred.");
}
/*** <- reinsert ***/
