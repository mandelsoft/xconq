/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Code relating to the reading and writing of mapfiles.  A mapfile comes */
/* in several independent sections, each of whose presence is flagged in the */
/* header line.  A mapfile may also specify the inclusion of other mapfiles. */

#include "config.h"
#include "misc.h"
#include "dir.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

/* A mapfile header has various pieces that we need to save. */

typedef struct a_header {
    int version;                /* format version */
    int numincls;               /* number of included files */
    char *includes[MAXINCLUDES];  /* raw names of included files */
    int numnotes;               /* number of lines in designer notes */
    char *notes[MAXMAPNOTES];   /* designer notes for mapfile */
    char *sections;             /* +/- string of mapfile sections */
    int sdetail;
    int udetail;
} Header;

/* Sections of a mapfile have their recognizers. */

#define vsect(s) (s[0] == '+')
#define psect(s) (s[1] == '+')
#define msect(s) (s[2] == '+')
#define gsect(s) (s[3] == '+')
#define ssect(s) (s[4] == '+')
#define usect(s) (s[5] == '+')

/* These vars tell init code what it has to do itself. */

extern bool periodmade, mapmade, globalsmade, sidesmade, unitsmade;

extern int nextid;

World world;                    /* the world structure itself! */

Header *mainhead = NULL;        /* the "top-level" map header - needs saving */

int nummaps;                    /* Number of mapfiles loaded */

int curgiven;                   /* tmp for side creation */
int nut;                       /* Number of read in units transport */
int nusn;                       /* Number of read in units side */

char *mapfilenames[MAXLOADED];  /* Names of loaded files */
char *perfilename = NULL;

bool sidecountsread;            /* Obscure, fixes numbers of restored units */
bool midturnrestore = FALSE;    /* True if game will start up in the middle
				   of a turn. */

/*** (UK) insert -> ***/
int restoresides;               /* flag to indicate to restore side hosts */

extern bool PartialGame;
extern bool ServerFlag;
/*** <- insert ***/ 

/* mxconq: defined in xconq.c */
extern bool singlePlayerMode;

/*** (UK) insert -> ***/
prep_string(s)
char *s;
{ 
  while (*s) {
    if (*s == ' ') *s='*';
    s++;
  }
}

unprep_string(s)
char *s;
{ 
  while (*s) {
    if (*s == '*') *s=' ';
    s++;
  }
}
/*** <- insert ***/ 

/* Malloc just enough space for the map. */

allocate_map()
{
    world.terrain = (Hex *) malloc(world.width * world.height * sizeof(Hex));
}

/* Game files can live in library directories or somewhere else.  This */
/* function tries to find a file, open it, and load the contents. */

load_mapfile(name)
char *name;
{
    bool loaded = FALSE;
    int i;
    FILE *fp = NULL;

/*** (UK) insert -> ***/
    restoresides=!numgivens && !ServerFlag;;
/*** <- insert ***/ 

    make_pathname(xconqlib, name, "", spbuf);
    for (i = 0; i < nummaps; ++i) {
	if (strcmp(name,  mapfilenames[i]) == 0) { loaded = TRUE; break; }
	if (strcmp(spbuf, mapfilenames[i]) == 0) { loaded = TRUE; break; }
    }
    if (loaded) {
	fprintf(stderr, "\"%s\" is already loaded.\n", name);
    } else if ((fp = fopen(spbuf, "r")) != NULL) {
	if (Debug) printf("Reading \"%s\" ...\n", spbuf);
	mapfilenames[nummaps++] = copy_string(spbuf);
    } else if ((fp = fopen(name, "r")) != NULL) {
	if (Debug) printf("Reading \"%s\" ...\n", name);
	mapfilenames[nummaps++] = copy_string(name);
    } else {
	fprintf(stderr, "Neither \"%s\" or \"%s\" could be opened!\n",
		spbuf, name);
	exit(1);
    }
    if (fp != NULL) {
	load_sections(fp);
	fclose(fp);
    }
    if (periodmade) {
	perfilename = name;
	periodmade = FALSE;
    }
}

/* Grab up all the components of a mapfile, and recurse if any submapfiles. */
/* The header line tells us which sections to try to load.  For some */
/* sections, flag them as loaded so we know not to synthesize replacements. */

int format_version=0;

load_sections(fp)
FILE *fp;
{
    char sect[BUFSIZE], *tmp;
    int numfiles, i;
    Header *head;

    /* Read and interpret the header */
    head = (Header *) malloc(sizeof(Header));
    if (mainhead == NULL) mainhead = head;

    format_version=0;
    if (3!=fscanf(fp, "%d-Xconq %d %s", &format_version, &numfiles, sect)) {
        if (2!=fscanf(fp, "Xconq %d %s", &numfiles, sect)) {
          fprintf(stderr, "Error: bad format in load_sections (maybe 0 length file)\n");
          exit(1);
        }
    }
    if (format_version < 0 || format_version > 1) {
          fprintf(stderr, "Error: unknown format version %d\n", format_version);
          exit(1);
    }
    printf("using format version %d\n", format_version);
    head->version = format_version;
    head->numincls = numfiles;
    head->sections = copy_string(sect);
    head->sdetail = head->udetail = 0;
    tmp = read_line(fp);
    head->notes[0] = tmp;
    head->numnotes = 1;
    if (tmp[strlen(tmp)-1] == ';') {
	for (i = 1; i < MAXMAPNOTES; ++i) {
	    head->notes[i] = read_line(fp);
	    if (strcmp(head->notes[i], ".") == 0) {
		head->numnotes = i;
		break;
	    }
	    if (Debug) printf("%s\n", head->notes[i]);
	}
    }
    for (i = 0; i < numfiles; ++i) {
	head->includes[i] = read_line(fp);
	if (Debug) printf("Including %s ...\n", head->includes[i]);
	load_mapfile(head->includes[i]);
    }
    sidecountsread = FALSE;
    /* Suck in all the sections */
    if (vsect(sect)) {
	read_version(fp);
    }
    if (psect(sect)) {
	read_period(fp);
	periodmade = TRUE;
    }
    if (msect(sect)) {
	read_map(fp);
	mapmade = TRUE;
    }
    if (gsect(sect)) {
	read_globals(fp);
	globalsmade = TRUE;
    }
    if (ssect(sect)) {
	head->sdetail = read_sides(fp);
	sidesmade = TRUE;
    }
    if (usect(sect)) {
	head->udetail = read_units(fp);
	unitsmade = TRUE;
    }
    sidecountsread = FALSE;
    /* not great, but at least free the memory. */
    if (mainhead != head) free(head);
}

/* Read the version line and test it against the program version. */
/* Continuation after failure is likely to result in core dump, so leave. */

read_version(fp)
FILE *fp;
{
    char *tmpver;

    if (Debug) printf("Will try to read version ...\n");
    tmpver = read_line(fp);
    if (strcmp(tmpver, version) != 0) {
	fprintf(stderr, "Sorry, mapfile is for some other version!\n");
	fprintf(stderr, "Version should be \"%s\", not \"%s\".\n",
		version, tmpver);
	exit(1);
    }
    if (Debug) printf("... Done reading version.\n");
}

/* Read the map section, starting with header, then working through data. */
/* The map data may be partly run-length encoded; this is recognized by the */
/* presence of digit strings (of arbitrary length) followed by the run char. */
/* Any or all of the data may be so encoded, otherwise the chars are 1-1 */
/* with map hexes; newlines are always at the end of each map row. */

read_map(fp)
FILE *fp;
{
    char ch;
    int extension, x, y, terr, run;
    int garbage = 0;

    fscanf(fp, "Map %d %d %d %d %d\n",
	   &world.width, &world.height,
	   &garbage, &world.known, &extension);
    if (Debug) printf("Will try to read %dx%d map ...\n",
		      world.width, world.height);
    allocate_map();
    for (y = world.height-1; y >= 0; --y) {
	for (x = 0; x < world.width; /* incr below */) {
	    ch = getc(fp);
	    if (isdigit(ch)) {
		run = ch - '0';
		while (isdigit(ch = getc(fp))) {
		    run = run * 10 + ch - '0';
		}
	    } else {
		run = 1;
	    }
	    terr = find_terr(ch);
	    if (terr < 0) terr = period.defaultterrain;
	    if (terr < 0) {
		fprintf(stderr, "'%c' is not valid terrain in this period!\n",
			ch);
		exit(1);
	    }
	    while (run-- > 0) {
		set_terrain_at(x, y, terr);
/*		set_people_at(x, y, NOBODY); */
		set_unit_at(x, y, NULL);
		++x;
	    }
	}
	fscanf(fp, "\n");
    }
    if (Debug) printf("... Done reading map.\n");
}

/* This should be more efficient.  Fortunately, run-length encoded maps */
/* don't call it for every single hex! */

find_terr(ch)
char ch;
{
    register int t;

    for_all_terrain_types(t) if (ch == ttypes[t].tchar) return t;
    return (-1);
}

/* Period reading is complicated, and therefore lives in its own file. */

/* Read the globals section.  Most info of interest is in the header, but */
/* win/lose conditions need extra reads. */

read_globals(fp)
FILE *fp;
{
    int extension, i, u, r;
    Condition *cond;

/*** (UK) change -> ***/
    fscanf(fp, "Globals %d %d %d %d %d %u %d\n",
	   &global.time, &global.endtime, &global.setproduct,
	   &global.leavemap, &global.numconds, &global.giventime, &extension);
/*** was:
    fscanf(fp, "Globals %d %d %d %d %d %d\n",
	   &global.time, &global.endtime, &global.setproduct,
	   &global.leavemap, &global.numconds, &global.giventime);
*** <- change ***/
    if (Debug) printf("Will try to read globals ...\n");
    for (i = 0; i < global.numconds; ++i) {
	cond = &(global.winlose[i]);
	fscanf(fp, "%d %d %d %d %d\n",
	       &(cond->win), &(cond->type),
	       &(cond->starttime), &(cond->endtime), &(cond->sidesn));
	switch (cond->type) {
	case TERRITORY:
	    fscanf(fp, "%d\n", &(cond->n));
	    break;
	case UNITCOUNT:
	    for_all_unit_types(u) {
		fscanf(fp, "%d", &(cond->units[u]));
	    }
	    fscanf(fp, "\n");
	    break;
	case RESOURCECOUNT:
	    for_all_resource_types(r) {
		fscanf(fp, "%d", &(cond->resources[r]));
	    }
	    fscanf(fp, "\n");
	    break;
	case POSSESSION:
	    fscanf(fp, "%d %d %d\n", &(cond->x), &(cond->y), &(cond->n));
	    break;
	default:
	    case_panic("condition type", cond->type);
	    break;
	}
    }
    if (Debug) printf("... Done reading globals.\n");
}

/* Read in the entire sides section.  Sides have several levels of detail, */
/* since sides' views of the world can be pretty large and we don't always */
/* need to remember them. */

int sidedetail;

read_sides(fp)
FILE *fp;
{
    int numtoread, detail, extension, i;
    Side *side;

    curgiven = 0;
    fscanf(fp, "Sides %d %d %d\n", &numtoread, &detail, &extension);
/*    if (detail < 0 || detail > SIDEALL) case_panic("detail", detail); */
    sidedetail = detail;
    if (Debug) printf("Will try to read %d sides at detail %d ...\n",
		      numtoread, detail);
    for (i = 0; i < numtoread; ++i) {
	if (detail >= SIDEBASIC) side = read_basic_side(fp);
	if (side != NULL) {
	    if (detail >= SIDESLOTS) read_side_attributes(side, numtoread, fp);
	    if (detail >= SIDEVIEW)  read_side_view(side, fp);
	    if (detail >= SIDEMISC) {
		read_side_misc(side, fp);
		read_side_statistics(side, fp);
	    }
	    if (Debug) printf("Got side named \"%s\",\n", side->name);
	}
    }
    if (Debug) printf("... Done reading sides.\n");
    return detail;
}

/* Create a side, which only requires a name - person-ness and display come */
/* from the command line if any were supplied. */

/* Somebody should react coherently to side creation failure... */

Side *
read_basic_side(fp)
FILE *fp;
{
    bool human = FALSE;
    char *host = NULL;
    int j;

    fscanf(fp, "%s\n", tmpbuf);
/*** (UK) change -> ***/
    unprep_string(tmpbuf);
/*** was:
    for (j = 0; j < strlen(tmpbuf); ++j) if (tmpbuf[j] == '*') tmpbuf[j] = ' ';
*** <- change ***/ 
    if (curgiven < numgivens) {
	human = humans[curgiven];
	host = hosts[curgiven];
	++curgiven;
    }
    return create_side(tmpbuf, human, host);
}

/* Read the most important attributes of a side. */

read_side_attributes(side, numtoread, fp)
Side *side;
int numtoread;
FILE *fp;
{
    int s, u;

    fscanf(fp, "%hd", &(side->ideology));
    for (s = 0; s < numtoread; ++s) fscanf(fp, "%hd", &(side->attitude[s]));
    for_all_unit_types(u) fscanf(fp, "%hd", &(side->counts[u]));
    fscanf(fp, "\n");
    sidecountsread = TRUE;
}

/* Read the goriest of details about a side - those things that are */
/* relevant only to a particular game. */

read_side_misc(side, fp)
Side *side;
FILE *fp;
{
    bool human;
    char host[BUFSIZE];

/*** (UK) change -> ***/
    int i;
    int extension;
    int state;
    int c=0;
    char passwd[BUFSIZE];

    fscanf(fp, "%d %s %s %hd %u %d",
	   &human, passwd, host, &(side->lost),
	   ((global.giventime > 0) ? &(side->timeleft) : &(side->timetaken)),
	   &extension);
    fscanf(fp, "\n");
    
    unprep_string(passwd);
    if (strcmp(passwd, " ") == 0) side->passwd=NULL;
    else side->passwd=copy_string(passwd);

    if (extension&1) { /* read save positions */
      fscanf(fp, "%d %d\n",&c,&state); /* number of entries*/
      for (i=0; i<c; i++) {
	int n,x,y;
	fscanf(fp,"%d ",&n);
	side->savstate[n].used=TRUE;
	if (state&1) {
	  fscanf(fp,"%d %d",&side->savstate[n].x,&side->savstate[n].y);
	}
	/* only x and y up to now */
      }
      fscanf(fp,"\n");
    }
/*** was:
    fscanf(fp, "%d %s %hd %hd ",
	   &human, host, &(side->lost),
	   ((global.giventime > 0) ? &(side->timeleft) : &(side->timetaken)));
    fscanf(fp, "\n");
*** <- change ***/

/* mxconq:  if in single player mode must get host and human attributes! */
/*** (UK) change -> ***/
    if (restoresides || singlePlayerMode || PartialGame) {
	if (!side->host) {
	  if (host[0] == '*')
	      side->host = NULL;
	  else
	      side->host = copy_string(host);
	}
        side->humanp = human;

	printf("restoring %s at %s (%s)\n",
		side->name,side->host,human?"human":"machine");

	if (restoresides) {
	  add_player(human,side->host);
	}
    }

/*** was:
    if (singlePlayerMode)  {
        if (host[0] == '*')
            side->host = NULL;
        else
            side->host = copy_string(host);
        side->humanp = human;
    }
*** <- change ***/
}

/* Read the performance statistics of a side. */

read_side_statistics(side, fp)
Side *side;
FILE *fp;
{
    int u, u2, i;

    for_all_unit_types(u) {
	for (i = 0; i < NUMREASONS; ++i) {
	    fscanf(fp, "%hd", &(side->balance[u][i]));
	}
	fscanf(fp, "\n");
	for_all_unit_types(u2) {
	    fscanf(fp, "%hd", &(side->atkstats[u][u2]));
	}
	fscanf(fp, "\n");
	for_all_unit_types(u2) {
	    fscanf(fp, "%hd", &(side->hitstats[u][u2]));
	}
	fscanf(fp, "\n");
    }
}

/* Read about what has been seen in the world.  This routine fails if
units have a unitchar of '.' or '?' */

read_side_view(side, fp)
Side *side;
FILE *fp;
{
    char ch1, ch2;
    int x, y;
    viewdata view;
    int run;

    for (y = 0; y < world.height; ++y) {
	for (x = 0; x < world.width;) {
	    ch1 = getc(fp);  ch2 = getc(fp);
	    run = 1;
	    if (ch1 == '?' || ch1 == '.') {
	      if (isdigit(ch2)) {
		run = ch2 - '0';
		while (isdigit(ch2 = getc(fp))) {
		  run = run * 10 + ch2 - '0';
		}
		ungetc(ch2, fp);
	      }
	    }
	    if (ch1 == '?')
		view = UNSEEN;
	    else if (ch1 == '.')
		view = EMPTY;
	    else
		view = buildview(ch2 - '0', find_unit_char(ch1));
	    while (run-- > 0) {
	      set_side_view(side, x++, y, view);
	    }
	  }
	fscanf(fp, "\n");
    }
}


/* Read the entire units section.  Units also have different levels of */
/* detail. */

read_units(fp)
FILE *fp;
{
    int numtoread, detail, extension, i;
    Unit *unit, *transport;
    Side *side;

    fscanf(fp, "Units %d %d %d\n", &numtoread, &detail, &extension);
/*    if (detail < 0 || detail > UNITALL) case_panic("detail", detail); */
    if (detail >= UNITMOVED) midturnrestore = TRUE;
    if (Debug) printf("Will try to restore %d units at detail %d ...\n",
		      numtoread, detail);
    for (i = 0; i < numtoread; ++i) {
      if (Debug) printf("Restoring unit %d \n", i);
        nut = -1;
	if (detail >= UNITBASIC) {
	    if ((unit = read_basic_unit(fp)) != NULL) {
		if (detail >= UNITSLOTS) read_unit_attributes(unit, fp);
		if (detail >= UNITORDERS) read_unit_orders(unit, fp);
		if (detail >= UNITMOVED) read_unit_moved(unit, fp);
		unit->transport = (Unit *) nut;
		if (nusn >= 0) {
		  unit_changes_side(unit, side_n(nusn), -1, -1);
	    }
		if (Debug) 
		  printf("Got %s,\n", unit_handle((Side *) NULL, unit));
	    }
	}
    }
    reverse_unit_order();
    /* now put them on the map */
    for_all_units(side, unit)
      if (alive(unit)) /* should not be needed, but check anyways. */
	if (((int) unit->transport) >= 0) {
	  transport = find_unit((int) unit->transport);
	  /* it is important to make sure that unit->x, and unit->y 
	     are negative at this point.  Otherwise, the coverage will
	     be messed up for units put into transports that have not yet
	     been placed.  They will be covered for entering the hex,
	     and again when the transport enters the hex. */
	  if (transport != NULL) {
	    occupy_unit(unit, transport);
	  } else {
	    printf("\nCould not find transport (%d) for %d\n",
		   (int) unit->transport, unit->id);
	    abort();
	    unit->transport = NULL;
	    occupy_hex(unit, unit->prevx, unit->prevy);
	  }
	} else if (unit->prevx >=0 && unit->prevx < world.width &&
		   unit->prevy > 0 && unit->prevy < world.height-1) {
	  unit->transport = NULL;
	  occupy_hex(unit, unit->prevx, unit->prevy);
	} else {
	  printf("Warning: unit %d has bad coordinates %d,%d\n", unit->id,
		 unit->prevx, unit->prevy);
	  unit->hp = 0;
	}
      else printf("Dead unit read in.");
    if (Debug) printf("... Done reading units.\n");
    return detail;
}

/* Read the barest info about a neutral unit - just type, name, and loc. */
/* Do weird stuff to handle empty names represented by "*". */
/* Give it full supplies (good idea?).  Can't place just yet, because it */
/* might actually be an occupant of something else. */

Unit *
read_basic_unit(fp)
FILE *fp;
{
    char ch;
    int j, u, nux, nuy;
    Unit *newunit;

/*** (UK) change -> ***/
    if (fscanf(fp, "%c %s %s %d,%d %d\n", &ch, tmpbuf, tmpbuf2, 
					  &nux, &nuy, &nusn) < 6)
/*** was:
    if (fscanf(fp, "%c %s %d,%d %d\n", &ch, tmpbuf, &nux, &nuy, &nusn) < 5)
*** <- change ***/ 
      printf("Error in format of saved unit.");
/*** (UK) change -> ***/
    unprep_string(tmpbuf);
    unprep_string(tmpbuf2);
/*** was:
    for (j = 0; j < strlen(tmpbuf); ++j) if (tmpbuf[j] == '*') tmpbuf[j] = ' ';
*** <- change ***/ 
    if (strcmp(tmpbuf, " ") == 0) strcpy(tmpbuf, "");
    if ((u = find_unit_char(ch)) != NOTHING) {
	if ((newunit = create_unit(u, tmpbuf)) != NULL) {
	    init_supply(newunit);
	    newunit->prevx = nux;
	    newunit->prevy = nuy;
/*** (UK) insert -> ***/
	    if (strcmp(tmpbuf2, " ") == 0) {
		newunit->group = NULL;
	    } else {
		newunit->group = copy_string(tmpbuf2);
	    }
/*** <- insert ***/ 
	}
    } else {
	fprintf(stderr, "Unit '%c' is not of this period!\n", ch);
	exit(1);
    }
    return newunit;
}

/* Read most of a unit's attributes, but not orders and suchlike. */

read_unit_attributes(unit, fp)
Unit *unit;
FILE *fp;
{
    int truesidenum, r;
    int garbage;

    fscanf(fp, "%d %hd %d %hd %hd %hd %hd %hd %hd %hd %d",
	   &(unit->id), &(unit->number), &truesidenum,
	   &(unit->hp), &(unit->quality), &garbage, &garbage,
	   &(unit->product), &(unit->schedule), &(unit->built), &nut);
    switch (format_version) {
      case 1:
        fscanf(fp, "%hd %hd %hd",
            &(unit->terraform), &(unit->tschedule), &(unit->last_terraform));
        break;
    }
    unit->next_product = unit->product / 256;
    unit->product = unit->product % 256;
    for_all_resource_types(r) {
	fscanf(fp, "%hd", &(unit->supply[r]));
    }

    fscanf(fp, "\n");
    unit->trueside = side_n(truesidenum);
    /* tricky way to ensure subsequent units will have unique ids */
    nextid = max(nextid, unit->id + 1);
}

/* Read everything back in and try to recreate the unit's orders exactly. */
/* The saved orders include a flag for the presence of standing orders. */

/*** (UK) insert -> ***/
StandingOrderEntry **read_order_entry(fp,oe)
FILE *fp;
StandingOrderEntry *oe;
{   int n;
    int next;
    StandingConditionEntry **cep;

    if (fscanf(fp, "%s %d %d\n", tmpbuf, &n, &next) < 2)
      printf("Error in format of saved unit.");

    unprep_string(tmpbuf);
    if (strcmp(tmpbuf, " ") == 0) {
	oe->group = NULL;
    } else {
	oe->group = copy_string(tmpbuf);
    }
    oe->next=0;
    oe->handled=0;
    oe->cond=NULL;

    cep=&oe->cond;
    while (n--) {
      (*cep)=(StandingConditionEntry *) malloc(sizeof(StandingConditionEntry));
      (*cep)->order=(Order *) malloc(sizeof(Order));
      (*cep)->next=NULL;
      read_condition(fp,*cep);
      read_one_order(fp,(*cep)->order);
      cep=&(*cep)->next;
      fscanf(fp, "\n");
    }
  
    return next?&oe->next:NULL;
}

read_condition(fp,c)
FILE *fp;
SCondition *c;
{ int u;
  int d;

  fscanf(fp,"%d %d",&c->type,&c->priority);
  switch (c->type) {
    case C_FULL:
    case C_NFULL:
	       for_all_unit_types(u) {
		 fscanf(fp,"%d",&d);
		 c->d.utypes[u]=d;
	       }
	       break;
    case C_GT:
    case C_LE: fscanf(fp,"%d",&c->d.amount);
	       break;
    default:   break;
  }
}

StandingOrderEntry *read_order_entries(fp)
FILE *fp;
{ StandingOrderEntry *first;
  StandingOrderEntry **oep=&first;

  do {
    *oep=(StandingOrderEntry*)malloc(sizeof(StandingOrderEntry));
  } while (oep=read_order_entry(fp,*oep));
  return first;
}

/*** <- insert ***/ 

read_unit_orders(unit, fp)
Unit *unit;
FILE *fp;
{
    int i, more, uord[MAXUTYPES];

    fscanf(fp, "%hd %hd", &(unit->lastdir), &(unit->awake));
    read_one_order(fp, &(unit->orders));
    fscanf(fp, "%d\n", &more);
    if (more) {
	unit->standing = (StandingOrder *) malloc(sizeof(StandingOrder));
	for_all_unit_types(i) {
	    fscanf(fp, "%d", &(uord[i]));
	}
	fscanf(fp, "\n");
	for_all_unit_types(i) {
/*** (UK) change -> ***/
	    if (uord[i]&1) {
		unit->standing->p_orders[i] = read_order_entries(fp);
	    }
	    else unit->standing->p_orders[i]=NULL;
	    if (uord[i]&2) {
		unit->standing->e_orders[i] = read_order_entries(fp);
	    }
	    else unit->standing->e_orders[i]=NULL;
	    if (uord[i]&4) {
		unit->standing->f_orders[i] = read_order_entries(fp);
	    }
	    else unit->standing->f_orders[i]=NULL;

/*** was:
	    if (uord[i]) {
		unit->standing->orders[i] = (Order *) malloc(sizeof(Order));
		read_one_order(fp, unit->standing->orders[i]);
		fscanf(fp, "\n");
	      }
	    else unit->standing->orders[i] = NULL;
*** <- change ***/
	}
    }
}

/* Write how much of the units movement is available.  This will allow
   saves at any time durring the turn without anyone losing part of a
   turn. */

read_unit_moved(unit, fp)
Unit *unit;
FILE *fp;
{
    fscanf(fp, "%hd %hd \n ", &(unit->movesleft), &(unit->actualmoves));
}

/* The exact way to read an order depends on what type it is. */

read_one_order(fp, order)
FILE *fp;
Order *order;
{ 
    fscanf(fp, "%hd %hd %hd", &(order->type), &(order->rept), &(order->flags));
    order->morder = order->type / 100;
    order->type = order->type % 100;
    switch (orderargs[order->type]) {
    case NOARG:
	break;
    case DIR:
	fscanf(fp, "%hd", &(order->p.dir));
	break;
    case POS:
	fscanf(fp, "%hd,%hd", &(order->p.pt[0].x), &(order->p.pt[0].y));
	break;
    case EDGE_A:
	fscanf(fp, "%hd/%hd", &(order->p.edge.forward),
	       &(order->p.edge.forward));
	break;
    case LEADER:
	fscanf(fp, "%d", &order->p.leader_id);
	break;
    case WAYPOINTS:
	fscanf(fp, "%hd,%hd %hd,%hd",
	       &(order->p.pt[0].x), &(order->p.pt[0].y),
	       &(order->p.pt[1].x), &(order->p.pt[1].y));
	break;
/*** (UK) insert -> ***/
    case STRING:
	fscanf(fp,"%s",order->p.string);
	unprep_string(order->p.string);
	break;
    case AMOUNT:
	fscanf(fp,"%d",&order->p.amount);
	break;
/*** <- insert ***/
    default:
	case_panic("order arg type", orderargs[order->type]);
	break;
    }
}

/* Output is quite similar to input of mapfiles, but of course everything */
/* is reversed.  */

/* A savefile has a particular format; it includes all sections except */
/* period, which may or may not be referenced as an included file. */
/* It is important that this routine not attempt to use graphics, since it */
/* may be called when graphics code fails. */

write_savefile(fname)
char *fname;
{
    Header *head;
    char * savedir;
    char savefname[1023];
    enter_procedure("write_savefile");

    if((savedir = getenv("XCONQSAVEDIR")) == NULL) 
	if((savedir= getenv("HOME")) == NULL) savedir = "";
    strcpy(savefname, savedir);
    strcat(savefname, "/");
    strcat(savefname, fname);

    head = (Header *) malloc(sizeof(Header));
    head->version = 1;
    head->numincls = (perfilename ? 1 : 0);
    head->includes[0] = perfilename;
    head->sections = "+-++++";
    head->numnotes = 0;
    head->sdetail = SIDEALL;
    head->udetail = UNITALL;
    exit_procedure();
    if (write_mapfile(savefname, head) == FALSE) {
	if (!strcmp(fname, SAVEFILE2)) return(FALSE);
	return(write_mapfile(SAVEFILE2, head));
    }
}

/* A scenario is considerably more variable than a savefile, but the */
/* principle is the same. */

write_scenario(fname, sections, sdetail, udetail)
char *fname, *sections;
int sdetail, udetail;
{
    int i;
    Header *head;

    enter_procedure("write_scenario");
    head = (Header *) malloc(sizeof(Header));
    head->numincls = head->numnotes = 0;
    if (mainhead) {
	head->numnotes = mainhead->numnotes;
	for (i = 0; i < head->numnotes; ++i) {
	    head->notes[i] = mainhead->notes[i];
	}
	head->numincls = mainhead->numincls;
	for (i = 0; i < head->numincls; ++i) {
	    head->includes[i] = mainhead->includes[i];
	}
    }
    head->sections = copy_string(sections);
    head->sdetail = sdetail;
    head->udetail = udetail;
    exit_procedure();
    return write_mapfile(fname, head);
}

/* Given a file name and a header telling what to put in, do the putting. */
/* Returns true if file opened and was written successfully. */

write_mapfile(fname, head)
char *fname;
Header *head;
{
    int i;
    FILE *fp;

    enter_procedure("write_mapfile");

    if ((fp = fopen(fname, "w")) != NULL) {
        switch (head->version) {
           case 0:
                fprintf(fp, "Xconq %d %s %s%s\n",
                        head->numincls, head->sections,
                        (head->numnotes > 0 ? head->notes[0] : ""),
                        (head->numnotes > 1 ? ";" : ""));
                break;
           default:
                fprintf(fp, "%d-Xconq %d %s %s%s\n",
                        head->version,
                        head->numincls, head->sections,
                        (head->numnotes > 0 ? head->notes[0] : ""),
                        (head->numnotes > 1 ? ";" : ""));
                break;
        }
        format_version = head->version;
	if (head->numnotes > 1) {
	    for (i = 1; i < head->numnotes; ++i) {
		fprintf(fp, "%s\n", head->notes[i]);
	    }
	    fprintf(fp, ".\n");
	}
	for (i = 0; i < head->numincls; ++i) {
	    fprintf(fp, "%s\n", head->includes[i]);
	}
	if (vsect(head->sections)) write_version(fp);
	/* no period writing */
	if (msect(head->sections)) write_map(fp);
	if (gsect(head->sections)) write_globals(fp);
	if (ssect(head->sections)) write_sides(fp, head->sdetail);
	if (usect(head->sections)) write_units(fp, head->udetail);
	i = fclose(fp);
	exit_procedure();
	return (i != EOF);
    } else {
        exit_procedure();
	return FALSE;
    }
}

/* Writing out the version is pretty easy. */

write_version(fp)
FILE *fp;
{
    fprintf(fp, "%s\n", version);
}

/* Write the map section.  Try to find runs of the same type and make a more */
/* compact output. */

write_map(fp)
FILE *fp;
{
    int extension = 0, x, y, run, runterr, terr, i;

    enter_procedure("write_map");
    fprintf(fp, "Map %d %d %d %d %d\n",
	    world.width, world.height, 0, world.known, extension);
    for (y = world.height-1; y >= 0; --y) {
	run = 0;
	runterr = terrain_at(0, y);
	for (x = 0; x < world.width; ++x) {
	    terr = terrain_at(x, y);
	    if (terr == runterr) {
		run++;
	    } else {
		if (run >= 3) {
		    fprintf(fp, "%d%c", run, ttypes[runterr].tchar);
		} else {
		    for (i = 0; i < run; ++i) putc(ttypes[runterr].tchar, fp);
		}
		runterr = terr;
		run = 1;
	    }
	}
	fprintf(fp, "%d%c\n", run, ttypes[terr].tchar);
    }
/*** (UK) insert -> ***/
    exit_procedure();
/*** <- insert ***/
}

/* Write the globals section, which mostly consists of win/lose conditions. */

write_globals(fp)
FILE *fp;
{
    int extension = 0, i, u, r;
    Condition *cond;

/*** (UK) change -> ***/
    fprintf(fp, "Globals %d %d %d %d %d %u %d\n",
	    global.time, global.endtime, global.setproduct,
	    global.leavemap, global.numconds, global.giventime, extension);
/*** was:
    fprintf(fp, "Globals %d %d %d %d %d %d\n",
	    global.time, global.endtime, global.setproduct,
	    global.leavemap, global.numconds, extension, global.giventime);
*** <- change ***/
    for (i = 0; i < global.numconds; ++i) {
	cond = &(global.winlose[i]);
	fprintf(fp, "%d %d %d %d %d\n",
		cond->win, cond->type,
		cond->starttime, cond->endtime, cond->sidesn);
	switch (cond->type) {
	case TERRITORY:
	    fprintf(fp, "%d\n", cond->n);
	    break;
	case UNITCOUNT:
	    for_all_unit_types(u) {
		fprintf(fp, "%d ", cond->units[u]);
	    }
	    fprintf(fp, "\n");
	    break;
	case RESOURCECOUNT:
	    for_all_resource_types(r) {
		fprintf(fp, "%d ", cond->resources[r]);
	    }
	    fprintf(fp, "\n");
	    break;
	case POSSESSION:
	    fprintf(fp, "%d %d %d\n", cond->x, cond->y, cond->n);
	    break;
	default:
	    case_panic("condition type", cond->type);
	    break;
	}
    }
}

/* Write the sides section, at the given level of detail. */

write_sides(fp, detail)
FILE *fp;
int detail;
{
    int extension = 0;
    Side *side;

    fprintf(fp, "Sides %d %d %d\n", numsides, detail, extension);
    if (detail > SIDEALL) detail = SIDEALL;
    if (detail < 0 || detail > SIDEALL) case_panic("detail", detail);
    if (Debug) printf("Will try to write %d sides at detail %d ...\n",
		      numsides, detail);
    for_all_sides(side) {
	if (detail >= SIDEBASIC) write_basic_side(side, fp);
	if (detail >= SIDESLOTS) write_side_attributes(side, fp);
	if (detail >= SIDEVIEW)  write_side_view(side, fp);
	if (detail >= SIDEMISC) {
	    write_side_misc(side, fp);
	    write_side_statistics(side, fp);
	}
	if (Debug) printf("Wrote side \"%s\",\n", side->name);
    }
    if (Debug) printf("... Done writing sides.\n");
}

/* A side can be saved with or without the entire view, which is a fairly */
/* large sort of thing.  Machine sides without displays have their names */
/* written as "*" - let us hope and pray that such a perverted name never */
/* appears as a valid X host/display name. */

write_basic_side(side, fp)
Side *side;
FILE *fp;
{
    int j;

    strcpy(tmpbuf, side->name);
/*** (UK) change -> ***/
    prep_string(tmpbuf);
/*** was:
    for (j = 0; j < strlen(tmpbuf); ++j) if (tmpbuf[j] == ' ') tmpbuf[j] = '*';
*** <- change ***/ 
    fprintf(fp, "%s\n", tmpbuf);
}

/* Write the random but important attributes of a side. */

write_side_attributes(side, fp)
Side *side;
FILE *fp;
{
    int u, s;

    fprintf(fp, "%d  ", side->ideology);
    for (s = 0; s < numsides; ++s) fprintf(fp, "%d ", side->attitude[s]);
    fprintf(fp, " ");
    for_all_unit_types(u) fprintf(fp, "%d ", side->counts[u]);
    fprintf(fp, "\n");
}

/* Write about what has been seen in the world. (should be more compact) */

write_side_view(side, fp)
Side *side;
FILE *fp;
{
    char ch1, ch2;
    int x, y, run;
    viewdata view, runview;

    for (y = 0; y < world.height; ++y) {
      runview = side_view(side, 0, y);
      run = 0;
      for (x = 0; x < world.width; ++x) {
	view = side_view(side, x, y);
	if (view == runview && (view == EMPTY || view == UNSEEN)) {
	  run++;
	} else {
	    if (runview == UNSEEN || runview == EMPTY) {
	      if (runview == UNSEEN)
		ch1 = '?';
	      else if (runview == EMPTY)
		ch1 = '.';
	      
	      fprintf(fp, "%c%d", ch1, run);
	    } else if (run > 0) {
	      ch1 = utypes[vtype(runview)].uchar;
	      ch2 = vside(runview) + '0';
	      putc(ch1, fp);  putc(ch2, fp);
	    }
	    runview = view;
	    run = 1;
	  }
      }
      if (runview == UNSEEN || runview == EMPTY) {
	if (view == UNSEEN)
	  ch1 = '?';
	else if (view == EMPTY)
	  ch1 = '.';
	
	fprintf(fp, "%c%d", ch1, run);
      } else {
	ch1 = utypes[vtype(view)].uchar;
	ch2 = vside(view) + '0';
	putc(ch1, fp);  putc(ch2, fp);
      }
      fprintf(fp, "\n");
    }
}

/* More volatile things, generally only of interest for saved games. */

write_side_misc(side, fp)
Side *side;
FILE *fp;
{
/*** (UK) change -> ***/
    int i;
    int extension=0;
    int state=1;
    int c=0;

    for (i=0; i<MAXSAVESTATE; i++) if (side->savstate[i].used) c++;
    if (c) extension|=1;

    if (side->passwd == NULL || strlen(side->passwd) < 1) {
	sprintf(tmpbuf, "*");
    } else {
	strcpy(tmpbuf, side->passwd);
	prep_string(tmpbuf);
    }
    fprintf(fp, "%d %s %s %d %u %d\n",
	    side->humanp, tmpbuf, (side->host ? side->host : "*"),
	    side->lost,
	    ((global.giventime > 0) ? side->timeleft : side->timetaken),
	    extension);

    if (c) {
      fprintf(fp, "%d %d\n",c,state); /* number of entries*/
      for (i=0; i<MAXSAVESTATE; i++) {
	if (side->savstate[i].used) {
	  fprintf(fp,"%d %d %d\n",
		       i,side->savstate[i].x,side->savstate[i].y);
	  /* only x and y up to now */
	}
      }
    }
/*** was:
    fprintf(fp, "%d %s %d %d \n",
	    side->humanp, (side->host ? side->host : "*"),
	    side->lost,
	    ((global.giventime > 0) ? side->timeleft : side->timetaken));
*** <- change ***/
}

/* Write the performance statistics of a side. (the unit record may be */
/* crucial to deciding win/lose conditions.) */

write_side_statistics(side, fp)
Side *side;
FILE *fp;
{
    int u, u2, i;

    for_all_unit_types(u) {
	for (i = 0; i < NUMREASONS; ++i) {
	    fprintf(fp, "%d ", side->balance[u][i]);
	}
	fprintf(fp, "\n");
	for_all_unit_types(u2) {
	    fprintf(fp, "%d ", side->atkstats[u][u2]);
	}
	fprintf(fp, "\n");
	for_all_unit_types(u2) {
	    fprintf(fp, "%d ", side->hitstats[u][u2]);
	}
	fprintf(fp, "\n");
    }
}

/* Write the unit section of a mapfile.  Each level of detail has its own */
/* line, while standing orders will take one line per unit type. */
/* Get rid of dead units and sort everything, so as to make sure that */
/* transports are always written before occupants. */

write_units(fp, detail)
FILE *fp;
int detail;
{
    int extension = 0;
    Unit *unit;
    int realnumunits = 0;
    Side *loop_side;

    enter_procedure("write_units");

    flush_dead_units();
    /*     sort_units(TRUE);  */
    if (detail > UNITALL) detail = UNITALL;
    if (detail < 0 || detail > UNITALL) case_panic("detail", detail);
    for_all_units(loop_side, unit) realnumunits++;
    if (realnumunits != numunits) {
      printf("Number of units incorrect %d vs %d, correcting\n",
	     realnumunits, numunits);
      numunits = realnumunits;
    }
    fprintf(fp, "Units %d %d %d\n", numunits, detail, extension);
    if (Debug) printf("Writing %d units at detail %d ...\n", numunits, detail);
    if (detail >= UNITBASIC) {
	for_all_units(loop_side, unit) {
	    if (detail >= UNITBASIC) write_basic_unit(unit, fp);
	    if (detail >= UNITSLOTS) write_unit_attributes(unit, fp);
	    if (detail >= UNITORDERS) write_unit_orders(unit, fp);
	    if (detail >= UNITMOVED) write_unit_moved(unit, fp);
	    if (Debug) printf("Wrote %s,\n", unit_handle((Side *)NULL, unit));
	}
    }
    if (Debug) printf("... Done writing units.\n");
/*** (UK) insert -> ***/
    exit_procedure();
/*** <- insert ***/
}

/* Write only the minimal info about a unit - type, name, and position. */
/* To make scanf happy, spaces in the name are replaced with stars. */
/* Fortunately, names never seem to have stars in them. */
/* Unnamed units get a star all by itself in the name position. */

write_basic_unit(unit, fp)
Unit *unit;
FILE *fp;
{
    int j;

    if (unit->name == NULL || strlen(unit->name) < 1) {
	sprintf(tmpbuf, "*");
    } else {
	strcpy(tmpbuf, unit->name);
/*** (UK) change -> ***/
	prep_string(tmpbuf);
/*** was:
	for (j = 0; j < strlen(tmpbuf); ++j)
	    if (tmpbuf[j] == ' ') tmpbuf[j] = '*';
*** <- change ***/ 
    }
/*** (UK) change -> ***/
    if (unit->group == NULL || strlen(unit->group) < 1) {
	sprintf(tmpbuf2, "*");
    } else {
	strcpy(tmpbuf2, unit->group);
	prep_string(tmpbuf2);
    }
    fprintf(fp, "%c %s %s %d,%d %d\n",
	    utypes[unit->type].uchar, tmpbuf, tmpbuf2, unit->x, unit->y,
	    side_number(unit->side));
/*** was:
    fprintf(fp, "%c %s %d,%d %d\n",
	    utypes[unit->type].uchar, tmpbuf, unit->x, unit->y,
	    side_number(unit->side));
*** <- change ***/ 
}

/* Write the most interesting attributes of a unit.  Just a long list of */
/* numbers, no special tricks needed. */

write_unit_attributes(unit, fp)
Unit *unit;
FILE *fp;
{
    int i;

    fprintf(fp, "%d %d %d  %d  %d %d %d  %d %d %d  %d  ",
	    unit->id, unit->number, side_number(unit->trueside), 
	    unit->hp, unit->quality, 0, 0,
	    unit->product+256*unit->next_product, unit->schedule, unit->built,
	    (unit->transport ? unit->transport->id : -1));
    switch (format_version) {
       case 1:
          fprintf(fp, "%hd %hd %hd  ",
             unit->terraform, unit->tschedule, unit->last_terraform);
          break;
    }
    for_all_resource_types(i) {
	fprintf(fp, "%d ", unit->supply[i]);
    }
    fprintf(fp, "\n");
}

/* Write the unit's orders and standing orders out. */
/* This is usually for game saving, although I suppose it has other uses. */

/*** (UK) insert -> ***/
write_order_entries(fp,oe)
FILE *fp;
StandingOrderEntry *oe;
{ while (oe) {
    write_order_entry(fp,oe);
    oe=oe->next;
  }
}

write_order_entry(fp,oe)
FILE *fp;
StandingOrderEntry *oe;
{   int n=0;
    StandingConditionEntry *ce;

    if (oe->group == NULL || strlen(oe->group) < 1) {
	sprintf(tmpbuf2, "*");
    } else {
	strcpy(tmpbuf2, oe->group);
	prep_string(tmpbuf2);
    }

    ce=oe->cond;
    while (ce) {
      if (ce->order) n++;
      ce=ce->next;
    }
    fprintf(fp, "  %s %d %d\n",tmpbuf2,n,oe->next!=NULL);

    ce=oe->cond;
    while (ce) {
      if (ce->order) {
	write_condition(fp,&ce->cond);
	write_one_order(fp, ce->order);
	fprintf(fp, "\n");
      }
      ce=ce->next;
    }
}

write_condition(fp,c)
FILE *fp;
SCondition *c;
{ int u;

  fprintf(fp,"  %d %d",c->type,c->priority);
  switch (c->type) {
    case C_FULL:
    case C_NFULL:
	       for_all_unit_types(u) {
		 fprintf(fp," %d",c->d.utypes[u]);
	       }
	       break;
    case C_GT:
    case C_LE: fprintf(fp," %d",c->d.amount);
	       break;
    default:   break;
  }
  fprintf(fp,"   ");
}

/*** <- insert ***/ 

write_unit_orders(unit, fp)
Unit *unit;
FILE *fp;
{
    int i;

    enter_procedure("write_unit orders");
    fprintf(fp, "%d  %d ", unit->lastdir, unit->awake);
    write_one_order(fp, &(unit->orders));
    fprintf(fp, " %d\n", (unit->standing != NULL));
    if (unit->standing != NULL) {
/*** (UK) change -> ***/
        StandingOrderEntry *oe;
	int state;
	for_all_unit_types(i) {
	    state=(unit->standing->p_orders[i]!=NULL);
	    state|=(unit->standing->e_orders[i]!=NULL)<<1;
	    state|=(unit->standing->f_orders[i]!=NULL)<<2;
	    fprintf(fp, "%d ", state);
	}
	fprintf(fp, "\n");
	for_all_unit_types(i) {
	    if (unit->standing->p_orders[i] != NULL) {
		write_order_entries(fp, unit->standing->p_orders[i]);
	    }
	    if (unit->standing->e_orders[i] != NULL) {
		write_order_entries(fp, unit->standing->e_orders[i]);
	    }
	    if (unit->standing->f_orders[i] != NULL) {
		write_order_entries(fp, unit->standing->f_orders[i]);
	    }
	}
/*** was:
	for_all_unit_types(i) {
	    fprintf(fp, "%d ", (unit->standing->orders[i] != NULL));
	}
	fprintf(fp, "\n");
	for_all_unit_types(i) {
	    if (unit->standing->orders[i] != NULL) {
		write_one_order(fp, unit->standing->orders[i]);
		fprintf(fp, "\n");
	    }
	}
*** <- change ***/
    }
    exit_procedure();
}

/* Write how much of the units movement is available.  This will allow
   saves at any time durring the turn without anyone losing part of a
   turn. */

write_unit_moved(unit, fp)
Unit *unit;
FILE *fp;
{
    fprintf(fp, "%d %d \n ", unit->movesleft, unit->actualmoves);
}

/* Write a single order object, which may be for a unit or a standing order. */

write_one_order(fp, order)
FILE *fp;
Order *order;
{
    fprintf(fp, " %d %d %d ", (order->type + 100 * order->morder), order->rept, order->flags);
    switch (orderargs[order->type]) {
    case NOARG:
	break;
    case DIR:
	fprintf(fp, "%d", order->p.dir);
	break;
    case POS:
	fprintf(fp, "%d,%d", order->p.pt[0].x, order->p.pt[0].y);
	break;
    case EDGE_A:
	fprintf(fp, "%d/%d", order->p.edge.forward, order->p.edge.ccw);
	break;
    case LEADER:
	fprintf(fp, "%d", order->p.leader_id);
	break;
    case WAYPOINTS:
	fprintf(fp, "%d,%d %d,%d",
		order->p.pt[0].x, order->p.pt[0].y,
		order->p.pt[1].x, order->p.pt[1].y);
	break;
/*** (UK) insert -> ***/
    case STRING:
	strcpy(tmpbuf2,order->p.string);
	prep_string(tmpbuf2);
	fprintf(fp,"%s",tmpbuf2);
	break;
    case AMOUNT:
	fprintf(fp,"%d",order->p.amount);
        break;
/*** <- insert ***/
    default:
	case_panic("order arg type", orderargs[order->type]);
	break;
    }
}

/* Display the mapfile header info. */

describe_mapfiles(side)
Side *side;
{
    int i;

    wprintf(side, "The world is %d hexes around by %d hexes high.",
	    world.width, world.height);
    wprintf(side, "");
    if (mainhead != NULL && mainhead->numnotes > 0) {
	for (i = 0; i < mainhead->numnotes; ++i) {
	    wprintf(side, "%s", mainhead->notes[i]);
	}
    } else {
	wprintf(side, "(No other documentation is available.)");
    }
}

/* Generalized area search routine.  It starts in the immediately adjacent */
/* hexes and expands outwards.  The basic structure is to examine successive */
/* "rings" out to the max distance; within each ring, we must scan each of */
/* six faces (picking a random one to start with) by iterating along that */
/* face, in a direction 120 degrees from the direction out to one corner of */
/* the face.  Draw a picture if you want to understand it... */

/* Incr is normally one.  It is set to area_size to search on areas
instead of hexes. */

/* Note that points far outside the map may be generated, but the predicate */
/* will not be called on them.  It may be applied to the same point several */
/* times, however. */

search_area(x0, y0, maxdist, pred, rxp, ryp, incr)
int x0, y0, maxdist, (*pred)(), *rxp, *ryp, incr;
{
    int clockwise, dist, dd, d, dir, x1, y1, i, dir2, x, y;

    maxdist = max(min(maxdist, world.width), min(maxdist, world.height));
    clockwise = (flip_coin() ? 1 : -1);
    for (dist = 1; dist <= maxdist; dist += incr) {
	dd = random_dir();
	for_all_directions(d) {
	    dir = (d + dd) % NUMDIRS;
	    x1 = x0 + dist * dirx[dir];
	    y1 = y0 + dist * diry[dir];
	    for (i = 0; i < dist; ++i) {
		dir2 = opposite_dir(dir + clockwise);
		x = x1 + i * dirx[dir2] * incr;
		y = y1 + i * diry[dir2] * incr;
		if (between(0, y, world.height-1)) {
		    if ((*pred)(wrap(x), y)) {
			*rxp = wrap(x);  *ryp = y;
			return TRUE;
		    }
		}
	    }
	}
    }
    return FALSE;
}

/* Apply a function to every hex within the given radius, being careful (for */
/* both safety and efficiency reasons) not to go past edges.  Note that the */
/* distance is inclusive, and that distance of 0 means x0,y0 only. */
/* This routine should be avoided in time-critical code. */

apply_to_area(x0, y0, dist, fn)
int x0, y0, dist, (*fn)();
{
    int x, y, x1, y1, x2, y2;

    dist = min(dist, max(world.width, world.height));
    y1 = y0 - dist;
    y2 = y0 + dist;
    for (y = y1; y <= y2; ++y) {
	if (between(0, y, world.height-1)) {
	    x1 = x0 - (y < y0 ? (y - y1) : dist);
	    x2 = x0 + (y > y0 ? (y2 - y) : dist);
	    for (x = x1; x <= x2; ++x) {
		((*fn)(wrap(x), y));
	    }
	}
    }
}
