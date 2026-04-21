/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* To accommodate simultaneous movement by several players, our setup must */
/* be a little elaborate.  Basically, the program keeps the initiative; */
/* when a side's movement phase needs input, it posts a "request" and a */
/* handler for that request.  When X gets an event, data about it is */
/* preprocessed and then stuffed into the request structure.  At the end */
/* of a movement subphase, the requests are fulfilled by calling the handler */
/* and passing it the side and a pointer to the request. */

/* The bad part about all this is that *every* *single* interaction must */
/* be handled this way - no more calling a routine to get a value and */
/* waiting until it is complete! :-( */

/* This last restriction does not seem necessary if the request is being */
/* made by the current side.  In that case, additional requests being */
/* made means that requests must be handled immediately anyways. */

   

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "global.h"
#include "map.h"

/* Magic values (unlikely to show up as player input) */

#define DFLTFLAG (-12345)       /* flags when default arg to be used */
#define NEGFLAG  (-12346)       /* flags when default arg to be used */
#define ITERFLAG (-12347)       /* flags args that are "itertime" */
#define NEXTPAGE (-101010)      /* Print these commands on the next page */

/*** (UK) remove -> ***
#define ESCAPE '\033'           
#define BACKSPACE '\010'       
*** <- remove ***/

/* The following structure is used only for the dispatch table. */

typedef struct func_tab {
    char fchar;                 /* character to match against */
    int (*code)();              /* pointer to command's function */
    int defaultarg;             /* default for argument */
    bool needunit;              /* true if cmd always applies to a unit */
    char *help;                 /* short documentation string */
} FuncTab;

extern x_help();
/* Predeclarations of all commands. */

int do_sentry(), do_delay(), do_wakeup(), do_wakemain(), do_embark(),
    do_fill(), do_exit(), do_resign(), do_message(), do_follow(),
    do_redraw(), do_flash(), do_disband(), do_return(), do_movetotransport(),
    do_sit(), do_product(), do_idle(), do_help(), do_version(),
    do_save(), do_standing(), do_ident(), do_moveto(), do_coast(),
    do_survey_mode(), do_unit_info(), do_printables(), do_occupant(),
    do_options(), do_name(), do_unit(), do_give_unit(), do_center(),
    do_mark_unit(), do_add_player(), do_patrol(), do_give(), do_take(),
    do_recenter(), do_look(), do_distance(), do_find_unit(), do_nothing(),
    do_find_next_unit(), do_next_product(), do_auto(), do_next_dead(),
    do_movetounit(),
    do_wakespec(),
/*** (HW) reinsert -> ***/
    do_next_producer(), do_show_producer(), do_next_unit(), do_show_unit(),
    do_sentry_occupants(),do_take_all(),
    do_remove_standing(), do_show_standing(), do_show_production(),
    do_visibility(),
/*** <- reinsert ***/
/*** (UK) insert -> ***/
    do_afteroccupants(), do_terraform(),
    do_pickup(), do_sentry_occupants(),do_take_all(),
    do_seenenemy(), do_returnenemy(), do_condition(),
    do_saveposition(), do_backposition(),
    do_open(), do_close(), do_passwd(),
    do_unload(), do_leave(), do_group(), do_cancel(), do_pickup_amount()
/*** <- insert ***/
  ;

#ifndef NO_DISEMBARK
int do_disembark();
#endif

char dirchars[] = DIRCHARS;     /* typable characters for directions */

/* The command table itself.  Default iterations of ITERFLAG actually turn */
/* into the value of "itertime", which can be set by options cmd. */

FuncTab commands[] = {
    '\200', NULL, 0, 0,
      "---------- UNIT COMMANDS ----------------------------------------",
    'd', do_delay, 1, TRUE, "delay unit's move until later in turn",
/*** (UK) insert -> ***/
    'Z', do_afteroccupants, 1, TRUE, "delay unit's move until occupants have moved",
/*** <- insert ***/
    's', do_sentry, ITERFLAG, TRUE, "put a unit on sentry duty",
/*** (UK) insert -> ***/
    '\023', do_sentry_occupants, 100, TRUE, "sentry occupants",
/*** <- insert ***/
    'w', do_wakemain, 0, FALSE, "wake units up from whatever they were doing",
    '\027', do_wakespec, 0, FALSE, "wake specific kind of unit",
    'W', do_wakeup, 0, FALSE, "wake ALL units in area, including occupants",
    'm', do_moveto, 1, TRUE, "move to given location",
    ' ', do_sit, 1, FALSE, "(Space Bar) make unit be on sentry this turn only",
    'r', do_return, ITERFLAG, TRUE, "return unit to nearest city/transport",
/*** (HW) change -> ***/
    '$', do_movetotransport, ITERFLAG, TRUE, "move to filling transport",
/*** was:
    'E', do_movetotransport, ITERFLAG, TRUE, "move to filling transport",
*** <- change ***/
    'f', do_follow, ITERFLAG, TRUE, "follow a designated leader",
/*** (UK) change -> ***/
    '\012', do_movetounit, ITERFLAG, TRUE, "move to the marked unit",
/*** was:
    '\015', do_movetounit, ITERFLAG, TRUE, "move to the marked unit",
*** <- change ***/
    'e', do_embark, 0, TRUE, "embark units onto transport occupying same unit",
/*** (UK) change -> ***/
    'E', do_pickup, 1, TRUE, "recursivly pickup units onto this unit",
    '\005', do_pickup_amount, 1, TRUE, "pickup units onto this unit",
/*** was: (in 5.5.old)
    'E', do_embark_all, 1, TRUE, "embark units onto this unit",  
*** <- change ***/
    '\006', do_fill, ITERFLAG, TRUE, "order transport to sleep until full",
/*** (UK) insert -> ***/
    'j', do_leave, 1, TRUE, "leave current transport",
    'J', do_unload, 1, TRUE, "unload units from this unit",
/*** <- insert ***/
#ifndef NO_DISEMBARK
    '@', do_disembark, 0, TRUE, "disembark units from one transport into unit", 
#endif
    'g', do_give, -1, TRUE, "give supplies to the transport",
    't', do_take, -1, TRUE, "take supplies from the transport",
    'x', do_mark_unit, 1, TRUE, "mark unit for later reference",
    'F', do_coast, ITERFLAG, TRUE, "follow a coastline",
    'p', do_patrol, ITERFLAG, TRUE, "go on a patrol",
    'P', do_product, 1, TRUE, "set/change unit production",
/*** (UK) insert -> ***/
    '_', do_terraform, 1, TRUE, "set/change unit terraforming",
    '*', do_group, 0,FALSE, "set/change unit group",
/** <- insert ***/
    '\020', do_next_product, 1, TRUE, "set next product",
    'I', do_idle, ITERFLAG, TRUE, "set unit to not produce anything",
    'C', do_name, 1, FALSE, "call a unit or side by a name",
    '\001', do_auto, 1, TRUE, "Turn unit over to machine control",
    'D', do_disband, 1, TRUE, "disband a unit and send it home",
    'G', do_give_unit, -2, TRUE, "turn unit over to another side",
/*** (HW) reinsert -> ***/
    'T', do_take_all, 1, TRUE, "take supplies from the transport recursively",
    '!', do_show_production, 1, FALSE, "show production of unit type",
/*** <- reinsert ***/
/*** (HW) rechange -> ***/
    'a', do_occupant, 1, TRUE, "look at the occupants",
/*** was
    'i', do_occupant, 1, TRUE, "look at the occupants",
*** <- rechange ***/
    ' ', NULL, 0, 0, "",
    '\200', NULL, 0, 0,
      "--------------- UNIT ITERATION COMMANDS --------------------------",
    '>', do_show_producer, 1, FALSE, "show producer of specified product",
    '.', do_next_producer, 1, TRUE, "show next producer of current product",
    '<', do_show_unit, 1, FALSE, "show unit of specified type",
    ',', do_next_unit, 1, TRUE, "show next unit of current type",
/*** (HW) change -> ***/
    '^', do_find_unit, -1, FALSE, "find a unit type, (arg gives unit number)",
/*** was:
    'T', do_find_unit, -1, FALSE, "find a unit type, (arg gives unit number)",
*** <- change ***/
    '\024', do_find_next_unit, -1, FALSE,
           "find next unit, (arg gives unit number)",
    '\004', do_next_dead, -1, FALSE, "Show next dead unit.",
/*** <- reinsert ***/
    ' ', NULL, NEXTPAGE, 0, "",
    '\200', NULL, 0, 0,
      "--------------- STANDING ORDER COMMANDS --------------------------",

/*** (UK) change -> ***/
    'O', do_standing, 0, TRUE, "set standing orders for occupants",
/*** was:
    'O', do_standing, 1, TRUE, "set standing orders for occupants",
*** <- change ***/
/*** (HW) reinsert -> ***/
    'i', do_show_standing, 0, TRUE, "inform about standing orders",
    'R', do_remove_standing, 0, TRUE, "remove standing order",
/*** (UK) change -> ***/
    DELETE, do_nothing, 1, FALSE, "delete a standing order",
/*** was:
    '\177', do_nothing, 1, FALSE, "delete a standing order",
*** <- change ***/ 
/*** (UK) insert -> ***/
    ESCAPE, do_cancel, 1, FALSE, "cancel teach mode",
    '\017', do_condition, 1, FALSE, "change condition while in teach mode",
/*** <- insert ***/ 
    ' ', NULL, 0, 0, "",
    '\200', NULL, 0, 0,
      "--------------- GLOBAL COMMANDS ----------------------------------",
    'z', do_survey_mode, 1, FALSE, "toggle between survey and move modes",
    'c', do_center, 1, FALSE, "designate current location as center of action",
    'M', do_message, -2, FALSE, "send a message to other sides",
    '\\', do_unit, 1, FALSE, "build a new unit (Build mode only)",
    'A', do_add_player, 1, TRUE, "add a new player to the game",
    'V', do_version, 1, FALSE, "display program version",
    'S', do_save, 1, FALSE, "save the game into a file",
    'X', do_resign, -2, FALSE, "resign from the game",
    'Q', do_exit, 1, FALSE, "kill game for all players",
    ' ', NULL, 0, 0, "",
    '\200', NULL, 0, 0,
      "--------------- DISPLAY OPTIONS ----------------------------------",
    'o', do_options, 6, FALSE, "set various options",
    ' ', NULL, 0, 0, "Change (d)isplay Mode, (g)raph, Win (h)eight,",
    ' ', NULL, 0, 0, "(i)nverse Video, (2)-color, (n)otice Buffer",
    ' ', NULL, 0, 0, "(r)obot, Start (b)eep Time, Win (w)idth",
    ' ', NULL, 0, 0, "World Map (m)agnification, (v)isibility, (M)overs only",
    ' ', NULL, 0, 0, "(p)roducts, (P)roducers, (o)rders, (Ss)tanding Orders",
    ' ', NULL, 0, 0, "(a)rrow Mode, (t)hin/Thick Lines, (e)Distinct Sorders",
    ' ', NULL, 0, 0, "(l)ook Mode, (z) Working Mode, (?) Help",
/*** (UK) change -> ***/
    '\b',do_backposition,0,FALSE,"back to last position",
    '\030',do_saveposition,1,FALSE,"save current position",
    '\t',do_seenenemy, 0, FALSE, "jump to last seen enemy",
    '\r',do_returnenemy,0, FALSE, "return to old or saved position",
    ' ', NULL, 0, 0, "   cancel old enemy return position with ESC",
    '`', do_recenter, 1, FALSE, "center the screen around current location",
/*** was:
    '.', do_recenter, 1, FALSE, "center the screen around current location",
*** <- change ***/
    'v', do_flash, 1, FALSE, "highlight current position",
    '\022', do_redraw, 10, FALSE, "redraw screen erase very old views",
    '\014', do_redraw, 0, FALSE, "redraw screen",
           /* this is really hardwired into X11.c */
    '\003', do_close, 0, FALSE, "close display",
    '\026', do_open, 0, FALSE, "open display", 
    '\016', do_passwd, 0, FALSE, "set password", 
    ' ', NULL, 0, 0, "",
    '\200', NULL, 0, 0,
      "--------------- HELP COMMANDS ------------------------------------",
    '?', do_help, 1, FALSE, "display help info",
    '/', do_ident, 1, FALSE, "identify things on screen",
    '=', do_unit_info, 1, FALSE, "display details about a type of unit",
/*** (UK) remove -> ***
    '\023', do_look, -1, FALSE, "watch the action point when not your turn",
*** <- remove ***/
    '#', do_distance, 1, FALSE, "measure the distance from the current point",
    '+', do_printables, 1, FALSE, "put various data into files for printing",
    '\0', NULL, 0, FALSE, NULL   /* end of table marker */
};

/* Just to get everything off on the right foot. */

init_requests(side)
Side *side;
{
    side->reqvalue = DFLTFLAG;
}

/* Post a request - need only know the handler that will be called to */
/* fulfill the request later, and sometimes a relevant unit. */

request_input(side, unit, handler)
Side *side;
Unit *unit;
int (*handler)();
{
    if (Debug) printf("Making %s request ", side->name);
    if (!humanside(side))
      fprintf(stderr, "Gack!\n");
    if (!side->reqactive) {
	if (Debug) printf("(accepted)\n");
	side->reqactive = TRUE;
	side->reqhandler = handler;
	side->reqtype = GARBAGE;
	side->requnit = unit;
	side->reqchange = TRUE;
	/* any other data slots are setup by callers of this routine */
    } else {
	if (Debug) printf("(ignored)\n");
    }
}

/* Make a request go away.  Careful about using this! */

cancel_request(side)
Side *side;
{
    if (side->reqactive) {
	if (Debug) printf("Canceling %s request\n", side->name);
	side->reqactive = FALSE;
	side->reqchange = TRUE;
	if (side->reqhandler == x_help) {
	  conceal_help(side);
	  redraw(side);
	} else {
	  erase_cursor(side);
	  clear_info(side);
	  flush_output(side);
	}
    }
    flush_input(side);
}



/* Handle all requests at once.  This routine *will* wait forever for */
/* input (polling is evil).  This is also the right place to draw the */
/* unit cursor, since it is unwanted unless we are waiting on input. */

handle_requests()
{
    bool waiters = FALSE;
    Side *side;
/*** (HW) insert -> ***/
    extern Side *curside;
/*** <- insert ***/

    for_all_sides(side) {
	if (side->reqactive) {
	    if (Debug) printf("Handling request for %s\n", side->name);
	    waiters = TRUE;
	    /* wasteful to call this for everybody... */
	    if (side->reqchange) { 
	      show_info(side);
	      draw_cursor(side);
	      flush_output(side);
	      side->reqchange = FALSE;
	    } 
	}
    }
    if (waiters) {
	get_input();
	for_all_sides(side) {
	    if (side->reqactive) {
		if (side->reqtype != GARBAGE) {
		    if (Debug) printf("Answering %s request with inptype %d\n",
				      side->name, side->reqtype);
		    side->reqactive = FALSE;
/*** (HW) insert -> ***/
		    curside = side;
/*** <- insert ***/
		    (*(side->reqhandler))(side);
		    side->reqchange = TRUE;
		    erase_cursor(side);
		    clear_info(side);
		    flush_output(side);
		    side->reqtype = GARBAGE;
		  }
	      }
	    if (!humanside(side) && side->reqtype == KEYBOARD &&
		side->reqch == ESCAPE) {
		side->humanp = TRUE;
		numhumans++;
		init_sighandlers();
	      }

	}
    }
}

/* Acquire a command from the player.  Command may be prefixed with a */
/* nonnegative number which is given as an argument to the command function */
/* (otherwise arg defaults to value in third column of command table). */
/* This routine takes in both keyboard and mouse input, other kinds of */
/* devices should be handled here also (rriiiight...).  Don't do any of */
/* this if the clock ran out, just supply an innocuous command. */

x_command(side)
Side *side;
{
    int dir, terr, sign = 1;

    switch (side->reqtype) {
    case KEYBOARD:
	if (Debug) printf("%s keyboard input: %c (%d)\n",
			  side->name, side->reqch, side->reqvalue);
	if (isdigit(side->reqch)) {
	    if (side->reqvalue == DFLTFLAG) {
		side->reqvalue = 0;
	    } else if (side->reqvalue == NEGFLAG) {
		side->reqvalue = 0;
		sign = -1;
	    } else {
		side->reqvalue *= 10;
	    }
	    side->reqvalue +=
		(side->reqvalue >= 0 ? 1 : -1) * (side->reqch - '0');
	    side->reqvalue *= sign;
	    sprintf(side->promptbuf, "Arg: %d", side->reqvalue);
	    show_prompt(side);
	    request_input(side, side->curunit, x_command);
	    return;
	} else {
	    clear_prompt(side);
	    if (Build && ((terr = find_terr(side->reqch)) >= 0)) {
		if (side->reqvalue == DFLTFLAG) side->reqvalue = 0;
		do_terrain(side, terr, side->reqvalue);
	    } else if (side->reqch == '-') {
		side->reqvalue = NEGFLAG;
		request_input(side, side->curunit, x_command);
		return;
	    } else if ((dir = iindex(side->reqch, dirchars)) >= 0) {
		if (side->reqvalue == DFLTFLAG) side->reqvalue = 1;
		do_dir(side, dir, side->reqvalue);
	    } else if ((dir = iindex(lowercase(side->reqch), dirchars)) >= 0) {
		if (side->reqvalue == DFLTFLAG)
		    side->reqvalue = side->itertime;
		if (side->mode == SURVEY) side->reqvalue = 10;
		do_dir(side, dir, side->reqvalue);
	    } else {
		execute_command(side, side->reqch, side->reqvalue);
	    }
	}
	break;
    case MAPPOS:
	if (Debug) printf("%s map input: %d,%d (%d)\n",
			  side->name, side->reqx, side->reqy, side->reqvalue);
	clear_prompt(side);
	if (side->reqx == side->curx && side->reqy == side->cury) {
	    if (side->curunit) do_sit(side, 1);
	    break;
	} else {
	    if (side->teach) {
		cache_moveto(side, side->reqx, side->reqy);
	    } else {
		switch (side->mode) {
		case MOVE:
		    if (side->curunit) {
		      Unit	*dest;
		      if (side->reqch &&
			  NULL!=(dest=unit_at(side->reqx, side->reqy))) {
			order_movetounit(side->curunit, dest, side->itertime);
		      } else {
			order_moveto(side->curunit, side->reqx, side->reqy);
			side->curunit->orders.flags &= ~SHORTESTPATH;
		      }
		    }
		    break;
		case SURVEY:
		    move_survey(side, side->reqx, side->reqy);
		    break;
		default:
		    case_panic("mode", side->mode);
		}
	    }
	}
	break;
    case SCROLLUP: 
	if (side->bottom_note + side->nh >= MAXNOTES) beep(side);
	else side->bottom_note = min(side->bottom_note + side->nh,
				     MAXNOTES - side->nh);
	show_note(side,TRUE);
	break;
    case SCROLLDOWN:
	if (side->bottom_note == 0) beep(side);
	else side->bottom_note = max(0, side->bottom_note - side->nh);
	show_note(side,TRUE);
	break;
    default:
	break;
    }
    if (global.giventime && !side->timedout && side->timeleft <= 0) {
	side->timedout = TRUE;
	notify(side, "You ran out of time!!");
	beep(side);
    }
    /* Reset the arg so we don't get confused next time around */
    side->reqvalue = DFLTFLAG;
}

/* The requester proper, which just sets up the hook to call the main */
/* command interpreter when some input comes by. */

request_command(side)
Side *side;
{
    request_input(side, side->curunit, x_command);
}

/* Search in command table and execute function if found, complaining if */
/* the command is not recognized.  Many commands operate on the "current */
/* unit", and all uniformly error out if there is no current unit, so put */
/* that test here.  Also fix up the arg if the value passed is one of the */
/* specially recognized ones. */

execute_command(side, ch, n)
Side *side;
char ch;
int n;
{
    struct func_tab *cmd;
    
    for (cmd = commands; cmd->fchar != '\0'; ++cmd) {
	if (ch == cmd->fchar) {
	    if (n == DFLTFLAG) n = cmd->defaultarg;
	    if (n == NEGFLAG)  n = 0 - cmd->defaultarg;
	    if (n == ITERFLAG) n = side->itertime;
	    if (Debug) printf("... actual arg is %d\n", n);
	    if (cmd->needunit) {
		if (side->curunit != NULL /* && unit belongs to side */ ) {
		    (*(cmd->code))(side, side->curunit, n);
		} else {
		    cmd_error(side, "No unit to operate on here!");
		}
	    } else {
		(*(cmd->code))(side, n);
	    }
	    return;
	}
    }
    cmd_error(side, "Unknown command '%c'", ch);
}

/* Help for the main command mode just dumps part of the table, and a little */
/* extra info about what's not in the table.  This may go onto a screen or */
/* into a file, depending on where this was called from. */

command_help(side, n)
Side *side;
int n;
{
    FuncTab *cmd;

    wprintf(side, "To move a unit, use the mouse or [hlyubn]");
    wprintf(side, "[HLYUBN] moves unit repeatedly in that direction");
    wprintf(side, "Mousing unit makes it sit, mousing enemy unit attacks");
    if (Build) wprintf(side, "Terrain characters set what will be painted.");
    wprintf(side, "  ");
    for (cmd = commands; cmd->fchar != '\0'; ++cmd) {
      if (n == 0) {
	if (cmd->fchar < ' ' || cmd->fchar > '~') 
/*** (UK) change -> ***/
	  if (cmd->fchar != '\200') {
	    switch (cmd->fchar) {
	      case ESCAPE:
		wprintf(side, "ESC %s", cmd->help); break;
	      case DELETE:
		wprintf(side, "DEL %s", cmd->help); break;
	      case BACKSPACE:
		wprintf(side, "BS  %s", cmd->help); break;
	      case '\t':
		wprintf(side, "TAB %s", cmd->help); break;
	      case '\r':
		wprintf(side, "RET %s", cmd->help); break;
	      case '\n':
		wprintf(side, "LF  %s", cmd->help); break;
	      default:
		wprintf(side, "^%c  %s", (cmd->fchar^0x40), cmd->help); break;
	    }
	  }
          else
	    wprintf(side, "    %s", cmd->help);
	else
	  wprintf(side, "%c   %s", cmd->fchar, cmd->help);
/*** was:
	  if (cmd->fchar != '\200')
	    wprintf(side, "^%c %s", (cmd->fchar^0x40), cmd->help);
          else
	    wprintf(side, "   %s", cmd->help);
	else
	  wprintf(side, "%c  %s", cmd->fchar, cmd->help);
*** <- change ***/
      }
      if (cmd->defaultarg == NEXTPAGE)
	n--;
      if (n < 0) {
	wprintf(side,
	  "            ---------- SEE NEXT PAGE FOR MORE COMMANDS ----------");
	return;
      }
    }
}

/*** (UK) insert -> ***/
y_terraform_type(side,unit,t)
Side *side;
Unit *unit;
int t;
{
      if (t != unit->terraform) {
        if (!can_terraform(unit,t)) {
            cmd_error(side,"You cannot terraform to %s here!",ttypes[t].name);
            return;
	}
        if (t==TNOTHING) {
            if (unit->terraform != TNOTHING) {
                notify(side, "%s is cancelling terraforming to %s.",
                       unit_handle(side, unit), ttypes[unit->terraform].name);
            }
        }
        else {
            notify(side, "%s is terraforming %s now.",
                   unit_handle(side, unit), ttypes[t].name);
        }
	set_terraform(unit, t);
	set_tschedule(unit);
      }
}

request_new_terrain(unit)
Unit *unit;
{
    int u;
    int t;
    Side *us = unit->side;
    short types[MAXTTYPES];

    if (humanside(us)) {
        for_all_terrain_types(t) {
            types[t]=utypes[unit->type].terraform[t];
        }
        int numtypes=0;
        types[terrain_at(unit->x, unit->y)]=0;
        for_all_terrain_types(t) {
            if (types[t] > 0) {
              numtypes++;
            }
        }
        if (numtypes == 0) {
            cmd_error("Useless to terraform to current terrain.");
        }
        else {
          sprintf(spbuf, "%s will terraform: ", unit_handle(us, unit));
          if (!(u=ask_terrain_type(us, unit, y_terraform_type, 
                        spbuf, types)))
            cmd_error("This unit cannot terraform anything!");
        }
    } else {
        /* TODO
	set_product(unit, machine_product(unit));
	set_schedule(unit);
        */
    }
}
/*** <- insert ***/

/* The handler for unit production needs to make sure valid input has */
/* been received, will put in another request if not. */

y_product_type(side,unit,u)
Side *side;
Unit *unit;
int u;
{
      if (u != unit->product) {
/*** (UK) insert -> ***/
        if (!can_build(unit,u)) {
	  cmd_error(side,"You cannot produce a %s here!",utypes[u].name);
	  return;
	}
/*** <- insert ***/
	set_product(unit, u);
	set_schedule(unit);
/*** (UK) insert -> ***/
	update_state(side,unit->product);
/*** <- insert ***/
      }
      else {
	unit->next_product=u;
      }
}
/* The handler for unit production needs to make sure valid input has */
/* been received, will put in another request if not. */

y_next_product_type(side,unit,u)
Side *side;
Unit *unit;
int u;
{
    unit->next_product=u;
}

/* This is called when production is to be set or changed.  Note that since */
/* the user has a pretty dull choice if there is only one possible type of */
/* unit to build, in such cases we can bypass requests altogether. */

request_new_product(unit)
Unit *unit;
{
    int u;
    Side *us = unit->side;

    if (humanside(us)) {
	sprintf(spbuf, "%s will build: ", unit_handle(us, unit));
	if (!(u=ask_unit_type(us, unit, y_product_type, 
		      spbuf, utypes[unit->type].make)))
	  cmd_error("This unit cannot produce anything!");
	if (u==2) make_current(us, unit);
    } else {
	set_product(unit, machine_product(unit));
	set_schedule(unit);
    }
}

/* This is called when production is to be set or changed.  This
   routine allows the user to specify what will be produced after the
   current production is finished. */

request_next_product(unit)
Unit *unit;
{
    int u;
    Side *us = unit->side;

    if (humanside(us)) {
	sprintf(spbuf, "%s's next product will be: ", unit_handle(us, unit));
	if (!(u=ask_unit_type(us, unit, y_next_product_type,
		 spbuf, utypes[unit->type].make)))
	  cmd_error("This unit cannot produce anything!");
	if (u==2) make_current(us, unit);
    } else {
       /* the machine should never do this.  Just ignore it.  */
    }
}

/* actually let wake up ... (gec)*/
y_wakespec(side,unit, u)
Side *side;
Unit *unit;
int u;
{
    actually_wakespec(side,(short)u);
}

/* Routine to find out which kind of unit the player wants to wake up */
/* with ^W ... May be NOTHING (gec)*/
request_wakespec(us)
Side *us;
{
    int u;

    if (humanside(us)) {
	sprintf(spbuf, "Wake up all: ");
	ask_unit_type(us, NULL, y_wakespec, spbuf, NULL);
    } else {
       /* the machine should never do this.  Just ignore it.  */

    }
}

/*** (UK) insert -> ***/

/* request handling for information requests */

static start_request(side,unit,handler,local)
Side *side;
Unit *unit;
RequestHandler handler;
RequestHandler local;
{
  if (!handler) return;
  show_prompt(side);
  side->reqnexthandler=handler;
  request_input(side,unit,local);
}

static continue_request(side)
Side *side;
{
  request_input(side,side->requnit,side->reqhandler);
}

#define finish_request(side,val) clear_prompt(side),(side)->reqnexthandler(side,(side)->requnit,val)
#define finish_request2(side,x,y) clear_prompt(side),(side)->reqnexthandler(side,(side)->requnit,x,y)
/*** <- insert ***/

/* Do something with the char or unit type that the player entered. */

static x_unit_type(side)
Side *side;
{
    int i, type = -2;

    switch (side->reqtype) {
      case KEYBOARD:
/*** (UK) remove -> ***
	  echo_at_prompt(side, side->reqch);
*** <- remove ***/
	  if (side->reqch == '?') {
	      help_unit_type(side);
	  } else if (side->reqch == ESCAPE) {
	      type = -1;
	  } else if (side->reqch == DELETE) {
	      type = NOTHING;
	  } else if (side->reqch == '\r') {
	      type = NOTHING;
	  } else if (side->reqch == ' ') {
	      if (side->requnit) {
		type = side->requnit->type;
		if (!side->bvec[type]) type=-2;
	      }
	  } else if (iindex(side->reqch, side->ustr) == -1) {
	      notify(side, "Unit type '%c' not in \"%s\"!",
		     side->reqch, side->ustr);
	  } else {
	      type = find_unit_char(side->reqch);
	  }
	  break;
      case UNITTYPE:
	  if (between(0, side->requtype, period.numutypes-1) &&
	      side->bvec[side->requtype]) {
	      type = side->requtype;
	  } else {
	      notify(side, "Not a valid unit type!");
	  }
	  break;
      default:
	  break;
    }
    if (type >= -1) {
	clear_prompt(side);
	for_all_unit_types(i) side->bvec[i] = 0;
	draw_unit_list(side, side->bvec);
	if (type >= 0) finish_request2(side,type,side->reqtvalue);
    }
    else continue_request(side);
}

/* Prompt for a type of a unit from player, maybe only allowing some types */
/* to be accepted.  Also allow specification of no unit type.  We do this */
/* by scanning the vector, building a string of chars and a vector of */
/* unit types, so as to be able to map back when done. */

ask_unit_type(side, unit, handler, prompt, possibles)
Side *side;
Unit *unit;
RequestHandler handler;
char *prompt;
short *possibles;
{
    int numtypes = 0, u, type;

    for_all_unit_types(u) {
	side->bvec[u] = 0;
	if (possibles == NULL || possibles[u]) {
	    side->bvec[u] = 1;
	    side->ustr[numtypes] = utypes[u].uchar;
	    side->uvec[numtypes] = u;
	    numtypes++;
	}
    }
    if (numtypes == 0) {
	return 0;
    } else if (numtypes == 1) {
	type = side->uvec[0];
	side->bvec[type] = 0;
	handler(side,unit,side->uvec[0],numtypes);
	return 1;
    } else {
	side->ustr[numtypes] = '\0';
	sprintf(side->promptbuf, "%s [%s] ", prompt, side->ustr);
	draw_unit_list(side, side->bvec);
	side->reqtvalue=numtypes;
	start_request(side,unit,handler,x_unit_type);
	return 2;
    }
}

/*** (UK) change -> ***/
static x_terrain_type(side)
Side *side;
{
    int i, type = -2;

    switch (side->reqtype) {
      case KEYBOARD:
	  if (side->reqch == '?') {
              printf("help terrain\n");
	      help_terrain_type(side);
	  } else if (side->reqch == ESCAPE) {
	      type = -1;
	  } else if (side->reqch == DELETE) {
	      type = TNOTHING;
	  } else if (side->reqch == '\r') {
	      type = TNOTHING;
	  } else if (side->reqch == ' ') {
	      if (side->requnit && !side->requnit->transport) {
		type = terrain_at(side->requnit->x, side->requnit->y);
		if (!side->tbvec[type]) type=-2;
	      }
	  } else if (iindex(side->reqch, side->tstr) != -1) {
             type = find_terr(side->reqch);
	  }
	  break;
      default:
	  break;
    }
    if (type >= -1) {
	clear_prompt(side);
	for_all_terrain_types(i) side->tbvec[i] = 0;
	if (type >= 0) finish_request2(side,type,side->reqtvalue);
    }
    else continue_request(side);
}
ask_terrain_type(side, unit, handler, prompt, possibles)
Side *side;
Unit *unit;
RequestHandler handler;
char *prompt;
short *possibles;
{
    int numtypes = 0, t, type;

    for_all_terrain_types(t) {
	side->tbvec[t] = 0;
	if (possibles == NULL || possibles[t]) {
	    side->tbvec[t] = 1;
	    side->tstr[numtypes] = ttypes[t].tchar;
	    side->tvec[numtypes] = t;
	    numtypes++;
	}
    }
    if (numtypes == 0) {
	return 0;
    } else if (numtypes == 1) {
	type = side->tvec[0];
	side->tbvec[type] = 0;
	handler(side,unit,side->tvec[0],numtypes);
	return 1;
    } else {
	side->tstr[numtypes] = '\0';
	sprintf(side->promptbuf, "%s [%s] ", prompt, side->tstr);
	side->reqtvalue=numtypes;
	start_request(side,unit,handler,x_terrain_type);
	return 2;
    }
}
/*** <- change ***/

/* Interpret the user's input in response to a position request.  All we */
/* have to comprehend is direction keys and mouse hits.  Space bars and */
/* unmoving mice both mean that a position has been decided on. */

static x_position(side)
Side *side;
{
    int dir;

    switch (side->reqtype) {
      case KEYBOARD:
	  if (side->reqch == ' ') {
	      clear_prompt(side);
	      finish_request2(side,side->reqposx,side->reqposy);
	      return;
	  } else if (side->reqch == ESCAPE) {
	      clear_prompt(side);
	      return; /* cancel */
	  } else if ((dir = iindex(side->reqch, dirchars)) >= 0) {
	      side->reqposx = wrap(side->reqposx + dirx[dir]);
	      side->reqposy = side->reqposy + diry[dir];
	  } else if ((dir = iindex(lowercase(side->reqch), dirchars)) >= 0) {
	      side->reqposx = wrap(side->reqposx + 10*dirx[dir]);
	      side->reqposy = side->reqposy + 10*diry[dir];
	  }
	  break;
      case MAPPOS:
	  if (side->reqx == side->reqposx && side->reqy == side->reqposy) {
	      clear_prompt(side);
	      finish_request2(side,side->reqposx,side->reqposy);
	      return;
	  } else {
	      side->reqposx = side->reqx;  side->reqposy = side->reqy;
	  }
	  break;
      default:
	  break;
    }
    side->curx = side->reqposx;  side->cury = side->reqposy;
    side->curunit = NULL;
    show_info(side);
    continue_request(side);
}

/* User is asked to pick a position on map.  This will iterate until the */
/* space bar designates the final position. */

ask_position(side, unit, handler, prompt)
Side *side;
Unit *unit;
RequestHandler handler;
char *prompt;
{
/*** (UK) change -> ***/
    sprintf(side->promptbuf, 
	    "%s [mouse or keys to move, space bar to set, ESC to cancel]",
	    prompt);
/*** was:
    sprintf(side->promptbuf, "%s [mouse or keys to move, space bar to set]",
	    prompt);
*** <- change ***/
    side->tmpcurx = side->curx;  side->tmpcury = side->cury;
    side->tmpcurunit = side->curunit;
    side->reqposx = side->curx;  side->reqposy = side->cury;
    start_request(side,unit,handler,x_position);
}

/* Restore the saved "cur" slots. */

restore_cur(side)
Side *side;
{
    side->curx = side->tmpcurx;  side->cury = side->tmpcury;
    side->curunit = side->tmpcurunit;
}

/* Figure out what the answer actually is, keeping the default in mind. */

static x_bool(side)
Side *side;
{
    bool dflt = side->reqvalue2;

    if (side->reqtype == KEYBOARD) {
	switch (side->reqch) {
	  case ESCAPE:
	      clear_prompt(side);
	      return;
	  case 'y':
	  case 'Y':
	      finish_request(side,TRUE);
	      return;
	  case 'n':
	  case 'N':
	      finish_request(side,FALSE);
	      return;
	  case '\n':
	  case '\r':
	      finish_request(side,dflt);
	      return;
	}
    }
    continue_request(side);
}

/* Prompt for a yes/no answer with a settable default. */

ask_bool(side, unit, handler, question, dflt)
Side *side;
Unit *unit;
RequestHandler handler;
char *question;
bool dflt;
{
    sprintf(side->promptbuf, "%s [%s]", question, (dflt ? "yn" : "ny"));
    side->reqvalue2 = dflt;
    start_request(side,unit,handler,x_bool);
}

/* The char has already been processed, so just pass it through. */

static x_char(side)
Side *side;
{   char *s=side->curprompt;

    if (side->reqtype == KEYBOARD) {
      if (side->reqch==ESCAPE) {
	clear_prompt(side);
	return;
      }
      if (s) while (*s) if (*s++==side->reqch) {
	finish_request(side, side->reqch);
	return;
      }
    }
    continue_request(side);
}

/* Prompt for a single character. */

ask_char(side, unit, handler, question, choices)
Side *side;
Unit *unit;
RequestHandler handler;
char *question, *choices;
{
    side->curprompt=choices;
    sprintf(side->promptbuf, "%s [%s]", question, choices);
    start_request(side,unit,handler,x_char);
}

/* Dig a character from the filled-in request and add it into the string. */
/* Keep returning NULL until we get something. */

static x_string(side)
Side *side;
{
  if (side->reqtype == KEYBOARD) {
    switch (side->reqch) {
      case ESCAPE:
	  clear_prompt(side);
      	  return; /* cancel */
      case '\r':
      case '\n':
	  if (side->reqcurstr == side->reqstrbeg && side->reqdeflt != NULL) {
	      strcpy(side->promptbuf+side->reqstrbeg, side->reqdeflt);
	  } else {
	      (side->promptbuf)[side->reqcurstr] = '\0';
	  }
	  finish_request(side,copy_string(side->promptbuf + side->reqstrbeg));
	  return;
      case DELETE:
	  finish_request(side,0);
	  return;
      case BACKSPACE:
	  if (side->reqcurstr > side->reqstrbeg) --side->reqcurstr;
	  break;
      default:
	  if (side->reqcurstr < BUFSIZE-2) {
	    if (side->reqch>=32 && side->reqch<127) {
		echo_at_prompt(side, side->reqch);
		(side->promptbuf)[side->reqcurstr++] = side->reqch;
	    }
	  }
          break;
    }
    write_str_cursor(side);
    (side->promptbuf)[side->reqcurstr+1] = '\0';
  }
  continue_request(side);
}

/* Read a string from the prompt window.  Deletion is allowed, and a */
/* cursor is displayed (this should definitely be a toolkit call...) */
/* Some restrictions on what strings can be read - for instance can't */
/* read or default to a NULL string. */

ask_string(side, unit, handler, prompt, dflt)
Side *side;
Unit *unit;
RequestHandler handler;
char *prompt, *dflt;
{
    if (dflt == NULL) {
	sprintf(side->promptbuf, "%s ", prompt);
    } else {
	sprintf(side->promptbuf, "%s (default \"%s\") ", prompt, dflt);
    }
    side->reqstrbeg = strlen(side->promptbuf);
    side->reqcurstr = side->reqstrbeg;
    write_str_cursor(side);
    side->reqdeflt = dflt;
    start_request(side,unit,handler,x_string);
    write_str_cursor(side);
}


/*** (UK) insert -> ***/
static show_number_prompt(side)
Side *side;
{
    if (side->reqvalue2==NEGFLAG) 
      sprintf(side->promptbuf, "%s: 0", side->curprompt);
    else 
      sprintf(side->promptbuf, "%s: %d", side->curprompt,side->reqvalue2);
    show_prompt(side);
}

static x_number(side)
Side *side;
{
  if (side->reqtype == KEYBOARD) {
    switch (side->reqch) {
      case ESCAPE:
	  clear_prompt(side);
	  return; /* cancel */
      case '\r':
      case '\n':
	finish_request(side,side->reqvalue2);
	return; 
      case DELETE:
      case BACKSPACE:
	if (side->reqvalue2==NEGFLAG) side->reqvalue2=0;
	else {
	  int f=side->reqvalue2<0;
	  side->reqvalue2/=10;
	  if (!side->reqvalue2) side->reqvalue2=f*NEGFLAG;
	}
	show_number_prompt(side);
	break;
      case '-':
	if (side->reqvalue2==NEGFLAG) side->reqvalue=0;
	else if (side->reqvalue2) side->reqvalue2=-side->reqvalue2;
	else side->reqvalue2=NEGFLAG;
	show_number_prompt(side);
	break;
      default:
	if (side->reqch>='0' && side->reqch<='9') {
	  if (side->reqvalue2==NEGFLAG) 
	    side->reqvalue2=-(side->reqch-'0');
	  else {
	    side->reqvalue2*=10;
	    if (side->reqvalue2>=0) side->reqvalue2+=side->reqch-'0';
	    else side->reqvalue2-=side->reqch-'0';
	  }
	  show_number_prompt(side);
	}
    }
  }
  continue_request(side);
}

ask_number(side,unit,handler,prompt)
Side *side;
Unit *unit;
RequestHandler handler;
char *prompt;
{
    side->reqvalue2=0;
    side->curprompt=prompt;
    show_number_prompt(side);
    start_request(side,unit,handler,x_number);
}

/* handle standing order type input */

static x_condition(side)
Side *side;
{ char c;

  if (side->reqtype==KEYBOARD) {
    switch (side->reqch) {
      case ESCAPE : clear_prompt(side); return;

      /* control modes */
      case '\r' : finish_request(side,-2); return;
      case 16   : finish_request(side,-3); return;
      case 7    : finish_request(side,-4); return;
      case 21   : finish_request(side,-5); return;
      case 15   : finish_request(side,-6); return;

      case 'a' : finish_request(side, C_NONE); return;
      case '!' : finish_request(side, -1); return;

      case 'g' : finish_request(side, C_GT); return;
      case 'G' : finish_request(side, C_LE); return;
      case 'l' : finish_request(side, C_LE); return;
      case 'L' : finish_request(side, C_GT); return;
      case 'e' : finish_request(side, C_PICKUP); return;
      case 'E' : finish_request(side, C_NPICKUP); return;
      case 't' : finish_request(side, C_EMBARK); return;
      case 'T' : finish_request(side, C_NEMBARK); return;
      case 'f' : finish_request(side, C_FULL); return;
      case 'F' : finish_request(side, C_NFULL); return;
      case 'c' : finish_request(side, C_TFULL); return;
      case 'C' : finish_request(side, C_NTFULL); return;

      case '?' : notify(side," a   no condition (always)");
		 notify(side," g   there are more than <n> units in transport");
		 notify(side," c   transport can carry more");
		 notify(side," f   unit is full with specified unit types");
		 notify(side," t   there is a transport to embark");
		 notify(side," e   there are embarkies for this unit");
		 notify(side," l   there are no more than <n> units in transport");
		 notify(side," !   negate condition");
		 notify(side," RET no more changes");

		 notify(side," ^P  change priority");
		 notify(side," ^G  change group");
		 notify(side," ^U  change unit type");
		 notify(side," ^O  change order type");
		 break;
    }
  }
  continue_request(side);
}

ask_condition(side,unit,handler,full)
Side *side;
Unit *unit;
RequestHandler handler;
int full;
{  
  if (side->sogroup) {
    sprintf(side->promptbuf,
    "%s standing %s <%s> order for %s of group %s: Change? [^P^G^U^O?agcftel!]",
	    (full?"Getting":"Removing"),
	    sorder_types[side->somode],
	    (full?cond_desig(side->socond):cond_spec(side->socond)),
	    utypes[side->soutype].name,
	    side->sogroup
    );
  }
  else {
    sprintf(side->promptbuf,
	    "%s standing %s <%s> order for %s: Change? [^P^G^U^O?agcftel!]",
	    (full?"Getting":"Removing"),
	    sorder_types[side->somode],
	    (full?cond_desig(side->socond):cond_spec(side->socond)),
	    utypes[side->soutype].name
    );
  }
  start_request(side,unit,handler,x_condition);
}

static x_somode(side)
Side *side;
{ char c;

  if (side->reqtype==KEYBOARD) {
    switch (side->reqch) {
      case ESCAPE : clear_prompt(side); return;
      case 'a' : if (!side->reqtvalue) break;
		 finish_request(side,0); return;
      case 'A' : if (!side->reqtvalue) break;
		 finish_request(side,-1); return;
      case 'P' : if (!side->sosimple) break;
		 side->sosimple=FALSE;
      case 'p' : finish_request(side,1); return;
      case 'E' : if (!side->sosimple) break;
		 side->sosimple=FALSE;
      case 'e' : finish_request(side,2); return;
      case 'F' : if (!side->sosimple) break;
		 side->sosimple=FALSE;
      case 'f' : finish_request(side,3); return;
      case '?' : notify(side," p  orders for units produced by current unit");
		 notify(side," e  orders for units entering current unit");
		 notify(side," f  orders for units inside current unit finishing an order");
		 if (side->sosimple)
		   notify(side,"use upper case to allow specification of conditions");
		 if (side->reqtvalue) 
		   notify(side," a  all orders for group");
		   notify(side," A  all order");
		 break;
    }
  }
  continue_request(side);
}
 

ask_somode(side,unit,handler)
Side *side;
Unit *unit;
RequestHandler handler;
{ 
  if (side->sosimple) 
    strcpy(side->promptbuf, "What type of standing order [?pefPEF]");
  else
    strcpy(side->promptbuf, "What type of standing order [?pef]");
  side->reqtvalue=0;
  start_request(side,unit,handler,x_somode);
}

ask_somode2(side,unit,handler)
Side *side;
Unit *unit;
RequestHandler handler;
{ 
  if (side->sosimple) 
    strcpy(side->promptbuf, "What type of standing order [aA?pefPEF]");
  else
    strcpy(side->promptbuf, "What type of standing order [aA?pef]");
  side->reqtvalue=1;
  start_request(side,unit,handler,x_somode);
}

/*** <- insert ***/

/* Return TRUE if someone is busy doing something. */

bool
someone_doing_something()
{
  Side *side;

    for_all_sides(side) {
/*** (UK) insert -> ***/
        if (humanside(side) && side->workmode) {
	  Side *s;
          for_all_sides(s) if (humanside(s) && !s->reqactive) 
			     request_command(s);
	  return TRUE;
	}
/*** <- insert ***/
	if (humanside(side) && side->reqactive &&
/*** (UK) change -> ***/
            (side->teach || side->reqhandler != x_command) &&
/*** was:
	    side->reqhandler != x_command &&
*** <- change ***/
	    side->reqhandler != x_help && !side->lost) 
	    return TRUE;
    }
    return FALSE;
}
