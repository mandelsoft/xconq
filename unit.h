/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Definitions about units and their orders. */

#define NOTHING (MAXUTYPES+1)  /* A number guaranteed to be a non-unit. */
/*** (UK) insert -> ***/
#define ALL (MAXUTYPES+2)  /* A number guaranteed to be a non-unit. */
/*** <- insert ***/

/* Unit order types.  Each type has a letter, name, short name, and some */
/* parameter types that guide how the order will be input. */

/*** (UK) insert -> ***/
#define MAXGROUP 20  /* max. length of group name */
/*** <- insert ***/
/*** (HW) change -> ***/
#ifndef NO_DISEMBARK
/*** was:
#ifdef NO_DISEMBARK
*** <- change ***/
/*** (UK) change -> ***/
#define NUMORDERTYPES 21
#else
#define NUMORDERTYPES 20
#endif
/*** was:
#define NUMORDERTYPES 19
#else
#define NUMORDERTYPES 18
#endif
*** <- change ***/

#define NONE     0
#define AWAKE    1
#define SENTRY   2
#define MOVEDIR  3
#define MOVETO   4
#define EDGE     5
#define FOLLOW   6
#define PATROL   7
#define ATTACK	 8 /*NYI*/
#define EMBARK   9
#define FILL		10
#define MOVETOTRANSPORT	11
#define MOVETOUNIT	12 /* NYI */
#define RETURN	13
#define DELAY	14	/* pseudo-order */
/*** (UK) insert -> ***/
#define RPICKUP  15
#define LEAVE   16
#define UNLOAD  17
#define GROUP   18
#define APICKUP 19
#define RGIVE	20
#define RTAKE	21
/*** <- insert ***/
#ifndef NO_DISEMBARK
/*** (UK) change ->  ***/
#define 	DISEMBARK 22 
/** was:
#define 	DISEMBARK 18 
*** <- change ***/
#endif


#ifndef NO_DISEMBARK
#define ORDERNAMES \
  { "None", "Awake", "Sentry", "Dir Move", "Move to", \
    "Follow Edge", "Follow Leader", "Patrol", "Attack", \
    "Embark", "Fill", "Move to transport", "Move to unit", \
    "Return", "Delay" , \
/*** (UK) insert -> ***/ \
    "Recursivly Pickup", "Leave", "Unload", "Set group to", "Pickup", \
    "Give", "Take", \
/*** <- insert ***/ \
    "Disembark" }
#else
#define ORDERNAMES \
  { "None", "Awake", "Sentry", "Dir Move", "Move to", \
    "Follow Edge", "Follow Leader", "Patrol", "Attack", \
    "Embark", "Fill", "Move to transport", "Move to unit", \
    "Return", "Delay", \
/*** (UK) insert -> ***/ \
    "Recursivly Pickup", "Leave", "Unload", "Set group to", "Pickup", \
    "Give", "Take" \
/*** <- insert ***/ \
  }	
#endif


/* Types of arguments that orders can have. */

#define NOARG     0
#define DIR       1
#define POS       2
#define LEADER    3
#define WAYPOINTS 4
#define EDGE_A	  5
/*** (UK) insert -> ***/
#define STRING	  6
#define AMOUNT	  7
/*** <- insert ***/

/*** (HW) change -> ***/
#ifndef NO_DISEMBARK
/*** was:
#ifdef NO_DISEMBARK
*** <- change ***/
#define ORDERARGS \
  { NOARG, NOARG, NOARG, DIR, POS, EDGE_A, LEADER, WAYPOINTS, \
      NOARG, NOARG, NOARG, NOARG, LEADER, NOARG, NOARG , \
/*** (UK) insert -> ***/ \
      NOARG, NOARG, NOARG, STRING, AMOUNT, \
      AMOUNT, AMOUNT, \
/*** <- insert ***/ \
      NOARG }
#else
#define ORDERARGS \
  { NOARG, NOARG, NOARG, DIR, POS, EDGE_A, LEADER, WAYPOINTS, \
      NOARG, NOARG, NOARG, NOARG, LEADER, NOARG, NOARG, \
/*** (UK) insert -> ***/ \
      NOARG, NOARG, NOARG, STRING, AMOUNT, \
      AMOUNT, AMOUNT \
/*** <- insert ***/ \
      }
#endif

#ifdef NO_DISEMBARK
#define ORDERARGNAMES \
  { "no args", "no args", "no args", "direction", \
    "destination", "direction", "unit", "waypoints", \
      "no args", "no args", "no args", "no args", "unit", \
      "no args", "no args", \
/*** (UK) insert -> ***/ \
      "no args", "no args","no args","string","amount", \
      "amount", "amount" \
/*** <- insert ***/ \
      }
#else
#define ORDERARGNAMES \
  { "no args", "no args", "no args", "direction", \
    "destination", "direction", "unit", "waypoints", \
      "no args", "no args", "no args", "no args", "unit", \
      "no args", "no args" , \
/*** (UK) insert -> ***/ \
      "no args", "no args","no args", "string", "amount", \
      "amount", "amount", \
/*** <- insert ***/ \
      "no args" }
#endif

/*** (UK) insert -> ***/
#define SORDERTYPES  { "none","product","enter","follow" }
/*** <- insert ***/

/* Number of points possible in order parameters.  If actual number not */
/* implicit in order type, then will need a count field somewhere. */

#define NUMWPS 2

/* Bit-encoded flags that specify's units behavior when under orders. */

#define ENEMYWAKE     0x01
#define NEUTRALWAKE   0x02
#define SUPPLYWAKE    0x04
#define ATTACKUNIT    0x08
#define SHORTESTPATH  0x10

#define ALLFLAGS      0x1f

#define NORMAL  ALLFLAGS

/* tiny hack structure */

typedef struct a_pt {
    short x, y;
} Pt;

typedef struct a_edge {
    short forward;	/* move direction that follows the edge */
    short ccw;		/* 1=edge on left, -1=edge on right,
			   0 edge both sides (urgh) */
} Edge;

/* Definition of an "order", which is the means by which a unit keeps */
/* track of what it should do.  Needs the encapsulation because orders */
/* show up in both units and in collections of standing orders. */

typedef int  unit_id;

/* if any pointers ever appear in this structure the procedure
   copy_orders will have to be modified */
typedef struct a_order {
    short type;               /* type of order (awake, random, etc) */
    short rept;               /* number to repeat order */
    short flags;              /* bit vector of special flags */
    union {
	short dir;            /* a direction */
	unit_id leader_id;    /* id of leader.  Use id in case it dies. */
	Pt pt[NUMWPS];        /* small array of coordinates */
	Edge edge;            /* edge following parameters*/
/*** (UK) insert -> ***/
	int  amount;	      /* amount of units */
	char string[MAXGROUP]; /* group name */
/*** <- insert ***/
    } p;                      /* parameters of an order */
    short morder;             /* general order for machine controlling unit */
} Order;

/* A standing order is actually an array with an order for each unit type. */
/* The structure is allocated only when a standing order is given. */

/*** (UK) insert -> ***/
typedef struct s_order_entry {
  char  *group;
  struct s_condition_entry *cond;
  struct s_order_entry *next;
  bool handled;
} StandingOrderEntry;

#define C_NONE		0	/* unconditional command */
#define C_GT		10	/* more than given amount of units in transp */
#define C_NPICKUP	20	/* nothing to pickup in transport */
#define C_NEMBARK	21	/* nothing to get into */
#define C_NFULL		30	/* unit is not full */
#define C_NTFULL	31	/* transport is not full */
#define C_TFULL		39	/* transport is already full */
#define C_FULL		40	/* unit is already full */
#define C_EMBARK	49	/* something to get into */
#define C_PICKUP	50	/* something to pickup  */
#define C_LE		60	/* no more than given amount */

typedef struct a_scondition {
  int type;
  int priority;
  union {
    int amount;
    short utypes[MAXUTYPES];
  } d;
} SCondition;

typedef struct s_condition_entry {
  SCondition cond;
  Order *order;
  struct s_condition_entry *next;
} StandingConditionEntry;
/*** <- insert ***/ 

typedef struct s_order {
/*** (UK) change -> ***/
    StandingOrderEntry *p_orders[MAXUTYPES]; /* product order */
    StandingOrderEntry *e_orders[MAXUTYPES]; /* embark order (UK) */
    StandingOrderEntry *f_orders[MAXUTYPES]; /* follow order (UK) */
/*** was:
    Order *orders[MAXUTYPES];
*** <- change ***/
} StandingOrder;

/* A plan is a sequence of actions that the unit will perform. */

typedef struct a_unitplan {
  short type;      /* type of step */
  short x,y;       /* goal coordinates of step */
  short unit_id;   /* an associated unit, usually transport */
  struct a_unitplan *next; /* next step in plan */
} Mplan;

/* plan types */
#define PMOVE 1  /* move to the indicated location */
#define PMOVEUNIT 2 /* move to the indicated unit */
#define PCAPTURE 3 /* capture location */
#define PATTACK 4 /* attack location */
#define PEMBARK 5 /* embark on the unit, PMOVEUNIT implies this */
#define PBASE 6 /* build a base */


/* This structure should be small, because there may be many of them. */
/* On the other hand, changing shorts to chars would entail fiddling with */
/* mapfile code, so beware. */

typedef struct a_unit {
    /* Level 1 detail */
    short type;                /* type (army, ship, etc) */
    char *name;                /* the name, if given */
/*** (UK) insert -> ***/
    char *group;               /* the group, if given */
/*** <- insert ***/
    short x, y;                /* position of unit on map */
    struct a_side *side;       /* whose side this unit is on */
    /* Level 2 detail */
    unit_id id;                /* truly unique id number */
    short number;              /* semi-unique id number */
    struct a_side *trueside;   /* whose side this unit is really on */
    short hp;                  /* how much more damage it can take */
    short quality;             /* "veteran-ness" */
    short product;             /* type of unit this unit is producing */
    short schedule;            /* when the unit will be done */
    short built;               /* how many units of current type made so far */
    short next_product;        /* what should we build after this product */
    struct a_unit *transport;  /* pointer to transporter if any */
    short supply[MAXRTYPES];   /* how much supply we're carrying */
    /* Level 3 detail */
    short movesleft;           /* how many moves left in this turn */
    short actualmoves;         /* hexes actually covered this turn */
    short lastdir;             /* last direction of move */
    short awake;               /* true if unit temporarily awake */
    Order orders;              /* current orders being carried out */
    StandingOrder *standing;   /* pointer to collection of standing orders */
    /* Never saved */
    short area;                /* area this unit will call home. */
    short goal;                /* personal goal of unit */
    short gx, gy;              /* current goal position */
    struct a_unit *occupant;   /* pointer to first unit being carried */
    struct a_unit *nexthere;   /* pointer to fellow occupant */
    struct a_unit *prev;       /* previous unit in list of all units */
    struct a_unit *next;       /* next unit in list of all units */
    struct a_unit *mlist;      /* next unit in machine lists of units */
    struct a_unit *nextinarea; /* next unit of this type in area. */
/*** (UK) insert -> ***/
    struct a_unit *last_unit;  /* last transport before move  */
/*** <- insert ***/
    struct a_unitplan *plan;   /* plan for machine units */
    long priority;              /* how important is the plan */
    short prevx, prevy;        /* where were we last */
    short selection_status;    /* Have we figured out what to do with this unit yet. */
    /* remember why you woke up */
    short wakeup_reason;       /* reason unit woke up */
    struct a_side *waking_side;
    short waking_type;
    /* Make sure machine does not infinite loop */
    int move_turn;
    int move_tries;

    /* deal with infinite loops caused by surrendering! */
    int loop_check;
} Unit;

#define WAKEOWNER 0
#define WAKEENEMY 1
#define WAKERESOURCE 2
#define WAKETIME 3
#define WAKELEADERDEAD 4
#define WAKELOST 5
/* Wake up completely.  Return full control to owner.  Other waking */
/* modes will leave control to the machine if unit is in auto mode. */
#define WAKEFULL 6

/* Some convenient macros. */

#define for_all_units(s,v) for (s = &neutral_placeholder; \
			      s != NULL; s = s->next) \
  for (v = s->unithead->next; v != s->unithead ; v = v->next)

#define for_all_side_units(side, v) \
    for (v = side->unithead->next; v != side->unithead ; v = v->next)

#define for_all_occupants(u1,u2) \
  for (u2 = u1->occupant; u2 != NULL; u2 = u2->nexthere)

#define alive(u) ((u)->hp > 0)

#define cripple(u) ((u)->hp <= utypes[(u)->type].crippled)

#define neutral(u) ((u)->side == NULL)

#define producing(u) ((u)->product != NOTHING)

#define busy(u) (producing(u) || (u)->schedule > 0)

#define idled(u) (utypes[(u)->type].maker && !busy(u))

#define mobile(u) (utypes[u].speed > 0)

#define could_occupy(u,t) (could_move(u,t))

#if 0 /* (UK) */
/* orders that don't consume moves until the very end of the turn */
#define delayed_order(o) ( (o) == SENTRY || (o) == EMBARK || (o) == FILL || \
			  (o) == DELAY )
/* orders that mean the unit will stay in one place for a while */
#define napping_order(o) ( (o) == SENTRY || (o) == EMBARK || (o) == FILL )

#define can_move_unit(s,u) ((u) != NULL && (u)->side == (s) && alive(u) && \
		(((u)->movesleft > 0 && !delayed_order((u)->orders.type)) \
		 || idled(u)))
#endif

/*** (UK) insert -> ***/
#define is_awake(u) ((u)->orders.type == AWAKE || (u)->orders.type == NONE || (u)->awake)
/*** <- insert ***/

/* Unit variables. */

extern Unit *unitlist, *tmpunit;
extern Unit *create_unit(), *read_basic_unit(), *random_start_unit();
extern Unit *find_unit();

extern int numunits, orderargs[];

extern char *ordernames[];

extern void init_units();
extern bool can_type_carry();
extern bool can_carry();
extern void occupy_unit(), leave_unit(), leave_hex();
extern bool occupy_hex();
extern bool embark_unit();
extern void set_product(), set_schedule(), flush_dead_units(), sort_units();
extern bool low_supplies();
extern void show_standing_order();
extern char *summarize_units();
extern Unit *create_unit(), *read_basic_unit(), *random_start_unit();
extern int find_unit_char(), build_time();
extern char *random_unit_name(), *unit_handle(), *short_unit_handle();
extern char *make_unit_name(), *order_desig();
extern void cancel_build();
extern Unit *first_unit(), *next_unit();
extern void insert_unit(), delete_unit();
/*** (UK) insert -> ***/
extern StandingOrderEntry **get_current_sorder_array();
extern StandingOrderEntry *search_order_entry();
extern StandingConditionEntry *search_condition_entry();
extern Order *select_standing_orders();
extern char *cond_desig();
extern char *sorder_types[];
extern char *cond_type();
extern char *cond_spec();
/*** <- insert ***/ 
