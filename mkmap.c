/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Terrain generation is based on fractal ideas, although this version */
/* is not directly derived from a published technique. */
/* Speed is important; most of the code has been integerized. */
/* Extra steps produce maps more suitable for conquest, including a way to */
/* control the amount of each type of terrain appearing in the result. */
/* The process is actually done for elevation and water separately, then */
/* the terrain type is dependent on both. */

/* (Note that this is no longer a separate program as in the first version.) */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "dir.h"
#include "map.h"

/* Values of altitude and moisture must be firmly bounded, so that */
/* histogramming doesn't take too much space. */

#define MAXALT 16384

/* Abstraction of 2D malloced array references.  Names from Lisp... */

#define aref(m,x,y) ((m)[(x)+world.width*(y)])

#define aset(m,x,y,v) ((m)[(x)+world.width*(y)] = (v))

#define aadd(m,x,y,v) ((m)[(x)+world.width*(y)] += (v))

/* Values ultimately arriving from the command line. */

extern int givenwidth, givenheight, givenseen;

/* Arrays here should contain only nonnegative values (for simplicity). */
/* They are potentially large and used only once, so we malloc and free... */

int *relief;            /* pointer to array of elevations */
int *water;             /* pointer to array of moisture levels */
int *aux;               /* auxiliary array */
int *histo;             /* histogram array */
int *alts;              /* percentile for each elevation */
int *wets;              /* percentile for level of moisture */

/* The main function looks a little strange in spots, since it is derived */
/* from a former standalone program (the mkmap of version 1). */

make_up_map()
{
    int width, height;

    width = givenwidth;  height = givenheight;
    if (Debug) printf("Going to make up a %dx%d map ...\n", width, height);
    /* Heuristic limit - algorithms get weird on small maps */
    if (width < 9 || height < 9) {
	fprintf(stderr, "Cannot generate such a small map!\n");
	exit(1);
    }
    world.width = width;  world.height = height;
    world.known = givenseen;
    relief = (int *) malloc(world.width * world.height * sizeof(int));
    aux    = (int *) malloc(world.width * world.height * sizeof(int));
    water  = (int *) malloc(world.width * world.height * sizeof(int));
    histo  = (int *) malloc(MAXALT * sizeof(int));
    alts   = (int *) malloc(MAXALT * sizeof(int));
    wets   = (int *) malloc(MAXALT * sizeof(int));
    /* Build a full relief map */
    make_bumps(relief, period.altroughness);
    smooth_map(relief);
    add_curves(relief, period.altroughness);
    smooth_map(relief);
    limit_map(relief, MAXALT-1, 0);
    percentile(relief, alts);
    /* Build a "moisture relief" map */
    make_bumps(water, period.wetroughness);
    smooth_map(water);
    smooth_map(water);
    limit_map(water, MAXALT-1, 0);
    percentile(water, wets);
    /* Put it all together */
    compose_map();
    free(relief);
    free(water);
    free(aux);
    free(histo);
    free(alts);
    free(wets);
    if (Debug) printf("... Done making up a map.\n");
}

/* The main map generators produces bumps, of sizes and numbers dictated */
/* by the roughness.  The size is a percentage of map dimensions, while */
/* number is set to approximately cover the entire map.  Each bump is a */
/* hexagonal shape, and the elevation of interior points varies. */

make_bumps(map, roughness)
int *map, roughness;
{
    int blocks, i, scale, var, dx, dy, tries;
    int x0, y0, x1, y1, d0, d1, xt, yt, flag, x, y, dz, z, oz;
    int dn1, dn2, dn3, dn4, dn5, dn6, numer1, denom1, numer2, denom2;
    int farx[10], fary[10];
    int peakx[10], peaky[10], peakz[10], peakf[10], numpeaks = 0, p;

    scale = max(1, ((100 - roughness) * min(world.width, world.height)) / 100);
    var = MAXALT/8 + (MAXALT/8 * roughness) / 100;
    blocks = (world.width * world.height) / (scale * scale);
    blocks *= 2;
    if (Debug) printf("Playing with blocks (%d)...\n", blocks);
    for (x = 0; x < world.width; ++x) {
	for (y = 0; y < world.height; ++y) {
	    aset(map, x, y, MAXALT/2);
	}
    }
    for (i = 0; i < blocks; ++i) {
	flag = ((i == 0 || flip_coin()) ? 1 : -1);
	if (scale <= 1) {
	    x = RANDOM(world.width) + world.width;
	    y = RANDOM(world.height);
	    z = var + RANDOM(var/2);
	    aadd(map, wrap(x), y, flag * z);
	} else {
	    dx = scale/2 + RANDOM(scale/2);
	    dy = scale/2 + RANDOM(scale/2);
	    /* All x values shifted, so as to avoid negatives */
	    x0 = RANDOM(world.width) + world.width;
	    y0 = RANDOM(world.height - dy - 1);
	    x1 = x0 + dx + 1;
	    y1 = y0 + dy + 1;
	    d0 = min(x1 - x0, y1 - y0) / 4;
	    d0 = d0 + RANDOM(3*d0);
	    d1 = min(x1 - x0, y1 - y0) / 4;
	    d1 = d1 + RANDOM(3*d1);
	    /* Cheap way to ensure most high points actually within hexagon */
	    xt = yt = -1;  tries = 10;
	    while (((xt-x0+yt-y0 < d0) || (x1-xt+y1-yt < d1)) && tries-- > 0) {
		xt = x0 + (RANDOM(x1 - x0) + RANDOM(x1 - x0)) / 2;
		yt = y0 + (RANDOM(y1 - y0) + RANDOM(y1 - y0)) / 2;
	    }
	    /* Compute distances from high point to hexagon corners */
	    dn1 = distance(xt, yt, x1-d1, y1);
	    dn2 = distance(xt, yt, x1, y1-d1);
	    dn3 = distance(xt, yt, x1, y0);
	    dn4 = distance(xt, yt, x0+d0, y0);
	    dn5 = distance(xt, yt, x0, y0+d0);
	    dn6 = distance(xt, yt, x0, y1);
	    for (y = y0; y <= y1; ++y) {
		for (x = x0; x <= x1; ++x) {
		    if ((x-x0+y-y0 > d0) && (x1-x+y1-y > d1)) {
			dx = x - xt;  dy = y - yt;
			if (dx > 0) {
			    if (dy > 0) {
				numer1 = distance(x, y, x1, y1-d1);
				denom1 = dn2;
				numer2 = distance(x, y, x1-d1, y1);
				denom2 = dn1;
			    } else if (dy > (0 - dx)) {
				numer1 = distance(x, y, x1, y0);
				denom1 = dn3;
				numer2 = distance(x, y, x1, y1-d1);
				denom2 = dn2;
			    } else {
				numer1 = distance(x, y, x0+d0, y0);
				denom1 = dn4;
				numer2 = distance(x, y, x1, y0);
				denom2 = dn3;
			    }
			} else {
			    if (dy < 0) {
				numer1 = distance(x, y, x0, y0+d0);
				denom1 = dn5;
				numer2 = distance(x, y, x0+d0, y0);
				denom2 = dn4;
			    } else if (dy < (0 - dx)) {
				numer1 = distance(x, y, x0, y1);
				denom1 = dn6;
				numer2 = distance(x, y, x0, y0+d0);
				denom2 = dn5;
			    } else {
				numer1 = distance(x, y, x1-d1, y1);
				denom1 = dn1;
				numer2 = distance(x, y, x0, y1);
				denom2 = dn6;
			    }
			}
			if (denom1 == 0) denom1 = 1;
			if (denom2 == 0) denom2 = 1;
			dz = (flag * ((var * numer1) / denom1 +
				      (var * numer2) / denom2));
			oz = aref(map, wrap(x), y);
			if (!between(0, dz + oz, MAXALT)) dz /= 2;
			aset(map, wrap(x), y, oz + dz + RANDOM(var/2));
		    }
		}
	    }
	}
	/* Remember some high points for use in hitting untouched areas */
	if (numpeaks < 10) {
	    peakx[numpeaks] = wrap(xt);
	    peaky[numpeaks] = yt;
	    peakf[numpeaks] = flag;
	    peakz[numpeaks] = var;
	    farx[numpeaks] = wrap(xt+world.width/2);
	    fary[numpeaks] = RANDOM(world.height);
	    numpeaks++;
	}
    }
    /* This is to make smoothly varying terrain in formerly flat areas */
    for (x = 0; x < world.width; ++x) {
	for (y = 0; y < world.height; ++y) {
	    if (aref(map, x, y) == MAXALT/2) {
		z = 0;
		for (p = 0; p < numpeaks; ++p) { 
		    dz = ((peakz[p] * cyldist(x, y, farx[p], fary[p])) /
			  (cyldist(peakx[p], peaky[p], farx[p], fary[p])+1));
		    dz -= (peakz[p] * scale) / world.width;
		    z += peakf[p] * dz;
		}
		z /= numpeaks;
		aadd(map, x, y, z + RANDOM(var/4));
	    }
	}
    }
    limit_map(map, MAXALT-1, 0); 
}

/* Returns shortest distance around the world (can be either direction). */

cyldist(x1, y1, x2, y2)
int x1, y1, x2, y2;
{
    int dist = distance(x1, y1, x2, y2), dist2;

    if ((dist2 = distance(x1+world.width, y1, x2, y2)) < dist) {
	dist = dist2;
    } else if ((dist2 = distance(x1, y1, x2+world.width, y2)) < dist) {
	dist = dist2;
    }
    return dist;
}

/* Exponent with *small* integer power. (used with Bezier curves) */

float
expt(f, i)
float f;
int i;
{
    float rslt = 1.0;

    while (i-- > 0) rslt = rslt * f;
    return rslt;
}

/* Combination of n objects taken i at a time. (used for Bezier) */

comb(n, i)
int n, i;
{
    if (i <= 0) return 1;
    else if (n <= 0) return 0;
    else return (comb(n-1, i) + comb(n-1,i-1));
}

/* Put in random Bezier mountain chains.  Number and size are scaled to */
/* the size of the map.  To prevent criss-crossing from creating very */
/* high spots, just set the aux map to values, and only add in later. */

add_curves(map, roughness)
int *map, roughness;
{
    int i, j, n, x, y, flag, curves, mindim, var;
    float u, f, cpx[4], cpy[4], px[500], py[500];

    curves = (world.width * world.height * roughness) / 50000;
    if (Debug) printf("Throwing curves (%d)...\n", curves);
    for (x = 0; x < world.width; ++x) {
	for (y = 0; y < world.height; ++y) {
	    aset(aux, x, y, 0);
	}
    }
    mindim = min(500, min(world.width, world.height));
    var = (MAXALT/8 * roughness) / 100;
    for (n = 0; n < curves; ++n) {
	cpx[0] = 1.0 * (RANDOM(world.width-2)+1);
	cpy[0] = 1.0 * (RANDOM(world.height-2)+1);
	for (j = 1; j < 4; ++j) {
	    cpx[j] = cpx[0] + (RANDOM(mindim) - mindim/2);
	    if (cpx[j] > world.width-1) cpx[j] = world.width-1;
	    if (cpx[j] < 0) cpx[j] = 0;
	    cpy[j] = cpy[0] + (RANDOM(mindim) - mindim/2);
	    if (cpy[j] > world.height-1) cpy[j] = world.height-1;
	    if (cpy[j] < 0) cpy[j] = 0;
	}
	for (i = 0; i < mindim; ++i) {
	    u = (1.0 * i) / mindim;
	    px[i] = py[i] = 0.0;
	    for (j = 0; j < 4; ++j) {
		f = comb(3,j)*expt(u,j)*expt(1-u,3-j);
		px[i] += cpx[j]*f; 
		py[i] += cpy[j]*f;
	    }
	}
	flag = RANDOM(3) ? 1 : -1;
	for (i = 0; i < mindim; ++i) {
	    x = (int) px[i];  y = (int) py[i];
	    aset(aux, x, y, flag * (MAXALT/8 + var));
	}
    }
    add_maps(map, aux, map);
    limit_map(map, MAXALT-1, 0);
}

/* Ensure that map values stay within given range. */

limit_map(map, hi, lo)
int *map, hi, lo;
{
    int x, y, m;
    
    for (x = 0; x < world.width; ++x) {
	for (y = 0; y < world.height; ++y) {
	    m = aref(map, x, y);
	    aset(map, x, y, max(lo, min(m, hi)));
	}
    }
}

/* Form the point-wise sum of two maps into a third one. */

add_maps(m1, m2, rslt)
int *m1, *m2, *rslt;
{
    int x, y;

    for (x = 0; x < world.width; ++x) {
	for (y = 0; y < world.height; ++y) {
	    aset(rslt, x, y, aref(m1, x, y) + aref(m2, x, y));
	}
    }
}

/* Average out things to keep peaks from getting too sharp. */
/* The array "aux" is the buffer for this operation. */
/* The edges of the map remain unaltered. (dubious) */

smooth_map(map)
int *map;
{
    int x, y, nx, px, sum;

    if (Debug) printf("Smoothing...\n");
    for (x = 0; x < world.width; ++x) {
	nx = wrap(x+1);  px = wrap(x-1);
	for (y = 1; y < world.height-1; ++y) {
	    sum  = aref(map, x, y);
	    sum += aref(map, x, y+1);
	    sum += aref(map, nx, y);
	    sum += aref(map, nx, y-1);
	    sum += aref(map, x, y-1);
	    sum += aref(map, px, y);
	    sum += aref(map, px, y+1);
	    aset(aux, x, y, sum / 7);
	}
    }
    for (x = 0; x < world.width; ++x) {
	for (y = 1; y < world.height-1; ++y) {
	    aset(map, x, y, aref(aux, x, y));
	}
    }
}

/* Terrain types are specified in terms of percentage cover on a map, so */
/* for instance the Earth is 70% sea.  Since each of several types will have */
/* its own percentages (both for elevation and moisture), the simplest thing */
/* to do is to calculate the percentile for each elevation and moisture */
/* level, and save them all away.  This would be a one-liner in APL... */

/* Percentile computation is inefficient, should be done incrementally */
/* somehow instead of with * and / */

percentile(map, percentiles)
int *map, *percentiles;
{
    int i, x, y, total;
    
    if (Debug) printf("Computing percentiles...\n");
    for (i = 0; i < MAXALT; ++i) {
	histo[i] = 0;
	percentiles[i] = 0;
    }
    /* Make the basic histogram, but don't count the edges */
    for (x = 0; x < world.width; ++x) {
	for (y = 1; y < world.height-1; ++y) {
	    ++histo[aref(map, x, y)];
	}
    }
    /* Integrate over the histogram */
    for (i = 1; i < MAXALT; ++i)
	histo[i] += histo[i-1];
    /* Total here should actually be the same as number of cells in the map */
    total = histo[MAXALT-1];
    /* Compute the percentile position */
    for (i = 0; i < MAXALT; ++i)
	percentiles[i] = (100 * histo[i]) / total;
}

/* Final creation and output of the map. */

compose_map()
{
    int x, y;

    if (Debug) printf("Assigning terrain types...\n");
    allocate_map();
    for (y = 0; y < world.height; ++y) {
	for (x = 0; x < world.width; ++x) {
	    set_terrain_at(x, y, terrain(x, y));
	    set_unit_at(x, y, NULL);
	}
    }
    /* Add a border if specified */
    if (period.edgeterrain >= 0) {
	for (x = 0; x < world.width; ++x) {
	    set_terrain_at(x, 0, period.edgeterrain);
	    set_terrain_at(x, world.height-1, period.edgeterrain);
	}
    }
}

/* Do final output of terrain types.  This is basically a process of */
/* checking the percentile limits on each type against what is actually */
/* there.  (could and maybe should be more efficient) */
/* Error checking important for period designers... */

terrain(x, y)
int x, y;
{
    int t, a = aref(relief, x, y), w = aref(water, x, y);

    for_all_terrain_types(t) {
	if (between(ttypes[t].minalt, alts[a], ttypes[t].maxalt) &&
	    between(ttypes[t].minwet, wets[w], ttypes[t].maxwet)) {
	    return t;
	}
    }
    printf("Unknown terrain in percentiles alt %d, wet %d.\n",
	   alts[a], wets[w]);
    /* Not great, but valid if nothing else */
    return (max(0, period.defaultterrain));
}
