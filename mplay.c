/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file implements the main machine strategy.  Not much room for fancy */
/* tricks, just solid basic play.  The code emphasizes avoidance of mistakes */
/* instead of strategic brilliance, so machine behavior is akin to bulldozer */
/* plodding.  Nevertheless, bulldozers can be very effective when they */
/* outnumber the human players... */

/* It is also very important to prevent infinite loops, so no action of the */
/* machine player is 100% certain. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"
#include "mplay.h"


int max_range = 10;  /* dummy value to start to prevent debugging crashes. */

/* At the beginning of each turn, can make plans and review the situation. */
/* This should be frequent at first, but rather expensive to do always, */
/* so only some chance of doing it on a random turn.  This routine can be */
/* for human players too if they want to use the machines strategy for building */
/* so we have to check that some things are not done for the human. */

init_machine_turn(side)
Side *side;
{
    Unit *unit;
    int x, y;

    if (side->lost) return;
/*    if ((global.time < 20 && probability(30)) || probability(10)) make_strategy(side);
    if ((global.time < 20 && probability(30)) || probability(15)) review_groups(side);  */
    make_unitlists(side);
    if (global.time > 20 || probability(25)) decide_resignation(side);
    if (Cheat && !humanside(side)) {
      for_all_hexes(x, y) 
	see_exact(side, x, y);
    }
    /* Completely reset the areas periodically, so that units tend not to want to go too far. */
    if (global.time % 4 == 0)
      for_all_side_units(side, unit)
	unit->area = area_index(unit->x, unit->y);
    update_areas(side);
    find_border_areas(side);
    determine_unit_request(side);
    assign_units(side);
    make_unitlists(side);
    defend_priority = 50;
    explore_priority = max(75 - global.time, 0);
    attack_priority = max(0, 50 - explore_priority);
}



/* Strategy is based on a study of the entire world map, looking for */
/* duties and opportunities. */

make_strategy(side)
Side *side;
{
}

/* Decides if unit has nothing covering it. */

defended(side, x, y)
Side *side;
int x, y;
{
}

/* Sometimes there is no point in going on, but be careful not to be too */
/* pessimistic.  Right now we only give up if no hope at all. */

decide_resignation(side)
Side *side;
{
    int u, u2, sn1, inrunning = FALSE, opposed, own, chance = 0;
    Side *side1, *side2;
    Plan *plan = side_plan(side);
    Unit *unit;

    if (humanside(side))
      return;
    enter_procedure("decide resignation");
    if (global.numconds == 0) {
      for_all_sides(side1) {
	sn1 = side_number(side1);
	for_all_unit_types(u) {
	  plan->allies[sn1][u] = plan->estimate[sn1][u];
	  for_all_sides(side2) {
	    if (side1 != side2 && allied_side(side1, side2)) {
	      plan->allies[sn1][u] +=
		plan->estimate[side_number(side2)][u];
	    }
	  }
	}
      }  
      for_all_unit_types(u) {
	    own = plan->allies[side_number(side)][u];
	    for_all_unit_types(u2) {
		if (could_make(u, u2) && mobile(u2))
		    inrunning = TRUE;
		for_all_sides(side1) {
		    if (enemy_side(side, side1)) {
			opposed = plan->allies[side_number(side1)][u2];
			if (own > 0 && opposed > 0) {
			    if (could_capture(u, u2) && mobile(u))
				inrunning = TRUE;
			    if (could_hit(u, u2) && mobile(u))
				inrunning = TRUE;
			}
		    }
		}
	    }
	}
	/* should use chance for doubtful situations, like relative strength */
	if (!inrunning || probability(chance)) 
	  resign_game(side, (Side *) NULL);  
    } else {
      if (!active_display(side)) {
	chance = 0;
	for_all_side_units(side, unit)
	  /* don't resign unless you have less than twenty units and */
	  /* those can't build bases (should be anything) and can't */
	  /* make */
	  if (chance++ > 20 || utypes[unit->type].can_make ||
	      utypes[unit->type].maker || utypes[unit->type].can_capture)
	    return;
	/* Try to give away all of our units to an ally */
	for_all_sides(side1) {
	  if (side != side1 && allied_side(side, side1)) {
	    resign_game(side, side1);
	    }
	  resign_game(side, (Side *) NULL);
	}
      }}    
	      
    exit_procedure();

}

/* Decide on and make a move or set orders for a machine player. */

machine_move(unit)
Unit *unit;
{

  routine("Machine Move");
  munit = unit;
  mside = unit->side;
  if (unit->move_turn != global.time) {
    unit->move_turn = global.time;
    unit->move_tries = 0;
  } else unit->move_tries++;
      
  if (unit->move_tries++ > utypes[unit->type].speed * 2) {
    unit->movesleft = 0;
    order_sentry(unit,1);
    return;
  }
  bestworth = 1000000; /* very good, until we actually evaluate */
  if (probability(5)) {
        mside->cx = unit->x;  mside->cy = unit->y;
      }
  route_max_distance = 0;
  if (Freeze) {
    order_sentry(unit, 1);
  } else {
    if (unit->goal == LOAD && probability(fullness(unit))) {
      if (unit->gx == 0 && unit->gy == 0) 
	unit->goal = DRIFT;
      else unit->goal = APPROACH;
    }
    if (istransport(unit) && probability(20 - fullness(unit)))
      unit->goal = LOAD;
    if (unit->goal == NOGOAL) unit->goal = DRIFT;
    wake_neighbors(unit);
    /* this can cause problems if unit is carrying some other sides
       units.  As such, occupant_can_capture_neighbor does not return 
       true for occupants of other sided. */
    if (occupant_can_capture_neighbor(unit)) {
/*      delaymove = TRUE; */
/*      mside->movunit = NULL;*/ /* set in occupant_can_capture_neighbor */
      return;
    }
/*    if (should_capture_maker(unit)) return;
      if (should_build_base(unit)) return;
      if (should_capture_base(unit)) return;
      if (should_transport_someone(unit)) return;
*/
    if (should_build_base(unit)) return; 
    if (follow_plan(unit)) return;
    if (no_possible_moves(unit)) {
      order_sentry(unit,1);
      return;
    }
    if (maybe_return_home(unit)) return;
    if (unit->area != area_index(unit->x, unit->y))
      get_unit_to_area(unit);
    else {
      if (mside->areas[unit->area].safe_area && probability(80)) {
	order_sentry(unit, 2);
	return;
      }
      if (probability(50) && short_term(unit)) return;
      search_for_best_move(unit);
    }
  }
  routine("Not Machine Move");
}

/* Set up goals for units that need them. */
/* Goals should differ according to unit's role in group... */

decide_goal(unit)
Unit *unit;
{
}

bool find_base(unit, pred, extra)
Unit *unit;
bool (*pred)();
int extra;
{
  int range;
  int ox, oy, ux = unit->x, uy = unit->y;

  enter_procedure("find base");
  route_max_distance = range = range_left(unit) + extra;
  if ((range * range < mside->numunits) ? 
      (search_area(ux, uy, range, pred, &ox, &oy, 1))  :
      (find_closest_unit(ux, uy, range, pred, &ox, &oy))) {
/*	order_moveto(unit, ox, oy); */
    if (munit->move_tries < 3)
      unit->orders.flags |= SHORTESTPATH;
    unit->orders.flags &=
      ~(ENEMYWAKE|NEUTRALWAKE|SUPPLYWAKE|ATTACKUNIT);
    if (Debug) printf("will resupply at %d,%d\n", ox, oy);
    exit_procedure();
    return TRUE;
  }
  exit_procedure();
  return FALSE;
}

/* See if we're in a bad way, either on supply or hits, and get to safety */
/* if possible.  If not, then move on to other actions. */
/* Can't be 100% though, there might be some problem preventing move */

maybe_return_home(unit)
Unit *unit;
{
    int u = unit->type;

    enter_procedure("maybe return home 1");

    if (Debug) printf("%s low supplies %d moves %d wakup %d - ", unit_desig(unit),
		      low_supplies(unit), moves_till_low_supplies(unit),
		      unit->wakeup_reason);
    unit->priority = 1000000;
    if ((low_supplies(unit) || munit->move_tries > 6 ||
	 (moves_till_low_supplies(unit) < min(3, unit->movesleft) &&
	  unit->wakeup_reason != WAKEENEMY))  &&
	!can_capture_neighbor(unit)
	&& probability(100)) {

      if (Debug) printf("%s should get supplies ? - ", unit_desig(unit));
      if (producing(unit) && survival_time(unit) > unit->schedule) {
	order_sentry(unit, 1);
	exit_procedure();
	return TRUE;
      }
      if (Debug) printf("%s should get supplies - ", unit_desig(unit));
      if (find_base(unit,good_haven_p, 0)) {
	exit_procedure();
	return TRUE;
      } else if (find_base(unit,haven_p, 0)) {
	exit_procedure();
	return TRUE;
      } else if (unit->transport != NULL) {
	order_sentry(unit, 1);
      } else if (survive_to_build_base(unit) && unit->transport == NULL) {
	if (Debug) printf("going to build something\n");
	set_product(unit, machine_product(unit));
	set_schedule(unit);
	order_sentry(unit, unit->schedule+1);
	exit_procedure();
	return TRUE;
      } else if (find_base(unit,haven_p, 1)) {
	exit_procedure();
	return TRUE;
      } else if (Debug) printf("but can't\n");
      
    }
    if ((cripple(unit) && probability(98)) ||
	probability(100 - ((100 * unit->hp) / utypes[u].hp))) {
	/* note that crippled units cannot repair themselves */
	if (Debug) printf("%s badly damaged - ", unit_desig(unit));
	if (unit->transport && could_repair(u, unit->transport->type)) {
	    if (Debug) printf("%s will repair\n", unit_desig(unit->transport));
	    order_sentry(unit, 1);
	    exit_procedure();
	    return TRUE;
	} else {
	    if (find_base(unit, shop_p, 0)) {
	        exit_procedure();
		return TRUE;
	    } else {
		if (Debug) printf("but no place to repair\n");
	    }
	}
    }
    if (out_of_ammo(unit) >= 0 && probability(80)) {
	if (Debug) printf("%s should reload - ", unit_desig(unit));
	if (find_base(unit, haven_p, 0)) {
	    exit_procedure();
	    return TRUE;
	} else {
	  if (survive_to_build_base(unit) && 
	      unit->transport == NULL && probability(90)) {
	    if (Debug) printf("going to build something\n");
	    set_product(unit, machine_product(unit));
	    set_schedule(unit);
	    order_sentry(unit, unit->schedule+1);
	  }
	  else if (Debug) printf("but can't\n");
	}
    }
    exit_procedure();
    return FALSE;
}

/* Return the distance that we can go by shortest path before running out */
/* of important supplies.  Will return at least 1, since we can *always* */
/* move one hex to safety.  This is a worst-case routine, too complicated */
/* to worry about units getting refreshed by terrain or whatever. */

range_left(unit)
Unit *unit;
{
    int u = unit->type, r, least = 12345;

    for_all_resource_types(r) {
	if (utypes[u].tomove[r] > 0) {
	  least = min(least, unit->supply[r] / utypes[u].tomove[r]);
	}
	if (utypes[u].consume[r] > 0)
	    least = min(least,
			(utypes[unit->type].speed * unit->supply[r]) / utypes[u].consume[r]);
    }
    return (least == 12345 ? 1 : least);
}

/* Try to get to the given destination using the router in preference
to the order_moveto command */

route_to(unit, x, y)
Unit *unit;
int x,y;
{
  Mplan *plan;
  int maxdist = moves_till_low_supplies(unit);
  Unit *dest_unit = unit_at(x,y);

  if (dest_unit != NULL && isbase(dest_unit))
    maxdist = range_left(unit);
  if (route_max_distance > maxdist)
    maxdist = route_max_distance;
  plan = find_route(munit->type, maxdist,
		    munit->x, munit->y, x, y);
  if (plan != NULL) {
    unit->priority = bestworth;
    free_plan(unit->plan);
    unit->plan = plan;
    /* Find the last plan step that we want to use */
    while (maxdist-- > 1 && plan != NULL)
      plan = plan->next;
    if (plan != NULL) {
      free_plan(plan->next);
      plan->next = NULL;
    }
/*    printf("Using plan ");
    printf("unit %d type %d (x,y) (%d,%d), to (%d,%d)\n",
	   unit->id, unit->type, unit->x, unit->y, x, y); */
  } else {
 /*    printf("Could not find plan.  Ordering move to %d.\n",
       route_max_distance); 
    printf("unit %d type %d (x,y) (%d,%d), to (%d,%d)\n",
	   unit->id, unit->type, unit->x, unit->y, x, y); */
    order_moveto(unit, x, y);
  }
}
  

/* Do short-range planning.  Only thing here is intended to be for defenders */
/* protecting a small area (5 moves is arb, should derive from defense */
/* group area). */

short_term(unit)
Unit *unit;
{
    int u = unit->type, range;

    if (!mobile(unit->type)) {
	order_sentry(unit, 100);
	return TRUE;
    }
    switch (unit->goal) {
    case DRIFT:
	range = max(9, min(5 * utypes[u].speed,
			    moves_till_low_supplies(unit)));
	if (probability(90)) {
	    find_worths(range);
	    if (bestworth >= 0) {
		if (Debug) printf("drifting to %d,%d (worth %d)\n",
				  bestx, besty, bestworth);
		if (unit->transport != NULL &&
		    moves_till_low_supplies(unit) < 8)
		  do_take(mside, unit, -1);
		route_to(unit, bestx, besty);
		unit->orders.flags &= ~SHORTESTPATH;
		return TRUE;
	    }
	}
	break;
    case LOAD:
    case APPROACH:
    case HITTARGET:
    case CAPTARGET:
	break;
    default:
        case_panic("unit goal", munit->goal);
	break;
    }
    return FALSE;
}

/* Search for most favorable odds anywhere in the area, but only for */
/* the remaining moves in this turn.  Multi-turn tactics is elsewhere. */

search_for_best_move(unit)
Unit *unit;
{
    int ux = unit->x, uy = unit->y,
        range = min(moves_till_low_supplies(unit), AREA_SIZE+2), goal;

    enter_procedure("search_for_best_move");
    if (!mobile(unit->type)) {
	order_sentry(unit, 100);
	exit_procedure();
	return;
    }
    if (utypes[unit->type].produce[0] > 0
	&& mside->areas[area_index(ux, uy)].enemy_strength == 0)
      range = min(range * 3, AREA_SIZE * 2);
    if (Debug) printf("%d: %s ", global.time, unit_desig(unit));
    find_worths(range);
    if (bestworth >= 0) {
      	if (unit->transport != NULL && mobile(unit->transport->type) &&
	    bestworth >= EXPLORE_VAL) {
	    if (Debug) printf("moving to %d,%d (worth %d)\n",
			      bestx, besty, bestworth);
	    if (unit->transport != NULL &&
		moves_till_low_supplies(unit) < 2)
	      do_take(mside, unit, -1);
	    route_to(unit, bestx, besty);
	    unit->orders.flags &= ~SHORTESTPATH;
	} else if (unit->transport != NULL && mobile(unit->transport->type) &&
	    /* force unit to try to leave transport occasionally */
	    probability(30)) {
	    if (Debug) printf("sleeping on transport\n");
	    order_sentry(unit, 1);
	} else if ((bestworth < 3000 ||
		    (moves_till_low_supplies(unit)) < distance(ux, uy, bestx, besty))
		   && survive_to_build_base(unit) &&
		   !base_nearby(unit, 2*utypes[unit->type].speed) &&
		   (utypes[unit->type].occproduce || (unit->transport == NULL))
		   && probability(40)) {
	      if (Debug) printf("going to build something\n");
	      set_product(unit, machine_product(unit));
	      set_schedule(unit);
	      order_sentry(unit, unit->schedule+1);
	    }
	else if ((ux == bestx && uy == besty) || !can_move(unit)) {
	    if (Debug) printf("staying put\n");
	    order_sentry(unit, 1);
	} else if (probability(90)) {
	    if (Debug) printf("moving to %d,%d (worth %d)\n",
			      bestx, besty, bestworth);
	    if (unit->transport != NULL &&
		moves_till_low_supplies(unit) < 2)
	      do_take(mside, unit, -1);
	    route_to(unit, bestx, besty);
	    unit->orders.flags &= ~SHORTESTPATH;
	} else {
	    if (unit->transport != NULL)
	      do_embark(mside, unit, RANDOM(5));
	    if (Debug) printf("hanging around\n");
	}
    } else {
	goal = unit->goal;
	/* jam alternative sometimes... */
	if (probability(95)) goal = DRIFT;
	switch (goal) {
	case DRIFT:
	    if (can_produce(unit) && survive_to_build_base(unit) &&
		(utypes[unit->type].occproduce || (unit->transport == NULL)) &&
		probability(90)) {
		if (Debug) printf("going to build something\n");
		set_product(unit, machine_product(unit));
		set_schedule(unit);
		order_sentry(unit, unit->schedule+1);
	    } else if (unit->transport != NULL && do_embark(mside, unit, 1)) {
		break;
	    } else if (probability(90)) {
		if (Debug) printf("going in random direction\n");
		order_movedir(unit, random_dir(), RANDOM(3)+1);
	    } else {
		if (Debug) printf("hanging around\n");
		order_sentry(unit, RANDOM(4)+1);
	    }
	    break;
	case LOAD:
	    if (unit->occupant != NULL) {
		if (Debug) printf("starting off to goal\n");
		unit->goal = APPROACH;
		route_to(unit, unit->gx, unit->gy);
	    } else {
		if (bestworth >= 0) {
		    if (Debug) printf("loading at %d,%d (worth %d)\n",
				      bestworth, bestx, besty);
		    route_to(unit, bestx, besty);
		    unit->orders.flags &= ~SHORTESTPATH;
		} else {
		    if (Debug) printf("moving slowly about\n");
		    order_movedir(unit, random_dir(), 1);
		}
	    }
	    break;
	case APPROACH:
	    if (survive_to_build_base(unit) && !base_nearby(unit, 8) &&
		(utypes[unit->type].occproduce || (unit->transport == NULL))
		&& probability(90) && unit->wakeup_reason != WAKEENEMY
		&& bestworth < 1000) {
	      if (Debug) printf("going to build something\n");
	      set_product(unit, machine_product(unit));
	      set_schedule(unit);
	      order_sentry(unit, unit->schedule+1);
	      break;
	    }
	    if (unit->transport != NULL && do_embark(mside, unit, 1))
	      break;
	case HITTARGET:
	case CAPTARGET:
	    if (unit->transport != NULL) {
	      if (!can_move(unit)) {
		    if (Debug) printf("waiting to get off\n");
		    order_sentry(unit, 2);
		} else {
		    if (Debug) printf("leaving for %d,%d\n",
				      unit->gx, unit->gy);
		    route_to(unit, unit->gx, unit->gy);
		}
	    } else {
		if (Debug) printf("approaching %d,%d\n", unit->gx, unit->gy);
		route_to(unit, unit->gx, unit->gy);
	    }
	    break;
	default:
            case_panic("unit goal", munit->goal);
	    break;
	}
    }
    exit_procedure();
}

find_worths(range)
int range;
{
  int ux = munit->x, uy = munit->y;

    bestworth = -10000;
#ifdef REGIONS
    munit_regions = regions_around(munit->type, ux, uy, FALSE);
#endif
    max_range = range;
    apply_to_area(ux, uy, range, evaluate_hex);
    apply_to_area(ux, uy, range, maximize_worth);
}

/* Given a position nearby the unit, evaluate it with respect to goals, */
/* general characteristics, and so forth.  -10000 is very bad, 0 is OK, */
/* 10000 or so is best possible. */

/* Should downrate hexes within reach of enemy retaliation. */
/* Should downrate hexes requiring supply consumption to enter/occupy. */

extern int printing;

evaluate_hex(x, y)
int x, y;
{
    bool adjhex, ownhex, reachable;
    int etype, dist, worth = 0;
    viewdata view;
    int terr = terrain_at(x, y);
    Side *es;
    Unit *eunit;

    if (!between(1, y, world.height-2))
/*** (HW) insert -> ***/
    {
/*** <- insert ***/
      aset(localworth,x,y,-10000);
      return -10000;
/*** (HW) insert -> ***/
    }
/*** <- insert ***/

    enter_procedure("evaluate hex"); 
printing = FALSE;
    view = side_view(mside, x, y);
    dist = distance(munit->x, munit->y, x, y);
    adjhex = (dist == 1);
    ownhex = (dist == 0);
    /* it would be cheating if we didn't check that we knew what
       the view was before we decided it was unreachable. */
       
    reachable =
#ifdef REGIONS
      (view == UNSEEN ||
		 (munit_regions & regions_around(munit->type, x, y, TRUE)) != 0);
#else
       TRUE;
#endif

    
    if (view == EMPTY && !reachable) {
      /* don't bother with empty hexes we can't reach without a transport. */
      aset(localworth, x, y, -1000);
      exit_procedure();
      return;
    }
    if (y <= 0 || y >= world.height-1) {
	worth = -10000;
    } else {
	switch (munit->goal) {
	case DRIFT:
	    if (ownhex) {
		worth = -1;
	    } else if (view == UNSEEN) {
		worth = EXPLORE_VAL + 200 /dist + 
		  ((dist < munit->movesleft - 1) ? 5 * dist : -20) +
		  ((dist < max_range - 1 ) ? 5 * dist : 0);
	    } else if (view == EMPTY) {
#ifdef PREVVIEW
		worth = PATROL_VAL + side_view_age(mside, x, y);
#else
		worth = PATROL_VAL;
#endif
		if (impassable(munit, x, y)) worth -= 900;
	    } else {
		es = side_n(vside(view));
		etype = vtype(view);
		if (es == NULL) {
		    if (could_capture(munit->type, etype)
			|| (occupant_could_capture(munit,etype) && !adjhex)) {
			worth = CAPTURE_OTHER + bcw[munit->type][etype] / dist;
		    } else {
			worth = -10000;
		    }
		} else if (enemy_side(mside, es)) {
		    worth = attack_worth(munit, etype);
		    if (worth > 0)
		      worth = FAVORABLE_COMBAT + 80 * attack_worth(munit, etype);
		    else worth = UNFAVORABLE_COMBAT + 80 * attack_worth(munit, etype);
		    worth += threat(mside, etype, x, y);
		    if (dist > range_left(munit) / 2)
		      worth /= dist;
		    worth += 100;
		    if (could_capture(munit->type, etype)
			|| occupant_could_capture(munit,etype)) {
			worth += CAPTURE_OTHER + bcw[munit->type][etype] / dist;
		      }
		} else {
		  if ((eunit = unit_at(x, y)) != NULL) {
		    if (can_carry(eunit, munit) &&
			mobile(eunit->type)) {
		      worth = MEET_TRANSPORT;
		      worth += 100/dist;
		    } else worth = -10000;
		  }
		}
	    }
	    break;
	case LOAD:
	    if (ownhex || view == UNSEEN || view == EMPTY) {
		worth = -1;
	    } else {
		es = side_n(vside(view));
		if (allied_side(mside, es)) {
		    if ((eunit = unit_at(x, y)) != NULL &&
			can_carry(munit, eunit)) {
			    worth = MEET_TRANSPORT;
			    worth += 100/dist;
			    worth += 50;
		    } else worth = -10000;
		} else {
		    worth = -10000;
		}
	    }
	    break;
	case APPROACH:
	case HITTARGET:
	case CAPTARGET:
	    if (ownhex) {
		worth = -100;
	    } else if (view == UNSEEN) {
		worth = EXPLORE_VAL + 200 /dist + 
		  ((dist < munit->movesleft - 1) ? 5 * dist : -20) +
		  ((dist < max_range - 1 ) ? 5 * dist : 0);
	    } else if (view == EMPTY) {
	        if (impassable(munit, x, y)) worth -= 900;
#ifdef PREVVIEW
		else worth = PATROL_VAL + side_view_age(mside, x, y);
#else
		else worth = PATROL_VAL;
#endif
	    } else {
		es = side_n(vside(view));
		etype = vtype(view);
		if (es == NULL) {
		    if (could_capture(munit->type, etype) ||
			(!adjhex && occupant_could_capture(munit,etype))) {
			worth = CAPTURE_OTHER + bcw[munit->type][etype] / dist;
		    } else {
			worth = -10000;
		    }
		} else if (!allied_side(mside, es)) {
		    worth = attack_worth(munit, etype);
		    if (worth > 0)
		      worth = FAVORABLE_COMBAT + 80 * attack_worth(munit, etype);
		    else worth = UNFAVORABLE_COMBAT + 80 * attack_worth(munit, etype);
		    worth += threat(mside, etype, x, y);
		    if (dist > range_left(munit) / 2)
		      worth /= dist;
		    worth += 100;
		    if (could_capture(munit->type, etype)
			|| occupant_could_capture(munit,etype)) {
			worth += CAPTURE_OTHER + bcw[munit->type][etype] / dist;
		      }
		} else {
		  /* must be ally */
		  if ((eunit = unit_at(x, y)) != NULL) {
		    if (can_carry(eunit, munit) &&
			mobile(eunit->type)) {
		      worth = MEET_TRANSPORT + 100/dist;
		    }
		  } else worth = -10000;
		}
	    }
	    break;
	default:
	    case_panic("unit goal", munit->goal);
	    break;
	}
    }
    if ((munit->gx > 0 || munit->gy > 0) && (worth < 3000) &&
	(distance(x, y, munit->gx, munit->gy) <
	 distance(munit->x, munit->y, munit->gx, munit->gy))  &&
	view == EMPTY) {
	worth += HEAD_FOR_GOAL - 5 * distance(x, y, munit->gx, munit->gy);
    }
    if ((view != EMPTY) && (view != UNSEEN) && !ownhex) {
      es = side_n(vside(view));
      etype = vtype(view);
      if (!allied_side(mside,es) && (could_capture(munit->type, etype) ||
				     occupant_could_capture(munit,etype))) {
	worth += 4000 / dist;
      }
    }
    /* try to count this hex less if other units could get to it more easily */

/*    if (worth > 1000) {
      nearer_units = units_nearby(x, y, dist - 1, munit->type);
      if (nearer_units > 0)
	worth = worth / 2 + worth / (nearer_units + 1);
    } */
    worth -= 100;
    worth += utypes[munit->type].productivity[terr];
    if (printing)
      notify(mside,"final worth %d", worth);
    aset(localworth, x, y, worth);
    exit_procedure(); 
    return worth;  /* debugging cludge */
}

/* Scan evaluated area looking for best overall hex. */


maximize_worth(x, y)
int x, y;
{
    int worth;

    worth = aref(localworth, x, y);
    if (worth >= 0) {
	if (worth > bestworth) {
	    bestworth = worth;  bestx = x;  besty = y;
	} else if (worth == bestworth && flip_coin()) {
	    bestworth = worth;  bestx = x;  besty = y;
	}
    }
}

/* This is a heuristic estimation of the value of one unit type hitting */
/* on another.  Should take cost of production into account as well as the */
/* chance and significance of any effect. */

attack_worth(unit, etype)
Unit *unit;
int etype;
{
    int utype = unit->type, worth;

    worth = bhw[utype][etype];
    if (utypes[utype].damage[etype] >= utypes[etype].hp)
	worth *= 2;
    if (utypes[etype].damage[utype] >= unit->hp)
	worth /= (could_capture(utype, etype) ? 1 : 4);
    if (could_capture(utype, etype)) worth *= 4;
    return worth;
}


/* Support functions. */

/* True if unit is in immediate danger of being captured. */
/* Needs check on capturer transport being seen. */

might_be_captured(unit)
Unit *unit;
{
    int d, x, y;
    Unit *unit2;

    for_all_directions(d) {
	x = wrap(unit->x + dirx[d]);  y = unit->y + diry[d];
	if (((unit2 = unit_at(x, y)) != NULL) &&
	    (enemy_side(unit->side, unit2->side)) &&
	    (could_capture(unit2->type, unit->type))) return TRUE;
    }
    return FALSE;
}

/* Return true if the given unit type at given position is threatened. */

threat(side, u, x0, y0)
Side *side;
int u, x0, y0;
{
    int d, x, y, thr = 0;
    Side *side2;
    viewdata view;

    for_all_directions(d) {
	x = wrap(x0 + dirx[d]);  y = y0 + diry[d];
	view = side_view(side, x, y);
	if (view != UNSEEN && view != EMPTY) {
	    side2 = side_n(vside(view));
	    if (allied_side(side, side2)) {
		if (could_capture(u, vtype(view))) thr += 1000;
		if (bhw[u][vtype(view)] > 0) thr += 100;
	    }
	}
    }
    return thr;
}

/* Returns the type of attack to plan for.  (Should balance relative */
/* effectiveness of each type of attack.) */

attack_type(e)
int e;
{
    int u;

    if (utypes[e].surrender > 0 || utypes[e].siege > 0) return BESIEGE;
    for_all_unit_types(u) if (could_capture(u, e)) return CAPTARGET;
    return HITTARGET;
}

bool should_build_base(unit)
Unit *unit;
{
  if (Debug) printf("Should build base survive %d base near %d wakeup %d \n",
		    survive_to_build_base(unit), base_nearby(unit,1),
		    unit->wakeup_reason);
  if ((exact_survive_to_build_base(unit))
      && unit->transport == NULL &&
      !any_base_nearby(unit,1) && probability(90) &&
      unit->wakeup_reason != WAKEENEMY) {
    if (Debug) printf("going to build something\n");
    set_product(unit, machine_product(unit));
    set_schedule(unit);
    order_sentry(unit, unit->schedule+1);
    return TRUE;
  }
  if ((survive_to_build_base(unit) && probability(20))
      && unit->transport == NULL &&
      !any_base_nearby(unit,1) && probability(90) &&
      !neutral_base_nearby(unit, 5) &&
      unit->wakeup_reason != WAKEENEMY) {
    if (Debug) printf("going to build something\n");
    set_product(unit, machine_product(unit));
    set_schedule(unit);
    order_sentry(unit, unit->schedule+1);
    return TRUE;
  }       
  else return FALSE;
}


get_unit_to_area(unit)
Unit *unit;
{
  unit->goal = APPROACH;
  unit->gx = area_info[unit->area].x;
  unit->gy = area_info[unit->area].y;
  search_for_best_move(unit);
}
