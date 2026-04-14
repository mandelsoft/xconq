/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Initialization is complicated, because xconq needs lots of setup for */
/* maps, units, sides, and the like.  The data must also be able to come */
/* from saved games, scenarios, bare maps in files, period descriptions, */
/* or be synthesized if necessary. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

extern char *rawfilenames[];

extern int numfiles;

int good_place();          /* to make gcc happy */

/* Keep track of whether each sort of data has been loaded or not. */
/* The version isn't important enough to need any sort of flag. */

bool periodmade, mapmade, globalsmade, sidesmade, unitsmade;
bool populations = FALSE;  /* true if populace ever nonzero */
bool inopen[MAXUTYPES];    /* true if unit type need not be occ at start */

int numhexes[MAXUTYPES];   /* temporaries used by country placement */
int countryx[MAXSIDES], countryy[MAXSIDES];  /* centers of countries */
int snameused[MAXSNAMES];  /* flags side names already used by somebody */
int unameused[MAXUNAMES];  /* flags unit names already used */

/* Either load a saved game or load mapfiles from cmd line and then make up */
/* any stuff that didn't get loaded. */

init_game()
{
    int i;

    periodmade = mapmade = globalsmade = sidesmade = unitsmade = FALSE;

    if (saved_game()) {
	printf("Restoring from \"%s\" ...\n", SAVEFILE);
	load_mapfile(SAVEFILE);
    } else {
	for (i = 0; i < numfiles; ++i) {
	    load_mapfile(rawfilenames[i]);
	}
	if (!mapmade) {
	    printf("Please wait six days while I create the world...\n");
	    make_up_map();
	}
	if (!globalsmade) {
	    make_up_globals();
	}
	if (!sidesmade) {
	    make_up_sides();
	}
	if (numsides <= 0) {
	    fprintf(stderr, "No player sides in this game!\n");
	    exit(1);
	} else if (numsides < numgivens) {
	    fprintf(stderr,
		    "Warning: only made %d of the %d sides requested.\n",
		    numsides, numgivens);
	}
	if (!unitsmade) {
	    make_up_units();
	}
    }
    init_unit_views();
    fixup_things();
    printf("\nThe time is %s.\n", period.name);
}

construct_alliances()
{
  Side	*side, *side2;
  int	rcount=0, rnum=0, rnum2,
	nblocks;

  for_all_sides(side) {
    if (!humanside(side))
      rcount++;
  }
  nblocks = max(rcount/5, 1);
  /* about 5-9 countries per robot alliance block */

  for_all_sides(side) {
    rnum2=0;
    for_all_sides(side2) {
      if (neutral_side(side, side2)) {
	if (!humanside(side) && !humanside(side2) &&
	    rnum%nblocks == rnum2%nblocks)
	  declare_alliance(side, side2);
	else
	  declare_war(side, side2);
      }
      if (!humanside(side2))
	rnum2++;
    }
    if (!humanside(side))
      rnum++;
  }
}

/* Sort out the mess created by loading some things and creating others. */
/* Should only be possible to start with 0 units when building a scenario. */
/* Need to make friends and enemies; normally the machine players gang up */
/* on the humans. */

fixup_things()
{
    bool hasany;
    int x, y;
    Unit *unit;
    Side *side, *side2;

    if (no_statistics()) {
	for_all_units(side, unit) {
	    if (!neutral(unit)) unit->side->balance[unit->type][FIRSTUNIT]++;
	}
    }
    for_all_sides(side) {
	hasany = FALSE;
	for_all_units(side2, unit) {
	    if (unit->side == side) {
		hasany = TRUE;
		break;
	    }
	}
	if (!hasany &&
	    !side->lost && (unit = random_start_unit()) != NULL) {
	    unit_changes_side(unit, side, FIRSTUNIT, -1);
	    unit->trueside = unit->side;
	    set_product(unit, period.firstptype);
	    set_schedule(unit);
	}
    }

    construct_alliances();

    /* this slowness is necessary to solve chicken/egg problems of loading */
    /* units and sides from files... */
    if (period.allseen || world.known) {
      for_all_sides(side) {
	for_all_hexes(x, y) {
	  if (side_view(side, x, y) == UNSEEN) {
	    /* Make sure we never erase views here */
	    set_side_view(side, x, y, EMPTY);
	    if ((unit = unit_at(x, y)) != NULL) {
	      if (period.allseen ||
		  utypes[unit->type].alreadyseen ||
		  utypes[unit->type].seealways ||
		  unit->side == side) {
		see_exact(side, x, y);
	      }
	      if (utypes[unit->type].seealways) {
		set_cover(side, x, y, 100);
	      }
	    }
	  }
	}
      }
    }
}

/* More code to initialize views of area. */

iview_hex(x, y)
int x, y;
{
    Unit *unit;

    if (side_view(tmpside, x, y) == UNSEEN) {
      set_side_view(tmpside, x, y, EMPTY);
      if ((unit = unit_at(x, y)) != NULL) {
	if (cover(tmpside, x, y) > 0 || utypes[unit->type].alreadyseen) {
	  see_exact(tmpside, unit->x, unit->y);
	}
      }
    }
}

init_unit_views()
{
    Unit *unit;
    Side *loop_side;

    for_all_units(loop_side, unit) {
	if (!neutral(unit) &&
	    alive(unit) ) { /* the alive check is only necessary because
			       of saved games with units with coordinates
			       -1,-1 (we set their hitpoints to 0 when
			       loading). */
	    see_exact(unit->side, unit->x, unit->y);
	    if (period.knownradius > 0) {
		tmpside = unit->side;
		apply_to_area(unit->x, unit->y, period.knownradius, iview_hex);
	    }
	}
    }
}

no_statistics()
{
    int u;
    Side *side = sidelist;

    for_all_unit_types(u) if (side->balance[u][FIRSTUNIT] > 0) return FALSE;
    return TRUE;
}

/* Invent global values as necessary. */

make_up_globals()
{
    if (Debug) printf("Going to make up some globals ...\n");
    global.time = 0;
    global.endtime = DEFAULTTURNS;
    global.setproduct = TRUE;
    global.leavemap = FALSE;
    global.numconds = 0;
    if (Debug) printf("... Done making up globals.\n");
}

/* Create some random sides with default characteristics. */

make_up_sides()
{
    int i;
    Side *side;

    if (Debug) printf("Going to make up some sides ...\n");
    for (i = 0; i < MAXSNAMES; ++i) snameused[i] = FALSE;
    for (i = 0; i < numgivens; ++i) {
	side = create_side(random_side_name(), humans[i], hosts[i]);
	/* Pretty bad if side creation fails... */
	if (side == NULL) abort();
	side->timeleft = global.giventime;
	side->timetaken = 0;
    }
    if (Debug) printf("... Done making up sides.\n");
}

/* If no units supplied with the map, then make some up and place them. */
/* There is an option to start with one or with a country. */
/* Favorite terrains are sorted by order of preference. */

make_up_units()
{
    int i;

    if (Debug) printf("Going to make up some units ...\n");
    for (i = 0; i < MAXUNAMES; ++i) unameused[i] = FALSE;
    place_countries();
    place_neutrals();
    if (Debug) printf("... Done making up units.\n");
}

give_machine_advantage(unit)
Unit *unit;
{
  int u = unit->type, x = unit->x, y = unit->y, t;
  int bestprod = 0, r;

  if (period.machineadvantage) {
    if (unit_at(x,y) == unit) {
      for_all_terrain_types(t) {
	if (utypes[u].productivity[t] > bestprod) {
	  bestprod = utypes[u].productivity[t];
	  set_terrain_at(x, y, t);
	}
      }
    }
    for_all_resource_types(r) {
      unit->supply[r] = utypes[u].storage[r];
    }
  }
}    
      
/* Place all the units belonging to countries.  Once placed, set ownership */
/* and production appropriately. */

place_countries()
{
    int x0, y0, u, t, i, num, first = period.firstutype, numleft[MAXUTYPES];
    Unit *unit, *firstunit;
    Side *side;

    /* Precompute some important info */
    for_all_unit_types(u) {
	inopen[u] = FALSE;
	for_all_terrain_types(t) {
	    if (utypes[u].favored[t] > 0) inopen[u] = TRUE;
	}
    }
    for_all_sides(side) {
        firstunit = NULL;
	find_a_place(&x0, &y0);
	if (Debug) printf("Country at %d,%d\n", x0, y0);
	countryx[side_number(side)] = x0;  countryy[side_number(side)] = y0;
/*	place_inhabitants(side, x0, y0); */
	if (first != NOTHING) {
	    unit = create_unit(first, random_unit_name(first));
	    firstunit = unit;
	    occupy_hex(unit, x0, y0);
	    init_supply(unit);
	    unit_changes_side(unit, side, FIRSTUNIT, -1);
	    unit->trueside = unit->side;
	    set_product(unit, period.firstptype);
	    set_schedule(unit);
	    if (!humanside(side))
	      give_machine_advantage(unit);
	}
	for_all_unit_types(u) {
	    num = utypes[u].incountry;
	    if (first == u) num--;
	    numleft[u] = num;
	    for (i = 0; i < num; ++i) {
		unit = create_unit(u, random_unit_name(u));
		if (!inopen[u] && (firstunit != NULL) &&
		    could_carry(first, u)) {
		  numleft[u]--;
		  unit_changes_side(unit, side, FIRSTUNIT, -1);
		  unit->trueside = unit->side;
		  occupy_hex(unit, x0, y0);
		  init_supply(unit);
		  if (!humanside(side))
		    give_machine_advantage(unit);
		} else if (place_unit(unit, x0, y0)) {
		    numleft[u]--;
		    if (first == NOTHING || !inopen[u]) {
			unit_changes_side(unit, side, FIRSTUNIT, -1);
			unit->trueside = unit->side;
		      }
		    if (!humanside(side))
		      give_machine_advantage(unit);
		  } else {
		    kill_unit(unit, -1);
		}
	    }
	}
	/* second pass for units that have to be occupants */
	for_all_unit_types(u) {
	    for (i = 0; i < numleft[u]; ++i) {
		unit = create_unit(u, random_unit_name(u));
		if (place_unit(unit, x0, y0)) {
		  if (!humanside(side))
		    give_machine_advantage(unit);
		    if (first == NOTHING || !inopen[u]) {
			unit_changes_side(unit, side, FIRSTUNIT, -1);
			unit->trueside = unit->side;
		      } else {
			/* give up finally - needs to be more informative */
			fprintf(stderr, "Can't place a %s!\n", utypes[u].name);
			exit(1);
		      }
		  }
	      }
	  }
      }
}

/* Work hard to find a place for a side's country.  First make some random *
/* trials, then start searching from the "center" of the map outwards. */
/* If neither approach works, things are too screwed up to keep going. */

find_a_place(cxp, cyp)
int *cxp, *cyp;
{
    int tries, x, y, diameter = (2 * period.countrysize + 1);

    for (tries = 0; tries < 200; ++tries) {
	x = RANDOM(world.width);
	x = wrap(x);
	y = RANDOM(world.height - diameter) + period.countrysize;
	if (good_place(x, y)) { 
	    *cxp = x;  *cyp = y;
	    if (Debug) printf("Country placed on try %d\n", tries);
	    return;
	}
    }
    if (search_area(world.width/2, world.height/2,
		    world.width/2+world.height/2, good_place, cxp, cyp, 1)) {
	if (Debug) printf("Country placed after search\n");
	return;
    } else {
	fprintf(stderr, "Can't place all the countries!\n");
	fprintf(stderr, "Try a bigger map, different map, or fewer sides.\n");
	exit(1);
    }
}    

/* Decide whether the given location is desirable for a country.  It should */
/* not be too near or too far from other sides' countries, and there must be */
/* enough terrain to place all the initial units.  In addition, the center */
/* of the country must have the right terrain for the firstutype. */

count_hexes(x, y)
int x, y;
{
    int u, t = terrain_at(x, y);

    for_all_unit_types(u) {
	if (probability(utypes[u].favored[t])) numhexes[u]++;
    }
}

good_place(cx, cy)
int cx, cy;
{
    bool toofar = TRUE, notfirst = FALSE;
    int c, px, py, u, toplace, allhexes, first = period.firstutype;

    for (c = 0; c < numsides; ++c) {
	px = countryx[c];  py = countryy[c];
	if (px > 0 && py > 0) {
	    notfirst = TRUE;
	    if (distance(cx, cy, px, py) < period.mindistance) return FALSE;
	    if (distance(cx, cy, px, py) < period.maxdistance) toofar = FALSE;
	}
    }
    if (toofar && notfirst) return FALSE;
    if (first != NOTHING &&
	!probability(utypes[first].favored[terrain_at(cx, cy)]))
	return FALSE;
    toplace = allhexes = 0;
    for_all_unit_types(u) numhexes[u] = 0;
    apply_to_area(cx, cy, period.countrysize, count_hexes);
    for_all_unit_types(u) {
	if (inopen[u] && utypes[u].incountry > numhexes[u]) return FALSE;
	toplace += utypes[u].incountry;
	allhexes += numhexes[u];
    }
    return (toplace < allhexes);
}

/* Place the populace on appropriate terrain within the country. */
/* If countries overlap, then flip coins to decide about intermix (heh-heh, */
/* so it resembles 17th-century Germany - more fun that way!). */
/* The loops are a standard regular hexagon filler. */

/*place_inhabitants(side, x0, y0)
Side *side;
int x0, y0;
{
    int x, y, x1, y1, x2, y2, dist = period.countrysize;

    y1 = interior(y0 - dist);
    y2 = interior(y0 + dist);
    for (y = y1; y <= y2; ++y) {
	x1 = x0 - (y < y0 ? (y - y1) : dist);
	x2 = x0 + (y > y0 ? (y2 - y) : dist);
	for (x = x1; x <= x2; ++x) {
	    if (ttypes[terrain_at(wrap(x), y)].inhabitants) {
		populations = TRUE;
		if (unpopulated(wrap(x), y) || flip_coin()) {
		    set_people_at(wrap(x), y, 8+side_number(side));
		    set_cover(side, wrap(x), y, 100);
		}
	    }
	}
    }
} */

/* The basic conditions that *must* be met by an initial unit placement. */

int tmputype;

valid_place(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);

    return ((unit == NULL && utypes[tmputype].favored[terrain_at(x, y)] > 0) ||
	    (unit != NULL && could_carry(unit->type, tmputype)));
}

/* Put a unit down somewhere in the designated area, following constraints */
/* on terrain and adjacency.  Returns success of placement. */

place_unit(unit, cx, cy)
Unit *unit;
int cx, cy;
{
    bool canmove = FALSE;
    int u = unit->type, t, tries, x, y, chance, r;
    int csize = period.countrysize;

    for_all_terrain_types(t) if (could_move(u, t)) canmove = TRUE;
    tmputype = u;
    for (tries = 0; tries < 500; ++tries) {
	x = RANDOM(2*csize+1) + cx - csize;  x = wrap(x);
	y = RANDOM(2*csize+1) + cy - csize;  y = interior(y);
	if (valid_place(x, y) && distance(cx, cy, x, y) <= csize) {
	    chance = 100;
	    t = terrain_at(x, y);
	    if (canmove && !could_move(u, t)) chance -= 90;
	    for_all_resource_types(r) {
		if (utypes[u].produce[r] > 0 && utypes[u].productivity[t] == 0)
		    chance -= 20;
	    }
	    if (adj_unit(x, y)) chance -= 70;
	    if (probability(chance)) {
		occupy_hex(unit, x, y);
		init_supply(unit);
		if (Debug) printf("Unit placed on try %d\n", tries);
		return TRUE;
	    }
	}
    }
    if (search_area(cx, cy, csize, valid_place, &x, &y, 1)) {
	occupy_hex(unit, x, y);
	init_supply(unit);
	if (Debug) printf("Unit placed after search\n");
	return TRUE;
    }
    return FALSE;
}

/* The number of neutral units is given by a parameter, and adjusted by */
/* the number of units assigned to specific countries.  The number is at */
/* least 1 (if nonzero density), even for small maps. */

place_neutrals()
{
    int u, num, i;

    for_all_unit_types(u) {
	if (utypes[u].density > 0) {
	    num = (((utypes[u].density * world.width * world.height) / 10000) -
		   (numsides * utypes[u].incountry));
	    num = max(1, num);
	    for (i = 0; i < num; ++i) {
		place_neutral(create_unit(u, random_unit_name(u)));
	    }
	}
    }
}

/* Neutral places should be uncommon in hostile terrain.  If we can't find */
/* a place, just blow it off and go to the next one. */

place_neutral(unit)
Unit *unit;
{
    int tries, x, y, u = unit->type;

    for (tries = 0; tries < 500; ++tries) {
	x = RANDOM(world.width);  y = RANDOM(world.height - 2) + 1;
	if ((probability(utypes[u].favored[terrain_at(x, y)])) &&
	    (unit_at(x, y) == NULL) &&
	    (!adj_unit(x, y) || flip_coin())) {
	    occupy_hex(unit, x, y);
	    init_supply(unit);
	    return;
	}
    }
    kill_unit(unit, ENDOFWORLD);
}

/* True if anybody already on given hex or adjacent to it. */

adj_unit(x, y)
int x, y;
{
    int d, x1, y1;

    for_all_directions(d) {
	x1 = wrap(x + dirx[d]);  y1 = y + diry[d];
	if (unit_at(x1, y1)) return TRUE;
    }
    return FALSE;
}

/* Give the unit what it is declared to have stockpiled already. */

init_supply(unit)
Unit *unit;
{
    int r, u = unit->type;

    for_all_resource_types(r) {
	unit->supply[r] =
	    (utypes[u].storage[r] * utypes[u].stockpile[r]) / 100;
    }
}

/* Pick any name not already used in the giant array of unit names. */
/* First try randomly, then sequentially. */

char *
random_unit_name(u)
int u;
{
    int i, n;

    if (utypes[u].named) {
	for (i = 0; i < period.numunames; ++i) {
	    if (!unameused[(n = RANDOM(period.numunames))]) {
		unameused[n] = TRUE;
		return unames[n];
	    }
	}
	for (i = 0; i < period.numunames; ++i) {
	    if (!unameused[i]) {
		unameused[i] = TRUE;
		return unames[i];
	    }
	}
	return NULL;
    } else {
	return NULL;
    }
}

/* Pick a side name not already used.  Don't get uptight if we ran out of */
/* names; perhaps a unit will lend a name, and certainly the players will */
/* notice and change side names manually! */

char *
random_side_name()
{
    int i, n;

    for (i = 0; i < period.numsnames; ++i) {
	if (!snameused[(n = RANDOM(period.numsnames))]) {
	    snameused[n] = TRUE;
	    return snames[n];
	}
    }
    for (i = 0; i < period.numsnames; ++i) {
	if (!snameused[i]) {
	    snameused[i] = TRUE;
	    return snames[i];
	}
    }
    return "???";
}

/* Quicky test needed in a couple places. */

saved_game()
{
    FILE *fp;

    if ((fp = fopen(SAVEFILE, "r")) != NULL) {
	fclose(fp);
	return TRUE;
    } else {
	return FALSE;
    }
}
