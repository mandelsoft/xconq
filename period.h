/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Structures containing period parameters. */
/* To simplify period compilation, the slots are arranged so that scalar */
/* values are generally grouped together.  However, we're not too hardnosed */
/* about it, and do segregate by general classes of values.  In any case, */
/* the period compiler *must* be kept consistent with these structures. */

/* There is only one period object, but it's useful to get all global */
/* period parameters glued into a single structure. */

typedef struct a_period {
    char *name;              /* name of period */
    char **notes;            /* designer's notes for period */
    char *fontname;          /* name of font for period (optional) */
    short countrysize;       /* radius of country in hexes */
    short mindistance;       /* distance between countries in hexes */
    short maxdistance;       /* distance between countries in hexes */
    short numutypes;         /* number of unit types */
    short numrtypes;         /* number of resource types */
    short numttypes;         /* number of terrain types */
    short numsnames;         /* number of random side names */
    short numunames;         /* number of random unit names */
    short firstutype;        /* starting unit if only one */
    short firstptype;        /* what it will try to produce (check plaus) */
    short knownradius;       /* area around the country that has been seen */
    short allseen;           /* true if all units always seen */
    short counterattack;     /* true if attacks go both ways */
    short capturemoves;      /* true if captured units get to move immediately */
    short nukehit;           /* how big a hit to qualify as nuke */
    short neutrality;        /* how much more difficult to get neutrals */
    short efficiency;        /* efficiency of unit recycling */
    short altroughness;      /* fractal dimension for altitude */
    short wetroughness;      /* fractal dimension for moisture */
    short machineadvantage;  /* Should we set the machine up in a
				superiour position. */
    short defaultterrain;    /* type of terrain to subst if unknown */
    short edgeterrain;       /* type of terrain to put on N/S edge */
    short spychance;         /* chance of spying each turn */
    short spyquality;        /* percentage of info gathered when spying */
    short leavemap;	     /* true if units can leave the map */
    short repairscale;	     /* how many hps are repaired each time */
} Period;

/* Unit type descriptions go into a single structure.  This structure */
/* contains every single piece of information about all unit types. */

typedef struct utype {
    char uchar;              /* character used in prompts */
    char *name;              /* the name (frequently used!) */
    char *help;              /* one-line description */
    char **notes;            /* longer description */
    char *bitmapname;        /* name of icon file (optional) */
    /* attributes for initialization */
    short incountry;         /* number that a side starts out with */
    short density;           /* number of units per ten thousand hexes */
    short named;             /* true if unit gets randomly-assigned name */
    short alreadyseen;       /* true if unit seen at the outset */
    short favored[MAXTTYPES];  /* chance to be found on given terrain */
    short stockpile[MAXRTYPES];  /* percentage of supply at outset */
    /* attributes for revolt phase */
    short revolt;            /* chance to revolt */
    short surrender;         /* base chance to surrender to nearby unit */
    short siege;             /* extra chance to be captured in siege */
    short attdamage;         /* damage when attrition happens */
    char *attritionmsg;      /* what to say when attrition happens */
    char *accidentmsg;       /* what to say when an accident happens */
    short attrition[MAXTTYPES];  /* chance to lose hp */
    short accident[MAXTTYPES];  /* chance to lose totally */
    /* attributes for build phase */
    short maker;             /* true if unit always builds */
    short occproduce;        /* true if unit can produce when an occupant */
    short startup;           /* extra prod schedule for first from unit */
    short research;          /* extra production schedule for first on side */
    short research_contrib[MAXUTYPES]; /* Fraction of other units research */
                                       /* to add to this units R&D */
    short make[MAXUTYPES];   /* base time to build a unit */
    short tomake[MAXRTYPES]; /* amount of resource needed to build */
    short repair[MAXUTYPES]; /* how many turns to regain a hit point */
    /* attributes for supply phase */
    short survival;          /* how long unit can starve */
    char *starvemsg;         /* what to say when unit runs out of supplies */
    short produce[MAXRTYPES];  /* rate of supply production */
    short productivity[MAXTTYPES];  /* effect of terrain on production */
    short storage[MAXRTYPES];  /* capacity for each type of resource */
    short consume[MAXRTYPES];  /* overhead consumption in each turn */
    short inlength[MAXRTYPES];  /* length of supply line coming in */
    short outlength[MAXRTYPES];  /* length of supply line going out */
    short consume_as_occupant;  /* should unit eat in base */
    /* attributes for move phase */
    short speed;             /* maximum theoretical speed in hexes/turn */
    short moves[MAXTTYPES];  /* additional overhead due to terrain */
    short randommove[MAXTTYPES];  /* randomness of move (.01% per turn) */
    short tomove[MAXRTYPES];  /* supplies used to move with */
    /* attributes for transportation */
    short volume;            /* how much space one part takes */
    short holdvolume;        /* space for passengers */
    short capacity[MAXUTYPES];  /* carrying capacity, by unit type */
    short entertime[MAXUTYPES];  /* moves to get on transport */
    short leavetime[MAXUTYPES];  /* moves to get off transport */
/*** (UK) insert -> ***/
    short freemove[MAXUTYPES];  /* insufficient movement allowance */
/*** <- insert ***/
    short bridge[MAXUTYPES];  /* true if transport accessible across terr */
    short mobility[MAXUTYPES];  /* true if unit is useless as occupant */
    /* attributes for viewing */
    short seealways;         /* true if unit view always up-to-date */
    short seebest;           /* chance of seeing other units */
    short seeworst;          /* see chance at max range */
    short seerange;          /* how far unit can see another */
    short visibility;        /* how easily others can see us */
    short conceal[MAXTTYPES];  /* how visible in given terrain */
    /* attributes for combat */
    short hp;                /* max number of hit points */
    short crippled;          /* hp at which unit loses functionality */
    short selfdestruct;      /* true if unit always dies in attack */
    short changeside;        /* chance of actually changing sides */
    short hittime;           /* extra moves to attack */
    short retreat;           /* chance of retreat to avoid a hit */
    short counterable;       /* true if can be counterattacked */
/*** (UK) insert -> ***/
    short cancounter;        /* true if can counterattack */
/*** <- insert ***/
    short arearadius;        /* how big an area is hit */
    char *destroymsg;        /* what to say when unit dies in combat */
    short hit[MAXUTYPES];    /* chance of hitting other units */
    short defense[MAXTTYPES];  /* how easy to hit in given terrain */
    short damage[MAXUTYPES];  /* how many points it hits for */
    short hitswith[MAXRTYPES];  /* amount of supply used for ammo */
    short hitby[MAXRTYPES];  /* kind of ammo that hits unit */
    short capture[MAXUTYPES];  /* chance of capturing a unit */
    short guard[MAXUTYPES];  /* num parts needed to garrison/guard a capture */
    short protect[MAXUTYPES];  /* effect on occupant on transport etc */
    /* random general attributes */
    short territory;         /* territorial value of unit */
    short isneutral;         /* true if unit can change side or be neutral */
    short maxquality;        /* most veteranness achievable */
    short skillf;            /* effect of quality on attack */
    short disciplinef;       /* effect of quality on defense etc */
    short disband;           /* true if unit can be gotten rid of */
    /* attributes to help machine */
    short attack_worth;      /* how many units should be used for attack */
    short defense_worth;     /* how many units do we need for defence. */
    short explore_worth;     /* how many units do we need for exploration. */
    short min_region_size;   /* how much territory do we need to produce these */
    short is_base;           /* unit is a type of base */
    short is_base_builder;   /* unit can produce bases */
    short is_transport;      /* unit can carry others */
    short is_carrier;      /* unit can carry others */
    short can_make;           /* unit can build something */
    short can_capture;       /* unit can capture something */
} Utype;

/* Definition of resource types.  These could maybe go in with utype stuff, */
/* but are more conveniently separated. */

typedef struct rtype {
    char rchar;              /* char representing resource */
    char *name;              /* short name of resource */
    char *help;              /* help string about resource */              
} Rtype;

/* Definition of terrain types.  Most parameters are to get generation */
/* right; unit interactions are all in utypes. */

typedef struct ttype {
    char tchar;              /* char representing type */
    char *name;              /* name of terrain */
    char *color;             /* name of a color for color displays */
    short dark;              /* true if terrain is "dark" */
    short nuked;             /* terrain type after being nuked */
    short minalt;            /* lowest altitude percentile for this type */
    short maxalt;            /* highest altitude percentile for this type */
    short minwet;            /* lowest moisture percentile */
    short maxwet;            /* highest moisture percentile */
} Ttype;

/* Macros for iterating over period description structures. */

#define for_all_unit_types(v)      for (v = 0; v < period.numutypes; ++v)

#define for_all_resource_types(v)  for (v = 0; v < period.numrtypes; ++v)

#define for_all_terrain_types(v)   for (v = 0; v < period.numttypes; ++v)

/* Macros to reduce the number of brackets and explicit structure refs. */

#define could_make(u1,u2) (utypes[u1].make[u2] > 0)

#define could_repair(u1, u2) (utypes[u1].repair[u2] > 0)

#define could_move(u,t) (utypes[u].moves[t] >= 0)
#define could_move_in_dir(u, d) \
  (could_move((u)->type, terrain_at(wrap((u)->x+dirx[(d)]), \
				    (u)->y+diry[(d)])))

#define could_carry(u1,u2) (utypes[u1].capacity[u2] > 0)

#define could_hit(u1,u2) (utypes[u1].hit[u2] > 0)

#define could_capture(u1,u2) (utypes[u1].capture[u2] > 0)

#define impassable(u, x, y) (!could_move((u)->type, terrain_at((x), (y))))

#define isbase(u) (utypes[(u)->type].is_base)

#define base_builder(u) (utypes[(u)->type].is_base_builder)

#define istransport(u) (utypes[(u)->type].is_transport)

/* Declarations of the period description structures.  The structures */
/* themselves will be in a compiled period description. */

extern Period period;

extern Utype utypes[];

extern Rtype rtypes[];

extern Ttype ttypes[];

extern char *snames[], *unames[];
