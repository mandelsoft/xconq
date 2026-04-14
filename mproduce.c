/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file implements production decisions for the machine players. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"
#include "mplay.h"


/* Decide whether a change of product is desirable. */

bool change_machine_product(unit)
Unit *unit;
{
    int u = unit->type;

    if (Freeze) {
	return FALSE;
    } else if (utypes[u].maker) {
	if (producing(unit)) {
	    if ((unit->built > 2) ||
		((utypes[u].make[unit->product] * unit->built) > 20)) {
		return TRUE;
	    }
	} else {
	    return TRUE;
	}
    }
    return FALSE;
}

/* Machine algorithm for deciding what a unit should build. This routine */
/* must return the type of unit decided upon.  Variety of production is */
/* important, as is favoring types which can leave the builder other than */
/* on a transport.  Capturers of valuable units are also highly preferable. */

int machine_product(unit)
Unit *unit;
{
    int u = unit->type, u2, u3,
            type, t, d, x, y, x1, y1, d1, value, bestvalue, besttype, tmp;
    int adjterr[MAXUTYPES];
    int units_close[MAXUTYPES];
    int demand[MAXUTYPES], total_demand, ucount[MAXUTYPES];
    int Values[MAXUTYPES];
    int others, total_units;
    Unit *unit2;
    int enemy_strength = unit->side->areas[area_index(unit->x, unit->y)].enemy_strength;
    int units_lost = unit->side->areas[area_index(unit->x, unit->y)].units_lost;

    enter_procedure("machine_product");
    mside = unit->side;
    besttype = period.firstptype;
    /* make sure we find a product if the unit can build. */
    if (!could_make(u, besttype)) {
      for_all_unit_types(u2)
	if (could_make(u, u2))
	  besttype = u2;
    }
    bestvalue = 0;
#ifdef REGIONS
    for_all_unit_types(u2) {
      adjterr[u2] = 0;
      units_close[u2] = 0;
      for_all_directions(d) {
	x = wrap(unit->x + dirx[d]);  y = unit->y + diry[d];
	if ((t = aref(unit_region[u2], x, y)) >= 0) {
	  tmp = TRUE;
	  for (d1 = 0; d1 < d; d1++) {
	    x1 = wrap(unit->x + dirx[d1]);  y1 = unit->y + diry[d1];
	    if (t == aref(unit_region[u2], x1, y1))
	      tmp = FALSE;
	  }
	  if (tmp) {
	    adjterr[u2] += unit_region_size[u2][t];
	    units_close[u2] += units_in_region[u2][t];
	  }
	}
      }
    }
#else
    for_all_unit_types(u2) {
      adjterr[u2] = 0;
      units_close[u2] = 0;
      for_all_directions(d) {
	x = wrap(unit->x + dirx[d]);  y = unit->y + diry[d];
	if (could_move(u2, terrain_at(x, y))) {
	  adjterr[u2] += 1;
	}
      }
      units_close[u2] = units_nearby(x, y, 7, munit->type);
    }
#endif
    if (!worths_known) {
      for_all_unit_types(u2) {
	tmp = utypes[u2].is_base;  /* really should be checking that can leave
				      either by moving or on transport. Bases
				      can always be made. */
	if (could_make(u, u2)) {
	  value = side_plan(mside)->demand[u2];
	  if (mobile(u2)) {
#ifdef REGIONS
	    value += adjterr[u2] * num_regions[u2] * 
	             600 / ((unit_hexes[u2] != 0) ? unit_hexes[u2]: 1);
#endif
	    if (adjterr[u2] > 0) tmp = TRUE;
	  } else tmp = TRUE;
	  value *= isqrt(utypes[u2].speed);
	  for_all_unit_types(u3)
	    if (bcw[u2][u3] > 100)
	      value += value / 8;
	  if (utypes[u2].is_transport && !utypes[u2].is_carrier)
	    value += value/4;
	  if (value > 0 && utypes[u2].is_carrier && base_building)
	    value /= 2;
	  if (utypes[u2].is_base) value += value;
	  if (value > 0)
	    value /= (mside->units[u2] + mside->building[u2] + 1);
	  others = units_nearby(unit->x, unit->y, min(utypes[u2].speed * 5, 14), u2);
	  if (others > 0 && value > 0) value /= others;
	  
	  /* might want to adjust by number of existing units? */
	  value = (value * (100 - build_time(unit, u2))) / 100;
	  if (build_time(unit, u2) < 10)
	    value += (value * (10 - build_time(unit, u2))) / 10;
	  if (utypes[u2].selfdestruct && value > 0) value /= 10;
	  if (utypes[u2].research > 0) {
	    if (mside->building[u2] > 0) {
	      if (value > 0)
		value = value / 100 - 1000;
	      else value -= 1000;
	    } else for_all_unit_types(u3) {
	      if (utypes[u2].research_contrib[u3] && mside->counts[u3] <= 1) {
		if (value > 0)
		  value = value / 100 - 1000;
		else value -= 1000;
	      }
	    }
	  }
	  if (tmp && value > bestvalue) {
	    besttype = u2;
	    bestvalue = value;
	  }
	}
      }
    } else {
      total_demand = 1; /* make sure we don't divide by zero */
      for_all_unit_types(u2) 
	if (could_make(u, u2)) {
	  demand[u2] = utypes[u2].attack_worth * attack_priority +
	    utypes[u2].defense_worth *
	      (defend_priority + 5 * (enemy_strength + units_lost)) +
	      utypes[u2].explore_worth * explore_priority;
	  /* if we are under attack, build quick units. */
	  if (utypes[u].make[u2] < 10 && enemy_strength > 3)
	    demand[u2] += demand[u2] / 2;
	  if (utypes[u].make[u2] < 5 && enemy_strength > 3)
	    demand[u2] += demand[u2] / 2;
	  total_demand += demand[u2];
	  ucount[u2] = 0;
      }
      total_units = 100;  /* actually hundreths of units */
      for_all_side_units(mside, unit2) {
	if (could_make(u, unit2->type))
	  ucount[unit2->type] += 100;
	if (unit != unit2 && producing(unit2) && !cripple(unit2)) {
	  ucount[unit2->product] +=
	    max(30, ((100 + utypes[unit2->type].make[unit2->product] - 
		      unit2->schedule) * 10) /
		utypes[unit2->type].make[unit2->product]);
	}
      }
      for_all_unit_types(u2)
	 if could_make(u, u2) {
	   ucount[u2] =  
	     region_portion(ucount[u2], u2, units_close, adjterr);
	   total_units += ucount[u2];
	 }
      for_all_unit_types(u2) {
	tmp = utypes[u2].is_base;  /* really should be checking that can leave
				      either by moving or on transport. Bases
				      can always be made. */
	if (could_make(u, u2)) {
	  value = demand[u2] - ((3 + ucount[u2]) * total_demand) / total_units;
	  if (unit->product == u2) value += max(1, value / 4);
/*	  others = units_nearby(unit->x, unit->y,
				min(utypes[u2].speed * 5, 14), u2); */
/*	  if (others > 0) value /= isqrt(others); */
	  
	  /* might want to adjust by number of existing units? */
	  Values[u2] = value;
	  if (utypes[u2].research > 0) {
	    if (mside->building[u2] > 0) {
	      if (value > 0)
		value = value / 100 - 1000;
	      else value -= 1000;
	    } else for_all_unit_types(u3) {
	      if (utypes[u2].research_contrib[u3] && mside->counts[u3] <= 1) {
		if (value > 0)
		  value = value / 100 - 1000;
		else value -= 1000;
	      }
	    }
	  }
	  if (value > bestvalue && adjterr[u2] >
#ifdef REGIONS
	      utypes[u2].min_region_size
#else
	    0
#endif
	      )
	    {
	    besttype = u2;
	    bestvalue = value;
	  }
	}
      }
      type= besttype;
/*      notify(side_n(0),"%d %d | %d %d %d %d %d %d",
	     type, total_units, Values[0], adjterr[0], adjterr[1], 
	     adjterr[2], adjterr[3], adjterr[4]); 
*/
/*      printf("\n%d: unit %s was building %s, now %s value %d\n", 
	     global.time, unit_desig(unit),
	     (unit->product == NOTHING ? "no" : utypes[unit->product].name),
	     (type == NOTHING ? "no" : utypes[type].name),
	     bestvalue); */
    }
    type = besttype;
    /* safety check */
    if (!could_make(unit->type, type)) type = NOTHING;
    if (Debug) printf("%d: %s will now build %s units\n",
		      global.time, unit_desig(unit),
		      (type == NOTHING ? "no" : utypes[type].name));
    exit_procedure();
    return type;
}

/* Take into account other units already in this region.  This will
tend to increase the unit count of regions which already have more
units than the portion of the land they account for. */

int region_portion(n, ut, units_close, adjterr)
int n, ut, units_close[MAXUTYPES], adjterr[MAXUTYPES];
{
  
  int result;

#ifdef REGIONS
  if (unit_hexes[ut] != 0) 
    result = n - (n * adjterr[ut]) / (3 * unit_hexes[ut]) +
      (n * units_close[ut]) / 
	(3 * max(mside->unitlistcount[ut], 1));
  else 
#endif /* REGIONS */
  result = n + (n * units_close[ut]) /
	         (3 * max(mside->unitlistcount[ut], 1));
  return result;
}
