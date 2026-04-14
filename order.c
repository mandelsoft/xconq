/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Handling of unit orders. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "global.h" /* XXX */
/*** (UK) insert -> ***/
#include "map.h"

char *sorder_types[]= SORDERTYPES; /* standing order types (UK) */
/*** <- insert ***/
char *ordernames[] = ORDERNAMES;   /* full names of orders */
char *dirnames[] = DIRNAMES;       /* short names of directions */
char orderbuf[BUFSIZE];            /* buffer for printed form of an order */
char oargbuf[BUFSIZE];             /* buffer for order's arguments */

int orderargs[] = ORDERARGS;       /* types of parameters for each order */

/* General routine to wake a unit up (and maybe all its cargo). */

/*** (UK) insert -> ***/
wake_unit_to_move_again(unit)
Unit *unit;
{
    if (unit->side != NULL && unit->movesleft > 0 &&
	alive(unit) && !unit->side->more_units) {
      unit->side->more_units = TRUE;
      update_sides(unit->side);
      /* XXX */
      if ((nextturnexecfile[0] != '\0') && (global.time == newturnsent) &&
	  (!Sequential)) {
        char buf[BUFSIZE];

        sprintf(buf, "%s side %d %d %d", nextturnexecfile,
                side_number(unit->side),
                (unit->side->lost ? 0 : 1), (unit->side->more_units ? 1 : 0));
        system(buf);
      }
      /*
      move_mode(unit->side);
      */
      unit->side->movunit = unit;
      /*
      cancel_request(unit->side);
      */
    }

    if (unit->side) UpdateMoveLineRedraw(unit->side,unit);
}
/*** <- insert ***/

wake_unit(unit, wakeocc, reason, other)
Unit *unit, *other;
int reason;
short wakeocc; /* what kind to wake NOTHING=no occ,-1=all occ (gec)*/
{
    Unit *occ;

/* make sure that a side always gets to move a unit that is woken up
late in the turn */

  if (wakeocc == NOTHING || wakeocc == (-1) || unit->type == wakeocc) {
    /*
    if (unit && unit->side) printf("wake %s\n",unit_handle(unit->side,unit));
    */
/*** (UK) change -> ***/
    wake_unit_to_move_again(unit);
/*** was:
    if (unit->side != NULL && unit->movesleft > 0 &&
	alive(unit) && !unit->side->more_units) {
      unit->side->more_units = TRUE;
      update_sides(unit->side);
      if ((nextturnexecfile[0] != '\0') && (global.time == newturnsent) &&
	  (!Sequential)) {
        char buf[BUFSIZE];

        sprintf(buf, "%s side %d %d %d", nextturnexecfile,
                side_number(unit->side),
                (unit->side->lost ? 0 : 1), (unit->side->more_units ? 1 : 0));
        system(buf);
      }
      move_mode(unit->side);
      unit->side->movunit = unit;
      cancel_request(unit->side);
    }
*** <- change ***/

    unit->orders.type = AWAKE;
    unit->orders.rept = 0;
    unit->orders.flags = NORMAL;
    unit->wakeup_reason = reason;
    if (reason == WAKEFULL) {
      unit->orders.morder = FALSE;
    }
    if (reason == WAKEOWNER)
      unit->area = area_index(unit->x, unit->y);
    if (reason == WAKEENEMY && other!=NULL) {
      unit->waking_side = other->side;
      unit->waking_type = other-> type;
      unit->area = area_index(unit->x, unit->y);
    }
  }
    if (wakeocc != NOTHING) {
/*** (UK) change -> ***/
	for_all_occupants(unit, occ) 
	  wake_occupant(occ, unit, wakeocc, reason, other);
/*** was:
	for_all_occupants(unit, occ) wake_unit(occ, wakeocc, reason, other);
*** <- change ***/
    }
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

/*** (UK) insert -> ***/
wake_occupant(unit, trans, wakeocc, reason, other)
Unit *unit, *trans, *other;
{ int d;
  Unit *occ;

  if (utypes[unit->type].speed>=utypes[unit->type].leavetime[trans->type] ||
      utypes[unit->type].freemove[trans->type]) {
    for_all_directions(d) {
      if (could_move_in_dir(unit,d)) {
	wake_unit(unit,wakeocc,reason,other);
	return;
      }
    }
  }
  if (wakeocc != NOTHING) {
      for_all_occupants(unit, occ) 
	wake_occupant(occ, unit, wakeocc, reason, other);
  }
}
/*** <- insert ***/

/* Stash a "wakeup call" - will only be for main unit, not occupants. */

cache_awake(side)
Side *side;
{
    side->tmporder->type = AWAKE;
    side->tmporder->rept = 0;
    side->tmporder->morder = FALSE;
    finish_teach(side);
}

/* Give a unit sentry orders. */

order_sentry(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = SENTRY;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

/* Stash sentry orders. */

cache_sentry(side, n)
Side *side;
int n;
{
    side->tmporder->type = SENTRY;
    side->tmporder->rept = n;
    finish_teach(side);
}

/* Fill in the given unit with direction-moving orders. */

order_movedir(unit, dir, n)
Unit *unit;
int dir, n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = MOVEDIR;
    unit->orders.p.dir = dir;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_movedir(side, dir, n)
Side *side;
int dir, n;
{
    side->tmporder->type = MOVEDIR;
    side->tmporder->p.dir = dir;
    side->tmporder->rept = n;
    finish_teach(side);
}

/* Give the unit orders to move to a given place - it only needs to do this */
/* once, repetition is nonsensical. */

order_moveto(unit, x, y)
Unit *unit;
int x, y;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = MOVETO;
    unit->orders.rept = 1;
    unit->orders.p.pt[0].x = x;
    unit->orders.p.pt[0].y = y;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_moveto(side, x, y)
Side *side;
int x, y;
{
    side->tmporder->type = MOVETO;
    side->tmporder->rept = 1;
    side->tmporder->p.pt[0].x = x;
    side->tmporder->p.pt[0].y = y;
    finish_teach(side);
}

/* order a unit to move toward another unit */

order_movetounit(unit, dest, n)
Unit	*unit, *dest;
int	n;
{
/*** (HW) insert -> ***/
  UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
  unit->orders.type = MOVETOUNIT;
  unit->orders.rept = n;
  unit->orders.p.leader_id = dest->id;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_movetounit(side, dest, n)
Side	*side;
Unit	*dest;
int	n;
{
  side->tmporder->type = MOVETOUNIT;
  side->tmporder->rept = n;
  side->tmporder->p.leader_id = dest->id;
  finish_teach(side);
}

/* order a unit to return to base */

order_return(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
  UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
  unit->orders.type = RETURN;
  unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_return(side, n)
Side *side;
int n;
{
  side->tmporder->type = RETURN;
  side->tmporder->rept = n;
  finish_teach(side);
}

/* Order to follow an edge needs to know the direction and which side the
   obstacle is on. */

order_edge(unit, d, ccw, n)
Unit *unit;
int d, ccw, n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = EDGE;
    unit->orders.rept = n;
    unit->orders.p.edge.forward = d;
    unit->orders.p.edge.ccw = ccw;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_edge(side, d, ccw, n)
Side *side;
int d, ccw, n;
{
    side->tmporder->type = EDGE;
    side->tmporder->rept = n;
    side->tmporder->p.edge.forward = d;
    side->tmporder->p.edge.ccw = ccw;
    finish_teach(side);
}

/* Order to move to the nearest filling transport */

order_movetotransport(unit, n)
     Unit	*unit;
     int	n;
{
/*** (HW) insert -> ***/
  UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
  unit->orders.type = MOVETOTRANSPORT;
  unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_movetotransport(side, n)
     Side	*side;
     int	n;
{
  side->tmporder->type = MOVETOTRANSPORT;
  side->tmporder->rept = n;
  finish_teach(side);
}

/* Order to follow a unit just needs the unit to follow. */

order_follow(unit, leader, n)
Unit *unit, *leader;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = FOLLOW;
    unit->orders.rept = n;
    unit->orders.p.leader_id = leader->id;;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_follow(side, leader, n)
Side *side;
Unit *leader;
int n;
{
    side->tmporder->type = FOLLOW;
    side->tmporder->rept = n;
    side->tmporder->p.leader_id = leader->id;
    finish_teach(side);
}

/* A two-waypoint patrol suffices for many purposes. */
/* Should have a more general patrol routine eventually (> 2 waypoints). */

order_patrol(unit, x0, y0, x1, y1, n)
Unit *unit;
int x0, y0, x1, y1, n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = PATROL;
    unit->orders.rept = n;
    unit->orders.p.pt[0].x = x0;
    unit->orders.p.pt[0].y = y0;
    unit->orders.p.pt[1].x = x1;
    unit->orders.p.pt[1].y = y1;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_patrol(side, x0, y0, x1, y1, n)
Side *side;
int x0, y0, x1, y1, n;
{
    side->tmporder->type = PATROL;
    side->tmporder->rept = n;
    side->tmporder->p.pt[0].x = x0;
    side->tmporder->p.pt[0].y = y0;
    side->tmporder->p.pt[1].x = x1;
    side->tmporder->p.pt[1].y = y1;
    finish_teach(side);
}


/* Wait around to embark on some unit when it comes to our location. */

order_embark(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = EMBARK;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_embark(side, n)
Side *side;
int n;
{
    side->tmporder->type = EMBARK;
    side->tmporder->rept = n;
    finish_teach(side);
}

/*** (UK) insert -> ***/
/* Give a unit orders to pickup recursivly. */

order_pickup(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = RPICKUP;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_pickup(side, n)
Side *side;
int n;
{
    side->tmporder->type = RPICKUP;
    side->tmporder->rept = n;
    finish_teach(side);
}

/* Give a unit orders to pick up not more than <a> units */

order_pickup_amount(unit, a,n)
Unit *unit;
int a,n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = APICKUP;
    unit->orders.rept = n;
    unit->orders.p.amount = a;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_pickup_amount(side, a,n)
Side *side;
int n;
{
    side->tmporder->type = APICKUP;
    side->tmporder->rept = n;
    side->tmporder->p.amount = a;
    finish_teach(side);
}

/* Give a unit leave orders. */

order_leave(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = LEAVE;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_leave(side, n)
Side *side;
int n;
{
    side->tmporder->type = LEAVE;
    side->tmporder->rept = n;
    finish_teach(side);
}

/* Give a unit unload orders. */

order_unload(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = UNLOAD;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_unload(side, n)
Side *side;
int n;
{
    side->tmporder->type = UNLOAD;
    side->tmporder->rept = n;
    finish_teach(side);
}

/* only caching, need no moves (time) */

/* Give a unit order to change its group. */
cache_group(side,name)
Side *side;
char *name;
{ side->tmporder->type=GROUP;
  side->tmporder->rept=1;
  if (name) {
    if (strlen(name)>=MAXGROUP) name[MAXGROUP-1]='\0';
    strcpy(side->tmporder->p.string,name);
  }
  else side->tmporder->p.string[0]='\0';
  finish_teach(side);
}

/* give and take supplies cacheing */
cache_give(side,n)
Side *side;
{ 
  side->tmporder->type=RGIVE;
  side->tmporder->rept=1;
  side->tmporder->p.amount=n;
  finish_teach(side);
}

cache_take(side,n)
Side *side;
{ 
  side->tmporder->type=RTAKE;
  side->tmporder->rept=1;
  side->tmporder->p.amount=n;
  finish_teach(side);
}

/*** <- insert ***/

#ifndef NO_DISEMBARK
cache_disembark(side, n)
Side *side;
int n;
{
    side->tmporder->type = DISEMBARK;
    side->tmporder->rept = n;
    finish_teach(side);
}
#endif

order_fill(unit, n)
Unit *unit;
int n;
{
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->orders.type = FILL;
    unit->orders.rept = n;
/*** (HW) insert -> ***/
    CheckRedraw(unit->side);
/*** <- insert ***/
}

cache_fill(side, n)
Side *side;
int n;
{
    side->tmporder->type = FILL;
    side->tmporder->rept = n;
    finish_teach(side);
}

/* Note that this leaves all other orders the same. */

cache_auto(side)
Side *side;
{
  Order *oldorder;

/*** (UK) change -> ***/
  StandingOrderEntry **oetab=get_current_sorder_array(side);
  StandingOrderEntry *oe=
    search_order_entry(oetab[side->soutype],side->sogroup);
  StandingConditionEntry *ce;

  if (oe && (ce=search_condition_entry(oe->cond,side->socond,TRUE)) &&
      (oldorder=ce->order)) {
/*** was:
  if ((oldorder = side->sounit->standing->orders[side->soutype]) != NULL) 
*** <- change ***/
    copy_orders(side->tmporder, oldorder);
/*** (UK) insert -> ***/
  }
/*** <- insert ***/
  else {
    side->tmporder->type = AWAKE;
    side->tmporder->rept = 0;
  }
  side->tmporder->morder = TRUE;
  finish_teach(side);
}

/* Switch from standing order teaching mode back to normal, and confirm */
/* that the standing order has been set. */

finish_teach(side)
Side *side;
{
    Unit *occ, *next;
/*** (UK) change -> ***/
    extern StandingOrderEntry *search_order_entry();
    StandingOrderEntry **oetab, *oe;
    StandingConditionEntry **cep,*ce;
    char buf[200];
    char sotype;

    oetab=get_current_sorder_array(side);
    if (!side->teach || !oetab) {
      printf("no teach mode!!!!\n");
      return;
    }
    oe=search_order_entry(oetab[side->soutype],side->sogroup);
    if (!oe) {
      oe=(StandingOrderEntry*)malloc(sizeof(StandingOrderEntry));
      if (side->sogroup) oe->group=side->sogroup;
      else oe->group=NULL;
      side->sogroup=0;
      oe->cond=NULL;
      oe->handled=FALSE;
      oe->next=oetab[side->soutype];
      oetab[side->soutype]=oe;
    }
    ce=search_condition_entry(oe->cond,side->socond,TRUE);
    if (!ce) {
      ce=(StandingConditionEntry*)malloc(sizeof(StandingConditionEntry));
      ce->cond=*(side->socond);
      ce->order=NULL;
      cep=&oe->cond;
      while (*cep && ((*cep)->cond.priority>ce->cond.priority ||
		        ((*cep)->cond.priority==ce->cond.priority &&
			(*cep)->cond.type>ce->cond.type)))
	cep=&(*cep)->next;
      ce->next=*cep;
      *cep=ce;
    }
    if (ce->order) {
       UpdateSOMoveLineRedraw(side,side->sounit,ce->order);
       notify(side, "Canceling old order.");
       free (ce->order);
    }
    ce->order = side->tmporder;


    if (side->tmporder==NULL) {
      notify(side, "standing orders for %s deleted",
	     utypes[side->soutype].name);
      delete_condition_entry(side->sounit,&oetab[side->soutype],oe,ce);
    } else {
      strcpy(buf,unit_handle(side,side->sounit));
      if (oe->group) {
	notify(side, "%s has %s <%s> orders for %s of group %s to %s.",
	       buf, sorder_types[side->somode], 
	       cond_desig(side->socond), utypes[side->soutype].name,
	       oe->group, order_desig(side->tmporder));
      }
      else {
	notify(side, "%s has %s <%s> orders for %s to %s.",
	       buf, sorder_types[side->somode], 
	       cond_desig(side->socond), utypes[side->soutype].name,
	       order_desig(side->tmporder));
      }
    }
    side->tmporder=NULL;
    CheckRedraw(side);
/*** was:
    side->teach = FALSE;
    if (side->sounit->standing->orders[side->soutype] != NULL)
      free(side->sounit->standing->orders[side->soutype]);
    side->sounit->standing->orders[side->soutype] = side->tmporder;
    if (side->tmporder==NULL) {
      notify(side, "standing orders for %s deleted",
	     utypes[side->soutype].name);
    } else {
      notify(side, "%s has orders for %s to %s%0s.",
	     unit_handle(side, side->sounit), utypes[side->soutype].name,
	     order_desig(side->tmporder),
	     ((side->tmporder->morder) ? " and auto" : ""));
      for_all_occupants(side->sounit, occ) {
	if (occ->type == side->soutype) {
	  get_standing_orders(occ, side->sounit);
	}
      }
    }
*** <- change ***/
    side->teach = FALSE;
    show_timemode(side,TRUE);
}

/* Display orders in some coherent fashion.  Use the information about the */
/* types of order parameters to decide how to display them. */

char *
order_desig(orders)
Order *orders;
{
/*** (UK) insert -> ***/
  Unit *u;
/*** <- insert ***/
    switch (orderargs[orders->type]) {
    case NOARG:
	sprintf(oargbuf, "");
	break;
    case DIR:
	sprintf(oargbuf, "%s ", dirnames[orders->p.dir]);
	break;
    case POS:
/*** (UK) insert -> ***/
	if ((u=unit_at(orders->p.pt[0].x, orders->p.pt[0].y)))
	  sprintf(oargbuf,"%s",unit_handle(NULL,u));
	else
/*** <- insert ***/
	sprintf(oargbuf, "%d,%d ", orders->p.pt[0].x, orders->p.pt[0].y);
	break;
    case EDGE_A:
	sprintf(oargbuf, "%d/%d ", orders->p.edge.forward, orders->p.edge.ccw);
	break;
    case LEADER:
	sprintf(oargbuf, "%s ", unit_handle((Side *) NULL,
					    find_unit(orders->p.leader_id)));
	break;
    case WAYPOINTS:
	sprintf(oargbuf, "%d,%d %d,%d ",
		orders->p.pt[0].x, orders->p.pt[0].y,
		orders->p.pt[1].x, orders->p.pt[1].y);
        break;
/*** (UK) insert -> ***/
    case STRING:
        if (strlen(orders->p.string))
          sprintf(oargbuf, "%s ",orders->p.string);
        else
          sprintf(oargbuf, "<none> ");
	break;
    case AMOUNT:
	sprintf(oargbuf,"%d ",orders->p.amount);
	break;
/*** <- insert ***/
    default:
	case_panic("order arg type", orderargs[orders->type]);
    }
    if (orders->rept > 1)
	sprintf(orderbuf, "%s %s(%d)",
		ordernames[orders->type], oargbuf, orders->rept);
    else
	sprintf(orderbuf, "%s %s",
		ordernames[orders->type], oargbuf);
    return orderbuf;
}

/* Yeah yeah, assignment statements supposedly copy structures. */

copy_orders(dst, src)
Order *dst, *src;
{
  /* the first two options should work fine since the Order
     is (theoretically) contiguous with no pointers. */
#ifdef BCOPY
  bcopy(src, dst, sizeof(*dst));
#else
  memcpy(dst, src, sizeof(*dst));
#endif
#if 0  /* fallback code */
    dst->type = src->type;
    dst->rept = src->rept;
    dst->flags = src->flags;
    switch (orderargs[src->type]) {
    case NOARG:
	break;
    case DIR:
	dst->p.dir = src->p.dir;
	break;
    case POS:
	dst->p.pt[0].x = src->p.pt[0].x;
	dst->p.pt[0].y = src->p.pt[0].y;
	break;
    case EDGE_A:
	dst->p.edge.forward = src->p.edge.forward;
	dst->p.edge.ccw     = src->p.edge.ccw;
	break;
    case LEADER:
	dst->p.leader_id = src->p.leader_id;
	break;
    case WAYPOINTS:	
	dst->p.pt[0].x = src->p.pt[0].x;
	dst->p.pt[0].y = src->p.pt[0].y;
	dst->p.pt[1].x = src->p.pt[1].x;
	dst->p.pt[1].y = src->p.pt[1].y;
	break;
/*** (UK) insert -> ***/
    case STRING:
	strcpy(dst->p.string,src->p.string);
	break;
    case AMOUNT:
	dst->p.amount=src->p.amount;
        break;
/*** <- insert ***/
    default:
	case_panic("order arg type", orderargs[src->type]);
    }
    dst->morder = src->morder;
#endif /* 0 */
}
