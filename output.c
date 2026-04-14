/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file concentrates on the more textual parts of the interface, mainly */
/* the windows dedicated to messages and the like. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

char *modenames[] = MODENAMES;  /* names of the modes */
char tmpnbuf[BUFSIZE];          /* tmp buf for notices only. */

/* Variables for the "window printf" utility. */

bool wprintmode;		/* true when printing is going into a file */

char wbuf[BUFSIZE];             /* accumulated line of text */

int wline;                      /* current position of output */

FILE *wfp;                      /* file pointer for wprintf */

/* mxconq: defined in xconq.c */
extern bool singlePlayerMode;


/* Send a message to everybody who's got a screen. */

/*VARARGS*/
notify_all(control, a1, a2, a3, a4, a5, a6)
char *control, *a1, *a2, *a3, *a4, *a5, *a6;
{
    Side *side;

    for_all_sides(side) notify(side, control, a1, a2, a3, a4, a5, a6);
}

/* Main message-sending routine - does its own formatting and spits out to */
/* the given side.  A large chunk of memory is allocated for notices.  Scrolling is by copying pointers to strings. */

/*VARARGS*/
notify(side, control, a1, a2, a3, a4, a5, a6, a7, a8)
Side *side;
char *control, *a1, *a2, *a3, *a4, *a5, *a6, *a7, *a8;
{
    int i;
    static char *note_space = NULL;
    static char *next_string = NULL;

    if (NULL == note_space) {
      note_space = (char *) malloc(NOTIFY_SPACE * sizeof(char));
      next_string = note_space;
    }
    if (active_display(side)) {
	for (i = MAXNOTES-1; i > 0; --i) {
	  side->noticebuf[i] =  side->noticebuf[i-1];
	}
	sprintf(tmpnbuf, control, a1, a2, a3, a4, a5, a6, a7, a8);
	if (islower(tmpnbuf[0])) tmpnbuf[0] = toupper(tmpnbuf[0]);

	/* Once we are out of space, just reuse it.  It is possible */
	/* that we will clobber old messages, but enough should be */
	/* around that that is not a real problem. */
	   
	if (next_string + strlen(tmpnbuf) + 10 > note_space + NOTIFY_SPACE)
	  next_string = note_space;
	side->noticebuf[0] = next_string;
	sprintf(side->noticebuf[0], "%d: %s", global.time, tmpnbuf);
	show_note(side,TRUE);
	flush_output(side);
	while (*next_string++ != '\0');
	side->bottom_note = 0;
	if (Debug) printf("%s\n", side->noticebuf[0]);
    }
    /* mxconq: save the messages to a .not file if in singlePlayerMode */
    if (singlePlayerMode && side && side->host && strlen(side->host) > 1) {
	FILE *fp;
        strcpy(tmpnbuf, side->name);
        strcat(tmpnbuf, ".not");
        if (fp = fopen(tmpnbuf, "a")) {
            sprintf(tmpnbuf, control, a1, a2, a3, a4, a5, a6);
            if (islower(tmpnbuf[0])) tmpnbuf[0] = toupper(tmpnbuf[0]);
            fprintf(fp, "%d: %s\n", global.time, tmpnbuf);
            fclose(fp);
	}
    }
}

/* Notice area refresher.  All notes except the most recent one are grayed */
/* out (no effect for monochrome). */

show_note(side,clear)
Side *side;
int clear;
{
    int i, sy = 0;

    if (active_display(side)) {
        if (clear)
	  clear_window(side, side->msg);
	for (i = side->nh+side->bottom_note-1; i >= side->bottom_note; --i) {
	    draw_text(side, side->msg, side->margin, sy, side->noticebuf[i],
		      ((i == 0) ? side->fgcolor : side->graycolor));
	    sy += side->fh;
	}
    }
}

/* Info about a unit is complicated by the overriding requirement that it */
/* be quickly graspable.  Graphics is helpful, so is fixed format (trains */
/* eyes to get a value of interest from same place each time). */

/* Verbal description of what you can see in your view.  Can't always show */
/* names because the view doesn't store them, and the unit at that spot may */
/* not exist anymore.  Units that are "always seen" (like cities) can be */
/* described in more detail, however. */

extern int *localworth;                /* for evaluation of nearby hexes */
#define aref(m,x,y) ((m)[(x)+world.width*(y)])
extern Unit *munit;
int printing = FALSE;
extern evaluate_hex();

show_info(side)
Side *side;
{
    bool more = (Debug || Build);
    int u, terr, age;
    char *filler = "";
    viewdata view, prevview;
    Unit *unit = side->curunit, *realunit;
    Side *side2;

    if (active_display(side)) {
      if (side->curx >= 0 && side->cury >= 0) {
/*** (UK) insert -> ***/
	if (unit && !alive(unit)) {
	  unit=side->curunit=unit_at(side->curx,side->cury);
	}
/*** <- insert ***/
	view = side_view(side, side->curx, side->cury);
	terr = terrain_at(side->curx, side->cury);
	realunit = unit_at(side->curx, side->cury);
	put_on_screen(side, side->curx, side->cury);
	if (unit != NULL) {
	  sprintf(tmpbuf, "%s", unit_handle(side, unit));
	  draw_info_text(side, 0, 0, 30, tmpbuf);
	  more = TRUE;
	} else {
	  /* Describe any unit image that might be present. */
	  if (view == UNSEEN || view == EMPTY) {
	      sprintf(tmpbuf, "");
	  } else {
	      side2 = side_n(vside(view));
	      u = vtype(view);
	      if ((utypes[u].seealways ||
		   cover(side, side->curx, side->cury) > 0
		   || more) && realunit != NULL) {
		  sprintf(tmpbuf, "%s", unit_handle(side, realunit));
	      } else {
		  sprintf(tmpbuf, "%s %s",
			  (side2 == NULL ? "neutral" : side2->name),
			  utypes[u].name);
	      }
	      filler = "In ";
	  }
	  draw_info_text(side, 0, 0, 60, tmpbuf);
	  /* Now describe terrain and position. */
	  sprintf(tmpbuf, "%s%s at %d,%d",
		  filler, (view == UNSEEN ? "(unknown)" : ttypes[terr].name),
		  side->curx, side->cury);
	  draw_info_text(side, 0, 1, 60, tmpbuf);
        }
	/* put here so overwrites any spillover from name writing */
 	if (more) {
	  if (unit == NULL) unit = realunit;
	  if (unit != NULL) show_intimate_details(side, unit);
	} else {
#ifdef PREVVIEW
	  /* Compose and display view history of this hex. */
	  if (Debug) printf("Drawing previous view info\n");
	  age = side_view_age(side, side->curx, side->cury);
	  prevview = side_prevview(side, side->curx, side->cury);
	  if (age == 0) {
	      if (prevview != view) {
		  if (prevview == EMPTY) {
		      /* misleading if prevview was set during init. */
		      sprintf(tmpbuf, "Up to date; had been empty.");
		  } else if (prevview == UNSEEN) {
		      sprintf(tmpbuf, "Up to date; had been unexplored.");
		  } else {
		      side2 = side_n(vside(prevview));
		      u = vtype(prevview);
		      if (side2 != side) {
			  sprintf(tmpbuf, "Up to date; had seen %s %s.",
				  (side2 == NULL ? "neutral" : side2->name),
				  utypes[u].name);
		      } else {
			  sprintf(tmpbuf,
				  "Up to date; had been occupied by your %s.",
				  utypes[u].name);
		      }
		  }
	      } else {
		  sprintf(tmpbuf, "Up to date.");
	      }
	  } else {
	      if (prevview == EMPTY) {
		  sprintf(tmpbuf, "Was empty %d turns ago.", age);
	      } else if (prevview == UNSEEN) {
/*** (HW) insert -> ***/
		  if (prevview == view)
		    sprintf(tmpbuf, "Terrain unexplored up to now.");
		  else
/*** <- insert ***/
		  sprintf(tmpbuf, "Terrain first seen %d turns ago.", age);
	      } else {
		  side2 = side_n(vside(prevview));
		  u = vtype(prevview);
		  if (side2 != side) {
		      sprintf(tmpbuf, "Saw %s %s, %d turns ago.",
			      (side2 == NULL ? "neutral" : side2->name),
			      utypes[u].name, age);
		  } else {
		      sprintf(tmpbuf, "Was occupied by your %s %d turns ago.",
			      utypes[u].name, age);
		  }
	      }
	  }
	  draw_info_text(side, 0, 2, 60, tmpbuf);
#endif
	  if (side->curdeadunit != NULL &&
	      side->curdeadunit->prevx == side->curx &&
	      side->curdeadunit->prevy == side->cury) {
	    sprintf(tmpbuf, "%s died here",
		    unit_handle(side, side->curdeadunit));
	    draw_info_text(side, 0, 4, 80, tmpbuf);
	  }
	}
      }
    }
}


/*** (UK) insert -> ***/
char *handle_occ(own,u,buf,e)
Unit *u;
char *buf;
char **e;
{

  static char braces[]= {
    '(',')','[',']','{','}','<','>'
  };

  char *tab[200];
  int  cnt[100];
  int n=0,i;
  Unit *o;
  char *p,*ep;

  p=buf;
  if (own) *p++=utypes[u->type].uchar;
  *p++='\0';

  for_all_occupants(u,o) {
    char *s=handle_occ(own+1,o,p+5,&ep);
    for (i=0; i<n; i++) {
      if (!strcmp(tab[i],s)) {
	cnt[i]++;
	break;
      }
    }
    if (i==n) {
      cnt[n]=0;
      tab[n++]=s;
      p=ep;
    }
  }
  if (n) {
    if (own) {
      p=buf+1;
      *p++=braces[(own%4)<<1];
    }
    else p=buf;
    for (i=0;i<n;i++) {
      char *s=tab[i];
      if (i) *p++=',';
      if (cnt[i]) {
	sprintf(p,"%d",cnt[i]+1);
	p+=strlen(p);
      }
      while (*s) *p++= *s++;
    }
    if (own) *p++=braces[((own%4)<<1)+1];
    *p++='\0';
    *e=p;
  }
  else *e=buf+2;
  return(buf);
}

char *show_occupants(u)
Unit *u;
{
  static char occbuf[1000];
  char *p;
  return handle_occ(0,u,occbuf,&p);
}
/*** <- insert ***/

/* Display all the details that only the owner or God (== debugger) sees. */
/*** (HW) draw bar graphs reinserted ***/

show_intimate_details(side, unit)
Side *side;
Unit *unit;
{
    int i, u = unit->type, r, nums[MAXUTYPES], xpos, p = unit->product;
    Unit *occ;
    char tmp2[BUFSIZE];

    sprintf(spbuf, "");
    sprintf(spbuf, "In %s at %d,%d",
	    (unit->transport != NULL ? short_unit_handle(unit->transport) :
	     ttypes[terrain_at(unit->x, unit->y)].name),
	     unit->x, unit->y);
    sprintf(tmpbuf, "");
    draw_info_text(side, 0, 1, 30, spbuf);
    sprintf(spbuf, "");
    if (unit->occupant != NULL) {
/*** (UK) remove ->
	strcpy(spbuf, "Occ ");
	for_all_unit_types(i) nums[i] = 0;
	for_all_occupants(unit, occ) nums[occ->type]++;
	for_all_unit_types(i) {
	    if (nums[i] > 0) {
		sprintf(tmpbuf, "%d %c  ", nums[i], utypes[i].uchar);
		strcat(spbuf, tmpbuf);
	    }
	}
*** <- remove ***/
/*** (UK) insert -> ***/
      sprintf(spbuf,"Occ %s",show_occupants(unit));
/*** <- insert ***/
    }
/*** (HW) change -> ***/
    draw_info_text(side, 0, 2, 100, spbuf);
/*** was:
    draw_info_text(side, 0, 2, 30, spbuf);
*** <- change ***/
    /* If we woke up, add an explanation. */
    tmp2[0] = '\0';
    if (unit->wakeup_reason) { /* and test that unit is awake? */
	if (unit->wakeup_reason == WAKEENEMY) 
	  sprintf(tmp2, " (Saw %s %s)",
		  (unit->waking_side == NULL ? "neutral" :
		   unit->waking_side->name),
		  utypes[unit->waking_type].name);
	if (unit->wakeup_reason == WAKERESOURCE) 
	  sprintf(tmp2, " (Low on fuel)");
	/*      if (unit->wakeup_reason == WAKETIME) 
		sprintf(tmp2, " (Last order done)"); */
	if (unit->wakeup_reason == WAKELEADERDEAD) 
	  sprintf(tmp2, " (Leader died)");
	if (unit->wakeup_reason == WAKELOST) 
	  sprintf(tmp2, " (Lost & confused)");
    }
    sprintf(spbuf, "%s%s %s Moves %d",
	    order_desig(&(unit->orders)), tmp2,
	    (unit->standing != NULL ? "*" : ""),
	    unit->movesleft);
    draw_info_text(side, 0, 4, 80, spbuf);
/*** (UK) insert -> ***/
    if (unit->group) {
      sprintf(spbuf,"Group %s",unit->group);
      draw_info_text(side, 0, 3, 30, spbuf);
    }
/*** <- insert ***/
/*** (HW) insert -> ***/
    if (side->graphical && bar_graphs(side)) {
	xpos = 0;
	draw_graph(side, xpos++, unit->hp,
		   utypes[u].hp, utypes[u].crippled, "HP");
	if (utypes[u].maxquality > 0)
	    draw_graph(side, xpos++, unit->quality,
		       utypes[u].maxquality, 0,"Quality");
/*** (HW) remove ->
 *** morale does not exist in 5.5.3
	if (utypes[u].maxmorale > 0)
	    draw_graph(side, xpos++, unit->morale,
		       utypes[u].maxmorale, 0, "Morale");
*** <- remove ***/
	if (producing(unit)) {
	    draw_graph(side, xpos++, unit->schedule,
		       utypes[u].make[p], 0, utypes[p].name);
	}
	for_all_resource_types (r) {
	    if (utypes[u].storage[r] > 0) {
		draw_graph(side, xpos++, unit->supply[r],
			   utypes[u].storage[r], utypes[u].storage[r]/2,
			   rtypes[r].name);
	    }
	}
    } else {
/*** <- insert ***/
      sprintf(spbuf, "HP %d/%d", unit->hp, utypes[u].hp);
      if (utypes[u].maxquality > 0)
	sprintf(spbuf+strlen(spbuf), "   Quality %d/%d",
		unit->quality, utypes[u].maxquality);
      draw_info_text(side, 30, 0, -1, spbuf);
      if (producing(unit)) {
	sprintf(spbuf, "New %s in %d turns",
		utypes[p].name, unit->schedule);
/*** (UK) insert -> ***/
	/*
        if (unit->next_product && unit->next_product!=unit->product) {
	  strcat(spbuf," (next ");
	  strcat(spbuf,utypes[unit->next_product].name);
	  strcat(spbuf,")");
	}
	*/
/*** <- insert ***/
	draw_info_text(side, 30, 1, -1, spbuf);
      }
      if (producing(unit) && unit->product != unit->next_product) {
	sprintf(spbuf, "Next product is %s",
		utypes[unit->next_product].name);
	draw_info_text(side, 30, 2, -1, spbuf);
      }
      sprintf(spbuf, "");
      for_all_resource_types(r) {
	if (utypes[u].storage[r] > 0) {
	  sprintf(tmpbuf, "%s %d/%d  ",
		  rtypes[r].name, unit->supply[r], utypes[u].storage[r]);
	  strcat(spbuf, tmpbuf);
	}
      }
      draw_info_text(side, 30, 3, -1, spbuf);
/*** (HW) insert -> ***/
    }
/*** <- insert ***/
}

/* Display improvement can be achieved by padding out lines with blanks, */
/* then the lines need not be cleared before redrawing. */

draw_info_text(side, x, y, len, buf)
Side *side;
int x, y, len;
char *buf;
{
    int i;
    char full[BUFSIZE];

    if (len < 0) len = BUFSIZE;
    strcpy(full, buf);
    for (i = strlen(buf); i < len; ++i) full[i] = ' ';
    full[len-1] = '\0';
    draw_fg_text(side, side->info,
		 side->margin + x * side->fw, y * side->fh, full);
}

/* Erase the info window.  This is done frequently so worthless stuff isn't */
/* hanging around, such as during someone else's turn. */

clear_info(side)
Side *side;
{
    if (active_display(side)) {
	clear_window(side, side->info);
    }
}

/* The prompt window consists of exactly one line, but we have to keep track */
/* of where the blank space begins, for when people type into it. */

show_prompt(side)
Side *side;
{
    if (active_display(side)) {
	clear_window(side, side->prompt);
	draw_fg_text(side, side->prompt, side->margin, 0, side->promptbuf);
    }
}

/* Spit a char onto the prompt line, hopefully after the previous one. */

echo_at_prompt(side, ch)
Side *side;
char ch;
{
    char tmp[2];

    if (active_display(side)) {
	sprintf(tmp, "%c", ch);
	draw_fg_text(side, side->prompt,
		     side->reqcurstr * side->fw + side->margin, 0, tmp);
	flush_output(side);
    }
}

/* Spit a cursor onto the prompt line. */

write_str_cursor(side)
Side *side;
{
    char tmp[3];

    if (active_display(side)) {
	sprintf(tmp, "_ ");
	draw_fg_text(side, side->prompt,
		     side->reqcurstr * side->fw + side->margin, 0, tmp);
	flush_output(side);
    }
}

/* Clear the prompt line. */

clear_prompt(side)
Side *side;
{
    if (active_display(side)) {
	clear_window(side, side->prompt);
	sprintf(side->promptbuf, " ");
    }
}

/* Show a list of all sides in action, drawing a line through any that have */
/* lost out.  This has to be called for each side if everybody's list is */
/* to be updated. */

/* Highlight the side whose turn it is, using reverse video for current */
/* player's own display (to wake she/he/it up) and white instead of gray */
/* for everybody else.  Also add a star for the benefit of monochrome. */

show_all_sides(side, clear)
Side *side;
int clear;
{
    int sx = side->margin + 2 * side->fw, sy = 0, i;
    Side *side2;

    if (active_display(side)) {
	if (clear)
	  clear_window(side, side->sides);
	for_all_sides(side2) {
	    sprintf(spbuf, "%2d", side_number(side2));
	    draw_text(side, side->sides, side->margin, sy, spbuf,
		      side_color(side, side2));
	    sprintf(tmpbuf, "");
	    if (side2->host != NULL) sprintf(tmpbuf, "(%s)", side2->host);
	    /* Don't bother printing host information past first . */ 
	    for (i = 0; i < strlen(tmpbuf); i++) 
	      if (tmpbuf[i] == '.') 
		{
		  strcpy(tmpbuf+i, ")");
		}
/*** (UK) insert -> ***/
             if (active_display(side2)) strcat(tmpbuf,"*");
/*** <- insert ***/ 
	    sprintf(spbuf, ": The %s %s", plural_form(side2->name), tmpbuf);
	    draw_text(side, side->sides, sx, sy, spbuf,
		      (side == side2 ? side->fgcolor : side->graycolor));
	    if (side2->lost) draw_scratchout(side, sy + side->fh / 2);
	    sy += side->fh;
	}
    }
    update_sides(side);
}

/* The side color here reflects ally/neutral/enemy status. */

side_color(side, side2)
Side *side, *side2;
{
    if (side == side2 || allied_side(side, side2)) return side->altcolor;
    if (enemy_side(side, side2)) return side->enemycolor;
    return side->neutcolor;
}

/* Show which human players still have units to move this turn
 * by displaying an asterisk in front of the side name in the sides list.
 */
update_sides (side)
Side *side;
{
    int sx = side->margin + 3*side->fw, sy = 0;
    Side *side2;

    if (active_display(side)) {
	for_all_sides(side2) {
	    if (!side2->lost) {
		draw_text(side, side->sides, sx, sy, 
			  (side2->more_units ? "*" : " "),
			  (side == side2 ? side->fgcolor : side->graycolor));
	    }
	    sy += side->fh;
	}
	flush_output2(side);
    }
}

/* Update the turn number and game mode display.  The mode is inverted */
/* so will stand out (it governs what player input will be accepted, so */
/* quite important). */

show_timemode(side,clear)
Side *side;
int clear;
{
    if (active_display(side)) {
	if (clear)
	  clear_window(side, side->timemode);
/*** (UK) change -> ***/
	sprintf(spbuf, "  Turn %4d", global.time);
/*** was:
	sprintf(spbuf, "Turn%4d", global.time);
*** <- change ***/
	draw_fg_text(side, side->timemode, side->margin, 0, spbuf);
/*** (UK) change -> ***/
	if (side->workmode) 
	  sprintf(spbuf, "      Work     ");
	else
	  sprintf(spbuf, "   %s    ", modenames[side->mode]);
/*** was:
	sprintf(spbuf, "%s%s",
		modenames[side->mode],
		(side->teach ? "*" : (Build ? "B" : (side->followaction ? "L" : " "))));
*** <- change ***/
/*** (HW) change -> ***/
	draw_text(side, side->timemode, side->margin, side->fh, spbuf,
		  (side->more_units ? side->bgcolor : side->fgcolor));
/*** was:
	draw_text(side, side->timemode, side->margin, side->fh, spbuf,
		  (side == curside ? side->bgcolor : side->fgcolor));
*** <- change ***/
/*** (UK) insert -> ***/
	if (side->teach || side->followaction || 
	    side->show_product || side->show_only_producers || 
	    side->show_orders || side->notifyvisibility || 
	    side->show_no_movers ||
	    side->showsorderutype != NOTHING) {
	  sprintf(spbuf, "Mode:%s%s%s%s%s%s%s%s%s",
		side->teach?"*":" ",
		side->followaction?"L":" ",
		side->notifyvisibility?"V":" ",
                side->show_product?"P":" ",
		side->show_only_producers?"B":" ",
		side->show_no_movers?"M":" ",
		side->show_orders?"O":" ",
		(side->showsorderutype!=NOTHING)?"S":" ",
		Build?"B":" ");
        }
	else sprintf(spbuf,"Mode: <none>");
	draw_fg_text(side, side->timemode, side->margin, 2*side->fh, 
			spbuf);
        if (side->curgroup) {
	  draw_fg_text(side, side->timemode, side->margin, 3*side->fh, 
			  side->curgroup);
	}
/*** <- insert ***/
	flush_output(side);
    }
}

/* The state display summarizes all the units and any other global info. */

show_state(side,clear)
Side *side;
int clear;
{
    int u;

    if (active_display(side)) {
	if (clear)
	  clear_window(side, side->state);
	draw_unit_list(side, side->bvec);
	for_all_unit_types(u) update_state(side, u);
    }
}

/* Alter the numbers for a single type of unit.  Should be called right */
/* after any changes.  Formatted to look nice, but kind of messy to set */
/* up correctly - Cobol isn't all bad! */

update_state(side, u)
Side *side;
int u;
{
    int ypos;
/*** (UK) insert -> ***/
    int nn;
/*** <- insert ***/
/*** (HW) change -> ***/
    static char	_23space[] = "                       ";
/*** was:
    static char	_15space[] = "               ";
*** <- change ***/

    if (active_display(side)) {
	sprintf(spbuf, "");
	if (side->units[u] > 0)	
	    sprintf(tmpbuf, "%3d", side->units[u]);
	else
	    sprintf(tmpbuf, "   ");
	strcat(spbuf, tmpbuf);
	if (side->building[u] > 0) {
	    sprintf(tmpbuf, "(%d)", side->building[u]);
	    strcat(spbuf, tmpbuf);
	}
/*** (HW) change -> ***/
	strcat(spbuf, _23space);
/*** was:
	strcat(spbuf, _15space);
*** <- change ***/
	if (total_gain(side, u) > 0) {
	    sprintf(spbuf+8, "%4d", total_gain(side, u));
	}
/*** (HW) change -> ***/
	strcat(spbuf, _23space);
/*** was:
	strcat(spbuf, _15space);
*** <- change ***/
	if (total_loss(side, u) > 0) {
	    sprintf(tmpbuf, "- %d", total_loss(side, u));
	    sprintf(spbuf+13, "%s", tmpbuf);
	}
/*** (UK/HW) insert -> ***/
	strcat(spbuf, _23space);
	if ((nn=next_ready(side, u)) > 0) {
	    sprintf(spbuf+19, "[%3d]", nn);
	}
/*** <- insert ***/
	ypos = u * max(side->hh, side->fh) + (side->hh-side->fh) / 2;
/*** (HW) insert -> ***/
	if (side->showsorderutype == u)
	  draw_bg_text(side, side->state, side->hw+side->margin, ypos, spbuf);
	else
/*** <- insert ***/
	  draw_fg_text(side, side->state, side->hw+side->margin, ypos, spbuf);
    }
}

/* Would be faster to stash these, but enough difference to care? */

total_gain(side, u)
Side *side;
int u;
{
    int i, total = 0;

    for (i = 0; i < DUMMYREAS; ++i) total += side->balance[u][i];
    return total;
}

total_loss(side, u)
Side *side;
int u;
{
    int i, total = 0;

    for (i = DUMMYREAS+1; i < NUMREASONS; ++i) total += side->balance[u][i];
    return total;
}

/* To get the traditional digital clock display, we need fixed-format digit */
/* printing.  There will *never* be an option for analog display... */

show_clock(side)
Side *side;
{
    unsigned gametime;

    /* If there's a game time limit, it's most important; show it. */
    /* Otherwise, if there's a per-turn time limit, show it. */
    /* Otherwise, show time taken so far. */
    gametime =
      ((global.giventime > 0) ? side->timeleft :
       (global.timeout > 0) ? global.starttime + global.timeout - time(0)
       : side->timetaken);

    if (active_display(side)) {
	clear_window(side, side->clock);
	sprintf(spbuf, "%03u:%02u:%02u",
		gametime/3600,		/* hour */
		(gametime/60)%60,	/* minute */
		gametime%60);		/* second */
	draw_fg_text(side, side->clock, 0, 0, spbuf);
    }
}

/* Updates to clock need to be sure that display changes immediately. */

update_clock(side)
Side *side;
{
    show_clock(side);
    flush_output2(side); 
}

/* Dump a file into the help window.  This routine is neither sophisticated */
/* nor robust - lines must be short enough and file must be one page. */
/* Returns success or failure of file opening. */

show_file(side, fname)
Side *side;
char *fname;
{
    FILE *fp;

    if ((fp = fopen(fname, "r")) != NULL) {
	while (fgets(spbuf, BUFSIZE, fp)) {
	    spbuf[strlen(spbuf)-1] = '\0';  /* cut off newline */
	    wprintf(side, spbuf);
	}
	fclose(fp);
	return TRUE;
    } else {
	return FALSE;
    }
}

/* Init our counters or open a file. */

init_wprintf(side, fname)
Side *side;
char *fname;
{
    wline = 0;
    if (fname != NULL) {
	wprintmode = TRUE;
	wfp = fopen(fname, "w");
	if (wfp == NULL) {
	    notify(side, "Warning: open failed.");
	}
    } else {
	wprintmode = FALSE;
	clear_window(side, side->help);
    }
}

/* Make like printf, but to the help window.  This is pretty crude - has an */
/* automatic newlining (mis)feature, and doesn't do anything about long */
/* lines, but it's good enough. */

/*VARARGS*/
wprintf(side, control, a1, a2, a3, a4, a5, a6)
Side *side;
char *control, *a1, *a2, *a3, *a4, *a5, *a6;
{
    if (active_display(side)) {
	sprintf(wbuf, control, a1, a2, a3, a4, a5, a6);
	if (wprintmode && wfp != NULL) {
	    fprintf(wfp, "%s\n", wbuf);
	} else {
	    if (strlen(wbuf) > 0) {
		draw_fg_text(side, side->help, side->margin, wline*side->hfh, wbuf);
	    }
	    wline++;
	}
    }
}

/* If we were actually printing to a file, close it down. */

finish_wprintf()
{
    if (wprintmode && wfp != NULL) fclose(wfp);
}

/* Frequently-called routine to draw text in the foreground color. */

draw_fg_text(side, win, x, y, str)
Side *side;
long win;
int x, y;
char *str;
{
    draw_text(side, win, x, y, str, side->fgcolor);
}

/*** (HW) insert -> ***/
draw_bg_text(side, win, x, y, str)
Side *side;
long win;
int x, y;
char *str;
{
    draw_text(side, win, x, y, str, side->bgcolor);
}
/*** <- insert ***/
