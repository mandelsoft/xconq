/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file includes all turn phases except init, movement, and endgame. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

/* Should probably be in period.h */

#define will_garrison(u1, u2) (utypes[u1].guard[u2] > 0)

bool anyrevolt = FALSE;
bool anysurrender = FALSE;
bool anyaccident = FALSE;
bool anyattrition = FALSE;

/* Spying phase reveals other sides' secrets.  This phase also automatically */
/* updates allies's view of each other's units; not every turn, but fairly */
/* regularly (can do briefing at any time, of course).  Spying will reveal */
/* hex even if never seen before (dubious). */

spy_phase()
{
    Side *side, *other;

    routine("Spy Phase");
    enter_procedure("spy_phase");
    if (Debug) printf("Entering spy phase\n");
    for_all_sides(side) {
	if (probability(period.spychance)) {
	    other = side_n(RANDOM(numsides));
	    if (other != NULL && !other->lost && !allied_side(side, other)) {
		if (period.spychance < 20) {
		    notify(side, "You found out some of the %s dispositions!!",
			   other->name);
		}
		reveal_side(other, side, period.spyquality);
	    }
	}
	for_all_sides(other) {
	    if (allied_side(other, side) && side != other) {
		reveal_side(side, other, 100);
	    }
	}
    }
    exit_procedure();
}

/* The disaster phase handles revolts, surrenders, accidents, and attrition. */
/* Note that accidents and attrition happen even if unit changes sides. */
/* Also, revolts happen first, so something that revolts may subsequently */
/* surrender to nearby unit. */


disaster_phase()
{
    Unit *unit;
    Side *loop_side;
    Unit *nextunit;

    routine("disaster_phase");
    enter_procedure("disaster_phase");
    if (Debug) printf("Entering disaster phase\n");
    {
	int u, t;

	for_all_unit_types(u) {
	    if (utypes[u].revolt > 0) anyrevolt = TRUE;
	    if (utypes[u].surrender > 0 || utypes[u].siege > 0) 
		anysurrender = TRUE;
	    for_all_terrain_types(t) {
		if (utypes[u].accident[t] > 0) anyaccident = TRUE;
		if (utypes[u].attrition[t] > 0) anyattrition = TRUE;
	    }
	}
    }
    if (anyrevolt) {
	for_all_units(loop_side, unit){
		nextunit = unit->next;
		if (alive(unit)) unit_revolt(unit);
		if( unit->next != nextunit){
		    if(Debug){
			printf(" %s:", unit_handle(loop_side, unit));
			printf("Revolt:Broken unit list, recovering\n");
		    }
		    unit = nextunit->prev;
		    /* so that for_all_units() works even when a unit
		    ** changes sides and the unit list order changes */
		}
	}
	
    }
    if (anysurrender) {
	for_all_units(loop_side, unit) {
		nextunit = unit->next;
		if (alive(unit)) unit_surrender(unit);
		if( unit->next != nextunit){
		    if(Debug){
			printf(" %s:", unit_handle(loop_side, unit));
			printf("Revolt:Broken unit list, recovering\n");
		    }
		    unit = nextunit->prev;
		    /* so that for_all_units() works even when a unit
		    ** changes sides and the unit list order changes */
		}
	}
    }
    if (anyaccident) {
	for_all_units(loop_side, unit){
		nextunit = unit->next;
		if (alive(unit)) unit_accident(unit);
		if( unit->next != nextunit){
		    if(Debug){
			printf(" %s:", unit_handle(loop_side, unit));
			printf("Revolt:Broken unit list, recovering\n");
		    }
		    unit = nextunit->prev;
		    /* so that for_all_units() works even when a unit
		    ** changes sides and the unit list order changes */
		}
	}
    }
    if (anyattrition) {
	for_all_units(loop_side, unit) {
		nextunit = unit->next;
		if (alive(unit)) unit_attrition(unit);
		if( unit->next != nextunit){
		    if(Debug){
			printf(" %s:", unit_handle(loop_side, unit));
			printf("Revolt:Broken unit list, recovering\n");
		    }
		    unit = nextunit->prev;
		    /* so that for_all_units() works even when a unit
		    ** changes sides and the unit list order changes */
		}
	}
    }
    exit_procedure();
}

/* A unit revolts by going back to its "true side" (which is usually set at */
/* unit creation time).  The base revolt chance is worst case if morale is */
/* variable; a unit at maximum (nonzero) morale will never revolt. */

unit_revolt(unit)
Unit *unit;
{
    int u = unit->type, ux = unit->x, uy = unit->y, chance;
    Side *oldside = unit->side;

    enter_procedure("unit_revolt");
    if (utypes[unit->type].revolt > 0) {
	chance = utypes[u].revolt;
/*	maxmor = utypes[u].maxmorale;
  if (maxmor > 0) chance = (chance * (maxmor - unit->morale)) / maxmor; */
	if (unit->trueside != oldside && RANDOM(10000) < chance) {
	    notify(oldside, "%s revolts!", unit_handle(oldside, unit));
	    unit_changes_side(unit, unit->trueside, CAPTURE, DISASTER);
	    see_exact(oldside, ux, uy);
	    draw_hex(oldside, ux, uy, TRUE);
	    all_see_hex(ux, uy);
	}
    }
    exit_procedure();
}

/* Units may also surrender to adjacent enemy units, but only to a type */
/* that is both visible and capable of capturing the unit surrendering. */
/* Neutrals have to be treated differently, since they don't have a view */
/* to work from.  We sort of compute the view "on the fly". */

unit_surrender(unit)
Unit *unit;
{
    bool surrounded = TRUE;
    int u = unit->type, chance, d, x, y;
    viewdata view;
    Unit *unit2, *eunit = NULL;
    Side *us = unit->side, *es;

    enter_procedure("unit_surrender");
    if (utypes[u].surrender > 0 || utypes[u].siege > 0) {
	chance = 0;
	for_all_directions(d) {
	    x = wrap(unit->x + dirx[d]);  y = limit(unit->y + diry[d]);
	    if (neutral(unit)) {
		if (((unit2 = unit_at(x, y)) != NULL) &&
		    (probability(utypes[unit2->type].visibility)) &&
		    (could_capture(unit2->type, u))) {
		    chance += utypes[u].surrender;
		    eunit = unit2;
		} else {
		    surrounded = FALSE;
		}
	    } else {
		view = side_view(us, x, y);
		if (view == EMPTY || view == UNSEEN) {
		    surrounded = FALSE;
		} else {
		    es = side_n(vside(view));
		    if (enemy_side(us, es) && could_capture(vtype(view), u)) {
			chance += utypes[u].surrender;
			if (unit_at(x, y)) eunit = unit_at(x, y);
		    }
		}
	    }
	}
	if (surrounded && eunit) chance += utypes[u].siege;
	if (RANDOM(10000) < chance) {
	    if (eunit) {
		notify(eunit->side, "%s has surrendered to you!",
		       unit_handle(eunit->side, unit));
		notify(us, "%s has surrendered to the %s!",
		       unit_handle(us, unit), 
			( eunit->side == 0 ? "Neutrals" : 
			plural_form(eunit->side->name)) );
		capture_unit(eunit, unit);
	    }
	}
    }
    exit_procedure();
}

/* Accidents will happen!... Kill off any occupants of course. */

unit_accident(unit)
Unit *unit;
{
    int u = unit->type;
    Side *us = unit->side;

    enter_procedure("unit_accident");
    if (RANDOM(10000) < utypes[u].accident[terrain_at(unit->x, unit->y)]) {
	if (strlen(utypes[u].accidentmsg) > 0)
	    notify(us, "%s %s!", unit_handle(us, unit), utypes[u].accidentmsg);
	kill_unit(unit, DISASTER);
    }
    exit_procedure();
}

/* Attrition only takes out a few hp at a time, but can be deadly... */
/* Occupants lost only if unit lost (do we care?) */

unit_attrition(unit)
Unit *unit;
{
    int u = unit->type;
    Side *us = unit->side;

    enter_procedure("unit_attrition");
    if (RANDOM(10000) < utypes[u].attrition[terrain_at(unit->x, unit->y)]) {
	if (unit->hp <= utypes[u].attdamage) {
	    notify(us, "%s dies from attrition!", unit_handle(us, unit));
	    kill_unit(unit, DISASTER);
	} else {
	    if (strlen(utypes[u].attritionmsg) > 0) {
		notify(us, "%s %s!",
		       unit_handle(us, unit), utypes[u].attritionmsg);
	    }
	    unit->hp -= utypes[u].attdamage;
	}
    }
    exit_procedure();
}

/* Repair always proceds each turn, as long as the repairing unit is not */
/* crippled.  A unit can repair itself, its transport, and its occupants. */

/* Another activity of units is to build other units.  The code here has to */
/* be a little tricky because players need to get opportunities to cancel */
/* and to idle units, plus not get asked about units that don't usually */
/* produce things. */

/* The order of repair vs building means scarce supplies used on existing */
/* units first. */

/*** (HW) insert -> ***/
int buildphase=FALSE;
/*** <- insert ***/

build_phase()
{
    Unit *unit, *occ;
    Side *side;

    routine("build_phase");
    enter_procedure("build_phase");
    if (Debug) printf("Entering build phase\n");
/*** (HW) insert -> ***/
    buildphase=TRUE;
/*** <- insert ***/
    for_all_units(side, unit) { 
      if (alive(unit) && !cripple(unit)) {
	repair_unit(unit, unit);
	for_all_occupants(unit, occ) repair_unit(unit, occ);
	if (unit->transport != NULL) repair_unit(unit, unit->transport);
	work_on_new_unit(unit);
	work_on_terraform(unit);
      }
    }
/*** (HW) insert -> ***/
    buildphase=FALSE;
/*** <- insert ***/
    exit_procedure();
}

/* One arbitrary unit repairs another only if actually needed.  Rate is */
/* always less than 1 hp/turn; this is a practical limit on max hp values */
/* if reasonably rapid repair is desired.  If unit being repaired was badly */
/* damaged (crippled), then we'll use up same resources as needed for */
/* construction, and if a resource type is missing, then repairs will not */
/* proced at all. */

repair_unit(unit, unit2)
     Unit *unit, *unit2;
{
  int u = unit->type, u2 = unit2->type, r;
  
  enter_procedure("repair_unit");
  if ((unit2->hp < utypes[u2].hp) && could_repair(u, u2)) {
    int rpchance = utypes[u].repair[u2], count, hprp = 0;

    for (count = 0; count < period.repairscale; count++) {
      if (rpchance == 1 || RANDOM(rpchance) == 0) {
	if (cripple(unit2)) {
	  for_all_resource_types(r) {
	    if (unit->supply[r] < utypes[u2].tomake[r]) {
	      if (Debug)
		printf("Unit not repaired for lack of supplies");
	      break;
	    }
	  }
	  /* Will underestimate because of rounding down. */
	  for_all_resource_types(r) {
	    unit->supply[r] -= utypes[u2].tomake[r] / utypes[u2].hp;
	  }
	}
	hprp++;
      }
    }
    if (Debug)
      printf("unit %d repaired %d. chance was 1 in %d. %d/%d\n",
	     u, u2, utypes[u].repair[u2], hprp, period.repairscale);
    unit2->hp += hprp;
    if (unit2->hp > utypes[u2].hp)
      unit2->hp = utypes[u2].hp;
  }
  exit_procedure();
}

/*** (UK) insert -> ***/
work_on_terraform(unit)
Unit *unit;
{
    int u = unit->type, r, mk, rmk, use;

    enter_procedure("work_on_terraform");
    if (!neutral(unit)) {
	if (terraforming(unit)) {
	    unit->tschedule--;
	    if (unit->tschedule <= 0) {
	        int  terrain = unit->terraform;
                set_terraform(unit, TNOTHING);
		notify(unit->side, "%s finished terraforming!", unit_handle(unit->side, unit));
                set_terrain_at(unit->x, unit->y, terrain);
                unit->last_terraform = terrain;
	    }
	}
    }
    exit_procedure();
}
/*** <- insert ***/

/* Machine players need opportunities to change their production. */
/* Neutrals never produce (what could they do with the results?). */
/* Construction may be constrained by lack of resources, so don't count */
/* down on schedule or use up building materials unless we actually have */
/* enough. */

work_on_new_unit(unit)
Unit *unit;
{
    int u = unit->type, r, mk, rmk, use;

    enter_procedure("work_on_new_unit");
    if (!neutral(unit)) {
	if (producing(unit)) {
	    /*  protection against div by zero */
	    mk = max(utypes[u].make[unit->product], 1); 
	    for_all_resource_types(r) {
		if ((rmk = utypes[unit->product].tomake[r]) > 0) {
		    use = (rmk / mk) + (unit->schedule <= (rmk % mk) ? 1 : 0);
		    if (unit->supply[r] < use) return;
		    unit->supply[r] -= use;
		}
	    }
	    unit->schedule--;
	    if (unit->schedule <= 0) {
	        int  product = unit->product;

		/* this monkey business is necessary to prevent an
		   occupying builder from giving a warning message */
		if (!utypes[u].maker)
		  set_product(unit, NOTHING);

		if (complete_new_unit(unit, product)) {
		    if (utypes[u].maker) {
		        if (unit->next_product != unit->product)
			  set_product(unit, unit->next_product);
			set_schedule(unit);
		    }
		} else {
		    if (!utypes[u].maker) /* oops, we zeroed the product */
		        set_product(unit, product);
/*** (UK) change -> ***/
		    unit->schedule=1; /* delay completion */
/*** was:
		    unit->schedule++;
*** <- change ***/
		}
	    }
	} else {
	    if (unit->schedule > 0) unit->schedule--;
	}
    }
    exit_procedure();
}

/* When one unit produces another, it may be that one can occupy the other, */
/* in either direction.  So we have to make sure that both ways work. */
/* Also the builder may become the garrison, so we have to get it out of */
/* the way before deciding about fiddling around. */
/* Morale of new unit is slightly more than morale of building unit. */
/* Returns success of the whole process. */

complete_new_unit(unit, type)
Unit *unit;
int type;
{
    bool success = FALSE;
    int ux = unit->x, uy = unit->y, u = unit->type;
    Unit *mainunit, *newunit;
    Side *us = unit->side;

    enter_procedure("complete_new_unit");
    if (will_garrison(unit->type, type)) leave_hex(unit);
    if ((newunit = create_unit(type, (char *) NULL)) != NULL) {
/*** (UK) insert -> ***/
	if (unit->group) newunit->group=copy_string(unit->group);
/*** <- insert ***/
	mainunit = unit_at(ux, uy);
	if (mainunit == NULL) {
	    if (could_occupy(type, terrain_at(ux, uy))) {
		assign_unit_to_side(newunit, us);
		occupy_hex(newunit, ux, uy);
		success = TRUE;
	    } else {
		notify(us, "%s can't go here!", unit_handle(us, newunit));
		kill_unit(newunit, DISASTER);
	    }
	}
#ifndef NO_OCCMAKE
	/* when the produced unit can be produced inside the maker, do so,
	** regardless of what the 'mainunit' can carry. */
	else if ( (utypes[unit->type].occproduce || utypes[unit->type].maker)
		&& could_carry(unit->type, type) && can_carry(unit, newunit)){
	    /* force new unit to be produced in maker... */
	    assign_unit_to_side(newunit, us);
	    occupy_unit(newunit, unit);
	    success = TRUE;
	}
#endif
        else if (could_carry(mainunit->type, type)) {
	    if (can_carry(mainunit, newunit)) {
		assign_unit_to_side(newunit, us);
		occupy_hex(newunit, ux, uy);
		success = TRUE;
	    } else {
		notify(us, "%s will delay completion of %s until it has room.",
		       unit_handle(us, unit), utypes[type].name);
		kill_unit(newunit, DISASTER);
	    }
	} else if (could_carry(type, mainunit->type)) {
	    if (could_occupy(type, terrain_at(ux, uy))) {
		assign_unit_to_side(newunit, us);
		/* Redundant code.  Eliminates messages complaining */
		/* about units building as occupants. */
		if (!utypes[u].maker) {
		  set_product(mainunit, NOTHING);
		} else {
		  set_schedule(mainunit);
		}
		occupy_hex(newunit, ux, uy);
		success = TRUE;
	    } else {
		notify(us, "%s can't go here!", unit_handle(us, newunit));
		kill_unit(newunit, DISASTER);
	    }
	} else {
	    notify(us, "Idiots! - %s can't build a %s while in a %s!",
		   unit_handle(us, unit),
		   utypes[type].name, utypes[mainunit->type].name);
	    kill_unit(newunit, DISASTER);
	}
    }
    if (will_garrison(unit->type, type)) {
      unit->prevx = -1;
      occupy_hex(unit, ux, uy);
    }
    if (success) {
	if (utypes[type].named) newunit->name = make_unit_name(newunit);
/*	newunit->morale = min(newunit->morale, unit->morale + 1); */
	unit->built++;
	(us->balance[type][PRODUCED])++;
	(us->units[type])++;
	if (will_garrison(unit->type, type)) {
	    Unit	*occ;
	    extern char killbuf[]; /* from attack.c */
	    extern int occdeath[]; /* from unit.c */

	    /* Put the new unit exactly in place of its garrison. */
	    if (newunit->transport == unit) {
		/* Get the garrisoning unit out of the way. */
		leave_hex(unit);
		if (unit->transport == NULL) {
		    occupy_hex(newunit, ux, uy);
		} else {
		    occupy_unit(newunit, unit->transport);
		}
	    }

	    do { /* this screwy loop handles the fact that the linked
		    list is being changed while we're scanning it.
		    The real solution would be to write a special loop,
		    but I'm lazy */
	        for_all_occupants(unit, occ) {
		    if (can_carry(newunit, occ)) {
		        leave_hex(occ);
		        occupy_unit(occ, newunit);
			break;
		    }
		}
	    } while (occ!=NULL && can_carry(newunit, occ));

	    /* Now we can kill the garrison and its other occupants. */
	    /* (or should occupants be put into the new unit?) */
	    kill_unit(unit, GARRISON);
	    summarize_units(killbuf, occdeath);
	    if (*killbuf)
	      notify(unit->side, "%s destroyed during garrison operation",
		     killbuf);
	}
	update_state(us, type);
	if (!humanside(unit->side) && change_machine_product(unit)) 
	  request_new_product(unit);
	if (Debug)
	  printf("%s is completed\n", unit_handle((Side *)NULL, newunit));
    }
    exit_procedure();
    return success;
}

/* The main routine does production, distribution, and discarding in order. */

supply_phase()
{
    int u, r, t, amt, dist, x, y, x1, y1, x2, y2;
    Unit *unit, *ounit;
    Side *loop_side;

    routine("supply_phase");
    enter_procedure("supply_phase");
    if (Debug) printf("Entering supply phase\n");
    /* Make new resources but don't clip to storage capacity yet */
    for_all_units(loop_side, unit) {
	u = unit->type;
	for_all_resource_types(r) {
	    t = terrain_at(unit->x, unit->y);
	    amt = (utypes[u].produce[r] * utypes[u].productivity[t]) / 100 +
	      (probability((utypes[u].produce[r] * 
			    utypes[u].productivity[t]) % 100) ? 1 : 0);
	    if (cripple(unit))
		amt = (amt * unit->hp) / (utypes[u].crippled + 1);
	    unit->supply[r] += amt;
	}
    }
    /* Move stuff around - hopefully will get rid of any excess */
    for_all_units(loop_side, unit)
      if (!neutral(unit)) {
        u = unit->type;
	for_all_resource_types(r) {
            dist = utypes[u].outlength[r];
	    y1 = unit->y - dist;
	    y2 = unit->y + dist;
	    for (y = y1; y <= y2; ++y) {
		if (between(0, y, world.height-1)) {
		    x1 = unit->x - (y < unit->y ? (y - y1) : dist);
		    x2 = unit->x + (y > unit->y ? (y2 - y) : dist);
		    for (x = x1; x <= x2; ++x) {
			ounit = unit_at(wrap(x), y);
			if (ounit != NULL && alive(ounit) &&
			    ounit->side == unit->side) {
			  try_transfer(unit, ounit, r);
			}
		    }
		}
	    }
	}
    }
    /* Throw away any excess */
    for_all_units(loop_side, unit) {
	u = unit->type;
	for_all_resource_types(r) {
	    unit->supply[r] = min(unit->supply[r], utypes[u].storage[r]);
	}
    }
    exit_procedure();
}

/* Give away supplies, but save enough to stay alive for a couple turns. */
try_transfer(from, to, r)
Unit *from, *to;
int r;
{
  int save_amount;  /* Hang on to this much at least. */
  int u = from->type;
  
  save_amount = max(utypes[u].consume[r] * 3,
		    2 * utypes[u].speed * utypes[u].tomove[r]);
  save_amount = min(save_amount, from->supply[r]);
  from->supply[r] -= save_amount;
  try_transfer_auxR(from, to, r);
  from->supply[r] += save_amount;
}

/* The middle subphase of supply uses this routine to move supplies around */
/* between units far apart or on the same hex. Try to do reasonable */
/* things with the resources.  Producers are much more willing to */
/* give away supplies that consumers. */

#if 1

try_transfer_auxR(from, to, r)
Unit *from, *to;
int r;
{
  Unit	*occ;

  try_transfer_aux(from, to, r);

  for_all_occupants(to, occ)
    try_transfer_auxR(from, occ, r);
}

/* the Carrier factor */
/*** (UK) change -> ***/
#define CFACTOR 2
/*** was:
#define CFACTOR 4
*** <- change ***/

try_transfer_aux(from, to, r)
Unit *from, *to;
int r;
{
  struct utype	*ftp = &utypes[from->type], *ttp = &utypes[to->type];
  int	fprate, tprate; /* production rate */
  int	xfr;

  /*
  if (side_number(from->side)==0)
  printf("*try transfer from %s to %s (%d,%d) d=%d\n",
	    unit_handle(from->side,from), unit_handle(to->side,to), ol, il,
	    distance(from->x, from->y, to->x, to->y));
  */
  if (from == to || to->supply[r] >= ttp->storage[r] ||
      utypes[to->type].inlength[r] < distance(from->x, from->y, to->x, to->y))
    return;

  fprate = ftp->produce[r]*ftp->productivity[terrain_at(from->x, from->y)]/100
    - ftp->consume[r];
  tprate = ttp->produce[r]*ttp->productivity[terrain_at(to->x, to->y)]/100
    - ttp->consume[r];

  /*
  if (side_number(from->side)==0)
  printf(" try transfer from %s to %s (%d,%d)\n",unit_handle(from->side,from),
	    unit_handle(to->side,to), fprate, tprate);
  */
  if (fprate>0) {
    if (tprate>0) {
      /* both are producers */
      float	ttf; /* time to full */

      if (ttp->storage[r]*CFACTOR < from->supply[r]) { /* a carrier */
	xfr = ttp->storage[r] - to->supply[r];
	if (can_satisfy_need(from, r, xfr)) {
	  transfer_supply(from, to, r, xfr);
	  return;
	}
      }

      ttf = ( (ftp->storage[r] + ttp->storage[r]) -
	      (from->supply[r] + to->supply[r])) / (fprate + tprate);
      xfr = ttp->storage[r] - tprate*ttf - to->supply[r];
      if (xfr<=0)
	return;
      if (can_satisfy_need(from, r, xfr)) {
	transfer_supply(from, to, r, xfr);
	return;
      }

      xfr = from->supply[r] - ftp->storage[r]/2;
      if (xfr>0 && can_satisfy_need(from, r, xfr)) {
	transfer_supply(from, to, r, xfr);
	return;
      }

      xfr = from->supply[r] - ftp->storage[r];
      if (xfr > 0) {
	transfer_supply(from, to, r, xfr);
	return;
      }
    } else {
      /* source is a producer, dest is a consumer */
      xfr = ttp->storage[r] - to->supply[r];
      if ((from->supply[r]-xfr)*2 > ftp->storage[r] &&
	  can_satisfy_need(from, r, xfr)) {
	transfer_supply(from, to, r, xfr);
	return;
      }

      xfr = from->supply[r]- ftp->storage[r]/2;
      if (xfr>0 &&
	  can_satisfy_need(from, r, xfr)) {
	transfer_supply(from, to, r, xfr);
	return;
      }

      xfr = (ttp->speed * ttp->tomove[r] + ttp->consume[r]) * 3 -
	to->supply[r];
      if (xfr>0 && can_satisfy_need(from, r, xfr)) {
	transfer_supply(from, to, r, xfr);
	return;
      }
    }
  } else if (fprate==0) {
    /* source is a transmitter */
    if (tprate>0) /* dest is a producer */
      return; /* sod off */
    else if (tprate<0) { /* dest is a consumer */
      xfr = min(from->supply[r], ttp->storage[r] - to->supply[r]);
      if (xfr>0)
	transfer_supply(from, to, r, xfr); /* fill up the dest */
    } else { /* dest is transmitter too */
      if (ttp->storage[r]*CFACTOR < ftp->storage[r]) {
	/* source is a carrier, fill up the smaller one */
	xfr = min(from->supply[r], ttp->storage[r] - to->supply[r]);
	if (xfr>0)
	  transfer_supply(from, to, r, xfr); /* fill up the dest */
      } /* else don't transfer */
    }
  } else {
    /* source is a consumer */
    if (tprate>0) /* dest is a producer */
      return; /* sod off */
    else if (tprate<0) { /* dest is a consumer */
      if (ttp->storage[r]*CFACTOR < ftp->storage[r]) {
	/* source is a carrier, fill up the smaller one */
	xfr = min(from->supply[r], ttp->storage[r] - to->supply[r]);
	if (xfr>0)
	  transfer_supply(from, to, r, xfr); /* fill up the dest */
      } /* else don't transfer */
    } /* else dest is a transmitter, sod off */
  }
  return;
}

#else

try_transfer_aux(from, to, r)
Unit *from, *to;
int r;
{
    int nd, ut = from->type, rate;
    Unit *occ;

    if (from != to && utypes[to->type].inlength[r] >=
                      distance(from->x, from->y, to->x, to->y)) {
	if ((nd = utypes[to->type].storage[r] - to->supply[r]) > 0) {
	  if ((utypes[ut].produce[r] - utypes[ut].consume[r] > 0) ||
	      survival_time(to) < 3 ||
	      (utypes[ut].storage[r] * 4 >=
	       utypes[to->type].storage[r])) {
	    if (can_satisfy_need(from, r, nd)) {
	      transfer_supply(from, to, r, nd);
	    } else if (can_satisfy_need(from, r, max(1, nd/2))) {
	      transfer_supply(from, to, r, max(1, nd/2));
	    } else if (from->supply[r] > utypes[ut].storage[r]) {
	      transfer_supply(from, to, r,
			      (from->supply[r] - utypes[ut].storage[r]));
	    }
	  } else if ((from->supply[r] /
		      (((rate = utypes[ut].speed * utypes[ut].tomove[r] * 3)
			> 0) ? rate : 1)) > 
		     (to->supply[r] /
		      (((rate = utypes[to->type].speed *
			 utypes[to->type].tomove[r] * 3) > 0) ? rate : 1))) {
	    transfer_supply(from, to, r, min(nd, (8 + from->supply[r]) / 9));
	  } 
	}

	  
    }
    for_all_occupants(to, occ)
      try_transfer_aux(from, occ, r);
}

#endif

/* This estimates what can be spared.  Note that total transfer of requested */
/* amount is not a good idea, since the supplies might be essential to the */
/* unit that has them first.  If we're more than half full, or the request */
/* is less than 1/5 of our supply, then we can spare it. */

can_satisfy_need(unit, r, need)
Unit *unit;
int r, need;
{
    return ((((2 * unit->supply[r]) > (utypes[unit->type].storage[r])) &&
	     (((unit->supply[r] * 4) / 5) > need)) ||
	    (need < (utypes[unit->type].storage[r] / 5)));
}

/* Move supply from one unit to another.  Don't move more than is possible; */
/* check both from and to amounts and capacities. */

transfer_supply(from, to, r, amount)
Unit *from, *to;
int r, amount;
{
    amount = min(amount, from->supply[r]);
    amount = min(amount, utypes[to->type].storage[r] - to->supply[r]);
    from->supply[r] -= amount;
    to->supply[r] += amount;
    if (Debug) {
      printf("%s receives %d %s ", unit_handle((Side *) NULL, to),
	     amount, rtypes[r].name);
      printf("from %s\n", unit_handle((Side *) NULL, from));
    }
    return amount;
}

/* Handle constant consumption, which is a post process to movement. */
/* Don't care about current side in this phase.  This is also a handy */
/* place to count up all of a side's resources, for use by win/lose tests. */

consumption_phase()
{
    int r;
    Unit *unit;
    Side *side;

    routine("consumption_phase");
    enter_procedure("consumption_phase");
    if (Debug) printf("Entering consumption phase\n");
    for_all_sides(side) {
	for_all_resource_types(r) {
	    side->resources[r] = 0;
	}
    }
    for_all_units(side, unit)
      if (alive(unit)) unit_consumes(unit);
    exit_procedure();
}

/* Consume the constant overhead part of supply consumption. */
/* Usage by movement is subtracted from overhead first. */
/* Finally, credit side with the unit's remaining supplies. */

unit_consumes(unit)
Unit *unit;
{
    int u = unit->type, r, usedup;
    
    for_all_resource_types(r) {
	if (utypes[u].consume[r] > 0  && (unit->transport == NULL ||
					  utypes[u].consume_as_occupant)) {
	    usedup = unit->actualmoves * utypes[u].tomove[r];
	    if (usedup < utypes[u].consume[r])
		unit->supply[r] -= (utypes[u].consume[r] - usedup);
	    if (unit->supply[r] <= 0 && !in_supply(unit)) {
		exhaust_supply(unit);
		return;
	    }
	}
	if (!neutral(unit)) unit->side->resources[r] += unit->supply[r];
    }
}

/* What happens to a unit that runs out of a supply.  If it can survive */
/* on no supplies, then there may be a few turns of grace, depending on */
/* how the dice roll... */

exhaust_supply(unit)
Unit *unit;
{
    if (probability(100 - utypes[unit->type].survival)) {
	notify(unit->side, "%s %s!",
	       unit_handle(unit->side, unit), utypes[unit->type].starvemsg);
	kill_unit(unit, STARVATION);
    }
}

/* Check if the unit has ready access to a source of supplies. */

in_supply(unit)
Unit *unit;
{
    if (unit->transport != NULL) return TRUE;
    /* needs to be more sophisticated and account for supply lines */
    return FALSE;
}
