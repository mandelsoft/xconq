/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This is a mini-tool that converts a period description file into a piece */
/* of C code that can be linked in as xconq's default period.  It uses the */
/* normal xconq period reader, and defines the output routine.  Works from */
/* stdio, only option is to turn on debugging if any args present. */
/* The code is simple but lengthy. */

#include "config.h"
#include "misc.h"
#include "period.h"

/* Declarations for variables set in the period file. */

Period period;

Utype utypes[MAXUTYPES];

Rtype rtypes[MAXRTYPES];

Ttype ttypes[MAXTTYPES];

char *snames[MAXSNAMES];

char *unames[MAXUNAMES];

int Debug = FALSE;

/* Very simple main program. We do need to soak up the first line of the *
/* period description, which is almost certainly a mapfile header. */
/* Any argument will enable debugging prints in the period reader. */

main(argc, argv)
int argc;
char *argv[];
{
    char dummy[BUFSIZE];

    if (argc > 1) Debug = TRUE;
    fgets(dummy, BUFSIZE-1, stdin);
    read_period(stdin);
    print_period();
    print_utypes();
    print_rtypes();
    print_ttypes();
    print_snames();
    print_unames();
    exit(0);
}

print_period()
{
    printf("#include \"config.h\"\n");
    printf("#include \"period.h\"\n");
    printf("\nPeriod period = { \"%s\", NULL, \"%s\", %d, %d, %d,\n",
	   period.name, /* period.notes */ period.fontname, 
	   period.countrysize, period.mindistance, period.maxdistance);
    printf(" %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d,\n",
	   period.numutypes, period.numrtypes, period.numttypes,
	   period.numsnames, period.numunames,
	   period.firstutype, period.firstptype,
	   period.knownradius, period.allseen,
	   period.counterattack, period.capturemoves, period.nukehit, period.neutrality,
	   period.efficiency);
    printf(" %d, %d, %d, %d, %d, %d, %d, %d, %d };\n",
	   period.altroughness, period.wetroughness, period.machineadvantage,
	   period.defaultterrain, period.edgeterrain,
	   period.spychance, period.spyquality,
	   period.leavemap, period.repairscale);
}

print_utypes()
{
    int u, t, u2, r;

    printf("\nUtype utypes[MAXUTYPES] = {\n");
    for_all_unit_types(u) {
	printf("{ '%c', \"%s\", \"%s\", NULL, \"%s\",\n",
	       utypes[u].uchar, utypes[u].name, utypes[u].help, /* notes, */
	       (utypes[u].bitmapname ? utypes[u].bitmapname : ""));
	printf("  %d, %d, %d, %d,\n",
	       utypes[u].incountry, utypes[u].density,
	       utypes[u].named, utypes[u].alreadyseen);
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].favored[t]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].stockpile[r]);
	printf(" },\n");
	printf("  %d, %d, %d, %d, \"%s\", \"%s\",\n",
	       utypes[u].revolt, utypes[u].surrender, utypes[u].siege,
	       utypes[u].attdamage, utypes[u].attritionmsg,
	       utypes[u].accidentmsg);
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].attrition[t]);
	printf(" },\n");
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].accident[t]);
	printf(" },\n");
	printf("  %d, %d, %d, %d, \n",
	       utypes[u].maker, utypes[u].occproduce, utypes[u].startup, utypes[u].research);
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].research_contrib[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].make[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].tomake[r]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].repair[u2]);
	printf(" },\n");
	printf("  %d, \"%s\",\n", utypes[u].survival, utypes[u].starvemsg);
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].produce[r]);
	printf(" },\n");
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].productivity[t]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].storage[r]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].consume[r]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].inlength[r]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].outlength[r]);
	printf(" },\n");
	printf(" %d, \n", utypes[u].consume_as_occupant);
	printf("  %d,\n",
	       utypes[u].speed);
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].moves[t]);
	printf(" },\n");
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].randommove[t]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].tomove[r]);
	printf(" },\n");
	printf("  %d, %d,\n", utypes[u].volume, utypes[u].holdvolume);
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].capacity[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].entertime[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].leavetime[u2]);
	printf(" },\n");
/*** (UK) -> insert ***/
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].freemove[u2]);
	printf(" },\n");
/*** <- insert ***/
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].bridge[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].mobility[u2]);
	printf(" },\n");
	printf("  %d, %d, %d, %d, %d,\n",
	       utypes[u].seealways, utypes[u].seebest, utypes[u].seeworst,
	       utypes[u].seerange, utypes[u].visibility);
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].conceal[t]);
	printf(" },\n");
/*** (UK) change -> ***/
	printf(" %d, %d, %d, %d, %d, %d, %d, %d, %d, \"%s\",\n",
/*** was:
	printf(" %d, %d, %d, %d, %d, %d, %d, %d, \"%s\",\n",
*** <- change ***/
	       utypes[u].hp, utypes[u].crippled,
	       utypes[u].selfdestruct, utypes[u].changeside,
	       utypes[u].hittime, utypes[u].retreat,
/*** (UK) change -> ***/
	       utypes[u].counterable, utypes[u].cancounter,
	       utypes[u].arearadius,
/*** was:
	       utypes[u].counterable, utypes[u].arearadius,
*** <- change ***/
	       utypes[u].destroymsg);
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].hit[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_terrain_types(t) printf("%d, ", utypes[u].defense[t]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].damage[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].hitswith[r]);
	printf(" },\n");
	printf("  { ");
	for_all_resource_types(r) printf("%d, ", utypes[u].hitby[r]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].capture[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].guard[u2]);
	printf(" },\n");
	printf("  { ");
	for_all_unit_types(u2) printf("%d, ", utypes[u].protect[u2]);
	printf(" },\n");
	printf("  %d, %d, %d, %d, %d, %d,\n",
	       utypes[u].territory, utypes[u].isneutral, 
	       utypes[u].maxquality, utypes[u].skillf, utypes[u].disciplinef,
	       utypes[u].disband);
	printf("  %d, %d, %d, %d,\n",
	       utypes[u].attack_worth, utypes[u].defense_worth,
	       utypes[u].explore_worth, utypes[u].min_region_size);
	printf("  %d, %d, %d, %d \n",
	       utypes[u].is_base, utypes[u].is_base_builder,
	       utypes[u].is_transport, utypes[u].is_carrier);
	printf(" },\n");
    }
    printf("};\n");
}

print_rtypes()
{
    int r;

    if (period.numrtypes > 0) {
	printf("\nRtype rtypes[MAXRTYPES] = {\n");
	for_all_resource_types(r) {
	    printf("{ '%c', \"%s\", \"%s\" },\n",
		   rtypes[r].rchar, rtypes[r].name, rtypes[r].help);
	}
	printf("};\n");
    } else {
	printf("\nRtype rtypes[MAXRTYPES];\n");
    }
}

print_ttypes()
{
    int t;

    printf("\nTtype ttypes[MAXTTYPES] = {\n");
    for_all_terrain_types(t) {
	printf("{ '%c', \"%s\", \"%s\",\n",
	       ttypes[t].tchar, ttypes[t].name, ttypes[t].color);
	printf("  %d, %d, %d, %d, %d, %d },\n",
	       ttypes[t].dark, ttypes[t].nuked,
	       ttypes[t].minalt, ttypes[t].maxalt,
	       ttypes[t].minwet, ttypes[t].maxwet);
    }
    printf("};\n");
}

print_snames()
{
    int i;

    if (period.numsnames > 0) {
	printf("\nchar *snames[MAXSNAMES] = {");
	for (i = 0; i < period.numsnames; ++i) {
	    if ((i % 5) == 0) printf("\n");
	    printf("\"%s\", ", snames[i]);
	}
	printf("};\n");
    }
}

print_unames()
{
    int i;

    if (period.numunames > 0) {
	printf("\nchar *unames[MAXUNAMES] = {");
	for (i = 0; i < period.numunames; ++i) {
	    if ((i % 5) == 0) printf("\n");
	    printf("\"%s\", ", unames[i]);
	}
	printf("};\n");
    }
}

/* Read a line and save it away.  This routine should be used sparingly, */
/* since the malloced space is never freed. */

char *
read_line(fp)
FILE *fp;
{
    char tmp[BUFSIZE], *line;

    fgets(tmp, BUFSIZE-1, fp); tmp[strlen(tmp)-1] = '\0';
    line = (char *) malloc(strlen(tmp)+2);
    strcpy(line, tmp);
    return line;
}

char *
copy_string(str)
char *str;
{
    char *rslt;

    rslt = (char *) malloc(strlen(str)+1);
    strcpy(rslt, str);
    return rslt;
}

/* This little routine goes at the end of all case statements on internal */
/* data that shouldn't go out of bounds.  We want a core dump to debug. */

case_panic(str, var)
char *str;
int var;
{
    fprintf(stderr, "Panic! Unknown %s %d\n", str, var);
    abort();
}
