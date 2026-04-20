/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file is devoted to commands and other functions related specifically */
/* to help.  Almost all of the xconq help code is here. */

/* At least in X, the help window covers most of the xconq display, so */
/* total redraws are necessary.  This can be slow - fix would be to do */
/* per-window redrawing only as need, and place help window to cover as few */
/* of the other windows as possible (especially world map). */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"

extern char *ordinal();
extern int helpwinlines;
/*** (HW) remove ->
static int helpstate;
*** <- remove ***/

/* Display the news file on stdout if it exists, be silent if it doesn't. */
/* (Used only during program startup.) */

maybe_dump_news()
{
    FILE *fp;

    make_pathname(xconqlib, NEWSFILE, "", spbuf);
    if ((fp = fopen(spbuf, "r")) != NULL) {
	while (fgets(spbuf, BUFSIZE-1, fp) != NULL) {
	    fputs(spbuf, stdout);
	}
	fclose(fp);
    }
}

/* The general help command.  It first lists the available commands, */
/* then a legend to map display, then the current news about xconq, then */
/* details about the period/units.  Player can page screens back and forth, */
/* which explains some odd index adjusts at bottom of loop. */

x_help(side)
Side *side;
{
    char opt = side->reqch;

    if (side->reqtype!=KEYBOARD)
      return;

    init_wprintf(side, (char *) NULL);
    switch (opt) {
    case '?':
      side->helpstate = 0;
      side->curnote = 0;
      break;
    case '\014':
      break;
    case ' ':
    case '+':
    case 'n':
/*** (HW) change -> ***/
	if (!period.notes || side->helpstate != 6 ||
	    !period.notes[side->curnote+helpwinlines-2]) {
	  if (side->helpstate < (period.notes ? 6 : 5)+period.numutypes)
	    side->helpstate++;
	} else
	    side->curnote += helpwinlines-2;
/*** was:
	if (side->helpstate < 5+period.numutypes)
	  side->helpstate++;
*** <- change ***/
	break;
/*** (UK) change -> ***/
    case DELETE:
/*** was:
    case '\177':
*** <- change ***/ 
    case '\b':
    case '-':
    case 'p':
/*** (HW) change -> ***/
	if (side->helpstate > 0)
	  if (!period.notes || side->helpstate != 6)
	    side->helpstate--;
          else
	    if (side->helpstate == 6)
	      if (side->curnote == 0)
		side->helpstate--;
              else
		side->curnote -= helpwinlines-2;
/*** was:
	if (side->helpstate > 0) side->helpstate--;
*** <- change ***/
	break;
    default:
	conceal_help(side);
        side->reqtype = GARBAGE;
#if 0
	redraw(side);
#endif
	return;
    }
    switch (side->helpstate) {
    case 0:
	command_help(side, 0);
	break;
    case 1:
	command_help(side, 1);
	break;
    case 2:
	legend_help(side);
	break;
    case 3:
	make_pathname(xconqlib, NEWSFILE, "", spbuf);
	if (!show_file(side, spbuf)) {
	    wprintf(side, "No news is good news.");
	}
	break;
    case 4:
	describe_mapfiles(side);
	break;
    case 5:
	describe_period(side);
/*** (HW) insert -> ***/
	side->curnote = 0;
/*** <- insert ***/
	break;
/*** (HW) insert -> ***/
    case 6:
	if (period.notes) { describe_period_notes(side); break; }
/*** <- insert ***/
    default:
	/* i guaranteed to be in right range */
/*** (HW) change -> ***/
	describe_utype(side, side->helpstate - (period.notes ? 7 : 6));
/*** was:
	describe_utype(side, side->helpstate - 6);
*** <- change ***/
	break;
    }
    ask_help_char(side);
#if 0
    request_input(side, (Unit *) NULL, x_help);
    side->reqvalue2 = i;
#else
    side->reqtype = GARBAGE;
#endif
}

/* The command proper must request a character for whether to go to next */
/* or previous screen, or to quit entirely. */

do_help(side, n)
Side *side;
int n;
{
    if (reveal_help(side)) {
	init_wprintf(side, (char *) NULL);
#if 0
	command_help(side, 0);
	ask_help_char(side);
	request_input(side, (Unit *) NULL, x_help);
	side->reqvalue2 = 0;
#else
	side->helpstate = 0;
	side->reqtype = KEYBOARD;
	side->reqch = '\014';
	x_help(side);
#endif
    }
}

/* Prompt for a char from help window. */

ask_help_char(side)
Side *side;
{
    char *help = "[Space bar,'+','n' to continue, DEL,'-','p' for prev, all else quits]";

    draw_fg_text(side, side->help, 0, (helpwinlines-1)*side->hfh, help);
}

/* Generate a legend for all the various symbols and pictures. */
/* (Does not include cursor box or tiny side numbers.) */

legend_help(side)
Side *side;
{
    int t, u, tcol = 2 * side->margin + side->hw;
    int spacing = side->hh + 4 * side->margin;
    int offset = side->hh/4, second = 30*side->hfw;

    /* first column of things is constant for all periods */
    draw_blast_icon(side, side->help,
		    side->margin, 0*spacing, 'b', side->enemycolor);
    draw_fg_text(side, side->help, tcol, 0*spacing+offset, "miss");
    draw_blast_icon(side, side->help,
		    side->margin, 1*spacing, 'c', side->enemycolor);
    draw_fg_text(side, side->help, tcol, 1*spacing+offset, "hit");
    draw_blast_icon(side, side->help,
		    side->margin, 2*spacing, 'd', side->enemycolor);
    draw_fg_text(side, side->help, tcol, 2*spacing+offset, "kill");
    for_all_terrain_types(t) {
	draw_hex_icon(side, side->help,
		      side->margin, (t+3)*spacing,
		      (side->monochrome ? side->fgcolor : side->hexcolor[t]),
		      (side->monochrome ? ttypes[t].tchar : HEX));
        char buf[4] = { '(', ttypes[t].tchar, ')', '\0' };
	draw_fg_text(side, side->help,
		     tcol, (t+3)*spacing+offset, buf);
	draw_fg_text(side, side->help,
		     tcol+5*side->hfw, (t+3)*spacing+offset, ttypes[t].name);
    }
    for_all_unit_types(u) {
	draw_unit_icon(side, side->help,
		       second, u*spacing, u, side->fgcolor);
	draw_fg_text(side, side->help,
		     second+tcol, u*spacing+offset, utypes[u].name);
    }
}

/* This command provides a short note about the current hex.  It is */
/* useful as a supplement to the general help command. */

do_ident(side, n)
Side *side;
int n;
{
    viewdata view = side_view(side, side->curx, side->cury);
    char t = terrain_at(side->curx, side->cury);
    Side *side2;

    if (view == UNSEEN) {
	notify(side, "You see unexplored territory");
    } else if (view == EMPTY) {
	notify(side, "You see unoccupied %s", ttypes[t].name);
    } else {
	side2 = side_n(vside(view));
	notify(side, "You see a %s %s (in the %s)",
	       (side2 == NULL ? "neutral" : side2->name),
	       utypes[vtype(view)].name, ttypes[t].name);
    }
}
/* Find next dead unit */

do_next_dead(side, n)
Side *side;
int n;
{
  if (side->mode == SURVEY) {
    flush_dead_units();
    if (side->deadunits == NULL) {
      notify(side, "No units dies in the last turn.");
      return;
    }
    if (side->curdeadunit == NULL || side->curdeadunit->next == NULL)
      side->curdeadunit = side->deadunits;
    else side->curdeadunit = side->curdeadunit->next;
    if (side->curdeadunit != NULL)
	move_survey(side, side->curdeadunit->prevx,
		    side->curdeadunit->prevy);
  } else {
    cmd_error(side, "Must be in survey mode to find dead units.");
  }
}


/* Find a specific unit */

void find_unit_by_number(side, type, number)
Side *side;
int type, number;
{
  Unit *unit;
  
  for_all_side_units(side, unit)
    if (unit->side == side && alive(unit) &&
	unit->type == type && unit->number == number) {
      make_current(side, unit);
      return;
    }
  cmd_error(side, "The %s %s does not exist.",
	 ordinal(number), utypes[type].name);
}

/* Find next unit of the given type. */

void find_next_unit_by_type(side, type)
Side *side;
int type;
{
  Unit *unit;
  if (Debug) printf("Finding next %d, side has %d\n",
		    type, side->units[type]);
  if (side->units[type] == 0) {
    cmd_error(side, "You don't have any %ss.",
	   utypes[type].name);
  } else {
    unit = side->curunit;
    for (;;) {
      if (unit != NULL) unit = unit->next;
      if (unit == NULL)
	unit = side->unithead;
      if (unit->side == side && alive(unit) && unit->type == type) {
	make_current(side, unit);
	return;
      }
    }
  }
}
	     
/* Find the next one of the given unit type. */

y_find_unit(side,unit,u)
Side *side;
Unit *unit;
int u;
{
	if (u != NOTHING) {
	  if (side->helpstate < 0)
	    find_next_unit_by_type(side, u);
/*** (HW) change -> ***/
	  else find_unit_by_number(side, u,
				   (period.notes ? side->helpstate-1 :
				    side->helpstate));
/*** was:
	  else find_unit_by_number(side, u, side->helpstate);
*** <- change ***/
	}
}

/* This command is useful for finding units in bases and a specific unit */
/* when a message mentioning is on the screen. */

do_find_unit(side, n)
Side *side;
int n;
{
  if (side->mode != SURVEY) {
    cmd_error(side, "Find unit only works in survey mode.");
  } else {
    ask_unit_type(side, side->requnit, y_find_unit,"Find which type of unit?", (short *) NULL);
/*** (HW) change -> ***/
    side->helpstate = (period.notes ? n+1 : n);
/*** was:
    side->helpstate = n;
*** <- change ***/
  }
}

/* This command works well with the preceding one for iterating through */
/* your units of one type */

do_find_next_unit(side, n)
Side *side;
int n;
{
  if (side->mode != SURVEY) {
    notify(side, "Find unit only works in survey mode.");
    beep(side);
  }
  else if (side->curunit == NULL) {
    do_find_unit(side,n);
  } else if (n < 0) {
    find_next_unit_by_type(side, side->curunit->type);
  }
  else find_unit_by_number(side, side->curunit->type, n);
}


/* Dump out the characteristics of a single unit type.  This works by */
/* jumping into the help loop, so all the other types can be looked at also. */

y_unit_info(side,unit,u)
Side *side;
Unit *unit;
int u;
{
	if (u != NOTHING) {
	    if (reveal_help(side)) {
		init_wprintf(side, (char *) NULL);
		describe_utype(side, u);
		ask_help_char(side);
		request_input(side, (Unit *) NULL, x_help);
/*** (HW) change -> ***/
		side->helpstate = (period.notes ? u+1 : u);
/*** was:
		side->helpstate = u;
*** <- change ***/
	    }
	}
}

/* The command proper just prompts and issues the request. */

do_unit_info(side, n)
Side *side;
int n;
{
    ask_unit_type(side, side->requnit, y_unit_info,"Details on which unit type?", (short *) NULL);
}

/* Spit out all the general period parameters in a readable fashion. */

/*** (HW) insert -> ***/
describe_all_period_notes(side)
Side *side;
{
  int i;
  
  if (period.notes != NULL) {
    for (i=0; period.notes[i] != NULL; ++i) {
      wprintf(side, "%s", period.notes[i]);
    }
  }
}

describe_period_notes(side)
Side *side;
{
  int i;
  
  if (period.notes != NULL) {
    for (i=side->curnote; period.notes[i] != NULL &&
			    i-side->curnote < helpwinlines-2; ++i) {
      wprintf(side, "%s", period.notes[i]);
    }
    if (period.notes[side->curnote+helpwinlines-1] != NULL)
      wprintf(side,
	  "              ---------- SEE NEXT PAGE FOR MORE NOTES ----------");
  }
}
/*** <- insert ***/

describe_period(side)
Side *side;
{
    int u, r, t, i;

    wprintf(side, "This period is named \"%s\".", period.name);
    wprintf(side, "");
    wprintf(side,
	"It includes %d unit types, %d resource types, and %d terrain types.",
	    period.numutypes, period.numrtypes, period.numttypes);
    wprintf(side, "First unit type is %s, first product type is %s.",
	    (period.firstutype == NOTHING ? "none" :
	     utypes[period.firstutype].name),
	    (period.firstptype == NOTHING ? "none" :
	     utypes[period.firstptype].name));
    wprintf(side, "");
    wprintf(side,
	    "Countries are %d hexes across, between %d and %d hexes apart.",
	    period.countrysize, period.mindistance, period.maxdistance);
    wprintf(side, "Known area is %d hexes across.", period.knownradius);
    if (period.allseen)
	wprintf(side, "All units are always seen by all sides.");
    wprintf(side, "");
    if (period.counterattack)
	wprintf(side, "Defender always gets a counter-attack.");
    else
	wprintf(side, "Defender does not get a counter-attack.");
    wprintf(side, "Neutral units add %d%% to defense, hit over %d is a nuke.",
	    period.neutrality, period.nukehit);
    if (period.efficiency > 0)
	wprintf(side, "Unit recycling is %d%% efficient.", period.efficiency);
    wprintf(side, "");
    for_all_terrain_types(t) {
	wprintf(side, "Terrain:  %c  %s (%s)",
		ttypes[t].tchar, ttypes[t].name, ttypes[t].color);
    }
    wprintf(side, "");
    for_all_resource_types(r) {
	wprintf(side, "Resource: %c  %s (%s)",
		' ', rtypes[r].name, rtypes[r].help);
    }
    wprintf(side, "");
    for_all_unit_types(u) {
	wprintf(side, "Unit:     %c  %s (%s)",
		utypes[u].uchar, utypes[u].name, utypes[u].help);
    }
/*** (HW) remove -> ***
    wprintf(side, "");
    if (period.notes != NULL) {
	for (i = 0; period.notes[i] != NULL; ++i) {
	    wprintf(side, "%s", period.notes[i]);
	}
    }
*** <- remove ***/
}

/* Full details on the given type of unit.  This may be used either for */
/* online help or for building a descriptive file.  The icon will only */
/* show up for online help, otherwise the display calls have no effect. */

describe_utype(side, u)
Side *side;
int u;
{
    int r, t, u2;

    wprintf(side, "     '%c' %s      (territory value %d)",
	    utypes[u].uchar, utypes[u].name, utypes[u].territory);
    wprintf(side, "            %s", utypes[u].help);
    if (utypes[u].bitmapname != NULL)
	wprintf(side, "     bitmap \"%s\"", utypes[u].bitmapname);
    else 
	wprintf(side, "");
    /* This works even if we're printing to files (ugh). */
    draw_hex_icon(side, side->help, side->margin, side->margin,
		  side->fgcolor, HEX);
    draw_unit_icon(side, side->help, side->margin, side->margin,
		   u, side->bgcolor);
    wprintf(side, "Initially: %d in country, %d/10000 hexes density, %s.",
	    utypes[u].incountry, utypes[u].density,
	    (utypes[u].named ? "named" : "unnamed"));
    wprintf(side, "Maximum speed %d hexes/turn.",
	    utypes[u].speed);
    wprintf(side, "%d HP, crippled at %d HP, chance to retreat %d%%.",
	    utypes[u].hp, utypes[u].crippled, utypes[u].retreat);
    wprintf(side, "%d%% extra for start up, %d%% extra for R&D.",
	    utypes[u].startup, utypes[u].research);
    if (utypes[u].research > 0) {
      for_all_unit_types(u2)
	if (utypes[u].research_contrib[u2] > 0)
	  wprintf(side, "Research decreased by %d%% of research on %s.",
		  utypes[u].research_contrib[u2], utypes[u2].name);
    }
    wprintf(side, "%d extra moves used up by an attack.",
	    utypes[u].hittime);
    wprintf(side,
        "%d%% to succumb to siege, %d%% to revolt, attrition damage %d HP.",
	    utypes[u].siege, utypes[u].revolt, utypes[u].attdamage);
    if (utypes[u].seerange == 1) {
	wprintf(side, "Chance to see others %d%%.", utypes[u].seebest);
    } else {
/*** (UK) insert -> ***/
	if (utypes[u].seerange > 1) 
/*** <- insert ***/
	wprintf(side, "Chance to see %d%% at 1 hex, to %d%% at %d hexes.",
		utypes[u].seebest, utypes[u].seeworst, utypes[u].seerange);
    }
/*** (UK) insert -> ***/
    for_all_unit_types(u2) {
      if (can_type_carry(u2,u)) {
        printf("unit %s: %s -> %d\n", utypes[u].name, utypes[u2].name, utypes[u].transportseerange[u2]); 
        if (utypes[u].transportseerange[u2] >=0 ) {
          if (utypes[u].transportseerange[u2] != utypes[u].seerange) {
            if (utypes[u].transportseerange[u2] ==0 ) {
              wprintf(side, "See range disabled as occupant of %s", utypes[u2].name);
            }
            else {
              if (utypes[u].transportseerange[u2] > utypes[u].seerange ) {
                wprintf(side, "See range increased to %d hexes as occupant of %s", utypes[u].transportseerange[u2], utypes[u2].name);
              }
              else {
                wprintf(side, "See range decreased to %d hexes as occupant of %s", utypes[u].transportseerange[u2], utypes[u2].name);
              }
            }
          }
        }
      }
    }
/*** <- insert ***/
    wprintf(side, "Own visibility is %d.", utypes[u].visibility);
    if (utypes[u].volume > 0 || utypes[u].holdvolume > 0)
	wprintf(side, "Volume is %d, volume of hold is %d.",
		utypes[u].volume, utypes[u].holdvolume);
    wprintf(side, "%s %s",
	    (utypes[u].changeside ? "Can be made to change sides." : ""),
	    (utypes[u].disband ? "Can be disbanded and sent home." : ""));
    wprintf(side, "%s %s",
	    (utypes[u].maker ? "Builds units all the time." : ""),
	    (utypes[u].selfdestruct ? "Hits by self-destruction." : ""));
    wprintf(side, "");
    wprintf(side, "%s",
	    "  Resource   ToBui  Prod Store  Eats ToMov  Hits HitBy");
    wprintf(side, "%s",
	    "               (0)   (0)   (0)   (0)   (0)   (0)   (0)");
    for_all_resource_types(r) {
	sprintf(spbuf, "%10s: ", rtypes[r].name);
	append_number(spbuf, utypes[u].tomake[r], 0);
	append_number(spbuf, utypes[u].produce[r], 0);
	append_number(spbuf, utypes[u].storage[r], 0);
	append_number(spbuf, utypes[u].consume[r], 0);
	append_number(spbuf, utypes[u].tomove[r], 0);
	append_number(spbuf, utypes[u].hitswith[r], 0);
	append_number(spbuf, utypes[u].hitby[r], 0);
	wprintf(side, "%s", spbuf);
    }
    wprintf(side, "");
    wprintf(side, "%s",
	    "   Terrain  Slowed Rand% Hide% Defn% Prod% Attr% Acdn% Terra");
    wprintf(side, "%s",
	    "               (-)   (0)   (0)   (0)   (0)   (0)   (0)  -  ");
    for_all_terrain_types(t) {
	sprintf(spbuf, "%10s: ", ttypes[t].name);
	append_number(spbuf, utypes[u].moves[t], -1);
	append_number(spbuf, utypes[u].randommove[t], 0);
	append_number(spbuf, utypes[u].conceal[t], 0);
	append_number(spbuf, utypes[u].defense[t], 0);
	append_number(spbuf, utypes[u].productivity[t], 0);
	append_number(spbuf, utypes[u].attrition[t], 0);
	append_number(spbuf, utypes[u].accident[t], 0);
        append_number(spbuf, utypes[u].terraform[t], 0);
        wprintf(side, "%s", spbuf);
    }
    wprintf(side, "");
    wprintf(side, "%s%s",
	    "     Hit%  Damg  Cap% Guard  Pro%",
	    " Holds Enter Leave  Mob% Bridg Build   Fix");
    wprintf(side, "%s%s",
	    "      (0)   (0)   (0)   (0)   (0)",
	    "   (0)   (1)   (0) (100)   (0)   (0)   (0)");
    for_all_unit_types(u2) {
	sprintf(spbuf, "%c: ", utypes[u2].uchar);
	append_number(spbuf, utypes[u].hit[u2], 0);
	append_number(spbuf, utypes[u].damage[u2], 0);
	append_number(spbuf, utypes[u].capture[u2], 0);
	append_number(spbuf, utypes[u].guard[u2], 0);
	append_number(spbuf, utypes[u].protect[u2], 0);
	append_number(spbuf, utypes[u].capacity[u2], 0);
	append_number(spbuf, utypes[u].entertime[u2], 1);
	append_number(spbuf, utypes[u].leavetime[u2], 0);
	append_number(spbuf, utypes[u].mobility[u2], 100);
	append_number(spbuf, utypes[u].bridge[u2], 0);
	append_number(spbuf, utypes[u].make[u2], 0);
	append_number(spbuf, utypes[u].repair[u2], 0);
	wprintf(side, "%s", spbuf);
    }
    wprintf(side, "");
}

/* A simple table-printing utility. Hyphens out default values so they don't */
/* clutter the table. */

append_number(buf, value, dflt)
char *buf;
int value, dflt;
{
    if (value != dflt) {
	sprintf(tmpbuf, "%5d ", value);
	strcat(buf, tmpbuf);
    } else {
	strcat(buf, "  -   ");
    }
}

/* Dump assorted information into files, so they can be studied at leisure, */
/* or by people with screens too small for online help. */

do_printables(side, n)
Side *side;
int n;
{
    int u;

    init_wprintf(side, CMDFILE);
    command_help(side, 0);
    command_help(side, 1);
    finish_wprintf();
    notify(side, "Dumped command list to \"%s\".", CMDFILE);
    init_wprintf(side, PARMSFILE);
    describe_period(side);
/*** (HW) insert -> ***/
    describe_all_period_notes(side);
/*** <- insert ***/
    for_all_unit_types(u) {
	wprintf(side, "--------------------------------------------------");
	wprintf(side, "");
	describe_utype(side, u);
	wprintf(side, "");
    }
    finish_wprintf();
    notify(side, "Dumped period description to \"%s\".", PARMSFILE);
    /* Do a sort of screen dump (for curses only right now). */
    dump_view(side);
}

/* This kicks in on a unit type prompt.  Just give barest details, rely */
/* on general help for full unit descriptions.  This could be spiffier */
/* and use the help window, but getting the interaction right is just too */
/* hard... */

help_unit_type(side)
Side *side;
{
    int u;

    for_all_unit_types(u) {
	if (side->bvec[u]) {
	    notify(side, " %c  %s;  %s",
		   utypes[u].uchar, utypes[u].name, utypes[u].help);
	}
    }
}
