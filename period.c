/* Copyright (c) 1987, 1988, 1991  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* This file defines a mini-postfix language for use in specifying periods. */
/* The language is very restrictive, both lexically and semantically.  The */
/* only kinds of objects are integers, strings, symbols, and vectors, while */
/* the only way to get new words is to make new unit, resource, and terrain */
/* types, or to create names for constants.  There is a strong flavor of APL */
/* in the kinds of vector operations available. */

/* Stack hacking is a little dubious - we pop the stack *before* doing the */
/* word, but don't copy args.  Potential for disaster if word pushes */
/* results too soon... Should use pointers to objects also, for portability. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"

/* Cute little macro from the MG people.  It computes an offset of the slot */
/* "m" from the beginning of structure type "t". */

#define OFFSET(t,m) ((char *) &(((t *) 0)->m) - ((char *) ((t *) 0)))
#define VOFFSET(t,m) ((char *) (((t *) 0)->m) - ((char *) ((t *) 0)))

/* The "dictionary" is a symbol table, thus needs to be large enough to hold */
/* all pre-defined symbols, as well as those added during execution. */
/* There are about 200 predefined words. */

#define DICTSIZE 400

/* The stack doesn't have to be very deep, unless the period description is */
/* really perverted.  Depth should be at least MAXUTYPES+5 or so... */

#define STACKSIZE 50

/* An object has four possible types.  Numbers can only be integers, and */
/* vectors may only contain integers.  Strings are unlimited, while symbols */
/* mustn't overflow the dictionary. */

#define INT 0
#define SYM 1
#define STR 2
#define VEC 3

/* States of tokenizer (along with symbols just above). */

#define ENDTOKEN (-99)
#define NIL (-1)

/* Types of words (avoids using C hook functions usually). */

#define FN 0			/* arbitrary function */
#define U1 1                    /* put number into utype slot */
#define S1 2                    /* put string into utype slot */
#define U2 3                    /* put number(s) into utype array slot */
#define VV 5                    /* push value onto stack */
#define P0 6                    /* put number into period structure */
#define T1 7                    /* put number into terrain type slot */

/* Note that this declaration results in largish objects, but anything more */
/* clever is probably overkill.  */

/* MAXUTYPES must be larger than MAX[RT]TYPES (obscure). */

typedef struct a_obj {
    int type;          /* type of the object */
    int len;           /* length if it's a vector */
    union {
	int num;       /* numeric value */
	char *str;     /* string value */
	int sym;       /* symbol value (index into dictionary) */
	short vec[MAXUTYPES];  /* vector value */
    } v;               /* value is pretty sizeable because of vector values */
} Obj;

/* The dictionary interfaces words to C functions. */

typedef struct dict {
    char *name;        /* a word of the language */
    int wtype;         /* type of word (1-d set, string set, etc) */
    int nargs;         /* how much to pull from the stack */
    union {
        int (*code)(); /* pointer to a C function */
        int offset;    /* for offsets into structures */
        Obj value;     /* a "default" value used sometimes */
    } f;
} Dict;

Dict words[DICTSIZE];  /* the dictionary itself */

Obj stack[STACKSIZE];  /* the evaluation stack */
Obj curobj;           /* the current word (used in weird way) */

FILE *pfp;             /* period file pointer */

bool mustbenew = TRUE;  /* flag for an efficiency hack during init */

int numwords;          /* number of words presently in dictionary */
int top = 0;           /* index to the top of the stack */

int push_value();      /* predeclare to cover some forward refs */

/* Read a period section of an already-opened mapfile. */
/* The debug prints are very important for debugging period files - they */
/* provide at least a simple test that things are being read correctly. */
/* There are also a number of consistency checks, though not all possible, */
/* and nothing is done to test play balance for the period :-). */

read_period(fp)
FILE *fp;
{
    int extension, endsym, si;

    pfp = fp;
    fscanf(pfp, "Period %d\n", &extension);
    if (Debug) printf("Trying to read period ...\n");
    init_words();
    endsym = lookup_symbol("end");
    if (extension == 0) {
	clear_period();
    }
    while (get_token(pfp)) {
	if (Debug) {
	    print_object(curobj, FALSE);
	}
	if (curobj.type == SYM) {
	    si = curobj.v.sym;
	    if (si == endsym) break;
	    if (top < words[si].nargs) {
		fprintf(stderr, "Stack underflow!\n");
		exit(1);
	    }
	    top -= words[si].nargs;
	    switch (words[si].wtype) {
	    case FN:
		execute_word(words[si].f.code, words[si].nargs);
		break;
	    case P0:
		set_0d_n(stack[top], words[si].f.offset);
		break;
	    case U1:
		set_1d_n(stack[top], stack[top+1], words[si].f.offset);
		break;
	    case S1:
		set_1d_s(stack[top], stack[top+1], words[si].f.offset);
		break;
	    case U2:
		set_2d(stack[top], stack[top+1], stack[top+2],
		       words[si].f.offset);
		break;
	    case VV:
		push_value();
		break;
	    case T1:
		set_1d_t(stack[top], stack[top+1], words[si].f.offset);
		break;
	    default:
	        case_panic("word type", words[si].wtype);
		break;
	    }
	} else {
	    push_word();
	}
    }
    check_period_numbers();
    if (Debug) printf("... Done reading period.\n");
}

/* Execution is straightforward, but we need to know how many arguments each */
/* C function expects. */

execute_word(fn, args)
int (*fn)(), args;
{
    if (fn == NULL) abort();
    switch (args) {
    case 0:
	(*fn)();
	break;
    case 1:
	(*fn)(stack[top]);
	break;
    case 2:
	(*fn)(stack[top], stack[top+1]);
	break;
    case 3:
	(*fn)(stack[top], stack[top+1], stack[top+2]);
	break;
    case 4:
	(*fn)(stack[top], stack[top+1], stack[top+2], stack[top+3]);
	break;
    default:
        case_panic("argument", args);
	break;
    }
}

/* Alas, the tokenizer is too simple to bother with lex, and does too much */
/* to be a neat and short bit of code.  The states are NIL, INT, STR, SYM, */
/* and ENDTOKEN.  The process starts in NIL, and succeeds in the state */
/* ENDTOKEN, but fails (returning flag) if there is any sort of error. */
/* Basically each token type is flagged by its special character(s), and */
/* is terminated by whitespace (i.e. state machine is a bunch of looping */
/* states with only other transitions to and from NIL). */

get_token(fp)
FILE *fp;
{
    bool minus;
    char ch, str[BUFSIZE];
    int state = NIL, j = 0;

    while (state != ENDTOKEN) {
	ch = getc(fp);
	switch (ch) {
	case EOF:
	    return FALSE;
	case ';':
	    while ((ch = getc(fp)) != '\n' && ch != EOF);
	    break;
	default:
	    switch (state) {
	    case NIL:
		if (isdigit(ch) || ch == '-') {
		    minus = (ch == '-');
		    curobj.v.num = (minus ? 0 : ch - '0');
		    state = curobj.type = INT;
		} else if (iswhite(ch)) {
		    /* nothing to do here */
		} else if (ch == '"') {
		    state = curobj.type = STR;
		} else {
		    str[j++] = ch;
		    state = curobj.type = SYM;
		}
		break;
	    case INT:
		if (isdigit(ch)) {
		    curobj.v.num = curobj.v.num * 10 + ch - '0';
		} else {
		    if (minus) curobj.v.num = 0 - curobj.v.num;
		    state = ENDTOKEN;
		}
		break;
	    case STR:
		if (ch == '"') {
		    str[j] = '\0';
		    curobj.v.str = copy_string(str);
		    state = ENDTOKEN;
		} else {
		    str[j++] = ch;
		}
		break;
	    case SYM:
		if (iswhite(ch)) {
		    str[j] = '\0';
		    curobj.v.sym = lookup_symbol(str);
		    if (curobj.v.sym < 0) {
			fprintf(stderr,
				"Warning: undefined word \"%s\" in period\n",
				str);
			/* Don't get bent out of shape, keep going */
			curobj.type = INT;
			curobj.v.num = 0;
			/* Blast the stack because it might overflow */
			top = 0;
		    }
		    state = ENDTOKEN;
		} else {
		    str[j++] = ch;
		}
		break;
	    default:
		case_panic("token state", state);
		break;
	    }
	    break;
	}
    }
    return TRUE;
}

/* The dictionary starts out empty.  During initialization before reading, */
/* the basic set of predefined words come in.  Then units and resources get */
/* words later on as requested.  The "mustbenew" flag enables/disables redef */
/* checks.  Return address of new word. as a convenience. */

add_word(word, type, args, fn)
char *word;
int type, args;
int (*fn)();
{
    int sym;

    if (mustbenew && (sym = lookup_symbol(word)) > 0) {
	fprintf(stderr, "May not redefine \"%s\"!\n", word);
	return sym;
    }
    if (numwords < DICTSIZE) {
	words[numwords].name = word;
	words[numwords].wtype = type;
	words[numwords].nargs = args;
	words[numwords].f.code = fn;
	numwords++;
	return (numwords - 1);
    } else {
	fprintf(stderr, "Dictionary is full!\n");
	exit(1);
    }
}

/* Look up a symbol in the (not so small) dictionary.  DO NOT add it if not */
/* found - don't want any screwups to be compounded by feeble DWIM efforts. */

lookup_symbol(str)
char *str;
{
    int i;

    for (i = 0; i < numwords; ++i) {
	if (strcmp(words[i].name, str) == 0) return i;
    }
    return (-1);
}

/* Put the current word on the stack, checking for overflow first. */

push_word()
{
    if (top < STACKSIZE) {
	stack[top++] = curobj;
    } else {
	fprintf(stderr, "Stack overflow!\n");
	exit(1);
    }
}

/* Push the "value" slot of the word being executed onto the stack. */
/* Used by all names and characters of units and resources - necessary */
/* because we can't define individual distinct C codes for each type. */

push_value()
{
    if (top < STACKSIZE) {
	stack[top++] = words[curobj.v.sym].f.value;
    } else {
	print_stack();
	fprintf(stderr, "Stack overflow!\n");
	exit(1);
    }
}

/* Generic number pusher. */

push_number(n)
int n;
{
    if (top < STACKSIZE) {
	stack[top].type = INT;
	stack[top].v.num = n;
	++top;
    } else {
	fprintf(stderr, "Stack overflow!\n");
	exit(1);
    }
}

/* Fill in some very basic numbers at the outset. This is essentially a */
/* "clear" operation, rampaging straight through the three period structures */
/* then filling in a few numbers that *must* have reasonable values for the */
/* program to work.  A couple are convenience values, such as "nukehit". */

clear_period()
{
    char *base;
    int i, u, u2, r, t, tmp1, tmp2;

    tmp1 = period.numsnames;  tmp2 = period.numunames;
    base = (char *) &period;
    for (i = 0; i < sizeof(Period); ++i) base[i] = '\0';
    base = (char *) utypes;
    for (i = 0; i < sizeof(Utype) * MAXUTYPES; ++i) base[i] = '\0';
    base = (char *) rtypes;
    for (i = 0; i < sizeof(Rtype) * MAXRTYPES; ++i) base[i] = '\0';
    base = (char *) ttypes;
    for (i = 0; i < sizeof(Ttype) * MAXTTYPES; ++i) base[i] = '\0';
    period.numsnames = tmp1;  period.numunames = tmp2;
    period.name = "unspecified";
    period.fontname = "";
    period.nukehit = 50;
    period.countrysize = 3;
    period.mindistance = 7;
    period.maxdistance = 60;
    period.firstutype = period.firstptype = NOTHING;
    period.counterattack = TRUE;
    period.capturemoves = TRUE;
    period.defaultterrain = period.edgeterrain = -1;
    period.altroughness = 80;  period.wetroughness = 70;
    period.machineadvantage = FALSE;
    period.spychance = 1;  period.spyquality = 50;
    period.leavemap = FALSE;
    period.repairscale = 1;
    for (u = 0; u < MAXUTYPES; ++u) {
	utypes[u].attdamage = 1;
	utypes[u].attritionmsg = "suffers attrition";
	utypes[u].accidentmsg = "has an accident";
	utypes[u].starvemsg = "runs out of supplies and dies";
	utypes[u].destroymsg = "destroys";
	utypes[u].visibility = 100;
	utypes[u].seebest  = 100;
	utypes[u].seeworst = 100;
	utypes[u].seerange = 1;
	utypes[u].hp = 1;
	utypes[u].maxquality = 0;
	utypes[u].skillf = 1;
	utypes[u].disciplinef = 1;
	utypes[u].counterable = TRUE;
	utypes[u].consume_as_occupant = TRUE;
/*** (UK) insert -> ***/
	utypes[u].counterable = TRUE;
/*** <- insert ***/
	for (u2 = 0; u2 < MAXUTYPES; ++u2) {
	    utypes[u].entertime[u2] = 1;
	    utypes[u].mobility[u2] = 100;
/*** (UK) insert -> ***/
	    utypes[u].freemove[u2] = TRUE;
	    utypes[u].transportseerange[u2] = -1;
/*** <- insert ***/
	}
	for (r = 0; r < MAXRTYPES; ++r) {
	    utypes[u].stockpile[r] = 100;
	}
	for (t = 0; t < MAXTTYPES; ++t) {
	    utypes[u].moves[t] = -1;
/*** (UK) insert -> ***/
	    utypes[u].terraform[r] = 0;
/*** <- insert ***/
	}
    }
    for (t = 0; t < MAXTTYPES; ++t) {
	ttypes[t].minalt = ttypes[t].minwet = 0;
	ttypes[t].maxalt = ttypes[t].maxwet = 100;
    }
}

/* Random side and unit names must be cleared separately by explicit cmd, */
/* since periods don't always want to define their own. */

clear_side_names() {  period.numsnames = 0;  }

clear_unit_names() {  period.numunames = 0;  }

/* Words that set the contents of arrays are polymorphic in that they can */
/* do array-like assignments as well as single value assignment. */

set_0d_n(n, offset)
Obj n;
int offset;
{
    *((short *) ((char *) ((char *) &period) + offset)) = n.v.num;
}

/* No string vectors, so all we can do is to assign the same string to */
/* one or more different array positions. */

#define s1ds(s,i) (*((char **)((char *)((char *)&(utypes[i])) + offset)) = s)

set_1d_s(s, i, offset)
Obj s, i;
int offset;
{
    int z;

    if (s.type == STR) {
	if (i.type == INT) {
	    s1ds(s.v.str, i.v.num);
	} else {
	    for (z = 0; z < i.len; ++z)	s1ds(s.v.str, i.v.vec[z]);
	}
    } else {
	arg_error_1d("1st arg must be string", s, i);
    }
}

/* One-dimensional arrays allow the combinations of scalar value & index SS, */
/* scalar value & vector index SV, and two vectors VV.  The 4th combo is not */
/* sensible. */

#define s1d(n,i) (*((short *)((char *)((char *)&(utypes[i])) + offset)) = n)

set_1d_n(n, i, offset)
Obj n, i;
int offset;
{
    int z;

    if (n.type == INT) {
	if (i.type == INT) {
	    s1d(n.v.num, i.v.num);
	} else {
	    for (z = 0; z < i.len; ++z)	s1d(n.v.num, i.v.vec[z]);
	}
    } else {
	if (i.type == INT) {
	    arg_error_1d("mismatched arguments", n, i);
	} else {
	    for (z = 0; z < n.len; ++z) s1d(n.v.vec[z], i.v.vec[z]);
	}
    }
}

/* This is identical to set_1d_n, but puts stuff into ttypes array instead. */

#define s1dt(n,i) (*((short *)((char *)((char *)&(ttypes[i])) + offset)) = n)

set_1d_t(n, i, offset)
Obj n, i;
int offset;
{
    int z;

    if (n.type == INT) {
	if (i.type == INT) {
	    s1dt(n.v.num, i.v.num);
	} else {
	    for (z = 0; z < i.len; ++z)	s1dt(n.v.num, i.v.vec[z]);
	}
    } else {
	if (i.type == INT) {
	    arg_error_1d("mismatched arguments", n, i);
	} else {
	    for (z = 0; z < n.len; ++z) s1dt(n.v.vec[z], i.v.vec[z]);
	}
    }
}

/* Dump the offending values and leave - any attempt to keep going will */
/* probably screw things up and maybe cause a core dump. */

arg_error_1d(msg, a, b)
char *msg;
Obj a, b;
{
    fprintf(stderr, "Error in array setting: %s\n", msg);
    print_object(a, TRUE);
    print_object(b, TRUE);
    exit(1);
}

/* Two-dimensional setting is more complicated.  VSS is not useful, and VVV */
/* can only set diagonals.  All others are useful. */

#define s2d(n,i,j) (*((short *)((char *)((char *)&(utypes[j]))+offset+2*i))=n)

set_2d(n, i1, i2, offset)
Obj n, i1, i2;
int offset;
{
    int z, zz;

    if (n.type == INT) {
	if (i1.type == INT) {
	    if (i2.type == INT) {
		s2d(n.v.num, i1.v.num, i2.v.num);
	    } else {
		for (z = 0; z < i2.len; ++z) {
		    s2d(n.v.num, i1.v.num, i2.v.vec[z]);
		}
	    }
	} else {
	    if (i2.type == INT) {
		for (z = 0; z < i1.len; ++z) {
		    s2d(n.v.num, i1.v.vec[z], i2.v.num);
		}
	    } else {
		for (z = 0; z < i1.len; ++z) {
		    for (zz = 0; zz < i2.len; ++zz) {
			s2d(n.v.num, i1.v.vec[z], i2.v.vec[zz]);
		    }
		}
	    }
	}
    } else {
	if (i1.type == INT) {
	    if (i2.type == INT) {
		arg_error_2d("mismatched argument types", n, i1, i2);
	    } else {
		if (n.len != i2.len)
		    arg_error_2d("mismatched vectors", n, i1, i2);
		for (z = 0; z < i2.len; ++z) {
		    s2d(n.v.vec[z], i1.v.num, i2.v.vec[z]);
		}
	    }
	} else {
	    if (i2.type == INT) {
		if (n.len != i1.len)
		    arg_error_2d("mismatched vectors", n, i1, i2);
		for (z = 0; z < i1.len; ++z) {
		    s2d(n.v.vec[z], i1.v.vec[z], i2.v.num);
		}
	    } else {
		//arg_error_2d("mismatched argument types", n, i1, i2);
		if (n.len != i1.len)
		    arg_error_2d("mismatched vectors", n, i1, i2);
		for (z = 0; z < i2.len; ++z) {
                  for (zz = 0; zz < i1.len; ++zz) {
                      s2d(n.v.vec[zz], i1.v.vec[zz], i2.v.vec[z]);
                  }
                }
	    }
	}
    }
}

/* Dump the offending values and die. */

arg_error_2d(msg, a, b, c)
char *msg;
Obj a, b, c;
{
    fprintf(stderr, "Error in array setting: %s\n", msg);
    print_object(a, TRUE);
    print_object(b, TRUE);
    print_object(c, TRUE);
    exit(1);
}

/* Fortunately there are no 3D arrays!! */

/* The dreary wasteland of implementations of words. */
/* Fortunately, most of them can use the multi-dimensional array hacking as */
/* defined already, but we have to have both a "setter" function and a */
/* "dictionary" function, so lots of short functions :-(. */

/* Build an arbitrary integer vector using [ ] words. */

start_vector()
{
    stack[top].type = STR;
    stack[top++].v.str = "start-vector";  /* check overflow? */
}

/* Vector creation is a little tricky, since we usually take a variable */
/* number of words from the stack.  The basic scheme scans the stack for */
/* for a non-integer or stack bottom, then builds a vector at that point. */

finish_vector()
{
    int i, bot, size;

    for (i = top-1; i >= 0; --i) {
	if (stack[i].type != INT) {
	    bot = i;
	    break;
	}
    }
    size = top - bot - 1;
    if (size <= MAXUTYPES) {
	stack[bot].type = VEC;
	stack[bot].len = size;
	for (i = 0; i < size; ++i) {
	    stack[bot].v.vec[i] = stack[bot+i+1].v.num;
	}
	top = bot + 1;
    } else {
	fprintf(stderr, "Vector too big!\n");
	exit(1);
    }
}

/* Assign a value to a variable, basically.  Actually, we're defining a new */
/* word and filling its value slot. */

set_define(a1, a2)
Obj a1, a2;
{
    words[add_word(a2.v.str, VV, 0, NULL)].f.value = a1;
}

push_true() { push_number(TRUE); }

push_false() { push_number(FALSE); }

push_nothing() { push_number(NOTHING); }

/* Things that don't fit in with all the "short" slots. */

set_periodname(a1) Obj a1; { period.name = a1.v.str; }

set_fontname(a1) Obj a1; { period.fontname = a1.v.str; }

/* Add a new type of unit.  All args must be strings, but after the defn, */
/* both char and full name may be used as normal words. */

define_utype(a1, a2, a3)
Obj a1, a2, a3;
{
    Obj obj;

    if (a1.v.str[0] == '.' || a1.v.str[0] == '?') {
      fprintf(stderr, "'.' and '?' can not be unit characters.\n");
      exit(1);
    }
    if (period.numutypes < MAXUTYPES) {
	obj.type = INT;
	obj.v.num = period.numutypes;
	utypes[period.numutypes].uchar = a1.v.str[0];
	utypes[period.numutypes].name  = a2.v.str;
	utypes[period.numutypes].help  = a3.v.str;
	words[add_word(a1.v.str, VV, 0, NULL)].f.value = obj;
	words[add_word(a2.v.str, VV, 0, NULL)].f.value = obj;
	period.numutypes++;
    } else {
	fprintf(stderr, "Limited to %d types of units! (failed on %s)\n",
		MAXUTYPES, a2.v.str);
	exit(1);
    }
}

/* Add a new type of resource (similar to adding unit type). */
 
define_rtype(a1, a2, a3)
Obj a1, a2, a3;
{
    Obj obj;

    if (period.numrtypes < MAXRTYPES) {
	obj.type = INT;
	obj.v.num = period.numrtypes;
	rtypes[period.numrtypes].rchar = a1.v.str[0];
	rtypes[period.numrtypes].name  = a2.v.str;
	rtypes[period.numrtypes].help  = a3.v.str;
	words[add_word(a1.v.str, VV, 0, NULL)].f.value = obj;
	words[add_word(a2.v.str, VV, 0, NULL)].f.value = obj;
	period.numrtypes++;
    } else {
	fprintf(stderr, "Limited to %d types of resources! (failed on %s)\n",
		MAXRTYPES, a2.v.str);
	exit(1);
    }
}

/* Add a new type of terrain.  All args must be strings, but after the defn, */
/* only full name may be used as a normal word. (Since most terrain chars */
/* seem to conflict with various important chars in this language.) */

define_ttype(a1, a2, a3)
Obj a1, a2, a3;
{
    Obj obj;

    if (period.numttypes < MAXTTYPES) {
	obj.type = INT;
	obj.v.num = period.numttypes;
	ttypes[period.numttypes].tchar = a1.v.str[0];
	ttypes[period.numttypes].name  = a2.v.str;
	ttypes[period.numttypes].color = a3.v.str;
	/* Terrain chars are weird, so don't add as new words */
	words[add_word(a2.v.str, VV, 0, NULL)].f.value = obj;
	period.numttypes++;
    } else {
	fprintf(stderr, "Limited to %d types of terrain! (failed on %s)\n",
		MAXTTYPES, a2.v.str);
	exit(1);
    }
}

/* Push vector of all types of objects for various classes. */

push_u_vec() { push_iota_vec(period.numutypes); }

push_r_vec() { push_iota_vec(period.numrtypes); }

push_t_vec() { push_iota_vec(period.numttypes); }

/* Simulate the "iota" operator of APL; push a vector of consecutive ints. */

push_iota_vec(n)
int n;
{
    int i;

    stack[top].type = VEC;
    stack[top].len = n;
    for (i = 0; i < n; ++i) stack[top].v.vec[i] = i;
    ++top;
}

/* Add the string into the array of random side names. */

add_side_name(a1)
Obj a1;
{
    snames[period.numsnames++] = a1.v.str;
}

/* Add the string into the array of random unit names. */

add_unit_name(a1)
Obj a1;
{
    unames[period.numunames++] = a1.v.str;
}

/* Absorb designer notes directly instead of trying to interpret words or */
/* strings or whatever.  This version limits commentary to about 8 pages, */
/* which should be more than enough. */

begin_notes()
{
    char *line;
    int i = 0;

    period.notes = (char **) malloc(400 * sizeof(char *));
    while ((line = read_line(pfp)) != NULL) {
	if (strcmp(line, "end{notes}") == 0) break;
	period.notes[i++] = line;
	if (i >= 399) break;
    }
    period.notes[i] = NULL;
}

/* Print the whole stack, attempting to fit it all on one line (hah!). */

print_stack()
{
    int i;

    if (top == 0) {
	printf("/* Stack is empty. */\n");
    } else {
	printf("/* Stack ");
	for (i = 0; i < top; ++i) {
	    print_object(stack[i], FALSE);
	}
	printf("*/\n");
    }
}

/* Ah, the joys of polymorphism.  Gotta print each type out differently. */
/* I suppose it's better than not knowing what the types of things are... */

print_object(obj, newline)
Obj obj;
bool newline;
{
    int i;

    switch (obj.type) {
    case INT:
	printf("%d ", obj.v.num);
	break;
    case STR:
	printf("\"%s\" ", obj.v.str);
	break;
    case SYM:
	printf("%s ", words[obj.v.sym].name);
	break;
    case VEC:
	printf("[");
	for (i = 0; i < obj.len; ++i) printf("%d,", obj.v.vec[i]);
	printf("] ");
	break;
    }
    if (newline) printf("\n");
}

/* Build the initial dictionary.  At the end of this file, to eliminate need */
/* for a zillion function declarations! */

init_words()
{
    numwords = 0;
    mustbenew = FALSE;   /* efficiency hack */
    add_word("[", FN, 0, start_vector);
    add_word("]", FN, 0, finish_vector);
    add_word("true", FN, 0, push_true);
    add_word("false", FN, 0, push_false);
    add_word("define", FN, 2, set_define);
    add_word("period-name", FN, 1, set_periodname);
    add_word("font-name", FN, 1, set_fontname);
    add_word("utype", FN, 3, define_utype);
    add_word("rtype", FN, 3, define_rtype);
    add_word("ttype", FN, 3, define_ttype);
    add_word("u*", FN, 0, push_u_vec);
    add_word("r*", FN, 0, push_r_vec);
    add_word("t*", FN, 0, push_t_vec);
    add_word("nothing", FN, 0, push_nothing);
    add_word("dark", T1, 2, OFFSET(Ttype, dark));
    add_word("nuked", T1, 2, OFFSET(Ttype, nuked));
    add_word("max-alt", T1, 2, OFFSET(Ttype, maxalt));
    add_word("min-alt", T1, 2, OFFSET(Ttype, minalt));
    add_word("max-wet", T1, 2, OFFSET(Ttype, maxwet));
    add_word("min-wet", T1, 2, OFFSET(Ttype, minwet));
    add_word("default-terrain", P0, 1, OFFSET(Period, defaultterrain));
    add_word("edge-terrain", P0, 1, OFFSET(Period, edgeterrain));
    add_word("alt-roughness", P0, 1, OFFSET(Period, altroughness));
    add_word("wet-roughness", P0, 1, OFFSET(Period, wetroughness));
    add_word("machine-advantage", P0, 1, OFFSET(Period, machineadvantage));
    add_word("icon-name", S1, 2, OFFSET(Utype, bitmapname));
    add_word("neutral", U1, 2, OFFSET(Utype, isneutral));
    add_word("territory", U1, 2, OFFSET(Utype, territory));
    add_word("can-disband", U1, 2, OFFSET(Utype, disband));
    add_word("efficiency", P0, 1, OFFSET(Period, efficiency));
    add_word("country-size", P0, 1, OFFSET(Period, countrysize));
    add_word("country-min-distance", P0, 1, OFFSET(Period, mindistance));
    add_word("country-max-distance", P0, 1, OFFSET(Period, maxdistance));
    add_word("first-unit", P0, 1, OFFSET(Period, firstutype));
    add_word("first-product", P0, 1, OFFSET(Period, firstptype));
    add_word("in-country", U1, 2, OFFSET(Utype, incountry));
    add_word("density", U1, 2, OFFSET(Utype, density));
    add_word("named", U1, 2, OFFSET(Utype, named));
    add_word("already-seen", U1, 2, OFFSET(Utype, alreadyseen));
    add_word("favored", U2, 3, VOFFSET(Utype, favored));
    add_word("stockpile", U2, 3, VOFFSET(Utype, stockpile));
    add_word("known-radius", P0, 1, OFFSET(Period, knownradius));
    add_word("spy-chance", P0, 1, OFFSET(Period, spychance));
    add_word("spy-quality", P0, 1, OFFSET(Period, spyquality));
    add_word("leave-map", P0, 1, OFFSET(Period, leavemap));
    add_word("repair-scale", P0, 1, OFFSET(Period, repairscale));
    add_word("revolt", U1, 2, OFFSET(Utype, revolt));
    add_word("surrender", U1, 2, OFFSET(Utype, surrender));
    add_word("siege", U1, 2, OFFSET(Utype, siege));
    add_word("attrition", U2, 3, VOFFSET(Utype, attrition));
    add_word("attrition-damage", U1, 2, OFFSET(Utype, attdamage));
    add_word("attrition-message", S1, 2, OFFSET(Utype, attritionmsg));
    add_word("accident", U2, 3, VOFFSET(Utype, accident));
    add_word("accident-message", S1, 2, OFFSET(Utype, accidentmsg));
    add_word("make", U2, 3, VOFFSET(Utype, make));
    add_word("maker", U1, 2, OFFSET(Utype, maker));
    add_word("occupant-produce", U1, 2, OFFSET(Utype, occproduce));
    add_word("startup", U1, 2, OFFSET(Utype, startup));
    add_word("research", U1, 2, OFFSET(Utype, research));
    add_word("research-contrib", U2, 3, VOFFSET(Utype, research_contrib));
    add_word("to-make", U2, 3, VOFFSET(Utype, tomake));
    add_word("repair", U2, 3, VOFFSET(Utype, repair));
    add_word("produce", U2, 3, VOFFSET(Utype, produce));
    add_word("productivity", U2, 3, VOFFSET(Utype, productivity));
    add_word("storage", U2, 3, VOFFSET(Utype, storage));
    add_word("consume", U2, 3, VOFFSET(Utype, consume));
    add_word("in-length", U2, 3, VOFFSET(Utype, inlength));
    add_word("out-length", U2, 3, VOFFSET(Utype, outlength));
    add_word("consume-as-occupant", U1, 2, OFFSET(Utype, consume_as_occupant));
    add_word("survival", U1, 2, OFFSET(Utype, survival));
    add_word("starve-message", S1, 2, OFFSET(Utype, starvemsg));
    add_word("speed", U1, 2, OFFSET(Utype, speed));
    add_word("moves", U2, 3, VOFFSET(Utype, moves));
    add_word("terraform", U2, 3, VOFFSET(Utype, terraform));
    add_word("random-move", U2, 3, VOFFSET(Utype, randommove));
    add_word("to-move", U2, 3, VOFFSET(Utype, tomove));
    add_word("capacity", U2, 3, VOFFSET(Utype, capacity));
    add_word("hold-volume", U1, 2, OFFSET(Utype, holdvolume));
    add_word("volume", U1, 2, OFFSET(Utype, volume));
    add_word("enter-time", U2, 3, VOFFSET(Utype, entertime));
    add_word("leave-time", U2, 3, VOFFSET(Utype, leavetime));
/*** (UK) insert -> ***/
    add_word("free-move", U2, 3, VOFFSET(Utype, freemove));
    add_word("see-range-in-transport", U2, 3, VOFFSET(Utype, transportseerange));
/*** <- insert ***/
    add_word("bridge", U2, 3, VOFFSET(Utype, bridge));
    add_word("alter-mobility", U2, 3, VOFFSET(Utype, mobility));
    add_word("see-best", U1, 2, OFFSET(Utype, seebest));
    add_word("see-worst", U1, 2, OFFSET(Utype, seeworst));
    add_word("see-range", U1, 2, OFFSET(Utype, seerange));
    add_word("visibility", U1, 2, OFFSET(Utype, visibility));
    add_word("conceal", U2, 3, VOFFSET(Utype, conceal));
    add_word("always-seen", U1, 2, OFFSET(Utype, seealways));
    add_word("all-seen", P0, 1, OFFSET(Period, allseen));
    add_word("hp", U1, 2, OFFSET(Utype, hp));
    add_word("max-quality", U1, 2, OFFSET(Utype, maxquality));
    add_word("skill-effect", U1, 2, OFFSET(Utype, skillf));
    add_word("discipline-effect", U1, 2, OFFSET(Utype, disciplinef));
    add_word("crippled", U1, 2, OFFSET(Utype, crippled));
    add_word("hit", U2, 3, VOFFSET(Utype, hit));
    add_word("neutrality", P0, 1, OFFSET(Period, neutrality));
    add_word("defense", U2, 3, VOFFSET(Utype, defense));
    add_word("damage", U2, 3, VOFFSET(Utype, damage));
/*** (UK) change -> ***/
    add_word("counterable", U1, 2, OFFSET(Utype, counterable));
    add_word("can-counter", U1, 2, OFFSET(Utype, cancounter));
/*** was:
    add_word("can-counter", U1, 2, OFFSET(Utype, counterable));
*** <- change ***/
    add_word("area-radius", U1, 2, OFFSET(Utype, arearadius));
    add_word("nuke-hit", P0, 1, OFFSET(Period, nukehit));
    add_word("self-destruct", U1, 2, OFFSET(Utype, selfdestruct));
    add_word("combat-time", U1, 2, OFFSET(Utype, hittime));
    add_word("counterattack", P0, 1, OFFSET(Period, counterattack));
    add_word("capturemoves", P0, 1, OFFSET(Period, capturemoves));
    add_word("capture", U2, 3, VOFFSET(Utype, capture));
    add_word("guard", U2, 3, VOFFSET(Utype, guard));
    add_word("protect", U2, 3, VOFFSET(Utype, protect));
    add_word("changes-side", U1, 2, OFFSET(Utype, changeside));
    add_word("retreat", U1, 2, OFFSET(Utype, retreat));
    add_word("hits-with", U2, 3, VOFFSET(Utype, hitswith));
    add_word("hit-by", U2, 3, VOFFSET(Utype, hitby));
    add_word("destroy-message", S1, 2, OFFSET(Utype, destroymsg));
    add_word("clear-side-names", FN, 0, clear_side_names);
    add_word("clear-unit-names", FN, 0, clear_unit_names);
    add_word("sname", FN, 1, add_side_name);
    add_word("uname", FN, 1, add_unit_name);
    add_word("begin{notes}", FN, 0, begin_notes);
    add_word("print", FN, 0, print_stack);
    add_word("attack-worth", U1, 2, OFFSET(Utype, attack_worth));
    add_word("defense-worth", U1, 2, OFFSET(Utype, defense_worth));
    add_word("explore-worth", U1, 2, OFFSET(Utype, explore_worth));
    add_word("min-region-size", U1, 2, OFFSET(Utype, min_region_size));
    add_word("end", FN, 0, NULL);
    mustbenew = TRUE;
}

/* Run a doublecheck on plausibility of period parameters.  Additional */
/* checks are performed elsewhere as needed, for instance during random */
/* generation.  Serious mistakes exit now, since they can cause all sorts */
/* of strange behavior and core dumps.  It's a little more friendly to only */
/* exit at the end of the tests, so all the mistakes can be found at once. */

check_period_numbers()
{
    bool failed = FALSE, movers = FALSE, makers = FALSE;
    int u1, u2, t1, t2;

    if (period.numutypes < 1) {
	fprintf(stderr, "No units have been defined at all!\n");
	exit(1);
    }
    if (period.numttypes < 1) {
	fprintf(stderr, "No terrain types have been defined at all!\n");
	exit(1);
    }
    if (!between(0, period.mindistance, period.maxdistance)) {
	fprintf(stderr, "Warning: Country distances %d to %d screwed up\n",
		period.mindistance, period.maxdistance);
    }
    for_all_unit_types(u1) {
	if (utypes[u1].hp <= 0) {
	    fprintf(stderr, "Utype %d has nonpositive hp!\n", u1);
	    failed = TRUE;
	}
	if (utypes[u1].speed > 0) {
	    movers = TRUE;
	}
	for_all_unit_types(u2) {
	    if (utypes[u1].make[u2] > 0) {
		if (utypes[u1].maker) makers = TRUE;
		if (!could_carry(u1, u2) && !could_carry(u2, u1)) {
		    fprintf(stderr,
			    "Utype %d cannot hold product %d or vice versa!\n",
			    u1, u2);
		    failed = TRUE;
		}
	    }
	}
      }
    for_all_terrain_types(t1) {
	for_all_terrain_types(t2) {
	    if (t1 != t2 && ttypes[t1].tchar == ttypes[t2].tchar) {
		fprintf(stderr, "Terrain types %d and %d both use '%c'!\n",
			t1, t2, ttypes[t1].tchar);
		failed = TRUE;
	    }
	}
    }
    if (!movers && !makers) {
	fprintf(stderr, "No movers or builders have been defined!\n");
	failed = TRUE;
    }
    if (failed) exit(1);
    if (Debug) {
	printf("\n    %d unit types, %d resource types, %d terrain types\n",
	       period.numutypes, period.numrtypes, period.numttypes);
    }
}
