/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file implements auxilary functions for the machine players. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"
#include "mplay.h"



Mplan *freeplans = NULL; /* Will be initialize to plan list first time */
			 /* plan is created. */
int route_max_distance;


/* Short unreadable but greppable listing of unit. */

char *unit_desig(unit)
Unit *unit;
{
  enter_procedure("unit_desig");
    sprintf(shortbuf, "s%d %d %c (%d,%d)",
	    side_number(unit->side), unit->number, utypes[unit->type].uchar,
	    unit->x, unit->y);
  exit_procedure();
    return shortbuf;
}

/* Count how many units on our side of the given type are within the */
/* specified distance. */

int units_nearby(x, y, dist, type)
int x, y;
{
  Unit *unit;
  int ux, uy;

/*  enter_procedure("units nearby"); */
  unit_count = 0;
  for (unit = mside->unitlist[type]; unit != NULL; unit = unit->mlist) {
    ux = unit->x;
    uy = unit->y;
    if (alive(unit) && distance(x, y, ux, uy) <= dist)
      unit_count++;
  }
/*  exit_procedure(); */
  return unit_count;
}

/* Can this unit build a base without dying. */

bool survive_to_build_base(unit)
Unit *unit;
{
  return (base_builder(unit) &&
	  survival_time(unit) > build_time(unit, base_builder(unit)));
}

/* Is this the last chance for a unit to build a base without dying. */

bool exact_survive_to_build_base(unit)
Unit *unit;
{
  return (base_builder(unit) &&
	  survival_time(unit) == (1 + build_time(unit, base_builder(unit))));
}

/* Is there a machine base here. */

base_here(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);

    return (unit != NULL && unit->side == mside && isbase(unit)) ;

}

/* Is there anybodies base here. */

any_base_here(x, y)
int x, y;
{
    int utype = vtype(side_view(mside, x, y));

    return (utype != EMPTY && utype != UNSEEN &&
	    utypes[utype].is_base);

}

/* Is there anybodies base here. */

neutral_base_here(x, y)
int x, y;
{
    viewdata view = side_view(mside, x, y);
    int utype = vtype(view);

    return (utype != EMPTY && utype != UNSEEN &&
	    utypes[utype].is_base && vside_neutral(view));

}

/* Is there a base within the given range.  Generally range is small. */

bool base_nearby(unit,range)
Unit *unit;
int range;
{
  int x,y;
  
  mside = unit->side;
  return search_area(unit->x, unit->y, range, base_here, &x, &y, 1);
}

/* Is there a base within the given range.  Generally range is small. */

bool any_base_nearby(unit,range)
Unit *unit;
int range;
{
  int x,y;
  
  mside = unit->side;
  return search_area(unit->x, unit->y, range, any_base_here, &x, &y, 1);
}

/* Is there a neutral base within the given range.  Generally range is small. */

bool neutral_base_nearby(unit,range)
Unit *unit;
int range;
{
  int x,y;
  
  mside = unit->side;
  return search_area(unit->x, unit->y, range, neutral_base_here, &x, &y, 1);
}


bool occupant_could_capture(unit,etype)
Unit *unit;
int etype;
{
  Unit *occ;

  for_all_occupants(unit,occ)
    if could_capture(occ->type, etype)
      return TRUE;
  return FALSE;
}

/* Check to see if there is anyone around to capture. */

bool can_capture_neighbor(unit)
Unit *unit;
{
  int d, x, y;
  viewdata view;
  Side *side2;

  for_all_directions(d) {
    x = wrap(unit->x + dirx[d]);  y = limit(unit->y + diry[d]);
    view = side_view(unit->side, x, y);
    if (view != UNSEEN && view != EMPTY) {
      side2 = side_n(vside(view));
      if (!allied_side(unit->side, side2)) {
	if (could_capture(unit->type, vtype(view))) {
	  mside->movunit = unit;
	  mside->last_unit = unit;
	  return TRUE;
	}
      }
    }
  }
  return FALSE;
}

/* check if our first occupant can capture something.  Doesn't look at
other occupants. */

bool occupant_can_capture_neighbor(unit)
Unit *unit;
{
  Unit *occ = unit->occupant;

  if (occ != NULL && occ->movesleft > 0 && occ->side == unit->side) {
    if (can_capture_neighbor(occ)) {
      mside->movunit = occ;
      mside->last_unit = occ;
      return TRUE;
    }
  }
  return FALSE;
}

/* Find the closes unit, first prefering bases, and then transports. */

bool find_closest_unit(x0, y0, maxdist, pred, rxp, ryp)
int x0, y0, maxdist, (*pred)(), *rxp, *ryp;
{
    Unit *unit;
    int ut, dist;
    bool found = FALSE;
    
    enter_procedure("find_closest_unit");
    for_all_unit_types(ut) {
      if (utypes[ut].is_base)
	for (unit = mside->unitlist[ut]; unit != NULL; unit = unit->mlist) {
	  if (alive(unit) && (dist = distance(x0, y0, unit->x, unit->y)) <= maxdist) {
	    if ((*pred)(unit->x, unit->y)) {
	      maxdist = dist - 1;
	      found = TRUE;
	      *rxp = unit->x;  *ryp = unit->y;
	    }
	  }
	}
    }
    if (found) {
      exit_procedure();
      return TRUE;
    }
    for_all_unit_types(ut) {
      if (!utypes[ut].is_base && utypes[ut].is_transport)
	for (unit = mside->unitlist[ut]; unit != NULL; unit = unit->mlist) {
	  if (alive(unit) && distance(x0, y0, unit->x, unit->y) <= maxdist) {
	    if ((*pred)(unit->x, unit->y)) {
	      *rxp = unit->x;  *ryp = unit->y;
	      maxdist = dist - 1;
	      found = TRUE;
	    }
	  }
	}
    }
    if (found) {
      exit_procedure();
      return TRUE;
    }
    for_all_unit_types(ut) {
      if (!utypes[ut].is_base && !utypes[ut].is_transport)
	for (unit = mside->unitlist[ut]; unit != NULL; unit = unit->mlist) {
	  if (alive(unit) && distance(x0, y0, unit->x, unit->y) <= maxdist) {
	    if ((*pred)(unit->x, unit->y)) {
	      *rxp = unit->x;  *ryp = unit->y;
	      maxdist = dist - 1;
	      found = TRUE;
	    }
	  }
	}
    }
    if (found) {
      exit_procedure();
      return TRUE;
    }
    exit_procedure();
    return FALSE;
}


/* Return percentage of capacity. */

int fullness(unit)
Unit *unit;
{
    int u = unit->type, o, cap = 0, num = 0, vol = 0;
    Unit *occ;

    for_all_unit_types(o) cap += utypes[u].capacity[o];
    for_all_occupants(unit, occ) {
	num++;
	vol += utypes[occ->type].volume;
    }
    return (cap > 0) ? max(((utypes[u].holdvolume > 0) ? (100 * vol) / utypes[u].holdvolume : 100),
	        (100 * num) / cap) : 100;

}

/* True if the given unit is a sort that can build other units. */

bool can_produce(unit)
Unit *unit;
{
    int p;

    for_all_unit_types(p) {
	if (could_make(unit->type, p)) return TRUE;
    }
    return FALSE;
}

/* Test if unit can move out into adjacent hexes. */

can_move(unit)
Unit *unit;
{
    int d, x, y;

    for_all_directions(d) {
	x = wrap(unit->x + dirx[d]);  y = limit(unit->y + diry[d]);
	if (could_move(unit->type, terrain_at(x, y))) return TRUE;
    }
    return FALSE;
}

/* Returns the type of missing supplies. Not great routine if first */
/* resource is a type of ammo. */

out_of_ammo(unit)
Unit *unit;
{
    int u = unit->type, r;

    for_all_resource_types(r) {
	if (utypes[u].hitswith[r] > 0 && unit->supply[r] <= 0)
	    return r;
    }
    return (-1);
}

/* Someplace that we can definitely get supplies at. */

good_haven_p(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);
    int r;
    Mplan *plan;

    if (unit != NULL) {
      if (allied_side(mside, unit->side) && alive(unit) &&
	  can_carry(unit, munit) && !might_be_captured(unit)) {
	for_all_resource_types(r)
	  /* could also add in distance calculation to see how much we */
	  /* really need. */
	  if (unit->supply[r] <
	      (utypes[munit->type].storage[r])) {
	    return FALSE;
	  }
	if ((plan = find_route(munit->type, route_max_distance,
			       munit->x, munit->y, x, y)) != NULL) {
	  free_plan(munit->plan);
	  munit->plan = plan;
	  return TRUE;
	}
      }
    } 
    return FALSE;
}

/* See if the location has a unit that can take us in for refueling */
/* (where's the check for refueling ability?) */

haven_p(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);
    Mplan *plan;

    if (unit != NULL &&
	allied_side(mside, unit->side) && alive(unit) &&
	can_carry(unit, munit) && !might_be_captured(unit) &&
	(plan = find_route(munit->type, route_max_distance,
			   munit->x, munit->y, x, y)) != NULL) {
          free_plan(munit->plan);
	  munit->plan = plan;
	  return TRUE;
	}
    else return FALSE;
}

/* See if the location has a unit that can repair us */

shop_p(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);
    Mplan *plan;

   if (unit != NULL && allied_side(munit->side, unit->side) && alive(unit) &&
       can_carry(unit, munit) && could_repair(unit->type, munit->type) &&
       !might_be_captured(unit) &&
       (plan = find_route(munit->type, route_max_distance,
			  munit->x, munit->y, x, y)) != NULL) {
          free_plan(munit->plan);
	  munit->plan = plan;
	  return TRUE;
	}
    else return FALSE;
}

/* Check how long a unit can sit where it is */

int survival_time(unit)
Unit *unit;
{
    int u = unit->type, r, least = 12345, rate;
    int t = terrain_at(unit->x, unit->y);


    for_all_resource_types(r) {
      rate = (utypes[u].consume[r] -
		   (utypes[u].produce[r] * utypes[u].productivity[t]) / 100);
      if (rate > 0)
	least = min(least, (unit->supply[r] + unit->actualmoves * utypes[u].tomove[r]) / rate);
    }
    return (least);
}

#ifdef REGIONS
long regions_around(u, x, y, center)
int u, x, y;
bool center;
{
  int d, nx, ny;
  long result = 0;

  for_all_directions(d) {
    nx = wrap(x + dirx[d]);  ny = y + diry[d];
    if (aref(unit_region[u], nx, ny) >= 0)
      result |= 1 << (aref(unit_region[u], nx, ny) % sizeof(long));
  }
  if (center && aref(unit_region[u], x, y) > 0)
    result |= 1 << (aref(unit_region[u], x, y) % sizeof(long));
  return result;
}
#endif

void init_plans()
{
    int i;
    Mplan *plans;

    plans = (Mplan *) malloc(INITMAXUNITS * sizeof(Mplan));
    freeplans = plans;
    for (i = 0; i < INITMAXUNITS; ++i) {
      plans[i].next = &plans[i+1];
    }
    plans[INITMAXUNITS-1].next = NULL;
}

void free_plan(plan)
Mplan *plan;
{
  Mplan *next;

  while (plan != NULL) {
    next = plan->next;
    plan->next = freeplans;
    freeplans = plan;
    plan = next;
  }
}

Mplan *make_plan_step(type, x, y, priority)
int type, x, y, priority;
{
  Mplan *plan;
  
  if (freeplans == NULL) init_plans();
  plan = freeplans;
  freeplans = plan->next;
  plan->next = NULL;
  plan->type = type;
  plan->x = x;
  plan->y = y;
  return plan;
}
  
/* should be using A* rather than this I think.? */

unsigned char mark = 255;
int counter = 0;
Mplan *find_route_aux(ut, maxdist, curdist, fromdir, sx, sy, fx, fy, tx, ty, flags)
int ut, fx, fy, tx, ty, flags;
{
  int d, i, x, y, td;
  viewdata view;
  int baddirs = (fromdir<0) ? 0 : ((1 << fromdir) |
				   (1 << ((fromdir + 1) % 6)) |
				   (1 << ((fromdir - 1 + 6) % 6)));
  int distleft = distance(fx, fy, tx, ty);
  Mplan *resulting_plan;

  if (!(flags & SAMEPATH)) {
    baddirs = 0;
    mark++;
    if (mark == 0) {
      for_all_hexes(x,y)
	markloc(x,y);
      mark++;
    }
  }
  flags |= SAMEPATH; /* add samepath flag */
  markloc(fx, fy);
  set_fromdir(fx, fy, fromdir);
  if (curdist + distleft > maxdist) {
    return NULL;
  }
  set_dist(fx, fy, curdist);
  curdist++;
  if (fx == tx && fy == ty)
    return make_plan_for_route(sx, sy, tx, ty);
  
  d = fromdir;
  do {
    d = (d + 1) % 6;
    x = wrap(fx + dirx[d]);
    y = fy + diry[d];
  } while (distance(x, y, tx, ty) >= distleft);

  td = (d + 5) % 6;
  if (distance(wrap(fx + dirx[td]), fy + diry[td], tx, ty) < distleft) 
    d = td;
  i = 0;

  x = wrap(fx + dirx[d]);
  y = fy + diry[d];
  if ((flags & EXPLORE_PATH) && UNSEEN == side_view(mside, x, y)) {
    return make_plan_for_route(sx, sy, fx, fy);
  }

  while (i <= 3) {
    x = wrap(fx + dirx[td = (d + i + 6) % 6]);
    y = fy + diry[td];
    view = side_view(mside, x, y);
    if ( (x == tx && y == ty) ||
	(between(1, y, world.height-2) &&
	 (!markedloc(x, y)  /*||
			      get_dist(x, y) > curdist */) &&
	 ((view == EMPTY) || view == UNSEEN ||
	  (allied_side(side_n(vside(view)),mside) &&
	   could_carry(vtype(view), ut))) &&
	 (baddirs & (1 << td)) == 0 &&
	 could_move(ut, terrain_at(x, y))
	 )) {
      resulting_plan = find_route_aux(ut, maxdist, curdist,
				  opposite_dir(td),
				  sx, sy, x, y, tx, ty,
				  flags);
      if (resulting_plan != NULL)
	return resulting_plan;
    }
    x = wrap(fx + dirx[td = (d - i + 6) % 6]);
    y = fy + diry[td];
    view = side_view(mside, x, y);
    if ((x == tx && y == ty) ||
	(between(1, y, world.height-2) &&
	 between(1, i, 2) && 
	 (!markedloc(x, y) /* ||
			      get_dist(x, y) > curdist*/) &&
	 (view == EMPTY) &&
	 (baddirs & (1 << td)) == 0 &&
	 could_move(ut, terrain_at(x, y))
	 )) {
      resulting_plan = find_route_aux(ut, maxdist, curdist,
				  opposite_dir(td),
				  sx, sy, x, y, tx, ty,
				  flags);
      if (resulting_plan != NULL)
	return resulting_plan;
    }
    i++;
  }
  return NULL;
}
				 
Mplan *make_plan_for_route(sx, sy, tx, ty)
int sx, sy, tx, ty;
{
  Mplan *plan = NULL, *new;
  int x = tx, y = ty, d;

  while (x != sx || y != sy) {
    new = make_plan_step(PMOVE, x, y, bestworth);
    new->next = plan;
    plan = new;
    d = get_fromdir(x, y);
    x = wrap(x + dirx[d]);
    y = y + diry[d];
  }
  return plan;
}  

Mplan *find_route(ut, maxdist, fx, fy, tx, ty)
int ut, maxdist, fx, fy, tx, ty;
{
  int mindist = distance(fx, fy, tx, ty), i, flags;
  Mplan *route;

  if (Debug)
    notify(mside, "finding route for %s (%d %d) to (%d %d)",
	   utypes[ut].name, fx, fy, tx, ty);

  flags=0;
#ifdef REGIONS
  if ((regions_around(ut, tx, ty, TRUE) &
       regions_around(ut, fx, fy, FALSE)) == 0)
    {
    int d, x, y;
    Unit *base;
    bool basefound = FALSE;
    for_all_directions(d) {
	x = wrap(fx + dirx[d]);  y = limit(fy + diry[d]);
	if ((base = unit_at(x, y)) != NULL &&
	    could_carry(base->type, ut))
	  basefound = TRUE;
    }
    if (!basefound) {
      return NULL;
    }
  }
#endif
  if (UNSEEN == side_view(mside, tx, ty))
    flags = EXPLORE_PATH;
  
  for (i = 0; (mindist + i*i) <= maxdist; i++) {
    route = find_route_aux(ut, min(maxdist, ((i+1)*(i+1)+mindist)),
			   0, -1, fx, fy,
			   fx, fy, tx, ty, flags);
    if (route != NULL)
      return route;
  }
  return NULL;
}

/* Follow plan unless we have some reason to change.  Right now we
assume the plan will only last for one turn, so we are the only ones
likely to have seen something relavent. */

bool follow_plan(unit)
Unit *unit;
{
  int x, y;
  viewdata view;
  Side *es;
  Mplan *plan = unit->plan;
  
  if (plan == NULL) return FALSE;
/*** (UK) change -> ***/
  find_worths(seerange(unit));
/*** was:
  find_worths(utypes[unit->type].seerange);
*** <- change ***/
  x = plan->x;
  y = plan->y;
  if (bestworth > unit->priority &&
      (bestx != x || besty != y)) {
    notify(mside, "Cancelling plan, better plan found. %d over %d", bestworth,
	   unit->priority);
    free_plan(unit->plan);
    unit->plan = NULL;
    return FALSE;
  }
  if (distance(unit->x, unit->y, x, y) > 1) {
    notify(mside, "Cancelling plan, something wrong.");
/* fix this.  The other unit can move between when we give the order and the order is followed. */
    free_plan(unit->plan);
    unit->plan = NULL;
    return FALSE;
  }
  view = side_view(mside, x, y); 

  if (view == EMPTY &&
      !could_move(unit->type, terrain_at(x, y))) {
    notify(mside, "Cancelling plan, bad terrain.");
    /* fix this.  The other unit can move between when we give the order and the order is followed. */
    free_plan(unit->plan);
    unit->plan = NULL;
    return FALSE;
  }

  /* code here is not good.  Have to make sure we are really looking
     at next hex on path */

  if (view != EMPTY) { 
    es = side_n(vside(view));
    if (!allied_side(mside, es)) {
      if (plan->next != NULL) {
	/* Something is in our way and it in not our target, replan. */
	notify(mside, "Cancelling plan, path is blocked.");
	free_plan(plan);
	unit->plan = NULL;
	munit->move_tries++;
	return FALSE;
      } else if (occupant_can_capture_neighbor(unit)) {
	/* capture attempt.  Other code will make this unit hang out
	   for a turn until the occupant can do its job. */
	free_plan(plan);
	unit->plan = NULL;
	munit->move_tries++;
	return FALSE;
      }
    } else if (!(can_carry(unit_at(x, y), unit) ||
		 can_carry(unit, unit_at(x, y)))) {
      /* An allied unit is in our way. */
    Unit *other=unit_at(x,y);
    if (other == NULL) {
        printf("no unit at (%d,%d)\n", x,y);
    } else {
      notify(mside, "Can plan of %s, path blocked by ally %s at (%d,%d).",
             utypes[unit->type].name,
             utypes[unit_at(x, y)->type].name,
             x, y);
      }
      free_plan(plan);
      unit->plan = NULL;
/*    if (active_display(mside)) get_input(); */
      munit->move_tries++;
      return FALSE;
    }
  }
  order_moveto(unit, x, y);
  unit->plan = plan->next;
  plan->next = NULL;
  free_plan(plan);
  return TRUE;
}


/* Optimize a plan to account for base building and running out of
supplies. */

optimize_plan(unit)
Unit *unit;
{
  int ut = unit->type;
  Utype *utype = utypes + ut;
  int consumed[MAXRTYPES]; /* amount consumed in turn under consideration */
  int supply[MAXRTYPES];
  Mplan *step, *prevstep, *savestep;
  int r, movesleft = unit->movesleft; 

/* ways to use resources, 
   1 movement.  
   2 consumption.  
   3 building things.  
   4 repairing things.  We'll skip this at least 
   5 attacking things.  This won't matter in this routine.  */

  for_all_resource_types(r) {
    supply[r] = unit->supply[r];
    consumed[r] = unit->actualmoves * utype->tomove[r];
  }

  /* now simulate plan */
  step = unit->plan;
  prevstep = NULL;
  while (step != NULL) {
    if (movesleft <= 0) {
      for_all_resource_types(r) {
 	if (utype->consume[r] > consumed[r])
	  supply[r] -= utype->consume[r] - consumed[r];
 	consumed[r] = 0;
	supply[r] += (utype->produce[r] * utype->productivity[terrain_at(step->x, step->y)]) / 100;
      }
      movesleft = utype->speed;
    }  
  }
}

make_unitlists(side)
Side *side;
{
  Unit *unit;
  int ut, i;
  Side *loop_side;

    side->numunits = 0;
    for_all_unit_types(ut) {
      side->unitlist[ut] = NULL;
      side->unitlistcount[ut] = 0;
    }
    for_all_unit_types(ut) 
#ifdef REGIONS      
      for (i = 0; i < num_regions[ut]; i++)
	units_in_region[ut][i] = 0;
#endif
   for_all_units(loop_side, unit) {
      if (side == unit->side && !humanside(side)) {
	if (!producing(unit) && probability(10))
	  wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
      }
      if (allied_side(unit->side, side)) {
	side->numunits++;
	ut = unit->type;
	side->unitlistcount[ut]++;
	unit->mlist = side->unitlist[ut];
	side->unitlist[ut] = unit;
#ifdef REGIONS
	if (unit->side == side &&
	    (i = aref(unit_region[ut], unit->x, unit->y)) >= 0)
	  units_in_region[ut][i]++;
#endif
      }
    }
}

void update_areas(side)
Side *side;
{
  int x, y, num, ut;
  viewdata view;
  Unit *unit;
  Side *loop_side;

  for (num = 0; num < areas_wide * areas_high; num++) {
    area_info[num].allied_units = 0;
    area_info[num].allied_makers = 0;
    area_info[num].neutral_makers = 0;
    area_info[num].makers = 0;
    area_info[num].unexplored = 0;
    area_info[num].border = FALSE;
    area_info[num].allied_bases = 0;
    for_all_unit_types(ut) {
      area_info[num].units[ut] = NULL;
      area_info[num].requested_units[ut] = 0;
    }
    if (Cheat)
      side->areas[num].enemy_strength = 0;
    else side->areas[num].enemy_strength /= 2;
    side->areas[num].units_lost /= 2;
    side->areas[num].enemy_seen_recently = 0;
  }
  for_all_units (loop_side, unit) {
    num = area_index(unit->x, unit->y);
    if (allied_side(unit->side, side)) {
      area_info[num].allied_units++;
      unit->nextinarea = area_info[num].units[unit->type];
      area_info[num].units[unit->type] = unit;
      if (utypes[unit->type].maker)
	area_info[num].allied_makers++;
      if (isbase(unit))
	area_info[num].allied_bases++;
    }
    if (utypes[unit->type].maker) {
      	area_info[num].makers++;
	if (unit->side == NULL)
	  area_info[num].neutral_makers++;
      }
    if (Cheat && enemy_side(unit->side, side)) {
      side->areas[num].enemy_strength += unit_strength(unit->type);
    }
  }
  for_all_interior_hexes(x, y) {
    num = area_index(x, y);
    view = side_view(side, x, y);
    if (view == UNSEEN)
      area_info[num].unexplored++;
    else {
      if (view == EMPTY && side_view_age(side, x, y) < 4)
	view = side_prevview(side, x, y);
      if (view != EMPTY && view != UNSEEN) {
	ut = vtype(view);
	if (enemy_side(side_n(vside(view)), side))
	  side->areas[num].enemy_seen_recently += unit_strength(ut);
      }
    }
  }
  for (num = 0; num < areas_wide * areas_high; num++) {
    side->areas[num].enemy_strength +=
      side->areas[num].enemy_seen_recently;
  }
}

int unit_strength(ut)
int ut;
{
  return ((ave_build_time[ut]) ? ave_build_time[ut] : 10);
}

find_border_areas(side)
Side *side;
{
  
  int num, x, y, i;
  bool safe;

  for (num = 0; num < areas_wide * areas_high; num++) {
    if (area_info[num].allied_bases == 0 ||
	area_info[num].unexplored > 0 ||
	area_info[num].allied_makers != area_info[num].makers ||
	side->areas[num].enemy_strength > 5) {
          area_info[num].border = TRUE;
	  side->areas[num].safe_area = FALSE;
	}
    else {
      area_info[num].nearby = TRUE;
      area_info[num].border = FALSE;
      safe = area_info[num].allied_units > 15;
      for_all_directions(i) {
	x = wrap(area_info[num].x + dirx[i] * AREA_SIZE);
	y = interior(area_info[num].y + diry[i] * AREA_SIZE);
	area_info[area_index(x,y)].nearby = TRUE;
	if (area_info[area_index(x,y)].allied_bases < 2) {
	  area_info[num].border = TRUE;
	  safe = FALSE;
	}
      }
      side->areas[num].safe_area = safe;
    }
  }
}

/* Figure out how many units to request for each area. */

determine_unit_request(side)
Side *side;
{

  int num;

    for (num = 0; num < areas_wide * areas_high; num++) {
      if (side->areas[num].enemy_strength < 5) {
	if (area_info[num].allied_makers == 0 &&
	    area_info[num].makers > 0 &&
	    area_info[num].nearby) 
	  area_info[num].unit_request =
	     GUARD_BORDER_TOWN + 2 * area_info[num].makers;
	else if (area_info[num].makers > 0) 
	  area_info[num].unit_request =
	      (area_info[num].border ? GUARD_BORDER_TOWN :
	       GUARD_TOWN) + 2 * area_info[num].allied_makers;
	else if (area_info[num].allied_bases > 0)
	  area_info[num].unit_request =
	      (area_info[num].border ? GUARD_BORDER: 
	       GUARD_BASE);
	else if (area_info[num].border)
	  area_info[num].unit_request = NO_UNITS;
	else area_info[num].unit_request = NO_UNITS;
      } else  {
	if (area_info[num].allied_makers > 0) 
	  area_info[num].unit_request =
	    DEFEND_TOWN + 5 * area_info[num].makers;
	else if (area_info[num].allied_bases > 0)
	  area_info[num].unit_request =
	    DEFEND_BASE + area_info[num].allied_bases;
	else area_info[num].unit_request = DEFEND_AREA;
      }
    }
}

assign_units(side)
Side *side;
{
  int total_units, num, total_requests, i, u, priority, d;
  Unit *unit, *prev;

  total_units = 0;
  for_all_side_units(side, unit) 
    if (mobile(unit->type))
      total_units += unit_strength(unit->type);

  for (i = 0; i < NUMTOPAREAS; i++) {
    top_areas[i] = NULL;
    }
  total_requests = 0;
  for (num = 0; num < areas_wide * areas_high; num++) {
    total_requests += area_info[num].unit_request;
  }

  /* Figure out how many units to request of each type. */
  if (total_requests>0) {
    for (num = 0; num < areas_wide * areas_high; num++) {
      /*    printf("area %d (%d %d), request %d allied makers %d enemy_strength %d \n", num, area_info[num].x, area_info[num].y,
	    area_info[num].unit_request, area_info[num].allied_makers,
	    side->areas[num].enemy_strength); */
      for_all_unit_types(u) {
	if (mobile(u)) {
	  area_info[num].requested_units[u] =
	    ((area_info[num].unit_request) * (side->unitlistcount[u]) +
	     total_requests - total_requests/4) 
	      / total_requests;
	  area_info[num].num_units[u] = 0;
	  /*	if (area_info[num].requested_units[u] > 0)
		printf("area %d requested %d %s\n", num,
		area_info[num].requested_units[u], utypes[u].name);  */
	} else area_info[num].requested_units[u] = 0;
      }
    }
  }

  /* Allocate units to locations where they are already going if they */
  /* are really needed there.  If not, then allocate them to the */
  /* current location. */
  /* Figure out how many units are already allocated. */
  for_all_unit_types(u) {
    if (!mobile(u)) { /* No need to allocate. */
      side->unitlist[u] = NULL;
    } else {
      for (priority = 1; priority <= 2; priority++) {
	allocate_as_is(side, u, priority);
      }
      /* Mark all remaining units as unselected.  We will try to
	 assign them to the nearest and highest priority task */
      for (unit = side->unitlist[u]; unit != NULL; unit = unit->mlist) {
	unit->selection_status = 10000;
      }
      /* This loop looks for high priority units that are not
	 at their destinations.  All units being placed are guaranteed
	 not to be placed in the area they currently are in. */
      for (num = 0; num < areas_wide * areas_high; num++)
	if (needs_units(1, side, num, u) || needs_units(2, side, num, u)) {
	  prev = NULL;
	  /* Send unit to priority 2 locations only if the distance is */
	  /* half as far as the nearest priority 1 location. */
	  i = (needs_units(1, side, num, u)) ? 2 : 1;
	  for (unit = side->unitlist[u]; unit != NULL; unit = unit->mlist) {
	    d = distance(unit->x, unit->y, area_info[num].x, area_info[num].y) / i;
	    if (d < unit->selection_status) {
	      if (unit->selection_status < 10000) {
		area_info[unit->area].num_units[u]--;
	      }
	      unit->area = num;
	      unit->selection_status = d;
	      area_info[num].num_units[u]++;
	      /* delete the unit immediately if it is close. no need */
	      /* to check any further */
	      if (d < AREA_SIZE) {
		if (prev == NULL)
		  side->unitlist[u] = unit->mlist;
		else prev->mlist = unit->mlist;
	      } else prev = unit;
	    } else prev = unit;
	  }
	}
      for (unit = side->unitlist[u]; unit != NULL; unit = unit->mlist) {
	prev = NULL;
	if (unit->selection_status < 10000) {
	  if (prev == NULL)
	    side->unitlist[u] = unit->mlist;
	  else prev->mlist = unit->mlist;
	} else prev = unit;
      }

      /* Now allocate all other units alreay in the correct locations. */
      allocate_as_is(side, u, 3);

      for (num = 0; num < areas_wide * areas_high; num++)
	if (needs_units(3, side, num, u)) {
	  prev = NULL;
	  for (unit = side->unitlist[u]; unit != NULL; unit = unit->mlist) {
	    d = distance(unit->x, unit->y, area_info[num].x, area_info[num].y);
	    if (d < unit->selection_status) {
	      if (unit->selection_status < 10000) {
		area_info[unit->area].num_units[u]--;
	      }
	      unit->area = num;
	      unit->selection_status = d;
	      area_info[num].num_units[u]++;
	      if (d < AREA_SIZE) {
		if (prev == NULL)
		  side->unitlist[u] = unit->mlist;
		else prev->mlist = unit->mlist;
	      } else prev = unit;
	    } else prev = unit;
	  }
	}
/* No needed, unit lists redone from start */
/*      for (unit = side->unitlist[u]; unit != NULL; unit = unit->mlist) {
	prev = NULL;
	if (unit->selection_status < 10000) {
	  if (prev == NULL)
	    side->unitlist[u] = unit->mlist;
	  else prev->mlist = unit->mlist;
	} else prev = unit;
      } */
    }
  }
  /* Allocate remaining units. */

/* Unit allocation. */

/* Allocate all units in war zones to that zone, unless they are */
 /* moving to another war zone.  

    Allocate units to other zones up to some fraction, provided that
    they have already been allocated to that area.

    Allocate the other units to the neediest area nearby.  War zones
    already have most of the units in them, so distance is not too
    important. 
*/


  
}

/* Are units for this area a top priority. */

bool needs_units(priority, side, area, utype)
int priority;
Side *side;
int area, utype;
{
  switch (priority) {
  case 1:
    if (side->areas[area].enemy_strength < 4 ||
	area_info[area].allied_bases == 0)
      return FALSE;
    else if (area_info[area].requested_units[utype] >
	     area_info[area].num_units[utype])
      return TRUE;
    else return FALSE;
    break;
  case 2:
    if (side->areas[area].enemy_strength < 4 ||
	area_info[area].allied_makers == 0)
      return FALSE;
    else if (area_info[area].requested_units[utype] / 2 >
	     area_info[area].num_units[utype])
      return TRUE;
    else return FALSE;
    break;
  case 3: 
    if (area_info[area].requested_units[utype] >
	area_info[area].num_units[utype])
      return TRUE;
    else return FALSE;
    break;
  default:
    return TRUE;
    break;
  }
}

allocate_as_is(side, u, priority)
Side *side;
int u, priority;
{
  Unit *unit, *prev;
  int num;

  prev = NULL;
  for (unit = side->unitlist[u]; unit != NULL; unit = unit->mlist) {
    num = area_index(unit->x, unit->y);
    if (unit->area >= 0 && needs_units(priority, side, unit->area, u)) {
      area_info[unit->area].num_units[u]++;
      if (prev == NULL)
	side->unitlist[u] = unit->mlist;
      else prev->mlist = unit->mlist;
    } else if (needs_units(priority, side, num, u)) {
      unit->area = num;
      area_info[num].num_units[u]++;
      if (prev == NULL)
	side->unitlist[u] = unit->mlist;
      else prev->mlist = unit->mlist;
    } else prev = unit;	  
  }
}

bool area_needs_capturer(x, y)
int x, y;
{
  int num = area_index(x, y);

  if (area_info[num].makers < area_info[num].allied_makers &&
      (mside->areas[num].capturers_approaching == 0 ||
       mside->areas[num].capture_time - global.time > 10))
    return TRUE;
  else return FALSE;
}

/* Check for any makers this unit should be capturing. */

bool should_capture_maker(unit)
Unit *unit;
{
  int x, y;
  if (search_area(unit->x, unit->y, 25, area_needs_capturer, &x, &y, AREA_SIZE))
    {
    }
  return FALSE;
}

bool no_possible_moves(unit)
Unit *unit;
{
  int fx = unit->x, fy = unit->y, ut = unit->type;
  int d, x, y;
  viewdata view;
  Side *side = unit->side;

  for_all_directions(d) {
    x = wrap(fx + dirx[d]);  y = limit(fy + diry[d]);
    view = side_view(side, x, y);
    if (view == EMPTY) {
      if (could_move(ut, terrain_at(x, y)))
	return FALSE;
    } else if (enemy_side(side_n(vside(view)) , side)
	       && could_hit(ut, vtype(view))) {
      return FALSE;
    } else if (could_carry(vtype(view), ut) &&
	       allied_side(side_n(vside(view)), side))
      return FALSE;
  }
  return TRUE;
}
