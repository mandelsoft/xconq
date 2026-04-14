/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Initialization of machine players. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"
#include "mplay.h"

/* Init used by all machine players.  Precompute useful information */
/* relating to unit types in general, and that usually gets referenced */
/* in inner loops. */

init_mplayers()
{
    int u, u1, u2, t, r1, numbuilders;
    Side *side;
    bool tmp;

    enter_procedure("init_mplayers");
    localworth = (int *) malloc(world.width*world.height*sizeof(int));
    for_all_terrain_types(t) {
	fraction[t] = ((ttypes[t].maxalt - ttypes[t].minalt) *
		       (ttypes[t].maxwet - ttypes[t].minwet)) / 100;
    }
    /* check for bases */
    for_all_unit_types(u1) {
      utypes[u1].is_base = FALSE;
      tmp = FALSE;
      for_all_resource_types(r1) {
	if (utypes[u1].produce[r1] > 0) {
	  tmp = TRUE;
	  continue;
	}
      }
      if (tmp)
	for_all_unit_types(u2) {
	  if ((u1 != u2) && could_carry(u1,u2)) {
	    utypes[u1].is_base = TRUE;
	    continue;
	  }
	}
    }
    /* Note that is_base_builder is set to the type of base that can */
    /* be built.  That means that unit zero can not be a base which */
    /* can be built. */
    for_all_unit_types(u1) {
      if (utypes[u1].defense_worth > 0)
	worths_known = TRUE;
      utypes[u1].is_transport = FALSE;
      utypes[u1].is_carrier = FALSE;
      utypes[u1].is_base_builder = FALSE;
      utypes[u1].can_make = FALSE;
      utypes[u1].can_capture = FALSE;
      numbuilders = 0;
      ave_build_time[u1] = 0;
      for_all_unit_types(u2) {
	if (utypes[u2].is_base && (utypes[u1].make[u2] > 0) &&
	     global.setproduct) {
	  utypes[u1].is_base_builder = u2;
	  base_building = TRUE;
	}
	if (utypes[u1].speed > 0 && could_carry(u1,u2)) {
	  utypes[u1].is_transport = TRUE;
	}
	if (utypes[u2].make[u1]) {
	  numbuilders++;
	  ave_build_time[u1] += utypes[u2].make[u1];
	  utypes[u2].can_make = TRUE;
	}
	if (could_capture(u1, u2))
	  utypes[u1].can_capture = TRUE;
      }
      if (numbuilders > 0)
	ave_build_time[u1] /= numbuilders;
    }
    /* a carrier is a unit that is a mobile base, but it can not
       move a unit anywhere it could not go itself. */
    for_all_unit_types(u1) 
      if (utypes[u1].is_transport) {
	utypes[u1].is_carrier = TRUE;
	for_all_unit_types(u2)
	  if (could_carry(u1, u2)) 
	    for_all_terrain_types(t)
	      if (utypes[u1].moves[t] >= 0 &&
		  utypes[u2].moves[t] < 0)
		utypes[u1].is_carrier = FALSE;
      }
    for_all_unit_types(u) 
	bw[u] = basic_worth(u);
    /* repeat to get makers values right */
    /* still not right if makers can produce makers */

    for_all_unit_types(u)   
	bw[u] = basic_worth(u);
    for_all_unit_types(u) {
	maxoccupant[u] = 0;
	for_all_unit_types(u2) {
	    bhw[u][u2] = basic_hit_worth(u, u2);
	    bcw[u][u2] = basic_capture_worth(u, u2);
	    maxoccupant[u] += utypes[u].capacity[u2];
	}
    }
#ifdef REGIONS
    determine_regions();
#endif
    init_areas();
    /* tell us about how things rated */
    if (Debug) {
	for_all_terrain_types(t) {
	    printf("%3d%% ", fraction[t]);
	}
	printf("\n\n");
	for_all_unit_types(u) {
	    for_all_unit_types(u2) printf("%5d", bhw[u][u2]);
	    printf("\n");
	}
	printf("\n");
	for_all_unit_types(u) {
	    for_all_unit_types(u2) printf(" %4d", bcw[u][u2]);
	    printf("\n");
	}
	printf("\n");
	printf("(unit, bases, transports, carriers, worth): \n");
	for_all_unit_types(u)
	    printf(" %c (%d, %d, %d) %d \n", utypes[u].uchar, utypes[u].is_base,
		   utypes[u].is_transport, utypes[u].is_carrier, bw[u]);
	printf("\n");
    }
    /* For all sides, because human might use "robot" option */
    for_all_sides(side) {
	side->plan = (long) (Plan *) malloc(sizeof(Plan));
	side_plan(side)->cx = side_plan(side)->cy = 0;
	side_plan(side)->lastreplan = -100;
    }
    exit_procedure();
}

/* A crude estimate of having one type of unit. */

int basic_worth(u)
int u;
{
  int worth = 0, u2, r, range;
  
  worth += utypes[u].hp * 10;
  for_all_unit_types(u2) {
    if (utypes[u].make[u2] > 0)
      worth += (bw[u2] * (utypes[u].maker ? 70 : 10)) / utypes[u].make[u2];
    if (utypes[u].capacity[u2] > 0)
      worth += (1 + utypes[u].speed) * utypes[u].capacity[u2] *
	(utypes[u].is_base ? 10 : 1) * bw[u2] / 30;
  }
  range = 12345;
  for_all_resource_types(r) {
    worth += utypes[u].produce[r] * (utypes[u].is_base ? 4 : 1);
    if (utypes[u].tomove[r] > 0)
      range = min(range, utypes[u].storage[r] / utypes[u].tomove[r]);
    if (utypes[u].consume[r] > 0) 
      range = min(range, utypes[u].speed *
		  utypes[u].storage[r] / utypes[u].consume[r]);
  }
  worth += utypes[u].speed * utypes[u].hp;
  worth += (range == 12345 ? max(world.width, world.height) : range)
    * utypes[u].hp / max(1, 10 - utypes[u].speed);
  for_all_unit_types(u2)
    worth += (worth * utypes[u].capture[u2] * (utypes[u2].maker ? 2 : 1)) /
      150;
  worth = isqrt(worth);
  if (Debug) printf("unit %c worth %d \n ", utypes[u].uchar, worth);
  return worth;
}
  
  

/* A crude estimate of the payoff of one unit type hitting on another type. */
/* This is just for general estimation, since actual worth may depend on */
/* damage already sustained, unit's goals, etc. */

basic_hit_worth(u, e)
int u, e;
{
    int worth, anti;

    worth = utypes[u].hit[e] * min(utypes[e].hp, utypes[u].damage[e]);
    if (utypes[e].hp > utypes[u].damage[e]) {
	worth /= utypes[e].hp;
    }
    if (period.counterattack) {
	anti = utypes[e].hit[u] * min(utypes[u].hp, utypes[e].damage[u]);
	if (utypes[u].hp > utypes[e].damage[u]) {
	    anti /= utypes[u].hp;
	}
    }
    else anti = 0;
    if (ave_build_time[u] > 0 && ave_build_time[e] > 0)
      worth =  (worth * ave_build_time[e] - anti * ave_build_time[u]) /
	(ave_build_time[u] + ave_build_time[e]);
    else worth -= anti;
    return worth;
}

/* A crude estimate of the payoff of one unit type trying to capture. */

basic_capture_worth(u, e)
int u, e;
{
    int worth = 0;

    if (could_capture(u, e)) {
	worth = utypes[u].capture[e] * bw[e];
    }
    return worth;
}

#ifdef REGIONS
/* Fill in a region */
void fill_region(x, y, u, region)
int x, y, u, region;
{
  coords *oldborder, *newborder;
  int oldn, newn, i, d;
  int nx, ny;
  int moreleft=1;

  /* intialize arrays */
  oldborder = (coords *) malloc(sizeof(coords));
  newborder = (coords *) malloc(sizeof(coords));
  oldborder[0].x = x; oldborder[0].y = y;
  oldn = 1; newn = 0;
  aset(unit_region[u], x, y, region);

  while(moreleft)
    {
      moreleft = 0;
      for (i=0; i<oldn; i++)
	{
	  for_all_directions(d)
	    {
	      nx = wrap(oldborder[i].x + dirx[d]);
	      ny = oldborder[i].y + diry[d];
	      if (aref(unit_region[u], nx, ny) == -1 &&
		  could_move(u, terrain_at(nx, ny)))
		{
		  newborder= (coords *)
		    realloc((char *) newborder, (++newn)*sizeof(coords));
		  newborder[newn-1].x = nx;
		  newborder[newn-1].y = ny;
		  aset(unit_region[u], nx, ny, region);
		  moreleft = 1;
		}
	    }
	}
      free(oldborder);
      oldborder = newborder;
      oldn = newn;
      newborder = (coords *) malloc(sizeof(coords));
      newn = 0;
    }
  free(oldborder);
  free(newborder);
  return;
}


/* Find continuous regions in which a unit can travel. */

determine_regions()
{
  int x, y, region, u, u1, i, t;
  long terr_profile[MAXUTYPES];
  bool already_done[MAXUTYPES];
  int same_as[MAXUTYPES];
  bool atv[MAXUTYPES];

  enter_procedure("Determine regions");

  for_all_unit_types(u) {
    terr_profile[u] = 0;
    atv[u] = TRUE;
    for_all_terrain_types(t)
      if (could_move(u, t))
	{terr_profile[u] |= 1<<t;}
      else atv[u] = FALSE;
    for (u1 = 0; u1 < u; u1++)
      if (terr_profile[u] == terr_profile[u1])
	break;
    if (u1 < u) {
      unit_region[u] = unit_region[u1];
      if (Debug)
	printf("Regions for %s's same as %s's\n",
	       utypes[u].name, utypes[u1].name);
      already_done[u] = TRUE;
      same_as[u] = u1;
    } else {
      already_done[u] = FALSE;
      unit_region[u] =
	(short *) malloc(world.width*world.height*sizeof(short));
      for_all_hexes(x,y)
	if (between(1, y, world.height - 2))
	  aset(unit_region[u], x, y, (atv[u] ? 0 : -1));
	else aset(unit_region[u], x, y, -2);
              /* top and bottom always off bounds */
    }
  }
  for_all_unit_types(u){
    if (!already_done[u]) {
      region = 0;
      unit_hexes[u] = 0;
      if (atv[u]) {
	region = 1;
	/* unit_hexes[u] = world.width * (world.height - 2); */
	} else {
	  for_all_hexes(x,y)
	    if (aref(unit_region[u], x, y) == -1 &&
		could_move(u, terrain_at(x, y))) {
	      if (Debug) printf("Filling region %d for %s's\n",
				region, utypes[u].name);
	      fill_region(x, y, u, region++);
	    }
	}
      num_regions[u] = region;
      unit_region_size[u] = (int *) malloc(num_regions[u] * sizeof(int));
      units_in_region[u] = (short *) malloc(num_regions[u] * sizeof(short));
      for (i = 0; i < num_regions[u]; i++)
	unit_region_size[u][i] = 0;
      for_all_hexes(x, y)
	if ((i = aref(unit_region[u], x, y)) >= 0){
	  unit_region_size[u][i]++;
	  unit_hexes[u]++;
	}
      if (Debug) printf("Found %d hexes for %s's\n",
			unit_hexes[u], utypes[u].name);
    } else {
      num_regions[u] = num_regions[same_as[u]];
      unit_region_size[u] = unit_region_size[same_as[u]];
      units_in_region[u] = (short *) malloc(num_regions[u] * sizeof(short));
      unit_hexes[u] = unit_hexes[same_as[u]];
    }
  }
  exit_procedure();
}
#endif

init_areas()
{
  int i, j, num;
  Side *side;
  
  areas_wide = (world.width + AREA_SIZE - 1) / AREA_SIZE;
  areas_high = (world.height + AREA_SIZE - 1) / AREA_SIZE;
  area_info = (Area *) malloc(areas_wide * areas_high * sizeof(Area));
  for_all_sides(side) {
      side->areas = (Control_area *) malloc(areas_wide * areas_high *
					    sizeof(Control_area));
    }
  for (i = 0; i < areas_wide; i++) 
    for (j = 0; j < areas_high ; j++) {
      num = j * areas_wide + i;
      area_info[num].number = num;
      area_info[num].x = (AREA_SIZE * i +
			  min(AREA_SIZE * (i+1), world.width)) / 2;
      area_info[num].y = (AREA_SIZE * j +
			  min(AREA_SIZE * (j+1), world.height)) / 2;
      for_all_sides(side) {
	side->areas[num].enemy_strength = 0;
	side->areas[num].enemy_seen_recently = 0;
	side->areas[num].units_lost = 0;
	side->areas[num].capturers_approaching = 0;
	side->areas[num].capture_time = 10000;
      }
    }
}

