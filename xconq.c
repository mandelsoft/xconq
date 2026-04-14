/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Unlike most multi-player network programs, xconq does not use multiple */
/* processes.  Instead, it relies on the network capability inherent in  */
/* some graphical interfaces (such as X) to open up multiple displays. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"
#include "version.h"

extern int nummaps;     /* needed to initialize number of maps read */

/* Definitions of globally global variables. */

Global global;          /* important (i.e. saved/restored) globals */

bool givenseen = FALSE; /* true if world known */
bool Debug = FALSE;     /* the debugging flag itself. */
bool Build = FALSE;     /* magic flag for scenario-builders */
bool Freeze = FALSE;    /* keeps machine players from moving */
int Cheat = 0;          /* If true allows machine to cheat */
bool No_give_r_and_d = FALSE; /* true if r&d cannot be given away XXX */
bool No_give_units = FALSE; /* true if units cannot be given away XXX */
bool Sequential = FALSE; /* true if sides move one at a time XXX */
bool humans[MAXSIDES];  /* flags for which players are human or machine */
/*** (UK) insert -> ***/
bool PartialGame=FALSE;  /* only a subset of players is playing */
bool ServerFlag=FALSE;   /* If true allows display requests */
int server_port=SERVER_PORT; /* port number for server facility */
/*** <- insert ***/ 

/* mxconq: Only current display initialized if set. */
bool singlePlayerMode = FALSE;


char *programname;      /* name of the main program */
char *xconqlib;		/* directory of xconq data files */
char *rawfilenames[MAXLOADED];  /* names specified on cmd line */
char version[] = VERSION;  /* version string */
char *hosts[MAXSIDES];  /* names of displays for each side */
char spbuf[BUFSIZE];    /* main printing buffer */
char tmpbuf[BUFSIZE];   /* a temp buffer */
/*** (UK) insert -> ***/
char tmpbuf2[BUFSIZE];   /* a temp buffer */
/*** <- insert ***/ 

int numfiles = 0;       /* number of data files asked for */
int numgivens = 0;      /* number of sides given on cmd line */
int numhumans = 0;      /* number of bona-fide human players */
int givenwidth = DEFAULTWIDTH;    /* requested width for a random map */
int givenheight = DEFAULTHEIGHT;  /* requested height for a random map */
/* XXX */
char nextturnexecfile[BUFSIZE]; /* execute file at beginning of each turn */

/* Indices of groups of mapfile types mentioned in collection of mapfiles. */


Side *winner = NULL;    /* the winning side (allies are also winners) */

/* Where it all begins... the main program's primary duty is command line */
/* interpretation, it hands off for everything else. */

main(argc, argv)
int argc;
char *argv[];
{
    bool eopt = FALSE;
    char ch;
    int i, numenemies;

    enter_procedure("Main");
    programname = argv[0];
    if ((xconqlib = getenv("XCONQLIB")) == NULL)
	xconqlib = XCONQLIB;
    nummaps = 0;
    nextturnexecfile[0] = '\0'; /* XXX */
    for (i = 0; i < MAXLOADED; ++i) rawfilenames[i] = "";
    global.giventime = global.timeout = 0;
    add_default_player();
    for (i = 1; i < argc; ++i) {
	if ((argv[i])[0] == '-') {
	    ch = (argv[i])[1];
	    if (Debug) printf("-%c\n", ch);
	    switch (ch) {
	    case 'A':
		if (i+1 >= argc) usage_error();
		add_player(FALSE, argv[++i]);
		break;
	    case 'B':
		Build = TRUE;
		Freeze = TRUE;
		break;
	    case 'C':
		Cheat = TRUE;
		break;
	    case 'd':  /* subsumes "-display" */
		if (i+1 >= argc) usage_error();
		hosts[0] = copy_string(argv[++i]);
		break;
	    case 'D':
		Debug++;
		break;
	    case 'e':
		if (i+1 >= argc) usage_error();
		eopt = TRUE;
		numenemies = atoi(argv[++i]);
		while (numenemies-- > 0) add_player(FALSE, (char *) NULL);
		break;
            case 'f':			/* XXX */
                if (i+1 >= argc) usage_error();
                strcpy(nextturnexecfile, argv[++i]);
                break;
	    case 'g':
		/* do -geometry for this player */
		break;
            case 'G':			/* XXX */
                No_give_units = TRUE;
                break;
	    case 'm':
		if (i+1 >= argc) usage_error();
		make_pathname((char *) NULL, argv[++i], "map", spbuf);
		rawfilenames[numfiles++] = copy_string(spbuf);
		break;
	    case 'M':	
		if (i+2 >= argc) usage_error();
		givenwidth = atoi(argv[++i]);
		givenheight = atoi(argv[++i]);
		break;
	    case 'p':
		if (i+1 >= argc) usage_error();
		make_pathname((char *) NULL, argv[++i], "per", spbuf);
		rawfilenames[numfiles++] = copy_string(spbuf);
		break;
	    case 'r':
		if (numgivens > 1) {
		    fprintf(stderr, "Warning: -r is resetting the list of\n");
		    fprintf(stderr, "players already given in the command.\n");
		}
		numgivens = numhumans = 0;
		break;
            case 'R':			/* XXX */
                No_give_r_and_d = TRUE;
                break;
	    case 's':
		if (i+1 >= argc) usage_error();
		make_pathname((char *) NULL, argv[++i], "scn", spbuf);
		rawfilenames[numfiles++] = copy_string(spbuf);
		break;
            case 'S':			/* XXX */
                Sequential = TRUE;
                break;
	    case 't':
		if (i+1 >= argc) usage_error();
		/* Converting to seconds for internal use */
		global.giventime = 60 * atoi(argv[++i]);
		if (!global.giventime) usage_error();
		break;
	    case 'T':
		if (i+1 >= argc) usage_error();
		/* Converting to seconds for internal use */
		global.timeout = 60 * atoi(argv[++i]);
		/* To keep clock from looking funny on startup: */
		global.starttime = time(0);
		break;
	    case 'v':
		givenseen = TRUE;
		break;
            case 'P':
                singlePlayerMode = TRUE;
                numgivens = numhumans = 0; /* same as -r */
                break;
/*** (UK) insert -> ***/
            case 'Q':
                PartialGame = TRUE;
		eopt=TRUE;
                numgivens = numhumans = 0; /* same as -r */
                break;
            case 'x':
                ServerFlag = TRUE;
                PartialGame = TRUE;
		eopt=TRUE;
                numgivens = numhumans = 0; /* same as -r */
                break;
            case 'X':
		if (i+1 >= argc) usage_error();
		server_port=atoi(argv[++i]);
                ServerFlag = TRUE;
                PartialGame = TRUE;
		eopt=TRUE;
                numgivens = numhumans = 0; /* same as -r */
                break;
/*** <- insert ***/ 
	    default:
		usage_error();
	    }
	} else {
	    /* We just have a host name for a human player */
	    add_player(TRUE, argv[i]);
	}
    }
    /* Put in a single machine opponent if no -e and no human opponents */
    if (!eopt && numgivens == 1) add_player(FALSE, (char *) NULL);
    /* Say hi to the user */
    printf("\n              Welcome to XCONQ version %s\n\n", version);
    maybe_dump_news();
   
    /* While away the hours setting everything up */
    init_random(-1);
    init_sighandlers();
    init_sides();
    init_units();
    init_game();
    init_displays1();
    init_mplayers();
    init_displays2();
    /* Relatively low chance of screwup now, so safe to delete saved game */
    /* if (saved_game()) remove_saved_game(); */
    /* Play! */
    play();
    return 0;
}

/* Complain and leave if command is garbage. */

usage_error()
{
    fprintf(stderr, "Usage: %s [display] [-A display] [-B] [-C] [-e n] [-m name]\n",
	    programname);
    fprintf(stderr, "  [-M w h] [-p name] [-r] [-s name] [-t n] [-T n] [-v]\n");
    fprintf(stderr, "  [-G] [-R] [-f filename]\n");
/*** (UK) insert -> ***/
    fprintf(stderr, "  [-P] [-Q] [-x] [-X port]\n");
/*** <- insert ***/
    exit(1);
}

/* Add a player into the array of players, making sure of no overflow. */
/* Fail if so, players probably made a typo on command line - help them */
/* out with a list of what players had been included. */

add_player(ahuman, host)
bool ahuman;
char *host;
{
    if (numgivens >= MAXSIDES) {
	fprintf(stderr, "At most %d player sides allowed!\n", MAXSIDES);
	fprintf(stderr, "(Valid ones were ");
	list_players(stderr);
	fprintf(stderr, ")\n");
	exit(1);
    }
    hosts[numgivens] = host;
    humans[numgivens] = ahuman;
    numgivens++;
    if (ahuman) numhumans++;
    if (Debug) printf("Added %s player (%s)\n",
		      (ahuman ? "human" : "machine"), (host ? host : ""));
}

/* The main loop that cycles through the turns and the phases. */
/* Always departs through game end phase (or by core dump :-( ). */

play()
{
/*** (HW) reinsert -> ***/
  int u;
  Side *side;
/*** <- reinsert ***/

    while (TRUE) {
	if (!midturnrestore) {
	  init_turn_phase();
	  spy_phase();
	  disaster_phase();
	  build_phase();
	  flush_dead_units();
	  supply_phase();
	  sort_units(FALSE);
	  auto_save_phase(); /* XXX */
	}
/*** (HW) reinsert -> ***/
	for_all_sides(side) for_all_unit_types(u) update_state(side, u);
        test_input();
/*** <- reinsert ***/
	movement_phase();
	consumption_phase();
	flush_dead_units();
	game_end_phase();
	if (numhumans == 0)
	  exit_robot_check();
    }
}

/* Do random small things to get the turn started, including resetting */
/* some side vars that could get things confused, if set wrong. */
/* Most movement-related things should get set later, at beginning of */
/* move phase. */

init_turn_phase()
{
    Side *side;

    ++global.time;
    if (Debug) printf("##### TURN %d #####\n", global.time);
    for_all_sides(side) {
	show_timemode(side,TRUE);
	update_clock(side);
	flush_input(side);
	flush_output(side);
    }
}

/* XXX perform an auto_save if the turn number is correct */

auto_save_phase()
{
        char filename[80];

        if (global.time % 5 != 0)
                return;
        sprintf(filename, "%s.auto.%d", SAVEFILE, global.time);
        if (!write_savefile(filename))
            notify_all("Can't open file \"%s\"!", filename);
}

/* The win/lose phase assesses the sides' performance during that turn, */
/* and can end the game.  In fact, it is the only way to get out, except */
/* for the quick exit command.  With multiple mixed players, there are a */
/* number of possibilities at game end, such as an all machine win.  Also, */
/* it is possible for all players to lose simultaneously! */

game_end_phase()
{
    bool enemiesleft = FALSE;
    int sidesleft = 0, testrslt;
    Side *side, *side2, *survivor;

    if (Debug) printf("Entering game end phase\n");
    /* See if any sides have won or lost */
    if (global.time >= global.endtime) {
	notify_all("The end of the world has arrived!");
	for_all_sides(side) side_loses(side, (Side *) NULL);
    } else if (Build) {
	/* No winning or losing while building */
    } else {
	for_all_sides(side) {
	    if (!side->lost) {
		if (side->timedout) {
		    notify_all("The %s ran out of time!",
			       plural_form(side->name));
		    side_loses(side, (Side *) NULL);
		} else if (all_units_gone(side)) {
		    notify_all("The %s have been wiped out!",
			       plural_form(side->name));
		    side_loses(side, (Side *) NULL);
		} else {
		    testrslt = condition_true(side);
		    if (testrslt > 0) side_wins(side);
		    if (testrslt < 0) side_loses(side, (Side *) NULL);
		}
	    }
	}
    }
    /* See if any opposing sides left */
    for_all_sides(side) {
	for_all_sides(side2) {
	    if (!side->lost && !side2->lost && enemy_side(side, side2))
		enemiesleft = TRUE;
	}
	if (!side->lost) {
	    survivor = side;
	    sidesleft++;
	}
    }
    /* Decide if the game is over */
    if (numsides == 1) {
	/* A one-player game can't end */
    } else if (sidesleft == 0) {
        exit_xconq();
    } else if (!enemiesleft) {
	winner = survivor;
	if (sidesleft == 1) {
	    notify(winner, "You have prevailed over your enemies!");
	} else {
	    for_all_sides(side) {
		if (!side->lost) {
		    notify(side, "Your alliance has defeated its enemies!");
		}
	    }
	}
	wait_to_exit(winner);
	exit_xconq();
    } else {
        /* Game isn't over yet */
    }
}

/* Quicky test for absence of all units. */

all_units_gone(side)
Side *side;
{
    int u;

    for_all_unit_types(u) if (side->units[u] > 0) return FALSE;
    return TRUE;
}

has_type(unit, t)
Unit *unit;
int   t;
{
    Unit *occ;

    if (t == -1 || unit->type == t) return TRUE;

    for_all_occupants(unit, occ)
	if (alive(occ) && occ->type == t)
	    return TRUE;

    return FALSE;
}

/* Check out all the winning and losing conditions, returning a flag */
/* on which, if any, are true.  The three return values are 1 for win, */
/* -1 for lose, and 0 for undecided. */

condition_true(side)
Side *side;
{
    int i, u, r, amt = 0, is_here = 0;
    Unit *unit, *occ;
    Condition *cond;

    for (i = 0; i < global.numconds; ++i) {
	cond = &(global.winlose[i]);
	if ((between(cond->starttime, global.time, cond->endtime)) &&
	    (cond->sidesn == -1 || cond->sidesn == side_number(side))) {
	    switch (cond->type) {
	    case TERRITORY:
		for_all_unit_types(u) {
		    amt += side->units[u] * utypes[u].territory;
		}
		if (cond->win) {
		    if (amt >= cond->n) {
			notify_all("The %s have enough territory to win!",
				   plural_form(side->name));
			return 1;
		    }
		} else {
		    if (amt < cond->n) {
			notify_all("The %s don't have enough territory!",
				   plural_form(side->name));
			return -1;
		    }
		}
		break;
	    case UNITCOUNT:
		if (cond->win) {
		    for_all_unit_types(u) {
			if (side->units[u] < cond->units[u]) return 0;
		    }
		    notify_all("The %s have enough units to win!",
			       plural_form(side->name));
		    return 1;
		} else {
		    for_all_unit_types(u) {
			if (side->units[u] > cond->units[u]) return 0;
		    }
		    notify_all("The %s don't have the required units!",
			       plural_form(side->name));
		    return -1;
		}
		break;
	    case RESOURCECOUNT:
		if (cond->win) {
		    for_all_resource_types(r) {
			if (side->resources[r] < cond->resources[r]) return 0;
		    }
		    notify_all("The %s have enough resources to win!",
			       plural_form(side->name));
		    return 1;
		} else {
		    for_all_resource_types(r) {
			if (side->resources[r] > cond->resources[r]) return 0;
		    }
		    notify_all("The %s don't have the required resources!",
			       plural_form(side->name));
		    return -1;
		}
		break;
	    case POSSESSION:
		unit = unit_at(cond->x, cond->y);
		if (cond->win) {
		    if (unit != NULL && unit->side == side &&
			has_type(unit, cond->n)) {
			notify_all("The %s have a %s in hex %d,%d!",
				   plural_form(side->name),
				   utypes[cond->n].name,
				   cond->x, cond->y);
			return 1;
		    }
		} else {
		    if (unit == NULL || unit->side != side ||
			!has_type(unit, cond->n)) {
			notify_all("The %s don't have a %s in hex %d,%d!",
				   plural_form(side->name),
				   utypes[cond->n].name,
				   cond->x, cond->y);
			return -1;
		    }
		}
		break;
	    default:
		case_panic("condition type", cond->type);
		break;
	    }
	}
    }
    return 0;
}

/* Winning is defined as not losing :-) */

side_wins(side)
Side *side;
{
    Side *side2;

    for_all_sides(side2) {
	if (!allied_side(side, side2)) side_loses(side2, (Side *) NULL);
    }
}

/* Be rude to losers and give all their units to their defeaters (might */
/* not be one, so units become neutral or dead).  Give the loser a chance */
/* to study the screen, mark the side as lost, then pass on this detail to */
/* all the other sides and remove spurious leftover images of units. */

side_loses(side, victor)
Side *side, *victor;
{
    int ux, uy;
    Unit *unit;
    Side *side2;

    notify(side, "You lost, sucker");
    if (active_display(side)) {
	wait_to_exit(side);
	close_display(side);
    }
    side->humanp = FALSE; /* just make sure */
    while (side->unithead->next != side->unithead) {
      unit = side->unithead->next;
      if (alive(unit)) {
	ux = unit->x;  uy = unit->y;
	unit_changes_side(unit, victor, CAPTURE, SURRENDER);
	all_see_hex(ux, uy);
      } else {
	/* units could die instead of changing sides.   Inefficient if */
	/* it happens often, but sides don't resign often.  We need to */
	/* flush the dead units to make sure that the side has no */
	/* units.  Easier this way, since unit_changes_side could get */
	/* a lot of occupants, so we can't just step through the list. */
	flush_dead_units();
      }
    }
    side->lost = TRUE;
    if (nextturnexecfile[0] != '\0') { /* XXX */
        char buf[BUFSIZE];

        sprintf(buf, "%s side %d %d %d", nextturnexecfile, side_number(side),
                (side->lost ? 0 : 1), (side->more_units ? 1 : 0));
        system(buf);
    }
    for_all_sides(side2) {
	if (active_display(side2)) {
	    remove_images(side2, side_number(side));
	    if (side != side2) show_all_sides(side2,TRUE);
	}
    }
}

/* Leave screen up so everybody can study it, and allow any input to take */
/* it down.  OK to freeze all the other sides at once here. */

wait_to_exit(side)
Side *side;
{
    if (humanside(side) && active_display(side)) {
	sprintf(side->promptbuf, "[Do anything to exit]");
	show_prompt(side);
	freeze_wait(side);
    }
}

/* This routine should be called before any sort of non-death exit. */
/* Among other things, it updates the statistics. */

exit_xconq()
{
    int n = 0;
    Unit *unit;
    Side *side;

    close_displays();
    /* Need to kill off all units to finish up statistics */
    for_all_units(side, unit)
      kill_unit(unit, (winner ? VICTOR : SURRENDER));
    print_statistics();
    /* Offer a one-line comment on the game and then leave */
    for_all_sides(side) if (!side->lost) n++;
    switch (n) {
    case 0:
	printf("\nEverybody lost!\n\n");
	break;
    case 1:
	printf("\nThe %s (%s) won!\n\n",
	       plural_form(winner->name),
	       (winner->host ? winner->host : "machine"));
	break;
    default:
	sprintf(spbuf, "");
	for_all_sides(side) {
	    if (!side->lost) {
		if (strlen(spbuf) > 0) {
		    sprintf(tmpbuf, "/");
		    strcat(spbuf, tmpbuf);
		}
		sprintf(tmpbuf, "%s", side->name);
		strcat(spbuf, tmpbuf);
		if (side->host) {
		    sprintf(tmpbuf, "(%s)", side->host);
		    strcat(spbuf, tmpbuf);
		}
	    }
	}
	printf("\nThe %s alliance won!\n\n", spbuf);
	break;
    }
    exit(0);
}

/* Shut down displays - should be done before any sort of exit. */

close_displays()
{
    Side *side;

/*** (HW) change -> ***/
    for_all_sides(side) {
      /* avoid drawing without display at the end of the game */
      side->show_orders = FALSE;
      side->showsorderutype = NOTHING;
      if (active_display(side)) close_display(side);
    }
/*** was:
    for_all_sides(side) if (active_display(side)) close_display(side);
*** <- change ***/
}

/* This is to find out how everybody did. */

print_statistics()
{
    Side *side;
    FILE *fp;

    enter_procedure("print stats");
#ifdef STATISTICS
    if ((fp = fopen(STATSFILE, "w")) != NULL) {
	for_all_sides(side) {
	    print_side_results(fp, side);
	    print_unit_record(fp, side);
	    print_combat_results(fp, side);
	    if (side->next != NULL) fprintf(fp, "\f\n");
	}
	fclose(fp);
    } else {
	fprintf(stderr, "Can't open statistics file \"%s\"\n", STATSFILE);
    }
#endif /* STATISTICS */
    exit_procedure();
}

/* List all the specified players briefly. */

list_players(fp)
FILE *fp;
{
    int i;

    if (numgivens > 0) {
	fprintf(fp, "%s", (hosts[0] ? hosts[0] : "machine"));
	for (i = 1; i < numgivens; ++i) {
	    fprintf(fp, ", %s", (hosts[i] ? hosts[i] : "machine"));
	}
    } else {
	fprintf(fp, "no players defined.");
    }
}
