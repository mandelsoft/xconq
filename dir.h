/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Definitions for directions of the compass. */

/* The terrain model is based on hexes arranged in horizontal rows.  This */
/* means that although east and west remain intact, the concepts of north */
/* and south have basically vanished. */

/* Unfortunately, not all hex-dependent definitions are here.  Pathfinding */
/* code has some knowledge of hexes also, as does map generation. */

#define NUMDIRS 6

#define NE    0
#define EAST  1
#define SE    2
#define SW    3
#define WEST  4
#define NW    5
			 
#define DIRNAMES { "NE", "E", "SE", "SW", "W", "NW" }

#define DIRX { 0, 1,  1,  0, -1, -1 }
#define DIRY { 1, 0, -1, -1,  0,  1 }

#define DIRCHARS "ulnbhy"

#define random_dir() (random(NUMDIRS))

#define for_all_directions(dir)  for (dir = 0; dir < NUMDIRS; ++dir)

#define normalize_dir(d)	(((d)+NUMDIRS)%NUMDIRS)
#define opposite_dir(d) (((d) + 3) % NUMDIRS)
#define	right_of(d)	(((d) + 1) % NUMDIRS)
#define	left_of(d)	(((d) + NUMDIRS - 1) % NUMDIRS)

extern char *dirnames[];

extern int dirx[], diry[];





