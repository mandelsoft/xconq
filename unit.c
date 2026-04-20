/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file contains all code relating specifically to units. */

/* Since units appear and disappear with distressing regularity (so it seems */
/* to the player trying to keep up with all of them!), we have a little */
/* storage manager for them.  Since this sort of thing is tricky, there is a */
/* two-level procedure for getting rid of units.  First the hit points are */
/* reduced to zero, at which point the unit is considered "dead".  At the */
/* end of a turn, we actually GC the dead ones.  At this point their type */
/* slot becomes NOTHING, and they are available for allocation. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "dir.h"
#include "global.h"

char *ordinal();

Unit *freeunits;           /* list of available units */
/* Unit *unitlist = NULL; */        /* pointer to head of list of units */
Unit *tmpunit;         /* global temporary used in several places */

/*** (UK) change -> ***/
static int unitbufselect=0;
static char unitbuf1[BUFSIZE];
static char unitbuf2[BUFSIZE];
/*** was:
char unitbuf[BUFSIZE];
*** <- change ***/
/*** (UK) insert -> ***/
char condbuf[BUFSIZE];
char condbuf2[BUFSIZE];
/*** <- insert ***/ 

int numunits = 0;          /* total number of units in existence */
int nextid = 0;            /* next number to be used for ids */
int occdeath[MAXUTYPES];  /* buffer for remembering occupant death */

bool recent_dead_flushed = TRUE; /* Have units died since dead unit lists made */

/* Init all the unit entries so we can find them when allocating. */

void init_units()
{
    int i;
    Unit *units;
    Side *side;

    units = (Unit *) malloc(INITMAXUNITS * sizeof(Unit));
    freeunits = units;
    for (i = 0; i < INITMAXUNITS; ++i) {
      units[i].type = NOTHING;
      units[i].next = &units[i+1];
    }
    units[INITMAXUNITS-1].next = NULL;

    side = &neutral_placeholder;
    if (side->unithead == NULL) {
      side->unithead = freeunits;
      freeunits = freeunits->next;
      side->unithead->next = side->unithead;
      side->unithead->prev = side->unithead;
      side->unithead->type = -1;
    }

}

/* Create a new unit of given type, with given name.  Default all the other */
/* slots - routines that need to will fill them in properly.  This routine */
/* will return a valid unit or NULL.  Note that unit morale starts out at */
/* max, signifying hopeful but inexperienced recruits marching off to war. */

Unit *
create_unit(type, name)
int type;
char *name;
{
    int r;
    Unit *newunit;

    enter_procedure("create_unit");
    if (freeunits == NULL) {
	if (!grow_unit_array()) {
	  return NULL;
	}
    }
    newunit = freeunits;
    freeunits = freeunits->next;
    newunit->type = type;
    if (name == NULL || strlen(name) == 0) {
	newunit->name = NULL;
    } else {
	newunit->name = copy_string(name);
    }
/*** (UK) insert -> ***/
    newunit->group = NULL;
    newunit->last_unit = NULL;

    newunit->terraform = TNOTHING;
    newunit->tschedule = 0;
    newunit->last_terraform = TNOTHING;
/*** <- insert ***/
    newunit->x = newunit->y = -1;
    newunit->prevx = newunit->prevy = -1;
    newunit->number = 0;
    newunit->side = NULL;
    newunit->id = nextid++;
    newunit->trueside = NULL;
    newunit->hp = (type>=0) ? utypes[type].hp : 1;
    newunit->quality = 0;
    newunit->product = NOTHING;
    newunit->next_product = NOTHING;
    newunit->schedule = 0;
    newunit->built = 0;
    newunit->transport = NULL;
    for_all_resource_types(r) newunit->supply[r] = 0;
    newunit->movesleft = 0;
    newunit->goal = 0;
    newunit->area = -1;
    newunit->awake = FALSE;   /* i.e., is usually not *temporarily* awake */
    newunit->standing = NULL;
    newunit->occupant = NULL;
    newunit->nexthere = NULL;
    newunit->mlist = NULL;
    newunit->nextinarea = NULL;
    newunit->plan = NULL;
    newunit->priority = 0;
    newunit->next = newunit;
    newunit->prev = newunit;
    if (type != -1) {
      insert_unit(neutral_placeholder.unithead, newunit);
      ++numunits;
    }
    wake_unit(newunit, NOTHING, WAKEFULL, (Unit *) NULL);
    newunit->move_turn = -1;
    exit_procedure();
    return newunit;
}

/* Just call the same initial code and get a fresh bunch of unused units */

grow_unit_array()
{

#ifdef GROWABLE
    init_units();
    notify_all("The unit array has been grown.");
    return TRUE;
#else
    notify_all("Can't make any more units!");
    return FALSE;
#endif
}

/* Find a good initial unit for the given side.  Only requirement these */
/* days is that it be of the type specified in the period file, and not */
/* already used by somebody.  If enough failed tries to find one, */
/* something may be wrong, but not necessarily. */

Unit *
random_start_unit()
{
  Unit *unit, *choices = NULL;
  int numchoices = 0, choice;
  Side *side = &neutral_placeholder;

  for_all_side_units(side, unit) 
      if (unit->type == period.firstutype || 
		period.firstutype == NOTHING) {
	unit->mlist = choices;
	choices = unit;
	numchoices++;
      }
  if (numchoices == 0) return NULL;
  choice = RANDOM(numchoices);
  for (unit = choices; choice > 0 ; unit = unit->mlist) choice--;
  return unit;
}

/* A unit occupies a hex either by entering a unit on that hex or by having */
/* the occupant pointer filled in.  If something goes wrong, return false. */
/* This is heavily used. */

bool occupy_hex(unit, x, y)
Unit *unit;
int x, y;
{
    register int u = unit->type, o;
    register Unit *other = unit_at(x, y);

    if (unit->x >= 0 || unit->y >= 0) {
      notify_all("Unit %d occupying hex (%d, %d), was at (%d %d)",
	     unit->id, x, y, unit->x, unit->y);
      printf("Unit %d occupying hex (%d, %d), was at (%d %d)\n",
	     unit->id, x, y, unit->x, unit->y);
    }
    if (!mobile(unit->type) && unit->prevx != -1 && !midturnrestore
	&& global.time > 1) {
      notify_all("nonmover %d has moved", unit->id);
      printf("nonmover %d has moved", unit->id);
    }
    if (other) {
	o = other->type;
	if (could_carry(o, u)) {
	    occupy_unit(unit, other);
	} else if (could_carry(u, o)) {
	    leave_hex(other);
	    occupy_hex(unit, x, y);
	    occupy_hex(other, x, y);
	} else {
	    return FALSE;
	}
    } else {
	set_unit_at(x, y, unit);
	occupy_hex_aux(unit, x, y);
	all_see_occupy(unit, x, y);
	all_see_hex(x, y);
    }
    return TRUE;
}

/* Recursive helper to update everybody's position.  This should be one of */
/* two routine that modify unit positions (leaving is the other). */
/* *Every* occupant will increment viewing coverage - strains realism, */
/* but prevents strange bugs. */

occupy_hex_aux(unit, x, y)
Unit *unit;
int x, y;
{
    register Unit *occ;

    unit->x = x;  unit->y = y;
    cover_area(unit, x, y, 1);
    for_all_occupants(unit, occ) occupy_hex_aux(occ, x, y);
/*** (UK) insert -> ***/
    if (!humanside(unit->side) && unit->area == -1) {
       unit-> area = area_index(x,y);
    }
/*** <- insert ***/
}

/* Decide whether transport has the capability to house the given unit. */
/* Check both basic capacity and relative volumes. */

can_type_carry(ttype, utype)
int	ttype;
int	utype;
{
  return utypes[ttype].capacity[utype];
}

can_carry_aux(transport, utype)
     Unit	*transport;
     int	utype;
{
  int	ttype = transport->type, total=0, volume=0;
  int	hold = utypes[transport->type].holdvolume;
  Unit	*occ;

/*** (UK) insert -> ***/
  if (!transport) return FALSE;
/*** <- insert ***/
  for_all_occupants(transport, occ) {
    if (occ->type == utype) total++;
    volume += utypes[occ->type].volume;
  }
  if (cripple(transport)) {
    hold = (hold * transport->hp) / (utypes[ttype].crippled + 1);
  }
  return ((total + 1 <= utypes[ttype].capacity[utype]) &&
	  (volume + utypes[utype].volume <= hold));
}

can_carry(transport, unit)
Unit *transport, *unit;
{
/*** (UK) insert -> ***/
  if (!transport || !unit) return FALSE;
/*** <- insert ***/
  if (transport == unit) return FALSE;
  return can_carry_aux(transport, unit->type);
}

/*** (UK/HW) insert -> ***/
number_of_occupants(transport,utype,group)
Unit *transport;
char *group;
{ Unit *occ;
  int total=0;

  for_all_occupants(transport, occ) {
    if (occ->type == utype) {
      if (!group || (occ->group && !strcmp(group,occ->group))) total++;
    }
  }
  /*
  printf("number of %d: %d\n",utype,total);
  */
  return total;
}

/* Check wether given unit can carry anymore */

is_full(transport,ut)
Unit *transport;
short *ut;
{ Unit	*occ;
  int	volume, u;

  volume = 0;
  for_all_occupants(transport, occ) {
    volume += utypes[occ->type].volume;
  }

  if ( volume < utypes[transport->type].holdvolume ) {
    /* we have room in the hold, but could we fit anything
       in that space? */
    for_all_unit_types(u) {
      if ( (!ut || ut[u]) && can_carry_aux(transport, u) ) {
	/* we are not full */
	/*printf("could fit a %d on unit 0x%x\n", u, unit);*/
	return FALSE;
      }
    }
  }
  /* couldn't find room for anything else
     We're full! */

  if (Debug) printf("unit 0x%x is full\n", transport);
  return TRUE;
}

can_unload(unit)
Unit *unit;
{ int u;
  Unit *transport=unit->transport;

  if (transport) while (transport=transport->transport) {
      if (can_carry(transport,unit)) return TRUE;
  }
  return FALSE;
}
/*** <- insert ***/

/* Check for units waiting to embark */

check_for_embarkies(unit, top_unit)
Unit *unit, *top_unit;
{
  Unit *occ, *next;
  int timeleft;

  next = top_unit->occupant;
  while (next != NULL) {
    occ = next;
    next = occ->nexthere;
    if (occ->orders.type == EMBARK && can_carry(unit, occ)) {
      timeleft = occ->orders.rept;
      leave_hex(occ);
      occupy_unit(occ, unit);
      /* put unit to sleep unless it was given standing orders */
/*** (UK) change -> ***/
      if (is_awake(occ)) 
	order_sentry(occ, 100);
/*** was:
      if (unit->standing == NULL ||
	  unit->standing->orders[occ->type] == NULL)
	order_sentry(occ, min(timeleft, 5));
*** <- change ***/
    }
  }
}

/* Wakeup if we were waiting to fill up with occupants and we actually
   did fill up. */

wake_if_full(unit)
     Unit	*unit;
{
/*** (UK) change -> ***/
  if (!is_full(unit,NULL)) return;
/*** was:
  Unit	*occ;
  int	volume, u;

  volume = 0;

  if (unit->orders.type != FILL) return;

  for_all_occupants(unit, occ) {
    volume += utypes[occ->type].volume;
  }

  if ( volume < utypes[unit->type].holdvolume ) {
    for_all_unit_types(u) {
      if ( can_carry_aux(unit, u) ) {
	return;
      }
    }
  }
  if (Debug) printf("unit 0x%x is full\n", unit);
*** <- change ***/
  wake_unit(unit, NOTHING, WAKEFULL, NULL);
}

/*** (UK) insert -> ***/


/*
 * Handling of standing orders
 */

add_utypes(s,ut)
char *s;
short *ut;
{ int u;
  int c=0;

  s=s+strlen(s);
  *s++='{';
  for_all_unit_types(u) {
    if (ut[u]) {
      *s++=utypes[u].uchar;
      c++;
    }
  }
  strcpy(s,"}");
  return c;
}

char *cond_desig(c)
SCondition *c;
{ char *msg=condbuf;
  int u;

  switch (c->type) {
    case C_NONE:
      msg="always";
      break;
    case C_GT: 
      sprintf(condbuf,"if >%d",c->d.amount);
      break;
    case C_LE: 
      sprintf(condbuf,"if <=%d",c->d.amount);
      break;
    case C_EMBARK:
      msg= "if transport";
      break;
    case C_NEMBARK:
      msg= "if no transport";
      break;
    case C_PICKUP:
      msg= "if embarkies";
      break;
    case C_NPICKUP:
      msg= "if no embarkies";
      break;
    case C_FULL:
      strcpy(condbuf,"if full");
      add_utypes(condbuf,c->d.utypes);
      msg=condbuf;
      break;
    case C_NFULL:
      strcpy(condbuf,"if not full");
      add_utypes(condbuf,c->d.utypes);
      msg=condbuf;
      break;
    case C_TFULL:
      msg= "if trans full";
      break;
    case C_NTFULL:
      msg= "if not trans full";
      break;
    default: case_panic("condition type",c->type);
	     return "error";
  }
  if (c->priority>0) {
    sprintf(condbuf2,"%s(%d)",msg,c->priority);
    return condbuf2;
  } 
  return msg;
}

char *cond_type(type)
{ char *msg;

  switch (type) {
    case C_NONE:    msg= "always"; break;
    case C_GT:      msg= "more than"; break;
    case C_LE:      msg= "no more than"; break;
    case C_EMBARK:  msg= "transport"; break;
    case C_NEMBARK: msg= "no transport"; break;
    case C_PICKUP:  msg= "embarkies"; break;
    case C_NPICKUP: msg= "no embarkies"; break;
    case C_FULL:    msg= "full"; break;
    case C_NFULL:   msg= "not full"; break;
    case C_TFULL:   msg= "transport full"; break;
    case C_NTFULL:  msg= "not transport full"; break;
    default:        return "error";
  }
  return msg;
}

char *cond_spec(c)
SCondition *c;
{ char *msg=cond_type(c->type);
  if (c->priority>0) {
    sprintf(condbuf2,"%s(%d)",msg,c->priority);
    return condbuf2;
  } 
  return msg;
}

negate_condition(c)
SCondition *c;
{
  switch (c->type) {
    case C_FULL: c->type=C_NFULL; break;
    case C_NFULL: c->type=C_FULL; break;
    case C_TFULL: c->type=C_NTFULL; break;
    case C_NTFULL: c->type=C_TFULL; break;
    case C_LE: c->type=C_GT; break;
    case C_GT: c->type=C_LE; break;
    case C_PICKUP: c->type=C_NPICKUP; break;
    case C_NPICKUP: c->type=C_PICKUP; break;
    case C_EMBARK: c->type=C_NEMBARK; break;
    case C_NEMBARK: c->type=C_EMBARK; break;
  }
}

evaluate_condition(transport,unit,c)
Unit *transport;
Unit *unit;
SCondition *c;
{ Unit *u;

  switch (c->type) {
    case C_NONE: 
      return TRUE;
    case C_GT: 
      return number_of_occupants(transport,unit->type,unit->group)>
	     c->d.amount; 
    case C_LE: 
      return number_of_occupants(transport,unit->type,unit->group)<=
	     c->d.amount; 
    case C_PICKUP:
      for_all_occupants(transport,u) {
	if (can_carry(unit, u)) return TRUE;
      }
      return FALSE;
    case C_NPICKUP:
      for_all_occupants(transport,u) {
	if (can_carry(unit, u)) return FALSE;
      }
      return TRUE;
    case C_EMBARK:
      for_all_occupants(transport,u) {
	if (can_carry(u, unit)) return TRUE;
      }
      return FALSE;
    case C_NEMBARK:
      for_all_occupants(transport,u) {
	if (can_carry(u, unit)) return FALSE;
      }
      return TRUE;
    case C_FULL:
      return is_full(unit,c->d.utypes);
    case C_NFULL:
      return !is_full(unit,c->d.utypes);
    case C_TFULL:
      return !can_carry(transport,unit);
    case C_NTFULL:
      return can_carry(transport,unit);
    default: case_panic("condition type",c->type);
  }
}

cleanup_standing_orders(unit)
Unit *unit;
{ StandingOrder *so;
  int u;

  so=unit->standing;
  if (so) {
    for_all_unit_types(u) {
      if (so->p_orders[u]) return;
      if (so->e_orders[u]) return;
      if (so->f_orders[u]) return;
    }
    /*
    printf("deleting so\n"); fflush(stdout);
    */
    free(so);
    unit->standing=NULL;
  }
}

cleanup_order_entry(unit,oep,oe)
Unit *unit;
StandingOrderEntry **oep;
StandingOrderEntry *oe;
{
  if (oe->cond) return;
  while (*oep && *oep!=oe) oep=&(*oep)->next;
  if (*oep==oe) {
    *oep=oe->next;
    /*
    printf("deleting group entry\n"); fflush(stdout);
    */
    if (oe->group) free(oe->group);
    free(oe);
  }
  else {
    /*
    printf("group entry to delete not found\n"); fflush(stdout);
    */
  }
  cleanup_standing_orders(unit);
}

void delete_condition_entries(unit,oe)
Unit *unit;
StandingOrderEntry *oe;
{ StandingConditionEntry *ce;

  while (ce=(oe->cond)) {
    oe->cond=ce->next;
    UpdateSOMoveLineRedraw(unit->side,unit,ce->order);
    if (ce->order) free(ce->order);
    free(ce);
  }
}

void delete_order_entries(unit,oe)
Unit *unit;
StandingOrderEntry *oe;
{ StandingOrderEntry *n;

  while (oe) {
    n=oe->next;
    delete_condition_entries(unit,oe);
    if (oe->group) free(oe->group);
    free(oe);
    oe=n;
  }
}

void delete_standing_orders(unit)
Unit *unit;
{ StandingOrder *so=unit->standing;
  int u;

  for_all_unit_types(u) {
    delete_order_entries(unit,so->p_orders[u]);
    delete_order_entries(unit,so->e_orders[u]);
    delete_order_entries(unit,so->f_orders[u]);
  }
  free(so);
  unit->standing=NULL;
}

delete_condition_entry(unit,oep,oe,ce)
Unit *unit;
StandingOrderEntry **oep;
StandingOrderEntry *oe;
StandingConditionEntry *ce;
{ StandingConditionEntry **cep;

  cep=&oe->cond;
  while (*cep && *cep!=ce) cep=&(*cep)->next;
  if (*cep==ce) {
    *cep=ce->next;
    /*
    printf("deleting condition entry\n"); fflush(stdout);
    */
    if (ce->order) free(ce->order);
    free(ce);
  }
  cleanup_order_entry(unit,oep,oe);
}

StandingConditionEntry *search_condition_entry(ce,c,flag)
StandingConditionEntry *ce;
SCondition *c;
int flag;
{ while (ce && (ce->cond.type!=c->type || 
		 (flag && ce->cond.priority!=c->priority)))
    ce=ce->next;
  return ce;
}

StandingOrderEntry *search_order_entry(oe,group)
StandingOrderEntry *oe;
char *group;
{ 
  enter_procedure("search_order_entry");
  /*
  printf("searching for group %s\n",group);
  */
  while (oe) {
    if (oe->group) {
      if (!strcmp(oe->group,"all") || (group && !strcmp(group,oe->group))) {
	/*
	printf("entry found\n");
	*/
	exit_procedure();
	return oe;
      }
    }
    else if (!group) {
      /*
      printf("entry found\n");
      */
      exit_procedure();
      return oe;
    }
    oe=oe->next;
  }
  exit_procedure();
  /*
  printf("not found\n");
  */
  return NULL;
}

Order *select_standing_orders(transport,unit, oetab)
Unit *transport , *unit;
StandingOrderEntry **oetab;
{ StandingOrderEntry *oe;
  StandingConditionEntry *ce;

  oe=search_order_entry(oetab[unit->type],unit->group);
  if (!oe) oe=search_order_entry(oetab[unit->type],NULL);
  if (oe) {
    ce=oe->cond;
    while (ce) {
      /*
      printf("order %s condition %s ",order_desig(ce->order),
				      cond_desig(&ce->cond));
      */
      if (evaluate_condition(transport,unit,&ce->cond)) {
	/*
	printf("true\n");
	*/
	return ce->order;
      }
      /*
      printf("false\n");
      */
      ce=ce->next;
    }
    oe=search_order_entry(oetab[unit->type],NULL);
    if (oe) {
      ce=oe->cond;
      while (ce) {
	if (ce->cond.type==C_NONE) return ce->order;
	ce=ce->next;
      }
    }
  }
  return NULL;
}

set_follow_order(unit)
Unit *unit;
{ Order *neworders;

  if (unit->transport && unit->transport->standing) {
    neworders=select_standing_orders(unit->transport,unit,
			   unit->transport->standing->f_orders);
    if (neworders && neworders->type!=NONE) {
      /*
      printf("set follow order for %d to %s\n",unit_handle(unit->side,unit),
		    order_desig(neworders));
      */
      copy_orders(&(unit->orders), neworders);
      return TRUE;
    }
  }
  return FALSE;
}

/*** <- insert ***/

/* check for and give out standing orders */

get_standing_orders(unit, transport)
Unit *unit, *transport;
{
  Order *newords;
  int r;
/*** (HW) insert -> ***/
  extern int buildphase;
/*** <- insert ***/

  enter_procedure("get_standing_orders");
  if (transport->standing != NULL && !midturnrestore) {
/*** (UK) change -> ***/
    if (buildphase) {
      newords=select_standing_orders(transport,unit,
				     transport->standing->p_orders);
    }
    else {
      newords=select_standing_orders(transport,unit,
				     transport->standing->e_orders);
    }
/*** was:
    newords = (transport->standing->orders)[unit->type];
*** <- change ***/
    if (newords && newords->type != NONE) {
      if (Debug) printf("%s getting orders %s\n",
			unit_desig(unit), order_desig(newords));
      if (newords->type != SENTRY && unit->movesleft > 0)
	for_all_resource_types(r) {
	  try_transfer(unit, transport, r);
	  try_transfer(transport, unit, r);
	}
      copy_orders(&(unit->orders), newords);
      if (newords->type == EMBARK) {
	unit->side->markunit = NULL;
	do_embark(unit->side, unit, newords->rept);
      }
    }
  }
  exit_procedure();
}

/* Units become passengers by linking into front of transport's passenger */
/* list.  They only wake up if the transport is a moving type, but always */
/* pick up standing orders if any defined.  A passenger will get sorted to */
/* move after transport, at end of turn. */

void occupy_unit(unit, transport)
Unit *unit, *transport;
{

    if (!utypes[unit->type].occproduce) cancel_build(unit);
/*** (UK) insert -> ***/
    cancel_terraform(unit); 
/*** <- insert ***/
    if (!midturnrestore)
      check_for_embarkies(unit, transport);
    unit->nexthere = transport->occupant;
    transport->occupant = unit;
    unit->transport = transport;
    sort_occupants_by_speed(transport);
    if (!midturnrestore)
/*** (UK) insert -> ***/
      if (transport->orders.type==FILL) 
/*** <- insert ***/
      wake_if_full(transport);
    if (mobile(transport->type) && !midturnrestore) 
      wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
    /* make sure not to give out standing orders to units after a restore
       from savefile */
    occupy_hex_aux(unit, transport->x, transport->y);
    all_see_occupy(unit, transport->x, transport->y);
    get_standing_orders(unit, transport);
}

/* Unit departs from a hex by zeroing out pointer if in hex or by being */
/* removed from the list of transport occupants. */
/* Dead units (hp = 0) may be run through here, so don't error out. */

void leave_hex(unit)
Unit *unit;
{
    int ux = unit->x, uy = unit->y;

    if (ux < 0 || uy < 0) {
	/* Sometimes called twice */
    } else if (unit->transport != NULL) {
	leave_unit(unit, unit->transport);
	leave_hex_aux(unit);
	all_see_leave(unit, ux, uy);
    } else {
	set_unit_at(ux, uy, NULL);
/*** (UK) insert -> ***/
	if (unit->side && cover(unit->side,ux,uy)>0)
/*** <- insert ***/
	see_exact(unit->side, ux, uy);
	see_hex(unit->side, ux, uy);
	leave_hex_aux(unit);
	all_see_leave(unit, ux, uy);
	all_see_hex(ux, uy);
    }
}

/* Trash old coordinates (recursively) just in case.  Catches many bugs... */

leave_hex_aux(unit)
Unit *unit;
{
    register Unit *occ;

    cover_area(unit, unit->x, unit->y, -1);
    unit->prevx = unit->x; unit->prevy = unit->y;
/*** (HW) insert -> ***/
    UpdateMoveLineRedraw(unit->side,unit);
/*** <- insert ***/
    unit->x = -1;  unit->y = -1;
    for_all_occupants(unit, occ) leave_hex_aux(occ);
}

/* Disembarking unlinks from the list of passengers. */

void leave_unit(unit, transport)
Unit *unit, *transport;
{
    Unit *occ;

    if (unit == transport->occupant) {
	transport->occupant = unit->nexthere;
    } else {
	for_all_occupants(transport, occ) {
	    if (unit == occ->nexthere) {
		occ->nexthere = occ->nexthere->nexthere;
		break;
	    }
	}
    }
    unit->transport = NULL;
}

/* Handle the general situation of a unit changing allegiance from one side */
/* to another.  This is a common internal routine, so no messages here. */

unit_changes_side(unit, newside, reason1, reason2)
Unit *unit;
Side *newside;
int reason1, reason2;
{
  static int dummy[MAXUTYPES];
  unit_changes_side_Ncount(unit, newside, reason1, reason2, dummy, dummy);
}

unit_changes_side_Ncount(unit, newside, reason1, reason2, captured, killed)
Unit *unit;
Side *newside;
int reason1, reason2;
int captured[MAXUTYPES], killed[MAXUTYPES]; /* arrays to increment */
{
    Side *oldside = unit->side;
    Unit *occ;

    delete_unit(unit);  /* no longer on this side. */

    if (newside != NULL)
      insert_unit(newside->unithead, unit);
    else insert_unit(neutral_placeholder.unithead, unit);
    if (reason1 == CAPTURE && !period.capturemoves)
      unit->movesleft = 0;
    if (oldside != NULL) {
	oldside->units[unit->type]--;
	if (producing(unit)) oldside->building[unit->product]--;
	if (reason2 >= 0) oldside->balance[unit->type][reason2]++;
	update_state(oldside, unit->type);
	if (reason2 == PRISONER)
	  oldside->areas[area_index(unit->x, unit->y)].units_lost +=
	    2 * unit_strength(unit->type);
      }
    if (newside == NULL && !utypes[unit->type].isneutral) {
	kill_unit(unit, -1);
    } else {
	if (newside != NULL) {
	    newside->units[unit->type]++;
	    if (producing(unit)) newside->building[unit->product]++;
	    if (reason1 >= 0) newside->balance[unit->type][reason1]++;
	    update_state(newside, unit->type);
	}
	for_all_occupants(unit, occ) {
	  if ((occ->side == unit->side || reason2==PRISONER)
	      && utypes[occ->type].changeside) {
	    unit_changes_side_Ncount(occ, newside, reason1, reason2,
				     captured, killed);
	    captured[occ->type]++;	/* count capture here */
	  } else if (reason2>0 && reason2!=GIFT) {
	    kill_unit(occ, COMBAT);
	    killed[occ->type]++; /* count slaughter here */
	  } else {
	    /* unit remains loyal */
	  }
	}
    }
    if (alive(unit)) {
	cover_area(unit, unit->x, unit->y, -1);
	assign_unit_to_side(unit, newside);
	cover_area(unit, unit->x, unit->y, 1);
    }
}

void set_terraform(unit, type)
Unit *unit;
int type;
{
    int oldproduct=unit->terraform;
    if (!neutral(unit) && global.setproduct && type != unit->terraform) {
	unit->terraform = type;
	if (type != TNOTHING) {
	    if (!utypes[unit->type].maker) {
	      order_sentry(unit, terraform_time(unit, type));
	    }
	}
    }
}

void set_tschedule(unit)
Unit *unit;
{
    if (terraforming(unit)) unit->tschedule = terraform_time(unit, unit->terraform);
}

/* Change the product of a unit.  Displays will need appropriate mods. */

void set_product(unit, type)
Unit *unit;
int type;
{
/*** (UK) insert -> ***/
    int oldproduct=unit->product;
/*** <- insert ***/
    if (!neutral(unit) && global.setproduct && type != unit->product) {
	if (unit->product != NOTHING) {
	    unit->side->building[unit->product]--;
/*** (UK) change -> ***/
	    unit->product=NOTHING;
	    update_state(unit->side, oldproduct);
/*** was:
	    update_state(unit->side, unit->product);
*** <- change ***/
	}
/*** (UK) insert -> ***/
	unit->product = type;
	unit->next_product = type;
	unit->built = 0;
/*** <- insert ***/
	if (type != NOTHING) {
	    unit->side->building[type]++;
	    update_state(unit->side, type);
	    if (!utypes[unit->type].maker) {
	      order_sentry(unit, build_time(unit, type));
	    }
	}
/*** (UK) remove -> ***
	unit->product = type;
	unit->next_product = type;
	unit->built = 0;
*** <- remove ***/
	}
}

/* Set product completion time.  Startup development cost incurred only */
/* if directed. */
/* Technology development cost only incurred for very first unit that the */
/* side constructs.  Both tech and startup are percent addons. */

void set_schedule(unit)
Unit *unit;
{
    if (producing(unit)) unit->schedule = build_time(unit, unit->product);
}

/*** (UK) change -> ***/
terraform_time(unit, type)
Unit *unit;
int type;
{
  int d, nx, ny;
  int diff=0;
  int research=0;
  if (type != unit->last_terraform) {
     research=utypes[unit->type].terraform[type]/2;
  }
  short cur = terrain_at(unit->x,unit->y);
  for_all_directions(d) {
      nx = wrap(unit->x + dirx[d]);  ny = limit(unit->y + diry[d]);
      short t = terrain_at(nx,ny);
      diff+t-cur;
  }
  printf("diff=%d\n", diff);
  return max(1, utypes[unit->type].terraform[type]+diff/NUMDIRS);
}

/*** <- change ***/

/* Basic routine to compute how long a unit will take to build something. */

build_time(unit, prod)
Unit *unit;
int prod;
{
    int schedule = utypes[unit->type].make[prod];
    int u, research_delay;

    if (unit->built == 0)
	schedule += ((schedule * utypes[prod].startup) / 100);
    if ((No_give_r_and_d && (unit->side->balance[prod][PRODUCED] < 1)) ||
       (!No_give_r_and_d && (unit->side->counts[prod] <= 1))) { /* XXX */
	research_delay = ((schedule * utypes[prod].research) / 100);
	for_all_unit_types(u)
          if ((No_give_r_and_d && (unit->side->balance[u][PRODUCED] > 0)) ||
            (!No_give_r_and_d && (unit->side->counts[u] > 1))) { /* XXX */
	    research_delay -= (utypes[unit->type].make[u] *
			       utypes[prod].research_contrib[u]) / 100;
	  }
	if (research_delay > 0)
	  schedule += research_delay;
      }
    return schedule;
}

/* Remove a unit from play.  This is different from making it available for */
/* reallocation - only the unit flusher can do that.  We remove all the */
/* passengers too, recursively.  Sometimes units are "killed twice", so */
/* be sure not to run all this twice.  Also count up occupant deaths, being */
/* sure not to count the unit itself as an occupant. */

kill_unit(unit, reason)
Unit *unit;
int reason;
{
    int u = unit->type, u2;

    enter_procedure("kill unit");
    if (alive(unit)) {
	for_all_unit_types(u2) occdeath[u2] = 0;
	/* avoid potential problems caused by displays already being closed. */
	if (reason != ENDOFWORLD) leave_hex(unit);
	kill_unit_aux(unit, reason);
	occdeath[u]--;
	recent_dead_flushed = FALSE;
    }
    exit_procedure();
}

/* Trash it now - occupant doesn't need to leave_hex.  Also record a reason */
/* for statistics, and update the apropriate display.  The unit here should */
/* be known to be alive. */

extern Unit *next_unit_to_move();

kill_unit_aux(unit, reason)
Unit *unit;
int reason;
{
    int u = unit->type;
    Unit *occ;
    
    unit->hp = 0;
    occdeath[u]++;
    free_plan(unit->plan);
    unit->plan = NULL;
    if (unit->side != NULL && reason >= 0) {
	unit->side->balance[u][reason]++;
	unit->side->units[u]--;
	if (producing(unit)) unit->side->building[unit->product]--;
	update_state(unit->side, u);
  	if(unit->side->areas != NULL)
	    unit->side->areas[area_index(unit->prevx, 
		unit->prevy)].units_lost += unit_strength(u);
    }
/*** (UK) insert -> ***/
    if (unit->side && unit==unit->side->movunit) {
      if (!(unit->side->movunit=next_unit_to_move(unit->side))) {
	update_sides(unit->side);
      }
    }
/*** <- insert ***/
    for_all_occupants(unit, occ) if (alive(occ)) kill_unit_aux(occ, reason);
}

/* Get rid of all dead units at once.  It's important to get rid of all */
/* of the dead units, so they don't come back and haunt us. */
/* (This routine is basically a garbage collector, and should not be called */
/* during a unit list traversal.) The process starts by finding the first */
/* live unit, making it the head, then linking around all in the middle. */
/* Dead units stay on the dead unit list for each side until that side has had */
/* a chance to move.  Then they are finally flushed in a permanent fashion. */ 

void flush_dead_units()
{
    Unit *unit, *prevunit;
    Side *side;

    /* This never ends up flushing neutral units, no big loss. */
    if (!recent_dead_flushed) {
      recent_dead_flushed = TRUE;
      for_all_units(side, unit) {
	if (!alive(unit)) {
	  prevunit = unit->prev;
	  delete_unit(unit);
	  put_unit_on_dead_list(unit);
	  unit = prevunit;
	}
      }
    }
}

/* Put dead units on sides dead unit lists. */
put_unit_on_dead_list(unit)
Unit *unit;
{
  enter_procedure("put unit on dead list");
  if (unit->side == NULL)
    flush_one_unit(unit);
  else {
    unit->next = unit->side->deadunits;
    unit->side->deadunits = unit;
  }
  --numunits;
  exit_procedure();
}

/* Clean up dead units and put them back on free list. */

flush_side_dead(side)
Side *side;
{
  Unit *unit, *next;
  
  next = side->deadunits;
  while (next != NULL) {
    unit = next;
    next = unit->next;
    flush_one_unit(unit);
  }
  side->deadunits = NULL;
  side->curdeadunit = NULL;
}

/* Keep it clean - hit all links to other places.  Some might not be */
/* strictly necessary, but this is not an area to take chances with. */

flush_one_unit(unit)
Unit *unit;
{
    unit->type = NOTHING;
    unit->occupant = NULL;
    unit->transport = NULL;
    unit->nexthere = NULL;
    if (unit->standing != NULL) {
      free(unit->standing);
      unit->standing = NULL;
    }
    unit->next = freeunits;
    freeunits = unit;
}

/*** (HW) change -> ***/
void sort_side_units(side)
Side *side;
{
    register Unit *unit, *nextunit, *u, *pu;
    Unit *newlist = NULL;
    Unit **actp;

    if (side->unithead == side->unithead->next) return; /* nothing to sort */
    
    nextunit = side->unithead->prev;
    nextunit->next = NULL;
    side->unithead->next->prev = NULL;

    while (nextunit) {
      unit = nextunit;
      nextunit = nextunit->prev;

      u = newlist;
      while (u && in_order(u,unit)) {
	pu = u;
	u = u->next;
      }
      if (u == newlist) {
	unit->next = u;
	newlist = unit;
      }
      else {
	unit->next = u;
	pu->next = unit;
      }
    }

    /* now insert into unithead and correct the prev pointers */
    side->unithead->next = newlist;
    newlist->prev = side->unithead;
    u = newlist;
    while(pu = u->next) {
      pu->prev = u;
      u = u->next;
    }
    u->next = side->unithead;
    side->unithead->prev = u;
}

void sort_units(doall)
bool doall;
{
    Side *side;

    for_all_sides(side) sort_side_units(side);
}

/* Do multiple passes of bubble sort.  This is intended to improve locality */
/* among unit positions and reduce the amount of scrolling around. */
/* Data is generally coherent, so bubble sort not too bad if we allow */
/* early termination when everything in order. */
/* If slowness objectionable, replace with something clever. */
/*** Was:

void sort_units(doall)
bool doall;
{
    bool flips;
    int passes = 0;
    register Unit *unit, *nextunit;
    Side *side;

    for_all_sides(side) {
      passes = 0;
      flips = TRUE;
      while (flips && (doall || passes < numunits / 10)) {
	flips = FALSE;
	for_all_side_units(side,unit) 
	  if (unit->next != side->unithead && !in_order(unit, unit->next)) {
	    flips = TRUE;
	    nextunit = unit->next;
	    unit->prev->next = nextunit;
	    nextunit->next->prev = unit;
	    nextunit->prev = unit->prev;
	    unit->next = nextunit->next;
	    nextunit->next = unit;
	    unit->prev = nextunit;
	  }
	passes++;
      }
    }
    if (Debug) printf("Sorting passes = %d\n", passes);
}
*** <- change ***/

in_order(unit1, unit2)
Unit *unit1, *unit2;
{
    int d1, d2;

    if (side_number(unit1->side) < side_number(unit2->side)) {
      printf("Sides mixed");
      return TRUE;
    }
    if (side_number(unit1->side) > side_number(unit2->side)) {
      printf("sides mixed");
      return FALSE;
    }
    if (!neutral(unit1)) {
	d1 = distance(unit1->side->cx, unit1->side->cy, unit1->x, unit1->y);
	d2 = distance(unit2->side->cx, unit2->side->cy, unit2->x, unit2->y);
	if (d1 < d2) return TRUE;
	if (d1 > d2) return FALSE;
    }
    if (unit1->y > unit2->y) return TRUE;
    if (unit1->y < unit2->y) return FALSE;
    if (unit1->x < unit2->x) return TRUE;
    if (unit1->x > unit2->x) return FALSE;
    if (unit1->transport == NULL && unit2->transport != NULL) return TRUE;
    if (unit1->transport != NULL && unit2->transport == NULL) return FALSE;
    if (unit1 == unit2->transport) return TRUE;
    if (unit1->transport == unit2) return FALSE;
    return TRUE;
}

/* A unit runs low on supplies at the halfway point, but only worries about */
/* the essential items.  Formula is the same no matter how/if occupants eat */
/* transports' supplies. */

low_supplies(unit)
Unit *unit;
{
    int u = unit->type, r;

    for_all_resource_types(r) {
	if (((utypes[u].consume[r] > 0) || (utypes[u].tomove[r] > 0)) &&
	    /* Really should check that the transport is adequate for */
	    /* supplying the fuel */ 
	    (unit->transport == NULL)) {
	    if (2 * unit->supply[r] <= utypes[u].storage[r]) return TRUE;
	}
    }
    return FALSE;
}

/* useful for the machine player to know how long it can move this
piece before it should go home. */

moves_till_low_supplies(unit)
Unit *unit;
{
    int u = unit->type, r, moves = 1000;

    for_all_resource_types(r) {
	if ((utypes[u].tomove[r] > 0)) {
	  moves = min(moves,
		      (unit->supply[r] - (utypes[u].storage[r] / 2)) /
		      utypes[u].tomove[r]);
	}
    }
    return moves;
}

/* Cancel the build of a unit because it attacked or moved.  Does */
/* nothing if not building or if a maker. */

void cancel_build(unit)
Unit *unit;
{
    Side *us = unit->side;

    if (global.setproduct && producing(unit) && !utypes[unit->type].maker) {
      notify(us, "%s moved, cancelling its build!", unit_handle(us, unit));
      if (active_display(us) && humanside(us)) beep(us);
      set_product(unit, NOTHING);
      unit->schedule = 0;
    }
}

/*** (UK) insert -> ***/
void cancel_terraform(unit)
Unit *unit;
{
    Side *us = unit->side;

    if (global.setproduct && terraforming(unit)) {
      notify(us, "%s moved, cancelling its terraforming!", unit_handle(us, unit));
      if (active_display(us) && humanside(us)) beep(us);
      set_terraform(unit, TNOTHING);
      unit->tschedule = 0;
    }
}
/*** <- insert ***/

/* Display the standing orders of the given unit. */

/*** (UK) insert -> ***/
static clear_orders_handled(oetab)
StandingOrderEntry **oetab;
{ StandingOrderEntry *oe;
  int u;

  enter_procedure("clear_orders_handled");
  for_all_unit_types(u) {
    oe=oetab[u];
    while (oe) {
      oe->handled=FALSE;
      oe=oe->next;
    }
  }
  exit_procedure();
}

static char *search_not_handled_group(oetab) 
StandingOrderEntry **oetab;
{ StandingOrderEntry *oe;
  int u;

  enter_procedure("search_not_handled_group");
  for_all_unit_types(u) {
    oe=oetab[u];
    while (oe) {
      if (!oe->handled) {
	/*
	printf("found group %s\n",oe->group); fflush(stdout);
	*/
	exit_procedure();
	return oe->group;
      }
      oe=oe->next;
    }
  }
  /*
  printf("no more group found\n"); fflush(stdout);
  */
  exit_procedure();
  return SCANCELED;
}

static show_order_group();

static show_order_class(side,oetab,name)
Side *side;
StandingOrderEntry **oetab;
char *name;
{ int total=0;
  char *group;

  enter_procedure("show_order_class");
  clear_orders_handled(oetab);
  while ((group=search_not_handled_group(oetab))!=SCANCELED) 
    total+=show_order_group(side,oetab,name,group);
  exit_procedure();
  return total;
}

static void new_line(side,name,group)
Side *side;
char *name;
char *group;
{
  if (*spbuf) notify(side, "%s", spbuf);
  if (group) 
    sprintf(spbuf, "[%s(%s)]: ",name,group);
  else
    sprintf(spbuf, "[%s<none>]: ",name);
}


static void notify_order(side,name,group,buf,follow)
Side *side;
char *name;
char *group;
char *buf;
int follow;
{ 
  enter_procedure("notify_order");
  if (strlen(spbuf) + strlen(buf) > side->nw) {
    new_line(side,name,group);
    if (follow && !side->compactmode) strcat(spbuf,"   ");
  }
  strcat(spbuf, buf);
  exit_procedure();
}

static show_order_group(side,oetab,name,group)
Side *side;
StandingOrderEntry **oetab;
char *name;
char *group;
{ int u;
  StandingOrderEntry *oe;
  StandingConditionEntry *ce;
  Order *ords;
  int total=0;
  int flush=0;
  int found;
  char *p;

  enter_procedure("show_order_group");
  /*
  printf("show_order_group\n"); fflush(stdout);
  printf("%s: %s %s\n",side->name,name,group);
  fflush(stdout);
  */

  *spbuf='\0';
  new_line(side,name,group);

  for_all_unit_types(u) {
      if (oe=search_order_entry(oetab[u],group)) {
	  sprintf(tmpbuf2,"%s: ",utypes[u].name);
          oe->handled=1;
	  found=0;
	  ce=oe->cond;
	  while (ce) {
            ords=ce->order;
	    if (ords && ords->type != NONE) {
		if (found) strcat(tmpbuf2,", ");
		notify_order(side,name,group,tmpbuf2,found);
		sprintf(tmpbuf2, "%s to %s%0s",
			cond_desig(&ce->cond),order_desig(ords),
			((ords->morder) ? " and auto" : ""));
		found++;
		total++;
		flush=1;
	    }
	    ce=ce->next;
	  }
	  if (found) {
	    strcat(tmpbuf2,"; ");
	    notify_order(side,name,group,tmpbuf2,found);
	    if (!side->compactmode) {
	      new_line(side,name,group);
	      flush=0;
	    }
	  }
      }
  }
  /*
  printf("found %d\n",total); fflush(stdout);
  */
  if (flush) notify(side, "%s", spbuf);
  exit_procedure();
  return total;
}
/*** <- insert ***/

show_standing_orders(side, unit)
Side *side;
Unit *unit;
{
/*** (UK) change -> ***/
    int total=0;
/*** was:
    int u;
    Order *ords;
*** <- change ***/ 

    enter_procedure("show standing orders");
    if (unit->standing != NULL) {
/*** (UK) change -> ***/
	if (side->curgroup) {
	  total+=show_order_group(side,unit->standing->p_orders,
					    sorder_types[1],side->curgroup);
	  total+=show_order_group(side,unit->standing->e_orders,
					    sorder_types[2],side->curgroup);
	  total+=show_order_group(side,unit->standing->f_orders,
					    sorder_types[3],side->curgroup);
	  if (!total) {
	    sprintf(tmpbuf,"There are no standing orders for group %s.",
			    side->curgroup);
	    notify(side, tmpbuf);
	  }
	}
	else {
	  total+=show_order_class(side,unit->standing->p_orders,
					    sorder_types[1]);
	  total+=show_order_class(side,unit->standing->e_orders,
					    sorder_types[2]);
	  total+=show_order_class(side,unit->standing->f_orders,
					    sorder_types[3]);
	  if (!total) notify(side,"There are no standing orders.");
	}
/*** was:
	sprintf(spbuf, "Orders:  ");
	for_all_unit_types(u) {
	    ords = unit->standing->orders[u];
	    if (ords && ords->type != NONE) {
		sprintf(tmpbuf, "%s to %s%0s;  ",
			utypes[u].name, order_desig(ords),
			((ords->morder) ? " and auto" : ""));
		if (strlen(spbuf) + strlen(tmpbuf) > 
		    side->nw) {
		  notify(side, "%s", spbuf);
		  sprintf(spbuf, "Orders:  ");
		}
		strcat(spbuf, tmpbuf);
	    }
	}
	notify(side, "%s", spbuf);
*** <- change ***/
    } else {
	notify(side, "No standing orders defined yet.");
    }
    exit_procedure();
}

/* Build a short phrase describing a given unit to a given side. */
/* First we supply identification of side, then of unit itself. */

char *
unit_handle(side, unit)
Side *side;
Unit *unit;
{
    char *utypename;
/*** (UK) insert -> ***/
    char *unitbuf;

    unitbuf=(unitbufselect++&1)?unitbuf2:unitbuf1;
/*** <- insert ***/

    if (unit == NULL || (!alive(unit) && unit != side->curdeadunit))
      return "???";
    utypename = utypes[unit->type].name;
    if (utypes[unit->type].named == 2 && unit->name) return unit->name;
    if (unit->side == NULL) {
	sprintf(unitbuf, "the neutral ");
    } else if (unit->side == side) {
	sprintf(unitbuf, "your%0s ",
		((unit->orders.morder) ? "*" : ""));
    } else {
	sprintf(unitbuf, "the %s ", unit->side->name);
    }
    if (unit->name != NULL) {
/*** (HW) change -> ***/
	sprintf(tmpbuf, "%s %s at %d,%d", utypename, unit->name,
		unit->x,unit->y);
/*** was:
	sprintf(tmpbuf, "%s %s", utypename, unit->name);
*** <- change ***/
    } else if (unit->number > 0) {
/*** (HW) change -> ***/
	sprintf(tmpbuf, "%s %s at %d,%d", ordinal(unit->number), utypename,
		unit->x,unit->y);
/*** was:
	sprintf(tmpbuf, "%s %s", ordinal(unit->number), utypename);
*** <- change ***/
    } else {
/*** (HW) change -> ***/
	sprintf(tmpbuf, "%s at %d,%d", utypename, unit->name,
		unit->x,unit->y);
/*** was:
	sprintf(tmpbuf, "%s", utypename);
*** <- change ***/
    }
    strcat(unitbuf, tmpbuf);
    return unitbuf;
}

/* Shorter unit description omits side name, but uses same buffer. */

char *
short_unit_handle(unit)
Unit *unit;
{
/*** (UK) insert -> ***/
    char *unitbuf;

    unitbuf=(unitbufselect++&1)?unitbuf2:unitbuf1;
/*** <- insert ***/
    if (unit->name == NULL) {
	sprintf(unitbuf, "%s %s",
		ordinal(unit->number), utypes[unit->type].name);
    } else {
	sprintf(unitbuf, "%s", unit->name);
    }
    return unitbuf;
}

/* General-purpose routine to take an array of anonymous unit types and */
/* summarize what's in it, using numbers and unit chars. */

char *
summarize_units(buf, ucnts)
char *buf;
int *ucnts;
{
    char tmp[BUFSIZE];
    int u;

    sprintf(buf, "");
    for_all_unit_types(u) {
	if (ucnts[u] > 0) {
	    sprintf(tmp, " %d %c", ucnts[u], utypes[u].uchar);
	    strcat(buf, tmp);
	}
    }
    return buf;
}

/* Search for a unit with the given id number. */

Unit *
find_unit(n)
int n;
{
    Unit *unit;
    Side *side;

    for_all_units(side,unit)
      if (alive(unit) && unit->id == n) return unit;
    return NULL;
}

/* Given a unit character, find the type. */

find_unit_char(ch)
char ch;
{
    int u;

    for_all_unit_types(u) if (utypes[u].uchar == ch) return u;
    return NOTHING;
}

/* Generate a name for a unit, using best acrynomese.  This is invoked when */
/* a new named unit has been created. */

char *
make_unit_name(unit)
Unit *unit;
{
    sprintf(spbuf, "%c%c-%c-%02d",
	    uppercase(unit->side->name[0]), uppercase(unit->side->name[1]),
	    utypes[unit->type].uchar, unit->number);
    return copy_string(spbuf);
}

/*** (UK/HW) insert -> ***/
int next_ready(side,u)
Side *side;
int u;
{ unsigned next=-1;

  Unit *unit;
  for_all_side_units(side,unit) {
    if (unit->side==side && producing(unit) && unit->product==u) {
      if (unit->schedule<next) next=unit->schedule;
    }
  }
  return next;
}

show_production(side,u)
Side *side;
{ Unit *unit;
  char buf[1000];

  notify(side,"Production of %s",plural_form(utypes[u].name));
  for_all_side_units(side,unit) {
    if (unit->side==side && producing(unit) && unit->product==u) {
      sprintf(buf,"%s in %d turns",unit_handle(side,unit),unit->schedule);
      notify(side,buf);
    }
  }
}
/*** <- insert ***/

/* Reverse the order of the unit list.  This is needed since the */
 /* loaded units from map files are put into the list in reverse */
 /* order. */

reverse_unit_order()
{
  Unit *next, *unit;
  Side *side;

  for_all_sides(side) {
    next = side->unithead->next;
    do {
      unit = next;
      next = unit->next;
      unit->next = unit->prev;
      unit->prev = next;
    } while (unit != side->unithead);
  }
}    

sort_occupants_by_speed(trans)
Unit *trans;
{
  bool flips = TRUE;
  Unit *prevunit, *unit, *nextunit;

  while (flips) {
    flips = FALSE;
    prevunit = NULL;
    unit = trans->occupant;
    while (unit != NULL) {
      if (unit->nexthere != NULL &&
	  utypes[unit->type].speed < utypes[unit->nexthere->type].speed) {
	flips = TRUE;
	nextunit = unit->nexthere;
	if (prevunit != NULL) 
	  prevunit->nexthere = nextunit;
	else trans->occupant = nextunit;
	unit->nexthere = nextunit->nexthere;
	nextunit->nexthere = unit;
	prevunit = nextunit;
      } else {
	prevunit = unit;
	unit = unit->nexthere;
      }
    }
  }
}

/* Return the first unit on the list. */

Unit *
first_unit (side)
Side *side;
{
    return side->unithead;
}

/*** (UK) insert -> ***/
extern Unit *filter_move_unit();
/*** <- insert ***/

/* Find the side's next unit that can move this turn.
 * Return NULL if not found.  This procedure is only
 * used by movement_phase.
 */
Unit *
next_unit (side, afterunit, onscreen)
Side *side;
Unit *afterunit;
bool onscreen;
{
    register Unit *unit, *start;
/*** (UK) insert -> ***/
    register Unit *found=NULL;
    extern bool ODebug;

    if (ODebug) pui(side,"search next unit:",afterunit->next);
/*** <- insert ***/

    unit = start = afterunit->next;

    do {
        if (!onscreen && unit->orders.type == DELAY)
	  wake_unit(unit, NOTHING, WAKEOWNER, NULL);
	if (unit != side->unithead
	    && can_move_unit(side, unit)
	    && (!onscreen || in_middle(side, unit->x, unit->y))) {
/*** (UK) insert -> ***/
            if (!onscreen) found=unit;
	    if (filter_move_unit(side,unit))
/*** <- insert ***/
	    return unit;
	}
	unit = unit->next;
    } while (unit != start); 
/*** (UK) change -> ***/
    if (ODebug && !onscreen) pui(side,"use direct order:",found);
    return found;
/*** was:
    return NULL;
*** <- change ***/
}

void insert_unit(unithead, unit)
Unit *unithead, *unit;
{

    unit->next = unithead->next;
    unit->prev = unithead;
    unithead->next->prev = unit;
    unithead->next = unit;

}

void insert_unit_tail(unithead, unit)
Unit *unithead, *unit;
{

    unit->next = unithead;
    unit->prev = unithead->prev;
    unithead->prev->next = unit;
    unithead->prev = unit;

}

/* delete the unit pointed to from some list */

void delete_unit(unit)
Unit *unit;
{
  unit->next->prev = unit->prev;
  unit->prev->next = unit->next;
  unit->next = NULL;
  unit->prev = NULL;
}

/*** (UK) insert -> ***/
/* orders that don't consume moves until the very end of the turn */
delayed_order(o)
int o;
{ return ( (o) == SENTRY || (o) == EMBARK || (o) == FILL || (o) == DELAY 
/*	   || (o) == APICKUP  */
	 );
}

/* orders that mean the unit will stay in one place for a while */
napping_order(o)
int o;
{ return ( (o) == SENTRY || (o) == EMBARK || (o) == FILL ||
	   (o) == RPICKUP || (o) == APICKUP
	 );
}

can_move_unit(s,u)
Side *s;
Unit *u;
{ return ((u) != NULL && (u)->side == (s) && alive(u) && 
		(((u)->movesleft > 0 && !delayed_order((u)->orders.type)) 
		 || idled(u)));
}

can_build(unit,type)
Unit *unit;
{ Unit *mainunit=unit_at(unit->x,unit->y);

	if (mainunit==NULL) {
	    if (could_occupy(type, terrain_at(unit->x, unit->y))) {
	      return TRUE;
	    }
	    return FALSE;
	}
#ifndef NO_OCCMAKE
	/* when the produced unit can be produced inside the maker, do so,
	** regardless of what the 'mainunit' can carry. */
	else if ( (utypes[unit->type].occproduce || utypes[unit->type].maker)
		&& could_carry(unit->type, type) && 
		   can_carry_aux(unit, type)) {
	  return TRUE;
	}
#endif
        else if (could_carry(mainunit->type, type)) {
	  return TRUE;
	} else if (could_carry(type, mainunit->type)) {
	    if (could_occupy(type, terrain_at(unit->x, unit->y))) {
		return TRUE;
	    } else {
		return FALSE;
	    }
	}
        return FALSE;
}
/*** <- insert ***/
