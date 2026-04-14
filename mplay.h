/* All the definitions needed by the machine code. */

/* the non-group */

#define NOGROUP 0

/* Goals for both groups and individuals. */

#define NOGOAL 0
#define DRIFT 1
#define HITTARGET 2
#define CAPTARGET 3
#define BESIEGE 4
#define OCCUPYHEX 5
#define EXPLORE 6
#define DEFEND 7
#define LOAD 8
#define APPROACH 9
#define RELOAD 10
#define FINDTRANS 11

/* router flags */

#define SAMEPATH 1
#define EXPLORE_PATH 2

extern bool delaymove;

typedef struct a_area {
  short number;           /* area number */
  short x, y;             /* center of area */
  short allied_units;     /* How many units on our side here. */
  short allied_makers;    /* How many of our makers */
  short neutral_makers;    /* How many of neutral makers */
  short makers;           /* Total number of makers */
  short unexplored;       /* Number of unseem hexes in area. */
  short allied_bases;     /* Total number of our bases, includes towns */
  short border;           /* True if this is a border area. */
  short nearby;           /* Is this close to a location we have bases. */
  short unit_request;     /* Priority on request for units. */
  struct a_unit *units[MAXUTYPES]; /* Units in area */
  short requested_units[MAXUTYPES]; /* units we should move to area. */
  short num_units[MAXUTYPES]; /* units we should move to area. */
} Area;

/* values for unit_request */
#define NO_UNITS 0
#define GUARD_BASE 1
#define PATROL_AREA 2
#define EXPLORE_AREA 3
#define DEFEND_AREA 4
#define GUARD_TOWN 6
#define GUARD_BORDER 2
#define GUARD_BORDER_TOWN 10
#define DEFEND_BASE 20
#define DEFEND_TOWN 50

/* This structure is where machine sides keep all the plans and planning */
/* related data. */
/* Group 0 is never actually used (a sort of a dummy for various purposes). */

typedef struct a_plan {
    short estimate[MAXSIDES][MAXUTYPES];  /* estimated numbers of units */
    short allies[MAXSIDES][MAXUTYPES];  /* strength of other alliances */
    short cx, cy;               /* "centroid" of all our units */
    short lastreplan;           /* last turn we rechecked the plans */
    short demand[MAXUTYPES];    /* worth of each utype w.r.t. strategy */
} Plan;

/* Encapsulate some pointer-chasing and casting messiness. */

#define side_plan(s) ((Plan *) (s)->plan)

/* Malloced integer array accessors and modifers. */

#define aref(m,x,y) ((m)[(x)+world.width*(y)])

#define aset(m,x,y,v) ((m)[(x)+world.width*(y)] = (v))

extern int movetries;

extern int unit_count;
extern int explore_priority;
extern int defend_priority;
extern int attack_priority;
extern bool worths_known;

extern int evaluate_hex(), maximize_worth();

extern char shortbuf[BUFSIZE];         /* buffer for short unit description */

/* General collections of numbers used by all machine players. */

extern int fraction[MAXTTYPES];        /* percentages of terrain types in world */
extern int unit_hexes[MAXUTYPES];      /* hexes of terrain in world unit can move on */
extern int bw[MAXUTYPES];              /* how much is a unit worth */
extern int bhw[MAXUTYPES][MAXUTYPES];  /* basic worth for hitting */
extern int bcw[MAXUTYPES][MAXUTYPES];  /* basic worth for capturing */
extern int maxoccupant[MAXUTYPES];     /* total capacity of a transport */
extern int ave_build_time[MAXUTYPES];  /* Average length of time to build a unit */
extern int *localworth;                /* for evaluation of nearby hexes */

#ifdef REGIONS
extern short *unit_region[MAXUTYPES];  
     /* Regions a unit within which unit can move */
extern int *unit_region_size[MAXUTYPES];
    /* How many hexes in each region. not necessarily contiguous,
       but decent estimate.  Allows lakes to be spotted. */
extern short num_regions[MAXUTYPES];   /* Number of regions for this unit type */
extern short *units_in_region[MAXUTYPES]; /* Number of units currently in region */
extern long munit_regions;            /* Regions current unit can reach */
typedef struct _coords
{
  int x, y;
} coords;

#endif /* REGIONS */

extern Area *area_info;       /* info about areas, shared for all sides */
extern short areas_wide;      /* number of areas in x direction */
extern short areas_high;      /* number of areas in y direction */
#define NUMTOPAREAS 5
extern Area *top_areas[NUMTOPAREAS];       /* Areas requesting the most units. */

extern bool can_move_in_dir[NUMDIRS];  /* directions it make sence to try to move in */

/* Some basic ranges for prioritizing a units tasks. */
#define EXPLORE_VAL 2500
#define FAVORABLE_COMBAT 5000
#define UNFAVORABLE_COMBAT 4000
#define CAPTURE_MAKER 50000
#define CAPTURE_OTHER 20000
#define PATROL_VAL 100
#define HEAD_FOR_GOAL 2300
#define MEET_TRANSPORT 3000

extern int bestworth, bestx, besty;

extern Unit *munit;                    /* Unit being decided about */

extern Side *mside;                    /* Side whose unit is being decided about */
extern bool base_building;             /* true if base building is possible. */

extern int explore_priority, defend_priority, attack_priority;
extern bool worths_known;
extern int route_max_distance;

/* in mutil.c */
extern char *unit_desig();
extern int units_nearby();
extern bool survive_to_build_base(), exact_survive_to_build_base();
extern bool base_nearby(), any_base_nearby();
extern bool occupant_can_capture();
extern bool occupant_can_capture_neighbor();
extern bool find_closest_unit(), can_produce(), can_move();
extern int out_of_ammo();
extern int fullness(), survival_time();
extern bool haven_p(), shop_p(), good_haven_p();
extern Mplan *find_route(), *find_route_aux(), *make_plan_for_route();
extern Mplan *make_plan_step();
extern void free_plan();
extern bool follow_plan();
extern int unit_strength();
extern void update_areas();

/* in mproduce.c */
extern bool change_machine_product();
extern int machine_product();

