/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Definitions of levels of detail in mapfiles.  These values must be */
/* ordered, so careful about fiddling with them. */

#define SIDEBASIC 1
#define SIDESLOTS 2
#define SIDEVIEW  3
#define SIDEMISC  4
#define SIDEALL   4  /* guaranteed max */
/* the max is set low enough so that new information can be added just by */
/* adding another layer. */

#define UNITBASIC  1
#define UNITSLOTS  2
#define UNITORDERS 3
#define UNITMOVED  4
#define UNITALL    4  /* guaranteed max */

/* These limit the junk in mapfile headers. */

#define MAXINCLUDES 9     /* max number of included files */
#define MAXMAPNOTES 40    /* max lines of notes */

/* Theoretically, there is no maximum size to xconq maps, but the minimum */
/* size is set by mystical properties of display, and is not negotiable. */

#define MINWIDTH 5
#define MINHEIGHT 5

/* Letters for hex and outlined hex.  Exact use is up to the interface - */
/* the X interface has hexagonal shapes in its xconq font for these.  No */
/* problems about conflicting with unit or terrain characters. */

#define HEX 'O'
#define OHEX 'o'

/* This structure should be as small as possible, since it is an */
/* individual entry in the array containing the entire world. */
/* The "type" and "people" slots could be combined, but this might */
/* slow accesses undesirably, because of bit field extraction. */

typedef struct a_hex {
    char type;            /* terrain type at this spot */
    unsigned char mark;   /* marker noting whether we have looked at hex yet. */
    char fromdir;         /* direction used to get to this hex */
    unsigned char dist;   /* distance from start point in search */
    struct a_unit *surf;  /* pointer to unit if any */
} Hex;

/* Just for convenience, global variables about map live in this structure. */

typedef struct a_world {
    int width, height;    /* size of the world */
    bool known;           /* true if everybody knows what world looks like */
    Hex *terrain;         /* pointer to array of hexes */
} World;

/* The type of a hex in the world is a small number representing the */
/* terrain type.  Only about 10 types are defined, in terrain.h */

#define terrain_at(x,y) ((world.terrain[(y)*world.width+(x)]).type)

#define set_terrain_at(x,y,t) \
  ((world.terrain[(y)*world.width+(x)]).type = (t))

/* The unit is a raw pointer - this macro is used a *lot*. (Any prospects */
/* for optimization?) */

#define unit_at(x,y) ((world.terrain[(y)*world.width+(x)]).surf)

#define set_unit_at(x,y,u) \
  ((world.terrain[(y)*world.width+(x)]).surf = (u))

/* This little macro implements wraparound in the x direction. */
/* The stupid add is for the benefit of brain-damaged mod operators */
/* that don't handle negative numbers properly. */

/*** (UK) change -> ***/
#define wrap(x) (((x)%world.width + world.width) % world.width)
/*** was:
#define wrap(x) (((x) + world.width) % world.width)
*** <- change ***/

/* Constrain y to northern and southern edges. */

#define limit(y) (max(0, min((y), (world.height-1))))

#define interior(y) (max(1, min((y), (world.height-2))))

#define for_all_hexes(x,y) for (x = 0; x < world.width; x++) \
                             for (y = 0; y < world.height; y++)

#define for_all_interior_hexes(x,y) for (x = 0; x < world.width; x++) \
                             for (y = 1; y < world.height - 1; y++)

#define markloc(x, y) \
  ((world.terrain[(y)*world.width+(x)]).mark = mark)

#define markedloc(x, y) \
  ((world.terrain[(y)*world.width+(x)]).mark == mark)

#define get_fromdir(x, y) \
  ((world.terrain[(y)*world.width+(x)]).fromdir)

#define set_fromdir(x, y, dir) \
  ((world.terrain[(y)*world.width+(x)]).fromdir = dir)

#define get_dist(x, y) \
  ((world.terrain[(y)*world.width+(x)]).dist)

#define set_dist(x, y, d) \
  ((world.terrain[(y)*world.width+(x)]).dist = d)



/* Declaration of the world itself. */

extern World world;

extern bool midturnrestore;  /* game restored in middle of turn */

extern unsigned char mark;
