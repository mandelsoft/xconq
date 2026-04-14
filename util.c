/* Copyright (c) 1987, 1988, 1991  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Random utilities not classifiable elsewhere. Most are not xconq-specific. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "map.h"

#ifndef DEBUG 
char *procedure_executing[PROCSTACKSIZE], *routine_executing;
int procedure_stack_ptr = -1;
#endif

char ordnum[BUFSIZE];           /* buffer for ordinal numbers */
char pluralbuf[BUFSIZE];        /* buffer for plurals of words */

int dirx[] = DIRX, diry[] = DIRY;  /* arrays for dir-to-delta conversion */

/* This generates a file name in wherever xconq keeps its assorted files. */

make_pathname(path, name, extn, pathbuf)
char *path, *name, *extn, *pathbuf;
{
#ifdef UNIX
    sprintf(pathbuf, "%s%s%s%s%s",
	    ((path && strlen(path) > 0) ? path : ""),
	    ((path && strlen(path) > 0) ? "/" : ""),
	    name,
	    ((extn && strlen(extn) > 0) ? "." : ""),
	    ((extn && strlen(extn) > 0) ? extn : ""));
#endif /* UNIX */
}

/* Remove a saved game from the system. */

remove_saved_game()
{
#ifdef UNIX
    unlink(SAVEFILE);
#endif /* UNIX */
}

/* Random number handling is important to game but terrible/nonexistent */
/* in some systems.  Do it ourselves and hope it's better... */

long randstate;           /* The random state *must* be at least 32 bits. */

/* Seed can come from elsewhere, for repeatability.  Otherwise, it comes */
/* the current time, scaled to a point where 32-bit arithmetic won't */
/* overflow.  Pid is not so good, usually a smaller range of values. */

void init_random(seed)
int seed;
{
    if (seed >= 0) {
	randstate = seed;
    } else {
#ifdef UNIX
	randstate = (time((long *) 0) % 100000L);
#endif /* UNIX */
    }
}

/* Numbers lifted from Numerical Recipes, p. 198. */
/* Arithmetic must be 32-bit. */

random(m)
int m;
{
    randstate = (8121 * randstate + 28411) % 134456L;
    return ((m * randstate) / 134456L);
}

/* Percentage probability, with bounds checking. */

probability(prob)
int prob;
{
    if (prob <= 0) return FALSE;
    if (prob >= 100) return TRUE;
    return (RANDOM(100) < prob);
}

/* Read a line and save it away.  This routine should be used sparingly, */
/* since the malloced space is never freed. */

char *
read_line(fp)
FILE *fp;
{
    char tmp[BUFSIZE], *line;

    fgets(tmp, BUFSIZE-1, fp);
    tmp[strlen(tmp)-1] = '\0';
    line = (char *) malloc(strlen(tmp)+2);
    strcpy(line, tmp);
    return line;
}

/* Copy to new-allocated space.  Again, the new space is never freed. */

char *
copy_string(str)
char *str;
{
    char *rslt;

    rslt = (char *) malloc(strlen(str)+1);
    strcpy(rslt, str);
    return rslt;
}

/* Computing distance in a hexagonal system is a little peculiar, since it's */
/* sometimes just delta x or y, and other times is the sum.  Basically there */
/* are six ways to compute distance, depending on the hextant we're in. */

distance(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    int dx = x2 - x1, dy = y2 - y1;

/*** (UK) insert -> ***/
    if (dx==0 && dy==0 ) return 0;
/*** <- insert ***/
    dx = (dx < 0 ? (dx < 0 - world.width / 2 ? world.width + dx : dx)
	         : (dx > world.width / 2 ? dx - world.width : dx));
    if (dx >= 0) {
	if (dy >= 0) {
	    return (dx + dy);
	} else if ((0 - dy) <= dx) {
	    return dx;
	} else {
	    return (0 - dy);
	}
    } else {
	if (dy <= 0) {
	    return (0 - (dx + dy));
	} else if (dy <= (0 - dx)) {
	    return (0 - dx);
	} else {
	    return dy;
	}
    }
}

/* Convert any vector into a direction (not necessarily the closest one). */
/* Fail horribly on zero vectors. */

find_dir(dx, dy)
int dx, dy;
{
  dx = (dx < 0 ? (dx < 0 - world.width / 2 ? world.width + dx : dx)
	: (dx > world.width / 2 ? dx - world.width : dx));
  if (dx < 0) {
    if (dy < 0) return SW;
	if (dy == 0) return WEST;
    return NW;
  } else if (dx == 0) {
    if (dy > 0) return NE;
    if (dy == 0) abort();
    return SW;
  } else {
    if (dy < 0) return SE;
    if (dy == 0) return EAST;
    return NE;
  }
}

/* Given a number, figure out what suffix should go with it. */
/* Note the use of static storage (to save a little mallocing) */

char *
ordinal(n)
int n;
{
    char *suff;

    if (n % 100 == 11 || n % 100 == 12 || n % 100 == 13) {
	suff = "th";
    } else {
	switch (n % 10) {
	case 1:   suff = "st"; break;
	case 2:   suff = "nd"; break;
	case 3:   suff = "rd"; break;
	default:  suff = "th"; break;
	}
    }
    sprintf(ordnum, "%d%s", n, suff);
    return ordnum;
}

/* Pluralize a word, attempting to be smart about various possibilities */
/* that don't have a different plural form (such as "Chinese" and "Swiss"). */
/* There should probably be a test for when to add "es" instead of "s". */

char *
plural_form(word)
char *word;
{
    char endch = ' ', nextend = ' ';

    if (strlen(word) > 0) endch   = word[strlen(word)-1];
    if (strlen(word) > 1) nextend = word[strlen(word)-2];
    if (endch == 'h' || endch == 's' || (endch == 'e' && nextend == 's')) {
	sprintf(pluralbuf, "%s", word);
    } else {
	sprintf(pluralbuf, "%ss", word);
    }
    return pluralbuf;
}

/* Get a *numeric* index into a string (more useful than ptr to xconq). */
/* Return -1 on failed search. */

iindex(ch, str)
char ch, *str;
{
    int i;

    if (str == NULL) return (-1);
    for (i = 0; str[i] != '\0'; ++i) if (ch == str[i]) return i;
    return (-1);
}

/* This little routine goes at the end of all switch statements on internal */
/* data values.  We want a core dump to debug. */

case_panic(str, var)
char *str;
int var;
{
    fprintf(stderr, "Panic! Unknown %s %d\n", str, var);
    abort();
}


#ifdef I_WANT_A_BAD_SQRT

/* crude integer square root */
int isqrt(i)
int i;
{
  int sqrt = i / 2;
  
  sqrt = sqrt / 2 - i / (2 * max(sqrt, 1));
  sqrt = sqrt / 2 - i / (2 * max(sqrt, 1));
  sqrt = sqrt / 2 - i / (2 * max(sqrt, 1));
  sqrt = sqrt / 2 - i / (2 * max(sqrt, 1));
  return max(sqrt, 1);
}
#else

int isqrt(i) /* Contributed by Bruce Fast */
int i;
{
int j,k;

if (i>0)
  {
  for (j=1,k=i;(abs(j-k)>1);j=(j+k)/2,k=i/j);
  return (j+k)/2;
  }
  else return 0;
}

#endif
