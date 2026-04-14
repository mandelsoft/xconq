/* Copyright (c) 1991 Robert Forsman, University of Florida */
/* Gnu General Public Licence */

/*

  this module was added to make 5.5 movement commands as "smart" as
the 5.3 movement commands.  5.4 was missing these features.  It turned
out to be a bit smarter and less dishonest than 5.3.

  5.4 lacked a move command that would bypass obstacles such as
mountain ranges.  Fighters that were returning to base would attempt
paths that were obviously wrong.  5.3 posessed these commands, but it
cheated by making use of information that the user didn't have.

  Many of the coding ideas from 5.3 were used to build this module.

   */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"

/* this file implements smarter human movement commands */

#define VISITCODE
#ifdef VISITCODE
char	*visited_array=NULL;

#define VISITED(x,y) (visited_array[(y)*world.width + (x)])
#endif

int
enter_cost(unit,x,y)
     Unit	*unit;
     int	x,y;
{
  viewdata	view = side_view(unit->side, x, y);

  if (vtype(view)==UNSEEN)
    return 1; /* we can't see the hex, be optimistic (or guess based on utype?)  */

  if (vtype(view)!=EMPTY)
    {
      Unit	*transport;

      if (allied_side(unit->side, side_n(vside(view))))
	/* allies held that hex */
	{
	  transport = unit_at(x,y); /* check to see if the unit is still
				       there */
	  if (transport == NULL) { /*if it isn't there then the hex is empty*/
	    if (could_move(unit->type, terrain_at(x,y)))
	      return 1+utypes[unit->type].moves[terrain_at(x,y)];
	    else
	      return -1;
	  } else { /* the transport is there, is it the same one (sort of :) */
	    if (side_number(transport->side) == vside(view)) {
	      if (can_carry(transport, unit)) /* and can it carry our unit? */
		return 1+utypes[unit->type].entertime[vtype(view)];
	      else if (unit->movesleft < distance(x,y, unit->x, unit->y) &&
		       !napping_order(transport->orders.type) &&
		       utypes[transport->type].speed>0 ) /* it can move before
							   we get there */
		return 2+utypes[unit->type].moves[terrain_at(x,y)];
	      else
		return -1; /* it's an immobile blockage */
	    } else
	      return -1; /* there's a different unit in the way */
	  }
	}
      else /* hex holds enemy */
	return -1;
    }
  else if (could_move(unit->type, terrain_at(x,y))) /* we can see the hex */
    return 1+utypes[unit->type].moves[terrain_at(x,y)];
  else
    return -1;
}

int
estimate_production(unit, x, y, rtype)
     Unit	*unit;
     int	x,y, rtype;
{
  viewdata	view;
  int	rval;
  
  view = side_view(unit->side, x, y);
  
  if (vtype(view) == UNSEEN)
    return 0;
  
  rval = utypes[unit->type].produce[rtype] *
    utypes[unit->type].productivity[terrain_at(x,y)] / 100;
  
  if (cripple(unit))
    rval = rval * unit->hp / (utypes[unit->type].crippled+1);
  
  if (vtype(view) != EMPTY && allied_side(unit->side, side_n(vside(view)))) {
    Unit *transport = unit_at(x,y);
    if (transport != NULL &&
	side_number(transport->side)==vside(view) &&
	can_carry(transport, unit)
	) {
      rval += utypes[unit->type].storage[rtype] /2; /* totally arbitrary */
    }
  }
  return rval;
}

struct pathelem {
  int	x,y;
  int	dist;
  short	supply[MAXRTYPES];
#ifndef sun
  signed
#endif
    char	dir;
  struct pathelem	*next;
} *SPath=NULL, *SPathBorder=NULL;

static bool SearchIsEnded;

struct pathelem *
already_searched(x,y,list)
     int	x,y;
     struct pathelem	*list;
{
  struct pathelem	*scan;

  for (scan=list; scan!=NULL; scan= scan->next)
    if (x==scan->x && y==scan->y)
      return scan;

  return NULL;
}

struct pathelem *
add_border(x, y, supply, dir, dist)
     int	x,y, dist, dir;
     short	*supply;
{
  /* insert the coordinates into the list sorted by cost */
  struct pathelem	*temp, **scan;
  int	r;

#ifdef VISITCODE
  VISITED(x,y) = 1; /* hehe, doesn't that look screwy :) */
#endif

#ifdef DEBUGVISIT
  printf("adding %d,%d (%d,%d) [", x, y, dir, dist);
  for_all_resource_types(r) {
    printf("%d ", supply[r]);
  }
  printf("] to border list\n");
#endif

  for (scan = &SPathBorder; *scan!=NULL; scan = &(*scan)->next)
    if ( (*scan)->dist >= dist )
      break;
  temp = (struct pathelem *)malloc(sizeof(*temp));
  temp->x = x;
  temp->y = y;
  for_all_resource_types(r) {
    temp->supply[r] = supply[r];
  }
  temp->dist = dist;
  temp->dir = dir;
  temp->next = *scan;
  *scan = temp;
  return temp;
}

void
add_searched(unit,x,y,maxdist,pred)
     Unit	*unit;
     int	x,y,maxdist;
     bool	(*pred)();
{
  struct pathelem	*temp,**scan;
  int	dir, newx,newy;

  for (scan = &SPathBorder; *scan!=NULL; scan = &(*scan)->next)
    if ((*scan)->x==x && (*scan)->y==y)
      break;
  if (*scan==NULL)
    {
      /* panic, x,y has to be on the border list!! */
      printf("panic, tried to add_searched %d,%d which wasn't on the border list\n", x,y);
      return;
    }
  temp = *scan;
  *scan = (*scan)->next; /* remove from border list */

  temp->next = SPath;
  SPath = temp; /* add to searched list */
  
  for (dir=0; dir<NUMDIRS; dir++)
    { /* add bordering units to the border list if we haven't
	 visited them before */
      static short	supply[MAXRTYPES];
      int	entercost,r;

      newx = wrap(x + dirx[dir]);
      newy = y + diry[dir];

      if ( (*pred)(unit, newx,newy) )
	{
	  entercost = 1;
	  SearchIsEnded = TRUE;
	}
      else
	{
	  if (!between(1, newy, world.height-2))
	    /* there should probably be a convenience function to
	       validate a sector coordinate */
	    continue;
#ifdef VISITCODE
	  if (VISITED(newx,newy))
#else
	  if (NULL != already_searched(newx,newy, SPath) ||
	      NULL != already_searched(newx,newy, SPathBorder) )
#endif
	    continue;

	  entercost = enter_cost(unit,newx,newy);
	  if (entercost<0)
	    continue;
	  if (SPath->dist+entercost > maxdist)
	    continue;
	}

      for_all_resource_types(r) {
	int	t = unit->type;
	supply[r] = SPath->supply[r];
	supply[r] -= utypes[t].tomove[r];
	supply[r] += estimate_production(unit, newx, newy, r);
	supply[r] = min(supply[r], utypes[t].storage[r]);
	if ((supply[r]<0 ||
	     supply[r]==0 && utypes[t].consume[r])
	    && (SPath->dist>0)) {
	  entercost = maxdist+1;
	}
      }

      temp = add_border(newx, newy, supply,
			(SPath->dir == -1) ? dir : SPath->dir,
			SPath->dist+entercost);
      /*printf("\n");*/
    }
}

int
search_path(unit, maxdist, pred)
     Unit	*unit;	/* unit with a destination */
     int	maxdist; /* maximum distance (move cost) to search */
     bool	(*pred)(); /* the function that tells us if we got there. */
{
  struct pathelem	*scan, *temp;
  int	rval;

  if (visited_array==NULL)
    visited_array = malloc(sizeof(*visited_array)*world.width*world.height);
  bzero(visited_array, sizeof(*visited_array)*world.width*world.height);

  if (pred(unit,unit->x,unit->y))
    return -1;

  SearchIsEnded = FALSE;
  add_border(unit->x,unit->y, unit->supply, -1, 0);

  while (!SearchIsEnded && SPathBorder!=NULL &&
	 SPathBorder->dist <= maxdist) {
    add_searched(unit, SPathBorder->x, SPathBorder->y, maxdist, pred);
  }

  for (scan=SPathBorder; scan!=NULL && !pred(unit, scan->x, scan->y);
       scan = scan->next)
    ;

  if ( scan != NULL && scan->dist <= maxdist) {
    rval = scan->dir;
  } else {
    rval = -1;
  }

  scan = SPath;
  while (scan) {
    scan = (temp = scan)->next;
    free (temp);
  }
  scan = SPathBorder;
  while (scan) {
    scan = (temp = scan)->next;
    free (temp);
  }
  SPath = SPathBorder = NULL;

  return rval;
}
