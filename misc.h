/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Random definitions useful for nearly any C program. */

/*
typedef unsigned char unchar;
*/
typedef unsigned short unshort;

/* If some system tries to redefine bool, we're screwed. */

#define bool int

/* Some systems define TRUE and FALSE themselves. */

#ifndef TRUE
#define TRUE (1)
#endif
#ifndef FALSE
#define FALSE (0)
#endif

#define abs(x) (((x) < 0) ? (0 - (x)) : (x))

#define min(x,y) (((x) < (y)) ? (x) : (y))

#define max(x,y) (((x) > (y)) ? (x) : (y))

#define between(lo,n,hi) ((lo) <= (n) && (n) <= (hi))

#define flip_coin() (random(257) % 2)

#define avg(a,b) (((a) + (b)) / 2)

#define iswhite(c) ((c) == ' ' || (c) == '\n' || (c) == '\t')

#define lowercase(c) (isupper(c) ? tolower(c) : (c))

#define uppercase(c) (islower(c) ? toupper(c) : (c))

/*** (UK) insert -> ***/
#define CANCELED (-1)           /* cancel for request */
#define SCANCELED ((char*)-1)   /* cancel for string request */
#define BACKSPACE '\010'        /* for fixing strings being entered */
#define ESCAPE    '\033'        /* standard abort character */
#define DELETE    '\177'        /* delete char */
/*** <- insert ***/
/* Miscellaneous declarations. */

extern bool Debug, Build, Freeze;
extern int Cheat;
extern bool No_give_r_and_d; /* XXX */
extern bool No_give_units; /* XXX */
extern bool Sequential; /* XXX */
extern int newturnsent;
extern char nextturnexecfile[BUFSIZE];

extern char spbuf[], tmpbuf[], version[];
/*** (UK) insert -> ***/
extern char tmpbuf2[];
/*** <- insert ***/ 
extern char *plural_form(), *copy_string(), *read_line();
extern char *xconqlib;

extern char *copy_string(), *read_line();
extern void init_random();
#define RANDOM(m) random(m)
void recenter();
#ifdef UNIX
extern char *malloc();
extern char *getenv();
#endif UNIX
extern int unwrap();

#ifndef DEBUG 

#define PROCSTACKSIZE 30
extern char *procedure_executing[PROCSTACKSIZE], *routine_executing;
extern int procedure_stack_ptr;
#define enter_procedure(x) {procedure_stack_ptr =\
 (procedure_stack_ptr + 1) % PROCSTACKSIZE; \
   procedure_executing[procedure_stack_ptr] = x;}
#define exit_procedure() {procedure_stack_ptr =\
(procedure_stack_ptr == 0 ? PROCSTACKSIZE - 1 : (procedure_stack_ptr - 1));}
#define cur_procedure() procedure_executing[procedure_stack_ptr]
#define routine(x) routine_executing = x

#else

#define enter_procedure(x)
#define exit_procedure()
#define cur_procedure()
#define routine(x)

#endif
