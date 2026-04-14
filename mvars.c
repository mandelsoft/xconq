/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This is the set of global variables available to the machine player
routines. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "unit.h"
#include "side.h"
#include "mplay.h"

int unit_count;

char shortbuf[BUFSIZE];         /* buffer for short unit description */

/* General collections of numbers used by all machine players. */

bool can_move_in_dir[NUMDIRS];  /* directions it make sence to try to move in */

int bestworth = -10000, bestx, besty;

Unit *munit;                    /* Unit being decided about */

Side *mside;                    /* Side whose unit is being decided about */
int explore_priority = 100;  /* how much do we want to explore, defend, or attack */
int defend_priority = 0;
int attack_priority = 0;
/* General collections of numbers used by all machine players. */

int fraction[MAXTTYPES];        /* percentages of terrain types in world */
int unit_hexes[MAXUTYPES];      /* hexes of terrain in world unit can move on */
int bw[MAXUTYPES];              /* how much is a unit worth */
int bhw[MAXUTYPES][MAXUTYPES];  /* basic worth for hitting */
int bcw[MAXUTYPES][MAXUTYPES];  /* basic worth for capturing */
int maxoccupant[MAXUTYPES];     /* total capacity of a transport */
int ave_build_time[MAXUTYPES];  /* Average length of time to build a unit */
int *localworth;                /* for evaluation of nearby hexes */
#ifdef REGIONS
short *unit_region[MAXUTYPES];  
     /* Regions a unit within which unit can move */
int *unit_region_size[MAXUTYPES];
    /* How many hexes in each region.  Regions not necessarily contiguous,
       but decent estimate.  Allows lakes to be spotted. */
short num_regions[MAXUTYPES];   /* Number of regions for this unit type */
short *units_in_region[MAXUTYPES]; /* Number of units currently in region */
long munit_regions;            /* Regions current unit can reach */
#endif
Area *area_info;       /* info about areas, shared for all sides */
short areas_wide;      /* number of areas in x direction */
short areas_high;      /* number of areas in y direction */
Area *top_areas[NUMTOPAREAS];       /* Areas requesting the most units. */

bool base_building = FALSE;     /* can some unit build a base */

bool worths_known = FALSE;  /* does the period file have portions info */
