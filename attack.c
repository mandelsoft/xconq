/* Copyright (c) 1987, 1988  Stanley T. Shebs. */
/* This program may be used, copied, modified, and redistributed freely */
/* for noncommercial purposes, so long as this notice remains intact. */

/* Although conducting xconq combat is rather simple (i.e. try to move */
/* into the hex), the outcome is under the control of many parameters. */

/* Rules of combat: the attacker hits the defender ("other") unit and its */
/* occupants, but the damage does not take effect right away.  If counter */
/* attacks are possible in this period, the defender always does so, with */
/* the same odds.  If the defender dies, then the attacker moves into the */
/* hex.  If the attacker dies, nothing happens.  If both survive, then the */
/* attacker may attempt to capture the defender. */

#include "config.h"
#include "misc.h"
#include "period.h"
#include "side.h"
#include "unit.h"
#include "map.h"
#include "global.h"

extern int occdeath[];

char *summarize_units();

/* Buffers for verbal descriptions of units from each other's */
/* point of view. */

char aabuf[BUFSIZE], aobuf[BUFSIZE], oabuf[BUFSIZE], oobuf[BUFSIZE];
char hitbuf[BUFSIZE], killbuf[BUFSIZE];

/* Remember what the main units involved are, so display is handled relative */
/* to them and not to any occupants. */

Unit *amain, *omain;

/* Hits on main units saved up, hits on occupants happen immediately. */

int ahit, ohit;

/* ... but the data is saved anyway, for message generation. */

int occhits[MAXUTYPES], occkills[MAXUTYPES];

/*** (UK) insert -> ***/
could_counterattack(a,o)
{
  return could_hit(o,a) && (period.counterattack || 
	  (utypes[a].counterable && utypes[o].cancounter));
}

/*** <- insert ***/

/* Return true if the attacker defeated the defender, and can therefore */
/* try to move into the defender's old position. */

attack(atker, other)
Unit *atker, *other;
{
    int u, ax = atker->x, ay = atker->y, ox = other->x, oy = other->y;
    Side *as = atker->side, *os = other->side;

/*** (UK) insert -> ***/
    bool counterattack=FALSE;

    if (could_hit(atker->type,other->type))
/*** <- insert ***/
    cancel_build(atker);
    amain = atker;  omain = other;
    ahit = ohit = 0;
    for_all_unit_types(u) occhits[u] = occkills[u] = 0;
/*** (UK) insert -> ***/
    if (could_hit(atker->type,other->type))
/*** <- insert ***/
    attack_unit(atker, other, TRUE);
/*** (UK) change -> ***/
    if (could_counterattack(atker->type, other->type)) {
        counterattack=TRUE;
/*** was:
    if (period.counterattack || utypes[atker->type].counterable)
*** <- change ***/
	attack_unit(other, atker, FALSE);
/*** (UK) insert -> ***/
    }
/*** <- insert ***/
    reckon_damage();
    see_exact(as, ox, oy);
    draw_hex(as, ox, oy, TRUE);
/*** (UK) insert -> ***/
    if (as && cover(as,ax,ay)>0) 
/*** <- insert ***/
    see_exact(as, ax, ay);
    draw_hex(as, ax, ay, TRUE);
    see_exact(os, ax, ay);
    draw_hex(os, ax, ay, TRUE);
/*** (UK) insert -> ***/
    if (os && cover(os,ox,oy)>0) 
/*** <- insert ***/
    see_exact(os, ox, oy);
    draw_hex(os, ox, oy, TRUE);
    all_see_hex(ax, ay);
    all_see_hex(ox, oy);
    attempt_to_capture_unit(atker, other);
#ifndef ORIGINAL
    /* moved from attack code, to allow units that capture then destruct */
    /* will allow these units to be attacked and not self-destruct, though */
    if (utypes[atker->type].selfdestruct) {
	kill_unit(atker, COMBAT);
    }
/*** (UK) insert -> ***/
    if (counterattack && utypes[other->type].selfdestruct) {
	kill_unit(other, COMBAT);
    }
/*** <- insert ***/
#endif
    return (alive(atker) && unit_at(ox, oy) == NULL);
}

/* Test to see if enough ammo is available to make the attack. */
/* Need enough of *all* types - semi-bogus but too complicated otherwise? */

enough_ammo(atker, other)
Unit *atker, *other;
{
    int r;

    for_all_resource_types(r) {
	if (utypes[other->type].hitby[r] > 0 &&
	    atker->supply[r] < utypes[atker->type].hitswith[r]) return FALSE;
    }
    return TRUE;
}

/* Single attack, no counterattack.  Check and use ammo - usage independent */
/* of outcome, but types used depend on unit types involved. */

attack_unit(atker, other, allq)
Unit *atker, *other;
bool	allq;
{
    int r,dist;
    int	x,y;

    wake_unit(other, -1, WAKEENEMY, atker);
    if (alive(atker) && alive(other)) {
	if (enough_ammo(atker, other)) {
	    if (!allq) {
		hit_unit(atker, other);
	    } else {
		for (x=other->x - utypes[atker->type].arearadius;
		     x<=other->x + utypes[atker->type].arearadius; x++)
		  for (y=other->y - utypes[atker->type].arearadius;
		       y <= other->y + utypes[atker->type].arearadius; y++) {
		      if ( (x==atker->x) && (y==atker->y) )
			break;
		      dist = distance(other->x,other->y,x,y);
		      if (dist>utypes[atker->type].arearadius)
			continue;
		      if (unit_at(x,y)) {
			  hit_unit(atker, unit_at(x,y));
		      }
		  }
	    }
	    hit_unit(atker, other);
	    for_all_resource_types(r) {
		if (utypes[other->type].hitby[r] > 0) {
		    atker->supply[r] -= utypes[atker->type].hitswith[r];
		}
	    }
	    atker->movesleft -= utypes[atker->type].hittime;
	}
    }
}

/* Make a single hit and maybe hit some passengers also.  Power of hit */
/* is constant, but chance is affected by neutrality, terrain, quality, */
/* and occupants' protective abilities.  If a hit is successful, it may */
/* have consequences on the defender's occupants, but limited by the */
/* protection that the transport provides. */

hit_unit(atker, other)
Unit *atker, *other;
{
    int chance, terr, hit = 0, a = atker->type, o = other->type;
    Unit *occ;
    Side *as = atker->side;

    chance = utypes[a].hit[o];
    if (neutral(atker)) chance += period.neutrality;
    terr = terrain_at(other->x, other->y);
    chance -= (chance * utypes[o].defense[terr]) / 100;
    if (utypes[a].maxquality > 0) {
	chance += ((chance * atker->quality * utypes[a].skillf) /
		   utypes[a].maxquality) / 100;
    }
    if (utypes[o].maxquality > 0) {
	chance -= ((chance * other->quality * utypes[o].disciplinef) /
		   utypes[o].maxquality) / 100;
    }
    for_all_occupants(other, occ) {
	chance -= (chance * utypes[occ->type].protect[o]) / 100;
    }
    if (probability(chance)) hit = utypes[a].damage[o];
    if (as != NULL) {
	as->atkstats[a][o]++;
	as->hitstats[a][o] += hit;
    }
    if (hit_unit_aux(atker, other, hit)) {
	for_all_occupants(other, occ) {
	    if (probability(100 - utypes[o].protect[occ->type])) {
		hit_unit(atker, occ);
	    }
	}
    }
}

/* Do the hit itself.  Occupants are always hit/killed immediately, while */
/* the "main units" of the hexes don't get it till later.  In any case, */
/* messages are delayed.  Return true if the victim was hit but not killed. */

hit_unit_aux(atker, other, hit)
Unit *atker, *other;
int hit;
{
    int o = other->type, a = atker->type, chance, i;
    Side *as = atker->side, *os = other->side;

    if (hit >= period.nukehit) {
	notify_all("%s has been hit by a nuclear attack!!!",
		   unit_handle((Side *) NULL, other));
	set_terrain_at(other->x, other->y,
		       ttypes[terrain_at(other->x, other->y)].nuked);
    }
    if (hit > 0 && mobile(o)) {
	chance = utypes[o].retreat;
	/* should adjust chance by morale etc */
	if (probability(chance)) {
	    if (retreat_unit(other)) {
		notify(as, "%s runs away!", unit_handle(as, other));
		notify(os, "%s runs away!", unit_handle(os, other));
		hit = 0;
	    }
	}
    }
    if (other == omain) {
	ohit = hit;
    } else if (other == amain) {
	ahit = hit;
    } else {
	if (hit >= other->hp) {
	    occkills[o]++;
	    kill_unit(other, COMBAT);
	    for_all_unit_types(i) occkills[i] += occdeath[i];
	} else if (hit > 0) {
	    occhits[o]++;
	    other->hp -= hit;
	}
#ifdef ORIGINAL
	/* move this code, so that something like a charm spell
	** ( which self-destructs after an attack )
  	** can work. Otherwise, it dies here, before  capture_unit
	** is called
	*/
	if (utypes[a].selfdestruct) {
	    kill_unit(atker, COMBAT);
	}
#endif
    }
    return (alive(other) && hit > 0);
}

/* Hits on the main units have to be done later, so that mutual */
/* destruction is possible.  This function also does all the notifying. */
/* (Only the main units of a hex rate messages, occupants' fates are */
/* summarized briefly.) */

reckon_damage()
{
    int o = omain->type, a = amain->type, i;
    Side *as = amain->side, *os = omain->side;

    strcpy(aabuf, unit_handle(as, amain));
    strcpy(aobuf, unit_handle(as, omain));
    strcpy(oabuf, unit_handle(os, amain));
    strcpy(oobuf, unit_handle(os, omain));

    if (ahit > 0) draw_blast(amain, omain->side, ahit);
    if (ohit > 0) draw_blast(omain, amain->side, ohit);
    if (ohit >= omain->hp) {
	if (utypes[a].maxquality > 0  &&  amain->quality < utypes[a].maxquality)
	    ++amain->quality;
	notify(as, "%s %s %s!", aabuf, utypes[o].destroymsg, aobuf);
	notify(os, "%s %s %s!", oabuf, utypes[o].destroymsg, oobuf);
	kill_unit(omain, COMBAT);
	for_all_unit_types(i) occkills[i] += occdeath[i];
    } else if (ohit > 0) {
	notify(as, "%s hits %s!", aabuf, aobuf);
	notify(os, "%s hits %s!", oabuf, oobuf);
	omain->hp -= ohit;
    } else {
	/* messages about missing not too useful */
    }
    summarize_units(hitbuf, occhits);
    summarize_units(killbuf, occkills);
    if (strlen(hitbuf) > 0) {
	if (strlen(killbuf) > 0) {
	    notify(as, "   (Also hit%s, killed%s)", hitbuf, killbuf);
	    notify(os, "   (%s hurt, %s killed)", hitbuf, killbuf);
	} else {
	    notify(as, "   (Also hit%s)", hitbuf);
	    notify(os, "   (%s hurt)", hitbuf);
	}
    } else {
	if (strlen(killbuf) > 0) {
	    notify(as, "   (Also killed%s)", killbuf);
	    notify(os, "   (%s killed)", killbuf);
	}
    }
    if (ahit >= amain->hp) {
	if (utypes[o].maxquality > 0  &&  omain->quality < utypes[o].maxquality)
	    ++omain->quality;
	notify(as, "%s %s %s!", aobuf, utypes[a].destroymsg, aabuf);
	notify(os, "%s %s %s!", oobuf, utypes[a].destroymsg, oabuf);
	kill_unit(amain, COMBAT);
    } else if (ahit > 0) {
	notify(as, "%s hits %s!", aobuf, aabuf);
	notify(os, "%s hits %s!", oobuf, oabuf);
	amain->hp -= ahit;
    } else {
	/* messages about missing not too useful */
    }
#ifdef ORIGINAL
    if (utypes[a].selfdestruct) kill_unit(amain, COMBAT);
    if (utypes[o].selfdestruct) kill_unit(omain, COMBAT);
#endif
}

/* Handle capture possibility and repulse/slaughter. */

/* The chance to capture an enemy is modified by several factors. */
/* Neutrals have a different chance to be captured, and presence of */
/* occupants should also has an effect.  Can't capture anything that is */
/* on a kind of terrain that the capturer can't go on, unless victim has */
/* "bridge effect". */

attempt_to_capture_unit(atker, other)
Unit *atker, *other;
{
    int a = atker->type, o = other->type, chance;
    int ox = other->x, oy = other->y;
    Unit *occ;
    Side *as = atker->side, *os = other->side;

    if (alive(atker) && alive(other) && could_capture(a, o)) {
	if (impassable(atker, ox, oy) && !utypes[o].bridge[a]) return;
	chance = utypes[a].capture[o];
	if (neutral(other)) chance -= period.neutrality;
	if (utypes[a].maxquality > 0) {
	    chance += ((chance * atker->quality * utypes[a].skillf) /
		       utypes[a].maxquality) / 100;
	}
	for_all_occupants(other, occ) {
	    chance -= (chance * utypes[occ->type].protect[o]) / 100;
	}
	if (probability(chance)) {
	    if (    utypes[a].maxquality > 0
		&&  atker->quality < utypes[a].maxquality)
		++atker->quality;
	    capture_unit(atker, other);
	    if (global.setproduct && utypes[o].maker) {
		request_new_product(other);
	    }
	} else if (atker->transport != NULL && 
		   (impassable(atker, ox, oy) ||
		    impassable(atker, atker->x, atker->y))) {
	    notify(as, "Resistance... %s was slaughtered!",
		   unit_handle(as, atker));
	    notify(os, "Resistance... %s was slaughtered!",
		   unit_handle(os, atker));
	    kill_unit(atker, COMBAT);
	} else {
	    strcpy(aabuf, unit_handle(as, atker));
	    notify(as, "%s repulses %s!", unit_handle(as, other), aabuf);
	    strcpy(oabuf, unit_handle(os, atker));
	    notify(os, "%s repulses %s!", unit_handle(os, other), oabuf);
	}
	atker->movesleft -= utypes[a].hittime;
    }
}


/* There are many consequences of a unit being captured. */
/* If the capturer is needed as a garrison, unload any occupants first. */
/* (what if erstwhile occupants can't stay there?) */

capture_unit(unit, pris)
Unit *unit, *pris;
{
    int u = unit->type, u2, px = pris->x, py = pris->y, i;
    int occs[MAXUTYPES], gains[MAXUTYPES], kills[MAXUTYPES];
    Unit *occ;
    Side *ps = pris->side, *us = unit->side;

    for_all_unit_types(u2) occs[u2] = gains[u2] = kills[u2] = 0;
    notify(us, "You captured %s!", unit_handle(us, pris));
    notify(ps, "%s has been captured by the %s!",
	   unit_handle(ps, pris), ((us != NULL) ?  
		plural_form(us->name) : "Neutrals!" ));
    if (global.setproduct) {
	set_product(pris, NOTHING);
	pris->schedule = 0;
    }
    /* clear saw enemy messages */
    wake_unit(pris, -1, WAKEFULL, (Unit *) NULL); 
#if 1
    unit_changes_side_Ncount(pris, us, CAPTURE, PRISONER, gains, kills);

    summarize_units(hitbuf,  gains);
    summarize_units(killbuf, kills);
    if (strlen(hitbuf) > 0) {
	if (strlen(killbuf) > 0) {
	    notify(us, "   (Also captured%s, killed%s)", hitbuf, killbuf);
	} else {
	    notify(us, "   (Also captured%s)", hitbuf);
	}
    } else if (strlen(killbuf) > 0) {
	notify(us, "   (Also killed%s)", killbuf);
    }

    for_all_unit_types(i)
      kills[i] += gains[i]; /* total up the losses for the victim */
    summarize_units(spbuf, kills);
    if (strlen(spbuf) > 0) {
	notify(ps, "   (Lost%s)", spbuf);
    }
#else
    for_all_occupants(pris, occ) {
	occs[occ->type]++;
	if (utypes[occ->type].changeside || could_capture(u, occ->type)) {
	    gains[occ->type]++;
	    /* side changing happens when whole unit changes */
	} else {
	    kills[occ->type]++;
	    kill_unit(occ, COMBAT);
	    for_all_unit_types(i) kills[i] += occdeath[i];
	}
    }
    summarize_units(hitbuf,  gains);
    summarize_units(killbuf, kills);
    summarize_units(spbuf, occs);
    if (strlen(hitbuf) > 0) {
	if (strlen(killbuf) > 0) {
	    notify(us, "   (Also captured%s, killed%s)", hitbuf, killbuf);
	} else {
	    notify(us, "   (Also captured%s)", hitbuf);
	}
    } else if (strlen(killbuf) > 0) {
	notify(us, "   (Also killed%s)", killbuf);
    }
    if (strlen(spbuf) > 0) {
	notify(ps, "   (Lost%s)", spbuf);
    }
    /* The sad event itself */
    unit_changes_side(pris, us, CAPTURE, PRISONER);
#endif
    /* Cancel standing orders */
    if (pris->standing != NULL)
/*** (UK) change -> ***/
      delete_standing_orders(pris);
/*** was:
      for_all_unit_types(u2) {
	if (pris->standing->orders[u2] != NULL)
	  pris->standing->orders[u2]->type = AWAKE;
      }
*** <- change ***/
    /* Guard the prisoner */
    if (utypes[u].guard[pris->type] > 0) {
	for_all_occupants(unit, occ) {
	    if (can_carry(pris, occ)) {
		leave_hex(occ);
		occupy_hex(occ, px, py);
	    }
	}
	/* This may kill some of the guard's occupants unnecessarily, but */
        /* I can't think of a better solution */
	kill_unit(unit, GARRISON);
    } else if (can_carry(pris, unit)) {
	leave_hex(unit);
	occupy_hex(unit, px, py);
    } else {
	/* Capturer doesn't guard or enter, so nothing to do */
    }
    wake_neighbors(unit); /* XXX */
    see_exact(ps, px, py);
    draw_hex(ps, px, py, TRUE);
    all_see_hex(px, py);
}

/* Nearly-raw combat statistics are hard to interpret, but they provide */
/* a useful check against subjective evaluation of performance. */

print_combat_results(fp, side)
FILE *fp;
Side *side;
{
    int a, d;

    fprintf(fp,
	    "Unit Performance (successes and attacks against enemy by type\n");
    fprintf(fp, "   ");
    for_all_unit_types(d) {
	fprintf(fp, "  %c  ", utypes[d].uchar);
    }
    fprintf(fp, "\n");
    for_all_unit_types(a) {
	fprintf(fp, " %c ", utypes[a].uchar);
	for_all_unit_types(d) {
	    if (side->atkstats[a][d] > 0) {
		fprintf(fp, " %4.2f",
			((float) side->hitstats[a][d]) /
			         side->atkstats[a][d]);
	    } else {
		fprintf(fp, "     ");
	    }
	}
	fprintf(fp, "\n   ");
	for_all_unit_types(d) {
	    if (side->atkstats[a][d] > 0) {
		fprintf(fp, " %3d ", side->atkstats[a][d]);
	    } else {
		fprintf(fp, "     ");
	    }
	}
	fprintf(fp, "\n");
    }
    fprintf(fp, "\n");
}
