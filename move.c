/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Nothing happens in xconq without movement, and it involves some rather */
/* complicated control structures in order to get the details right. */
/* Units only actually move by following preset orders; when they appear */
/* to be moving under manual control, they are just following orders that */
/* only last for one move before new orders are requested from the player. */

/* Order of movement is sequential by sides, but non-moving sides are in */
/* survey mode and can do things. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

#define ROUNDTIMEOUT ((time(0)>global.startroundtime+global.maxroundtime) && global.maxroundtime)

/* A perimeter is a list of hexes that can be reached in n moves. */

typedef struct _position {
    short x, y;		/* location of hex */
    char dir;		/* first move direction */
} Position;

static Position *perimeters[2];
static char *visited_flags;

/*** (UK) insert -> ***/
bool ODebug=TRUE;
/*** <- insert ***/

extern x_command();

/* mxconq: defined in xconq.c */
extern bool singlePlayerMode;

/* Complaints about inability to do various things should only appear if */
/* a human player issues the order to an awake unit. */

#define human_order(u) ((u)->side->directorder && humanside((u)->side))

/* Unit can be awake permanently or temporarily. */

/*** (HW) remove ->
#define is_awake(u) ((u)->orders.type == AWAKE || (u)->awake)
*** (HW) ***/

Side *curside;     /* current side for when turns are sequential */


/* Notify the user that his turn is beginning if appropriate */

void start_side_turn(side)
Side *side;
{
/*  bool other_human = FALSE; */
  bool units_move = FALSE;
  Unit *unit;
  Side *loop_side;


  if (humanside(side) && !side->lost) {
    notify(side, "Turn %d beginning ", global.time);
    if ((time(0) - side->laststarttime) > side->startbeeptime) {
      for_all_units(loop_side, unit)
	if ((unit->side == side) && (utypes[unit->type].speed >0)) {
	  units_move = TRUE;
	  continue;
	}
      if (active_display(side) && /* other_human && */ units_move) beep(side);
    }
  }
}

/* Return TRUE if there are units that can move this turn. */

static bool
more_units_to_move ()
{
  Side *side;

    for_all_sides(side) {
	if (side->more_units && !side->lost && !ROUNDTIMEOUT) /*gec*/
	    return TRUE;
    }
    return FALSE;
}

/*** (UK) insert -> ***/
void duprint(side,s,u)
Side *side;
char *s;
Unit *u;
{
  /*
  if (side_number(side)!=0) return;
  if (u && u!=side->unithead) {
    printf("%s: %s\n",s, unit_handle(side,u));
  }
  else {
    printf("%s: <none>\n",s);
  }
  */
}

Unit *filter_move_unit(side,unit)
Side *side;
Unit *unit;
{
  if (unit && side && humanside(side) && 
      !active_display(side) && is_awake(unit)) {
    if (ODebug) pui(side,"  skipped:",unit); 
    return NULL;
  }
  if (ODebug) pui(side,"ok  to move:",unit); 
  return unit;
}

Unit *occupant_to_move(side,unit)
Side *side;
Unit *unit;
{
  extern Unit *occupant_to_move2();
  Unit *u=occupant_to_move2(side,unit);
  if (ODebug) pui(side," found occupant:",u); 
  return u;
}

Unit *occupant_to_move2(side,unit)
Side *side;
Unit *unit;
{ Unit *u,*u2;

  if (!unit || unit==side->unithead || !alive(unit)) return NULL;

  for_all_occupants(unit,u) {
    if (can_move_unit(side,u) && (filter_move_unit(side,u))) 
      return u;
  }
  for_all_occupants(unit,u) {
    if (u2=occupant_to_move(side,u)) 
      return u2;
  }
  return NULL;
}

static Unit *next_unit_to_move2();
/*** <- insert ***/

/* Find the next unit that the side can move this turn. */

Unit *
next_unit_to_move (side)
Side *side;
/*** (UK) insert -> ***/
{ Unit *unit=(Unit*)next_unit_to_move2 (side);
  if (ODebug) pui(side,"next_unit_to_move:",unit); 
  return unit;
}

pui(side,s,unit)
Side *side;
char *s;
Unit *unit;
{
    if (side_number(side)==0 && !active_display(side)) 
      if (unit && unit!=side->unithead)
	printf("%s %s (%s, %s, %s, %s, %dx)\n",s,
	      unit_handle(side,unit),
	      side->mode==MOVE?"move":"survey",
	      can_move_unit(side,unit)?"movable":"not mavable",
	      is_awake(unit)?"awake":"no awake",
	      order_desig(&(unit->orders)), unit->movesleft
	    );
      else printf("%s <no unit>\n",s);
}

bool TestCond(side,u,unitp)
Side *side;
Unit *u;
Unit **unitp;
{   bool b;

    if (!u) {
      if (ODebug) pui(side,"TestCond: FALSE (!u)",u);
      return FALSE;
    }
    *unitp=occupant_to_move(side,u);
    if (*unitp) {
      if (ODebug) pui(side,"TestCond: FALSE (unit)",*unitp);
      return FALSE;
    }

    b=can_move_unit(side,u);
    if (b) {
	if (ODebug) pui(side,"TestCond: can move",u);
       *unitp=filter_move_unit(side,u);
       if (*unitp) {
	 if (ODebug) pui(side,"TestCond: FALSE after filtering ",*unitp);
	 return FALSE;
       }
       if (ODebug) pui(side,"TestCond: TRUE after filtering ",*unitp);
       return TRUE;
    }
    if (ODebug) pui(side,"TestCond: TRUE cannot move ",u);
    return TRUE;
}

static Unit *
next_unit_to_move2 (side)
Side *side;
/*** <- insert ***/
{
    Unit *unit;

/*** (UK) insert -> ***/
    Unit *u=side->last_unit;
    unit=NULL;

    if (ODebug) pui(side,"start search for moveable unit with ",u);
    duprint(side,"start search movable occupant with",u);

    while (u && !(unit=occupant_to_move(side,u)) &&
      !(can_move_unit(side,u) && (unit=filter_move_unit(side,u)))) {
    /*
    while (TestCond(side,u,&unit)) {
    */
      unit=u->last_unit;
      u->last_unit=NULL;
      u=unit;
      if (u) side->last_unit=u;
      unit=NULL;
      if (ODebug) pui(side," continue search for moveable unit with ",u);
      duprint(side," next search movable occupant with",u);
    }
    duprint(side," found",unit);
    if (!unit) {

/*** <- insert ***/
    unit = next_unit(side, side->last_unit, TRUE);
    if (unit==NULL) {
      unit = next_unit(side, side->last_unit, FALSE);
      if (Debug)
	printf("%s: no movable units in_middle\n",side->name);
    }
/*** (UK) insert -> ***/

    } else {
      if (ODebug) pui(side," found occupant",unit);
      if (u!=unit) {
	duprint(side,"set last unit to",u);
	unit->last_unit=u;
      }
    }
/*** <- insert ***/
    if (unit == NULL) {
	side->more_units = FALSE;
    } else {
	side->last_unit = unit;
    }
    return unit;
}

set_side_movunit(side)
Side *side;
{
  side->movunit = NULL;
  side->curunit = NULL;
  side->last_unit = first_unit(side);
  side->more_units = TRUE;
  side->movunit = next_unit_to_move(side);
}

/* The movement phases starts by precomputing theoretical maxima for moves, */
/* then does each side one-by-one.  Before iterating, each human side must */
/* be wedged (to ignore input). */
int newturnsent; /* XXX */

movement_phase_sequential() /* XXX */
{
    Unit *unit;
    Side *side, *side2;
    bool human_moving;
    int sidenum;

    for_all_sides(side) {
/*** (UK) insert -> ***/
        side->enemyseen=FALSE;
	side->savflag=FALSE;
/*** <- insert ***/
	if (humanside(side)) {
	  survey_mode(side);
	  request_command(side);
	}
        if (!midturnrestore)
	  for_all_side_units(side, unit) {
	    unit->movesleft = 0;
	    unit->actualmoves = 0;
	  }
    }
    sidenum = -1;
    for_all_sides(side) {
      sidenum++;
      if (!side->lost) {
	cancel_request(side);
        if (!midturnrestore)
	  for_all_side_units(side, unit) {
	    unit->movesleft = compute_move(unit);
	    unit->actualmoves = 0;
	  }
	side->directorder = FALSE;
	set_side_movunit(side);
	if (can_move_unit(side, side->movunit)) {
	  move_mode(side);
	}
        else {
          for_all_side_units(side, unit) {
	    unit->movesleft = 0;
          }
	  continue;
	}

        if (midturnrestore)
          midturnrestore = FALSE;
	init_machine_turn(side);  /* Humans could be part robot */
	start_side_turn(side);
	for_all_sides(side2) {
	  update_sides(side2);
	}
	side->lasttime = time(0);
        if (nextturnexecfile[0] != '\0') { /* XXX */
          char buf[BUFSIZE], tmp[BUFSIZE];
          extern numsides;

          sprintf(buf, "%s seq %d sides %d %d", nextturnexecfile, global.time,
                  numsides, sidenum);
          newturnsent = global.time;
          system(buf);
        }
	while (side->more_units && !side->lost) {
          human_moving = FALSE;
	  if (!side->lost) {
	     if (!can_move_unit(side, side->movunit) &&
	         side->mode == MOVE) {
	         /* unit probably died while moving or was captured. */
	         cancel_request(side);
	     }

	     if (!side->reqactive) {
	        if (can_move_unit(side, side->movunit)) {
	           flush_input(side);
		   move_1(side, side->movunit);
	        }
	        if (!can_move_unit(side, side->movunit) &&
		    side->more_units) {
		  side->movunit = next_unit_to_move(side);
		  if (!side->more_units) {
		     for_all_sides(side2) {
		       update_sides(side2);
		     }
		   }
	        }
	      }
	      if (humanside(side) &&
		  (can_move_unit(side, side->movunit) ||
		   side->reqhandler != x_command)
	  	   && side->reqactive) {
	        human_moving = TRUE;
	      }
	      for_all_sides(side2) {
	        if (!side2->reqactive &&
		    !can_move_unit(side2, side2->movunit) &&
		    humanside(side2) && side2->mode != SURVEY) {
	  	  survey_mode(side2);
	        }
	        if (!side2->reqactive && humanside(side2) &&
	  	    side2->mode == SURVEY) {
	          request_command(side2);
	        }
	      }
	  }
          if (human_moving)
            handle_requests();
        }
      }
      for_all_side_units(side, unit) {
	unit->movesleft = 0;
      }
    }
    while (someone_doing_something()) 
      handle_requests();

/*** (UK) insert -> ***/
    for_all_sides(side) side->workmode=side->teach=FALSE;
/*** <- insert ***/
    if (midturnrestore)
      midturnrestore = FALSE;
    newturnsent = -1; /* XXX */
    /* Remove dead units that have been dead more than a turn. */
    for_all_sides (side) {
      flush_side_dead(side);
      /* Clear wakeup reasons for non-movers. */
      /* need to fix this so that these stay around for a turn. */
      for_all_side_units(side, unit) 
	if (utypes[unit->type].speed == 0 && unit->side == side)
	  unit->wakeup_reason = WAKEOWNER;
      side->laststarttime = time(0);
    }

    /* For display hacking, also to find misc bugs :-) */
    curside = NULL;
    for_all_sides(side)  {
	if (humanside(side) && !side->lost) {
	    update_sides(side);
	    cancel_request(side);
	}
    }
    /* Do all sentrys now. */
    for_all_sides(side)
      for_all_side_units(side, unit) {
	if (delayed_order(unit->orders.type)) {
	  unit->orders.rept--;
	  maybe_wakeup(unit);
	}
      }
}

movement_phase()
{
    Unit *unit;
    Side *side, *side2;
    bool human_moving;


    routine("Movement phase");
    enter_procedure("movement_phase");
    if (Debug) printf("Entering movement phase\n");
    if (Sequential) { /* XXX */
      movement_phase_sequential();
      exit_procedure();
      return;
    }
    compute_moves();
    for_all_sides(side) 
      if (!side->lost) {
/*** (UK) insert -> ***/
        side->enemyseen=FALSE;
	side->savflag=FALSE;
/*** <- insert ***/
	side->directorder = FALSE;
	set_side_movunit(side);
	if (can_move_unit(side, side->movunit)) {
	  move_mode(side);
	} else if (humanside(side)) {
	  survey_mode(side);
	  request_command(side);
	}
	init_machine_turn(side);  /* Humans could be part robot */
	start_side_turn(side);
    }

    for_all_sides(side2) {
	update_sides(side2);
	side2->lasttime = time(0);
    }

    global.starttime = time(0);
    if (nextturnexecfile[0] != '\0') { /* XXX */
        char buf[BUFSIZE], tmp[BUFSIZE];
        extern numsides;

        sprintf(buf, "%s turn %d sides %d", nextturnexecfile, global.time,
                numsides);
        newturnsent = global.time;
        for_all_sides(side2) {
                sprintf(tmp, " %d %d", (side2->lost ? 0 : 1),
                        (side2->more_units ? 1 : 0));
                strcat(buf, tmp);
        }
        system(buf);
    }
    global.startroundtime=time(0); /* gec */

    /* Move the units. */
    while (more_units_to_move() &&
	   (global.timeout <= 0 ||
	    time(0) - global.starttime < global.timeout)) {
	human_moving = FALSE;
	for_all_sides(side) {
	    if (!side->lost) {
	        if (!can_move_unit(side, side->movunit) &&
		    side->mode == MOVE) {
		  /* unit probably died while moving or was captured. */
		  cancel_request(side);
		}

		if (!side->reqactive) {
		  if (can_move_unit(side, side->movunit)) {
		    flush_input(side);
/*** (UK) insert -> ***/
                    if (!filter_move_unit(side,side->movunit)) {
		      if (side_number(side)==0) 
			if (ODebug) printf("skip movunit\n");
		      side->movunit=next_unit_to_move(side);
		    }
		    if (can_move_unit(side, side->movunit))
/*** <- insert ***/
		    move_1(side, side->movunit);
		  }
		  if (!can_move_unit(side, side->movunit) &&
		      side->more_units) {
		    side->movunit = next_unit_to_move(side);
		    if (!side->more_units) {
		       if (nextturnexecfile[0] != '\0') { /* XXX */
		         char buf[BUFSIZE];
        		 sprintf(buf, "%s side %d %d %d", nextturnexecfile,
				side_number(side),
		                (side->lost ? 0 : 1),
				(side->more_units ? 1 : 0));
		         system(buf);
		      }
		      for_all_sides(side2) {
			update_sides(side2);
		      }
		    }
		  }
		  if (!can_move_unit(side, side->movunit) &&
		      humanside(side) && side->mode != SURVEY) {
/*** (UK) insert -> ***/
		    /*
		    for_all_sides(side2) {
		      update_sides(side2);
		    }
                    */
/*** <- insert ***/
		    survey_mode(side);
		  }
/*** (UK) insert -> ***/
		  if (humanside(side)) { 
		    test_input();
		    if (side->reqtype==KEYBOARD) {
		      if (side->reqch=='z') {
			printf("    Hallo\n");
                        side->reqtype=GARBAGE;
			survey_mode(side);
			request_command(side);
			human_moving = TRUE;
		      }
		    }
		  }
/*** <- insert ***/
		}
		if (humanside(side) &&
		    (can_move_unit(side, side->movunit) ||
		     side->reqhandler != x_command || side->more_units) 
		    && side->reqactive)
		    human_moving = TRUE;
		if (!side->reqactive && humanside(side) &&
		    side->mode == SURVEY) {
		    request_command(side);
		}
	    }
	}
	if (human_moving) handle_requests();
    }
/*** (UK) insert -> ***/
    for_all_sides(side) {
      for_all_side_units(side, unit) {
	unit->movesleft = 0;
      }
    }
/*** <- insert ***/

    while (someone_doing_something() &&
	   (global.timeout <= 0 ||
	    time(0) - global.starttime < global.timeout))
      {
	handle_requests();
      }

/*** (UK) insert -> ***/
    for_all_sides(side) side->workmode=side->teach=FALSE;
/*** <- insert ***/
    if (global.timeout > 0 && time(0) - global.starttime >= global.timeout)
      {
	for_all_sides(side)
	  notify(side, "Turn %d ends after %d minutes.", global.time,
		 global.timeout/60);
      }

/*** (UK) remove -> ***
    if (singlePlayerMode) {
	Side *currentSide = NULL;
	bool firstSide = TRUE;
	char *displayName;

	displayName = getenv("DISPLAY");
	currentSide = sidelist;
	while (currentSide && (strcmp(currentSide->host, displayName) != 0)) {
	    if (humanside(currentSide) && (!currentSide->lost))
		firstSide = FALSE;
	    currentSide = currentSide->next;
	}
	while (!firstSide) {
	    request_command(currentSide);
	    handle_requests();
	}
    }
*** <- remove ***/

    newturnsent = -1; /* XXX */
    /* Remove dead units that have been dead more than a turn. */
    for_all_sides (side) {
      flush_side_dead(side);
      /* Clear wakeup reasons for non-movers. */
      /* need to fix this so that these stay around for a turn. */
      for_all_side_units(side, unit) 
	if (utypes[unit->type].speed == 0 && unit->side == side)
	  unit->wakeup_reason = WAKEOWNER;
      side->laststarttime = time(0);
    }

    /* For display hacking, also to find misc bugs :-) */
    curside = NULL;
    for_all_sides(side)  {
	if (humanside(side) && !side->lost) {
	    update_sides(side);
	    cancel_request(side);
	}
    }

    /* Do all sentrys now. */
    for_all_sides(side)
      for_all_side_units(side, unit) {
	if (delayed_order(unit->orders.type)) {
	  unit->orders.rept--;
	  maybe_wakeup(unit);
	}
      }
    exit_procedure();
}

/* Compute moves for all the units at once. */

compute_moves()
{
    Unit *unit;
    Side *side;

    if (midturnrestore)
      midturnrestore = FALSE;
    else /* for all non-neutral units. */
      for_all_sides(side)
	for_all_side_units(side, unit) {
	  unit->movesleft = compute_move(unit);
	  unit->actualmoves = 0;
/*** (UK) insert -> ***/
	  unit->last_unit = NULL;
/*** <- insert ***/
	}
}

/* Compute number of moves available to the units.  This is complicated by */
/* reduction of movement due to damage and the effect of occupants on */
/* mobility.  Also, we never let a moving unit have a movement of zero, */
/* unless it is out of movement supplies. */

/* Moves should be recomputed and possibly adjusted downward after a hit. */

compute_move(unit)
Unit *unit;
{
    int u = unit->type, r, moves = 0;
    Unit *occ;

    if (!neutral(unit) && (moves = utypes[u].speed) > 0) {
	if (cripple(unit)) {
	    moves = (moves * unit->hp) / (utypes[u].crippled + 1);
	}
	for_all_occupants(unit, occ) {
	    if (utypes[u].mobility[occ->type] != 100) {
		moves = (moves * utypes[u].mobility[occ->type]) / 100;
		break;
	    }
	}
	moves = max(1, moves);
	for_all_resource_types(r) {
	    if (utypes[u].tomove[r] > 0 && unit->supply[r] <= 0) {
		moves = 0;
		break;
	    }
	}
    }
    return moves;
}

/* To move a single unit, keep iterating until all its moves are used up, */
/* or it dies, or its movement has been postponed until the other units */
/* have been done.  A single move is either under preset orders, or must be */
/* supplied by a human player, or computed by a machine player. */

move_1(side, unit)
Side *side;
Unit *unit;
{

    if (ODebug) pui(side,"going to move",unit);
    if (Debug) printf("%s going to move\n", unit_handle((Side *) side, unit));
    if (can_move_unit(side, unit)) {
/*** (HW) insert -> ***/
        extern Side *curside;
        curside = side;
/*** <- insert ***/
	if (side->mode == MOVE) make_current(side, unit);
	if (idled(unit) && global.setproduct) {
	  request_new_product(unit);
	} else if (!is_awake(unit) && side->mode == MOVE) {
	    follow_order(unit);
	} else if (humanside(side) && under_control(unit)) {
	  side->directorder = TRUE;
	  request_command(side);
	  return; /* why? */
	} else {
	    show_info(side);
	    machine_move(unit);
	}
    }
}

/* Switching to move mode involves shifting from wherever the cursor is, */
/* back to the unit that was being moved earlier. */

move_mode(side)
Side *side;
{
    int oldmode = side->mode;

    if (Debug) printf("%s side in move mode\n", side->name);
/*** (UK) insert -> ***/
    if (!can_move_unit(side, side->movunit) &&
	side->more_units) {
      side->movunit = next_unit_to_move(side);
    }
    if (side->more_units)
/*** <- insert ***/
    side->mode = MOVE;
    if (side->mode != oldmode) show_timemode(side,TRUE);
}

/* Switching to survey mode */

survey_mode(side)
Side *side;
{
    int oldmode = side->mode;

    if (Debug) printf("%s side in survey mode\n", side->name);
    side->mode = SURVEY;
    if (side->movunit == NULL) side->movunit = side->curunit;
    make_current(side, unit_at(side->curx, side->cury));
    /* always do show info to get up to dateness info. */
    /* if (side->curunit != side->movunit) */ show_info(side);
    if (side->mode != oldmode) show_timemode(side,TRUE);
}

/* Test if a unit (on a human side) is actually under manual control. */

under_control(unit)
Unit *unit;
{
    return (!unit->orders.morder);
}

/* Set the "current" unit of a side - the one being displayed, moved, etc. */

make_current(side, unit)
Side *side;
Unit *unit;
{
    if (unit != NULL && alive(unit) &&
	(allied_side(unit->side,side) || Debug || Build)) {
	side->curunit = unit;
/*** (UK) insert -> ***/
	if (unit->x!=side->curx || unit->y!=side->cury) {
	  side->last_x=side->curx;
	  side->last_y=side->cury;
	}
/*** <- insert ***/
	side->curx = unit->x;  side->cury = unit->y;
    } else {
	side->curunit = NULL;
    }
}

unowned_p(x, y)
int x, y;
{
    Unit *unit = unit_at(x, y);

    return (unit == NULL || unit->side != tmpside);
}

goto_empty_hex(side)
Side *side;
{
    int x, y;

    tmpside = side;
    if (search_area(side->curx, side->cury, 20, unowned_p, &x, &y, 1)) {
	side->curx = x;  side->cury = y;
	side->curunit = NULL;
    }
}

/* Do single move of a single order for a given unit. */

char *unit_desig();

/*** (UK) insert -> ***/
extern Order *select_standing_orders();
/*** <- insert ***/

follow_order(unit)
Unit *unit;
{
/*** (UK) insert -> ***/
    bool domove  = TRUE;
/*** <- insert ***/
    bool success = FALSE;
    int u = unit->type;
    Side *us = unit->side;

    if (Debug) printf("%d: %s doing %s with %d moves left\n",
		      global.time, unit_desig(unit),
		      order_desig(&(unit->orders)), unit->movesleft);
/* somewhere this has a right home */
/*    unit->awake = FALSE;  */
    switch (unit->orders.type) {
    case FILL:
    case SENTRY:
    case EMBARK:
      /* handled in code at end of movement phase */
	break;
    case MOVEDIR:
	success = move_dir(unit, unit->orders.p.dir);
	break;
    case MOVETO:
	success = move_to_dest(unit);
	break;
    case MOVETOTRANSPORT:
	success = move_to_transport(unit);
	break;
    case MOVETOUNIT:
	success = move_toward_unit(unit,unit->orders.p.leader_id);
	break;
    case EDGE:
	success = follow_coast(unit);
	break;
    case FOLLOW:
	success = follow_leader(unit);
	break;
    case PATROL:
	success = move_patrol(unit);
	break;
    case RETURN:
	success = return_to_base(unit);
	break;
/*** (UK) insert -> ***/
    case RPICKUP:
	success=rpickup(us,unit);
	domove=FALSE;
	break;
    case LEAVE:
	if (unit->orders.rept-- > 0) {
	  success = try_leave(us,unit);	
	  if (!success) unit->movesleft=0;
	}
	else success=TRUE;
	domove=FALSE;
	break;
    case UNLOAD:
	if (unit->orders.rept-->0 ) {
          try_unload(us,unit);
	  success=!can_unload(unit);
	  if (!success) unit->movesleft=0;
	}
	else success=TRUE;
	domove=FALSE;
	break;
    case GROUP:
	if (unit->group) free(unit->group);
	if (strlen(unit->orders.p.string))
	  unit->group=copy_string(unit->orders.p.string);
        else unit->group=0;
	unit->orders.rept=0;
	domove=FALSE;
	success=TRUE;
        break;
    case RGIVE:
	unit->orders.rept--;
	handle_order_give(us,unit,unit->orders.p.amount,FALSE);
	success=TRUE;
	break;
    case RTAKE:
	unit->orders.rept--;
	handle_order_take(us,unit,unit->orders.p.amount,FALSE);
	success=TRUE;
	break;
    case APICKUP:
	success=apickup(us,unit);
#if 0
	if (!success) { /* delay */
	  delete_unit(unit);
	  insert_unit_tail(us->unithead, unit);
	  us->movunit = NULL;
	}
#endif
	domove=FALSE;
	break;
/*** <- insert ***/
    case AWAKE:
    case NONE:
    default:
        case_panic("order type", unit->orders.type);
    }
/*** (UK/HW) change -> ***/
    if (alive(unit)) {
	if (success || !humanside(us)) {
	    if (domove) unit->movesleft -=
		1 + utypes[u].moves[terrain_at(unit->x, unit->y)];
	}
/*** was:
    if (alive(unit)) {
	if (success) {
	    unit->movesleft -=
		max(1,1 + utypes[u].moves[terrain_at(unit->x, unit->y)]);
	}
*** <- change ***/
	/* clear all wakeup messages */
	unit->wakeup_reason = WAKEOWNER;
	if (!us->directorder || success) {
/*** (UK) insert -> ***/
          if (!is_awake(unit))
/*** <- insert ***/
	    maybe_wakeup(unit);
	}
    }
    us->directorder = FALSE;
    if (!success) unit->move_tries++;
}

/*** (UK) insert -> ***/
rpickup(side, unit)
Side *side;
Unit *unit;
{ int success;
  if (unit->orders.rept-->0 ) {
    /* printf("try pickup %s\n",unit_handle(unit->side,unit)); */
    try_pickup(side,unit,100);
    success=is_full(unit,NULL);
    if (!success) unit->movesleft=0;
    else unit->orders.rept=0;
  }
  else success=TRUE;
  return success;
}

apickup(side, unit)
Side *side;
Unit *unit;
{ int success;
  printf("apickup %s\n",unit_handle(side,unit));
  if (unit->orders.rept-->0 ) {
    unit->orders.p.amount-=try_unit_pickup(side,unit,unit->orders.p.amount);
    success=!unit->orders.p.amount || is_full(unit,NULL);
/*  if (!success) unit->movesleft=0;  */
    if (success) unit->orders.rept=0;
  }
  else success=TRUE;
  return success;
}
/*** <- insert ***/

/* Force unit to try to move in given direction. */

move_dir(unit, dir)
Unit *unit;
int dir;
{
    if (unit->orders.rept-- > 0) {
	return move_to_next(unit, dirx[dir], diry[dir], TRUE, TRUE);
    }
    return FALSE;
}

/* Have unit try to move to its ordered position. */

move_to_dest(unit)
Unit *unit;
{
  return move_to(unit, unit->orders.p.pt[0].x, unit->orders.p.pt[0].y,
		 (unit->orders.flags & SHORTESTPATH));
}

/* move to transport */

bool
filling_transport(transport, unit)
     Unit	*transport, *unit;
{
  return transport->orders.type == FILL &&
    can_carry(transport, unit);
}

/* static variable set whenever reached_filling_transport_aux finds a filling
   transport */
static Unit	*fillingtransport;

reached_filling_transport_aux(unit, maybe)
     Unit	*unit, *maybe;
{
  Unit	*occ;

  if (maybe==NULL)
    return FALSE;

  if (filling_transport(maybe, unit)) {
    fillingtransport = maybe;
    return TRUE;
  }
  for_all_occupants(maybe, occ) {
    if (reached_filling_transport_aux(unit, occ)) {
      return TRUE;
    }
  }
  return FALSE;
}
reached_filling_transport(unit, x, y)
     Unit	*unit;
     int	x,y;
{
  return reached_filling_transport_aux(unit, unit_at(x,y));
}

move_to_transport(unit)
     Unit	*unit;
{
  int	dir;
  if (reached_filling_transport(unit, unit->x, unit->y)) {
    wake_unit(unit, NOTHING, WAKEOWNER, NULL); /* wake now in case there are
						standing orders */
    leave_hex(unit);
    occupy_unit(unit, fillingtransport);
    return FALSE;
  }

  if (--unit->orders.rept <= 0) {
    wake_unit(unit, NOTHING, WAKETIME, NULL);
    return FALSE;
  }

  dir = search_path(unit, 100, reached_filling_transport);
  if (dir<0) {
    notify(unit->side, "%s couldn't find filling transport",
	   unit_handle(unit->side, unit));
    wake_unit(unit, NOTHING, WAKEOWNER, NULL);
    return FALSE;
  }
  return move_to_next(unit, dirx[dir], diry[dir], TRUE, TRUE);
}

/* move toward a unit */
move_toward_unit(unit, lID)
     Unit	*unit;
     int	lID; /* leader ID */
{
  Unit	*dest;

  if (--unit->orders.rept <= 0) {
    wake_unit(unit, NOTHING, WAKETIME, NULL);
    return FALSE;
  }
  dest = find_unit(lID);
  if (dest==NULL) {
    notify(unit->side, "Destination unit is dead!");
    wake_unit(unit, NOTHING, WAKELEADERDEAD, (Unit *) NULL);
    return FALSE;
  } else if (!can_carry(dest, unit)) {
    notify(unit->side, "%s can't carry us!", unit_handle(unit->side, dest));
    wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
    return FALSE;
  } else if (unit->x == dest->x && unit->y == dest->y) {
    leave_hex(unit);
    occupy_unit(unit, dest);
    wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
    return FALSE;
  } else {
    return move_to(unit, dest->x, dest->y,
		   (unit->orders.flags & SHORTESTPATH));
  }
}

/* this algorithm requires the direction of movement and the position
of the border hex.
  forward is the direction the unit last moved in.
  ccw tells us where the border hex WAS. */

/* The direction-munging things should be abstracted. */

follow_coast(unit)
Unit *unit;
{
    Edge	*edge = &unit->orders.p.edge;
    int ccw = edge->ccw, olddir = edge->forward;
    int newdir;

    if (ccw==0) {
      /* we are trapped in an isthmus */
      int	count=0, workingdir = (-1);

#define trydir(d) newdir = (d); \
      if (could_move_in_dir(unit,newdir)) \
	  { count++; workingdir = newdir; }

      trydir(left_of(olddir));
      trydir(olddir);
      trydir(right_of(olddir));
#undef trydir
      if (count>1) {/* too many choices */
	wake_unit(unit, NOTHING, WAKELOST, NULL);
	notify(unit->side, "%s can't decide which coast to follow!",
	       unit_handle(unit->side, unit));
	return FALSE;
      } else if (count == 1) /* follow the isthmus */
	move_dir(unit, edge->forward = workingdir);
      else /* found the end.  backtrack */
	move_dir(unit, edge->forward = opposite_dir(olddir));

      return TRUE; /* the failure returns FALSE; for itself */

    } else {
      /* we have an edge direction */
      int	lastchance = normalize_dir(olddir + ccw*2);
      for ( newdir = normalize_dir(olddir + ccw);
	   newdir != lastchance; newdir = normalize_dir(newdir-ccw)) {
	if (could_move_in_dir(unit, newdir))
	  break;
      }
      if (could_move_in_dir(unit, newdir)) {
	move_dir(unit, edge->forward = newdir);
	return TRUE;
      } else {
	notify(unit->side, "ERROR: forward:%d ccw:%d @%d,%d", edge->forward,
	       edge->ccw, unit->x, unit->y);
	wake_unit(unit, NOTHING, WAKELOST, NULL);
	notify(unit->side, "%s can't figure out where to move to!",
	       unit_handle(unit->side, unit));
	return FALSE;
      }
    }
}

direction_works(unit, dir)
Unit *unit;
int dir;
{
    int nx, ny;

    nx = wrap(unit->x + dirx[dir]);  ny = limit(unit->y + diry[dir]);
    if (could_move(unit->type, terrain_at(nx, ny)) &&
	adj_obstacle(unit->type, nx, ny)) {
	move_dir(unit, dir);
	return TRUE;
    } else {
	return FALSE;
    }
}

adj_obstacle(type, x, y)
int type, x, y;
{
    int d, x1, y1;

    for_all_directions(d) {
	x1 = wrap(x + dirx[d]);  y1 = limit(y + diry[d]);
	if (!could_move(type, terrain_at(x1, y1))) return TRUE;
    }
    return FALSE;
}

/* Unit attempts to follow its leader around, but not too closely. */

follow_leader(unit)
Unit *unit;
{
    Unit *leader = find_unit(unit->orders.p.leader_id);
    int	dx;

    if ( --unit->orders.rept <=0 ) {
      wake_unit(unit, NOTHING, WAKETIME, NULL);
      return FALSE;
    }
    if (leader == NULL) {   /* find unit won't return dead leader */
	notify(unit->side, "Leader is dead.  I am without guidance!");
	wake_unit(unit, NOTHING, WAKELEADERDEAD, (Unit *) NULL);
	return FALSE;
    } else if ( ( (dx=abs(unit->x - leader->x)) < 2 ||
		 /* handle map wraps! */ dx > world.width-2 )  &&
	       abs(unit->y - leader->y) < 2) {
	unit->movesleft = 0;
	return TRUE;
    } else {
	return move_to(unit, leader->x, leader->y, FALSE);
    }
}

/* Patrol just does move_to, but cycling waypoints around when the first */
/* one has been reached. */

move_patrol(unit)
Unit *unit;
{
    int tx, ty;

    if (unit->orders.rept-- > 0) {
	if (unit->x == unit->orders.p.pt[0].x &&
	    unit->y == unit->orders.p.pt[0].y) {
	    tx = unit->orders.p.pt[0].x;
	    ty = unit->orders.p.pt[0].y;
	    unit->orders.p.pt[0].x = unit->orders.p.pt[1].x;
	    unit->orders.p.pt[0].y = unit->orders.p.pt[1].y;
	    unit->orders.p.pt[1].x = tx;
	    unit->orders.p.pt[1].y = ty;
	}
	return move_to(unit, unit->orders.p.pt[0].x, unit->orders.p.pt[0].y,
		       (unit->orders.flags & SHORTESTPATH));
    }
    return TRUE;
}


/* Auxiliary stuff used when searching for place to return to.  Note that */
/* a good refueling spot will be woken up, so it won't get too far away */
/* before unit has a chance to get there. */
/* Won't find refueling places inside other units, sigh. */

refuel_here(unit, x, y)
     Unit	*unit;
     int	x, y;
{
    Unit *transport = unit_at(x, y);

    enter_procedure("refuel_here");
    if (transport != NULL && transport->side == unit->side &&
	can_carry(transport, unit)) {
      wake_unit(transport, NOTHING, WAKEOWNER, (Unit *) NULL);
      exit_procedure();
      return TRUE;
    }
    exit_procedure();
    return FALSE;
}

return_to_base(unit)
Unit *unit;
{
  int	dir;
  if (refuel_here(unit, unit->x, unit->y)) {
    wake_unit(unit, NOTHING, WAKEOWNER, NULL);
    return FALSE;
  }

  if (--unit->orders.rept <= 0) {
    wake_unit(unit, NOTHING, WAKETIME, NULL);
    return FALSE;
  }

  dir = search_path(unit, 100, refuel_here);
  if (dir<0) {
    notify(unit->side, "%s couldn't find refueling point",
	   unit_handle(unit->side, unit));
    wake_unit(unit, NOTHING, WAKEOWNER, NULL);
    return FALSE;
  }
  return move_to_next(unit, dirx[dir], diry[dir], TRUE, TRUE);
}

/* Retreat is a special kind of movement. */
/* Veterans should get several tries at retreating to a good place, perhaps */
/* one try per point of "veteranness"? */

retreat_unit(unit)
Unit *unit;
{
    int dir;
    bool success;

    dir = random_dir();
    success = move_to_next(unit, dirx[dir], diry[dir], FALSE, FALSE);
    return success;
}


static int	unitdestx, unitdesty;
static bool
reached_dest(unit, x, y)
     Unit	*unit;
     int	x,y;
{
  return x == unitdestx
    && y == unitdesty;
}

/* This is the general routine for finding a path to a given point. */

move_to(unit, tx, ty, shortest)
     Unit *unit;
     int tx, ty;
     bool shortest;
{
    Side *side = unit->side;
    int	dir;
    int r;

    unitdestx = tx;  unitdesty = ty;
    if (unit->x == tx && unit->y == ty) {
	wake_unit(unit, NOTHING, WAKEOWNER, NULL);
	return FALSE;
    }
    dir = search_path(unit, 100, reached_dest);
    if (dir < 0) {
	/* We failed to find a path, but could just try moving anyway. */
#if 0
	if (side_view(unit->side, tx, ty) == UNSEEN) {
	    return wander_to(unit, tx, ty, shortest);
	} else {
#endif
	    wake_unit(unit, NOTHING, WAKEOWNER, NULL);
	    notify(unit->side, "%s couldn't find path from %d,%d to %d,%d",
		   unit_handle(side, unit), unit->x, unit->y, tx, ty);
	    if (Debug) {
		printf("%s couldn't find path from %d,%d(%s) to %d,%d(%s)\n",
		       unit_handle(side, unit), unit->x, unit->y,
		       ttypes[terrain_at(unit->x,unit->y)].name,
		       tx, ty, ttypes[terrain_at(tx, ty)].name);
		for_all_resource_types (r) {
		    printf("%s:%d ",rtypes[r].name, unit->supply[r]);
		}
		printf("\n");
	    }
	    return FALSE;
#if 0
	}
#endif
    } else {
	/* I'm completely ignoring any orders flags */
	return move_to_next(unit, dirx[dir], diry[dir], TRUE, TRUE);
    }
}

/* This weird-looking routine computes next directions for moving to a */
/* given spot.  The basic strategy is to prefer to go in the x or y distance */
/* that is the greatest difference, so as to even the two displacements out. */
/* (This leaves more options open if blockage several hexes away.)  Make it */
/* probabilistic, so repeated travel will eventually trace the envelope of */
/* possible moves.  The number of directions ranges from 1 to 4, depending */
/* on whether there is a straight-line path to the dest, and whether we are */
/* required to take a direct path or are allowed to move in dirs that don't */
/* the unit any closer (we never increase our distance though). */
/* Some trickinesses:  world is cylindrical, so must resolve ambiguity about */
/* getting to the same place going either direction (we pick shortest). */

wander_to(unit, tx, ty, shortest)
Unit *unit;
int tx, ty;
bool shortest;
{
    bool closer, success;
    int dx, dxa, dy, dist, d1, d2, d3, d4, axis = -1, hextant = -1, tmp;

    dist = distance(unit->x, unit->y, tx, ty);
    dx = tx - unit->x;  dy = ty - unit->y;

    dxa = (tx + world.width) - unit->x;
    if (abs(dx) > abs(dxa)) dx = dxa;
    dxa = (tx - world.width) - unit->x;
    if (abs(dx) > abs(dxa)) dx = dxa;
    if (dx == 0 && dy == 0) {
	wake_unit(unit, NOTHING, WAKEOWNER, (Unit *) NULL);
	return FALSE;
    }
    axis = hextant = -1;
    if (dx == 0) {
	axis = (dy > 0 ? NE : SW);
    } else if (dy == 0) {
	axis = (dx > 0 ? EAST : WEST);
    } else if (dx == (0 - dy)) {
	axis = (dy > 0 ? NW : SE);
    } else if (dx > 0) {
	hextant = (dy > 0 ? EAST : (abs(dx) > abs(dy) ? SE : SW));
    } else {
	hextant = (dy < 0 ? WEST : (abs(dx) > abs(dy) ? NW : NE));
    }
    if (axis >= 0) {
	d1 = d2 = axis;
    }
    if (hextant >= 0) {
	d1 = left_of(hextant);
	d2 = hextant;
    }
    d3 = left_of(d1);
    d4 = right_of(d2);
    closer = (shortest || dist == 1);
    if (flip_coin()) {
        tmp = d1;  d1 = d2; d2 = tmp;
    }
    success = move_to_next(unit, dirx[d1], diry[d1], FALSE, 1);
    if (!success)
	success = move_to_next(unit, dirx[d2], diry[d2], closer, 1);
    if (!success && !closer) {
	if (opposite_dir(unit->lastdir) == d3) {
	    success = move_to_next(unit, dirx[d4], diry[d4], TRUE, 1);
	} else if (opposite_dir(unit->lastdir) == d4) {
	    success = move_to_next(unit, dirx[d3], diry[d3], TRUE, 1);
	} else {
	    success = move_to_next(unit, dirx[d3], diry[d3], FALSE, 1);
	    if (!success)
		success = move_to_next(unit, dirx[d4], diry[d4], TRUE, 1);
	}
    }
    return success;
}

/* This function and a couple auxes encode most of the rules about how */
/* units can move.  Attempts to move onto other units are handled */
/* by other functions below.  Unit will also refuse to move onto the edge of */
/* the map or into the wrong kind of terrain.  Otherwise, it succeeds in its */
/* movement and we put it at the new spot.  If at any time, the unit */
/* could not move and yet was supposed to, it will wake up. */

move_to_next(unit, dx, dy, mustgo, atk)
Unit *unit;
int dx, dy;
bool mustgo, atk;
{
    bool offcourse = FALSE, success = FALSE;
    int nx, ny, utype = unit->type;
    Unit *unit2;
    Side *us = unit->side;

    nx = wrap(unit->x + dx);  ny = unit->y + dy;

/*** (UK) insert -> ***/
    if ((unit2 = unit->transport) != NULL && 
	(unit->movesleft-=utypes[utype].leavetime[unit2->type])<0 &&
	!utypes[utype].freemove[unit2->type]) {
      if (human_order(unit) && mustgo) {
	  cmd_error(us, "%s cannot leave %s!",unit_handle(unit->side,unit),
	    unit_handle(unit->side,unit2));
      }
    } else
/*** <- insert ***/
    if ((unit2 = unit_at(nx, ny)) != NULL) {
	if (neutral(unit)) return(FALSE);
	success = move_to_unit(unit, unit2, dx, dy, mustgo, atk, offcourse);
    } else if (!between(1, ny, world.height-2)) {
	if (neutral(unit)) return(FALSE);
	if (global.leavemap) {
	    kill_unit(unit, DISBAND);
	} else if (offcourse) {
	    notify(us, "%s has fallen off the edge of the world!",
		   unit_handle(us, unit));
	    kill_unit(unit, DISASTER);
	} else if (human_order(unit) && mustgo) {
	    cmd_error(us, "%s can't leave this map!",
		      unit_handle(us, unit));
	}
    } else if (!could_move(utype, terrain_at(nx, ny))) {
	if (neutral(unit)) return(FALSE);
	if (offcourse) {
	    notify(us, "%s has met with disaster!", unit_handle(us, unit));
	    kill_unit(unit, DISASTER);
	} else if (human_order(unit) && mustgo) {
	    cmd_error(us, "%s won't go into the %s!",
		      unit_handle(us, unit), ttypes[terrain_at(nx, ny)].name);
	}
    } else {
	move_unit(unit, nx, ny);
	unit->lastdir = find_dir(dx, dy);
	success = TRUE;
    }
    /* Units don't get dead by failing to move, so test not needed here. */
    if (!success && mustgo) {
/*	unit->awake = TRUE;  */
	wake_unit(unit, NOTHING, WAKELOST, (Unit *) NULL);
    }
    return success;
}

/* An enemy unit will be attacked, unless unit is on a transport *and* it */
/* cannot move to that hex anyway. */
/* Also will refuse if hit prob < 10% (can still defend tho) */
/* If the attackee is destroyed, then unit will attempt to move in again. */
/* A friendly transport will be boarded unless it is full. */
/* Blank refusal to move if any other unit. */

/* Allies treat each other's units as their own. */

move_to_unit(unit, unit2, dx, dy, mustgo, atk, offcourse)
Unit *unit, *unit2;
int dx, dy;
bool mustgo, atk, offcourse;
{
    int u = unit->type, u2 = unit2->type, u2x = unit2->x, u2y = unit2->y;
    Side *us = unit->side;

    if (!allied_side(us, unit2->side)) {
	if (unit->transport != NULL && impassable(unit, u2x, u2y)
	    && !utypes[u2].bridge[u]) {
	    if (human_order(unit) && mustgo) {
		cmd_error(us, "%s can't attack there!", unit_handle(us, unit));
	    }
	} else if ( us != NULL && side_view(us, u2x, u2y) == EMPTY) {
	    if( us != NULL ){
		notify(us, "%s spots something!", unit_handle(us, unit));
		see_exact(us, u2x, u2y);
		draw_hex(us, u2x, u2y, TRUE);
/*** (UK) insert -> ***/
		/* if you want to occupy a hex of someone who can attack you */
		/* it will do it */
		/* may be used to support passive mines */
                if (could_counterattack(u,u2)) {
		    if (attack(unit, unit2)) {
			/* if battle won, can try moving again */
			return move_to_next(unit, dx, dy, FALSE, FALSE);
		    }
		    return FALSE;
                }
/*** <- insert ***/
	    }
	} else if (atk && (unit->orders.flags & ATTACKUNIT)) {
	    if (!could_hit(u, u2) && !could_capture(u, u2)) {
/*** (UK) insert -> ***/
		/* if you want to occupy a hex of someone who can attack you */
		/* it will do it */
		/* may be used to support passive mines */
                if (could_counterattack(u,u2)) {
		    if (attack(unit, unit2)) {
			/* if battle won, can try moving again */
			return move_to_next(unit, dx, dy, FALSE, FALSE);
		    }
		    return FALSE;
                }
/*** <- insert ***/
		if (human_order(unit) && mustgo) {
/*** (UK) change -> ***/
		    cmd_error(us, "%s cannot attack a %s!",
/*** was:
		    cmd_error(us, "%s refuses to attack a %s!",
*** <- change ***/
			      unit_handle(us, unit), utypes[u2].name);
		}
	    } else if (!enough_ammo(unit, unit2)) {
		if (human_order(unit) && mustgo) {
		    cmd_error(us, "%s is out of ammo!", unit_handle(us, unit));
		}
	    } else {
		if (attack(unit, unit2)) {
		    /* if battle won, can try moving again */
		    return move_to_next(unit, dx, dy, FALSE, FALSE);
		}
		return TRUE;
	    }
	} else {
	    /* here if aircraft blocked on return, etc - no action needed */
	}
    } else if (could_carry(u2, u)) {
	if (!can_carry(unit2, unit)) {
	    if (human_order(unit) && mustgo) {
		cmd_error(us, "%s is full already!", unit_handle(us, unit2));
	    }
	} else {
/*** (UK) insert -> ***/
            if (utypes[u].freemove[u2] || 
		unit->movesleft >= utypes[u].entertime[u2]) {
/*** <- insert ***/
	    move_unit(unit, u2x, u2y);
	    unit->movesleft -= utypes[u].entertime[u2];
	    unit->lastdir = find_dir(dx, dy);
	    return TRUE;
/*** (UK) insert -> ***/
            }
	    else {
	      if (human_order(unit) && mustgo) {
		  cmd_error(us, "%s cannot enter %s!",
		    unit_handle(us, unit),
		    unit_handle(us, unit2));
	      }
	    }
/*** <- insert ***/
 	}
    } else if (could_carry(u, u2)) {
	if (impassable(unit, u2x, u2y)) {
	    if (human_order(unit) && mustgo) {
		cmd_error(us, "%s can't pick up anybody there!",
			  unit_handle(us, unit));
	    }
/*** (UK) insert -> ***/
        } else if (!utypes[u2].freemove[u] && 
		   utypes[u2].entertime[u]>utypes[u2].speed) {
	    if (human_order(unit) && mustgo) {
		cmd_error(us, "%s can't pick up %s!",
			  unit_handle(us, unit),unit_handle(us,unit2));
	    }
/*** <- insert ***/
	} else if (!can_carry(unit, unit2)) {
	    if (human_order(unit) && mustgo) {
		cmd_error(us, "%s is full already!", unit_handle(us, unit));
	    }
	} else if (unit->orders.flags == 0) {
	    /* blow out at the bottom */
	} else {
	    move_unit(unit, u2x, u2y);
	    unit->lastdir = find_dir(dx, dy);
	    return TRUE;
	}
    } else {
	if (offcourse) {
	    notify(us, "%s is involved in a wreck!", unit_handle(us, unit));
	    kill_unit(unit, DISASTER);
	    kill_unit(unit2, DISASTER);
	} else if (human_order(unit) && mustgo) {
	    cmd_error(us, "%s refuses to attack its friends!",
		      unit_handle(us, unit));
	}
    }
    return FALSE;
}

/* Perform the act of moving proper. (very simple, never fails) */
/* This is also the right place to put in anything that happens if the *
/* unit actually changes its location. */

/*** (HW) insert -> ***/
UpdateMoveLineRedraw(side,unit)
Side *side;
Unit *unit;
{
  if (side && side->show_orders && & unit->orders.type == MOVETO) {
    RedrawHexArray(side,unit->x,unit->y,
		   unit->orders.p.pt[0].x,unit->orders.p.pt[0].y);
    side->redraw = TRUE;
  }
}

UpdateSOMoveLineRedraw(side,unit,order)
Side *side;
Unit *unit;
Order *order;
{
  if (side->showsorderutype != NOTHING) {
    if (order->type == MOVETO) {
      RedrawHexArray(side,unit->x,unit->y,order->p.pt[0].x,order->p.pt[0].y);
      side->redraw = TRUE;
    }
  }
}

CheckRedraw(side)
Side *side;
{
  if (side && side->redraw && active_display(side)) {
    draw_smoveorders(side,-1,-1);
    draw_moveorders(side,-1,-1);
    side->redraw = FALSE;
  }
}
/*** <- insert ***/

move_unit(unit, nx, ny)
Unit *unit;
int nx, ny;
{
/*** (HW) insert -> ***/
    int x2,y2;
    
    IncrementSORedrawLevel(unit->side);
/*** <- insert ***/
    cancel_build(unit);
/*** (HW) insert -> ***/
    if (unit->orders.type == MOVETO) {
      x2 = unit->orders.p.pt[0].x; y2 = unit->orders.p.pt[0].y;
      UpdateMoveLineRedraw(unit->side,unit);
      if (x2 == nx && y2 == ny) unit->orders.type = AWAKE;
    }
/*** <- insert ***/
    leave_hex(unit);
    occupy_hex(unit, nx, ny);
    wake_neighbors(unit);
    consume_move_supplies(unit);
/*** (HW) insert -> ***/
    DecrementSORedrawLevel(unit->side);
/*** <- insert ***/
}

/* This routine is too strict, doesn't account for resupply at start of next */
/* turn.  Hacked to check on transport at least. */
/* Only die now if will die during consumption phase. */

consume_move_supplies(unit)
Unit *unit;
{
    int u = unit->type, r;
    
    unit->actualmoves++;
    for_all_resource_types(r) {
	if (utypes[u].tomove[r] > 0) {
	    unit->supply[r] -= utypes[u].tomove[r];
	    if (unit->supply[r] <= 0 && unit->transport == NULL) {
		if (utypes[u].consume[r] > 0) {
		    exhaust_supply(unit);
		    return;
		}
	    }
	}
    }
}

/* When doing a survey, you can move the cursor anywhere and it will show */
/* what is at that hex, but not give away too much! */
extern Side *mside;
extern Unit *munit;
extern int max_range, munit_regions;
move_survey(side, nx, ny)
Side *side;
int nx, ny;
{
/*** (UK) insert -> ***/
    if (nx!=side->curx || ny!=side->cury) {
      side->last_x=side->curx;
      side->last_y=side->cury;
    }
/*** <- insert ***/
    if (between(1, ny, world.height-2)) {
	side->curx = nx;  side->cury = ny;
	make_current(side, unit_at(side->curx, side->cury));
    } else {
	if (active_display(side)) beep(side);
    }
    put_on_screen(side, side->curx, side->cury);
    show_info(side);
/*    if (side->movunit != NULL) {
      mside = side;
      munit = side->movunit;
      munit->goal = 1;
      munit_regions = regions_around(munit->type, nx, ny, FALSE);
      max_range = 1000;
      notify(side, "Cost for hex is %d ",
	     evaluate_hex(nx, ny));
    } */
}

/* This routine encodes nearly all of the conditions under which a unit */
/* following orders might wake up and request new instructions. */

maybe_wakeup(unit)
Unit *unit;
{
    bool goinghome;
    int tx, ty;

    if (unit->orders.rept <= 0) {
/*** (UK) insert -> ***/
        if (set_follow_order(unit)) {
	  wake_unit_to_move_again(unit);
	  CheckRedraw(unit->side);
        }
	else wake_unit(unit, NOTHING, WAKETIME, (Unit *) NULL);
/*** <- insert ***/
    } else if ((unit->orders.flags & SUPPLYWAKE) && low_supplies(unit)) {
	goinghome = FALSE;
	if (unit->orders.type == MOVETO) {
	    tx = unit->orders.p.pt[0].x;  ty = unit->orders.p.pt[0].y;
	    if (refuel_here(unit,tx,ty)) { /* shouldn't wake the transport? */
		goinghome = TRUE;
	    }
	}
	if (!goinghome) {
	    wake_unit(unit, NOTHING, WAKERESOURCE, (Unit *) NULL);
	    unit->orders.flags &= ~SUPPLYWAKE;
	}
    } /*
	Don't need this since a unit is woken up when another moves
	next to it or it moves next to another.  If you give a unit
	orders near the enemy, your problem.
	else if ((unit->orders.flags & ENEMYWAKE) && adj_enemy(unit)) {
	wake_unit(unit, NOTHING, WAKEENEMY, other);
    } */
}

/* True if unit is adjacent to an unfriendly. */

adj_enemy(unit)
Unit *unit;
{
    int d, x, y;
    viewdata view;

    for_all_directions(d) {
	x = wrap(unit->x + dirx[d]);  y = unit->y + diry[d];
	if (!neutral(unit)) {
	    view = side_view(unit->side, x, y);
	    if (view != EMPTY && enemy_side(side_n(vside(view)), unit->side))
		return TRUE;
	}
    }
    return FALSE;
}

/* Wake up anyone who is next to us if they see us. */

wake_neighbors(unit)
Unit *unit;
{
  int d, x, y;
  viewdata view;
  Unit *other;
  
  for_all_directions(d) {
    x = wrap(unit->x + dirx[d]);  y = unit->y + diry[d];
    if (!neutral(unit)) {
      view = side_view(unit->side, x, y);
      if (view != EMPTY && enemy_side(side_n(vside(view)), unit->side)) 
	wake_unit(unit, NOTHING, WAKEENEMY, unit_at(x,y));
    }
    if ((other = unit_at(x,y)) != NULL) 
      if (!neutral(other)) {
	view = side_view(other->side, unit->x, unit->y);
	if (view != EMPTY && enemy_side(other->side, unit->side)) 
	  wake_unit(other, -1, WAKEENEMY, unit);
      }
  }
}
