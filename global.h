/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Global data structures. */

/* There is actually no inherent limit on the number of winning and losing */
/* conditions, but scenarios usually only need a couple. */

#define NUMCONDS 10

/* Not many types of conditions (probably could think of a few more). */

#define TERRITORY 0
#define UNITCOUNT 1
#define RESOURCECOUNT 2
#define POSSESSION 3

/* Win/lose conditions can take several different forms.  Some of these */
/* slots could be glommed into a union, but so what... */

typedef struct a_condition {
    bool win;                   /* is this a condition of winning or losing? */
    int type;                   /* based on units, resources, or location? */
    int starttime;              /* first turn to check on condition */
    int endtime;                /* last turn to check on condition */
    int sidesn;                 /* side number to which this cond applies */
    int units[MAXUTYPES];       /* numbers for each type of unit */
    int resources[MAXRTYPES];   /* numbers for each type of resource */
    int x, y;                   /* a location */
    int n;                      /* a vanilla value */
} Condition;

/* Definition of structure containing all global variables. */

typedef struct a_global {
    int time;			/* the current turn */
    int endtime;                /* the last turn of the game */
    bool setproduct;            /* true if production changes allowed */
    bool leavemap;              /* true if units can leave the map */
    int numconds;               /* number of conditions... */
    Condition winlose[NUMCONDS];  /* and space for the conditions themselves */
    unsigned  giventime;              /* length of the game is seconds for chess clock */
    int timeout;		/* how long before a turn times out */
    unsigned long starttime;		/* when this turn started accepting input */
    unsigned long startroundtime,maxroundtime; /* timeout per round gec */
} Global;

/* Just have the one "global" object. */

extern Global global;

/*** (UK) insert -> ***/
typedef int (*RequestHandler)();
/*** <- insert ***/
