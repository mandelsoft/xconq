.TL
LIBERATING THE WORLD (Made Simple)
.AU
Stan Shebs
.AI
Department of Computer Science
University of Utah
.AB
This is an in-depth document on version 5 of the \fIxconq\fP
family of programs.
Version 5 is quite different from earlier versions;
players familiar with those should read everything in here,
especially the section on important changes.
All aspects of play are covered.
Details on customization may be found in a companion document.
.AE
.SH INTRODUCTION
.PP
.I
War is a matter of vital importance to the State; the province of life
or death; the road to survival or ruin.  It is mandatory that it be
thoroughly studied.  -- SUN TZU (ca 400 BC)
.R
.LP
Welcome to \fIxconq\fP,
a chance for you to free the world from domination by evil empires.
It is similar to previously distributed empire-building games,
but with many more features.
The same basic game is available with several different interfaces.
\fIXconq\fP
is a fully X-based multi-player game, allowing almost any combination
of human and machine players, and opening up remote X windows as necessary.
There is also a restricted variant \fIcconq\fP
that needs the curses terminal package only, but allows only one human
player.
.LP
In the standard game,
you start with one city and no knowledge of the world beyond your
immediate vicinity.  You must then explore, contact, and win wars against
all the other players, who are trying to do exactly the same things to you!
This is made harder by the limited information that the game supplies;
except for the vicinity of your own possessions (and for certain type of
units), the entire view
is out-of-date, and you won't see enemies until they're close by.
.LP
The "standard game" is played on a small (usually 60x30 hexes) randomly
generated map, against one machine player.  Your first
city will automatically start building your first military unit (usually
infantry).
When it is ready, the starting city will be overwritten by a picture of
the unit, which is itself surrounded by a box-shaped
cursor (the "unit cursor").
The mouse (or standard Unix direction keys) may then be used to designate
any location for that unit to move to.  This movement may take several
turns, or the unit may stop before it gets there, usually because it is
adjacent to something unfriendly.  To attack, just direct the unit to
move into a hex that shows another unit, and see what
happens (flashes, and maybe a notice at the top of the screen).
When you capture some kinds of units (usually cities),
\fIxconq\fP will ask you what sort of units
you want that unit to produce; '?' will display the possibilities.
.LP
In general, '?' will always work.  When typed during normal movement,
you will get a series of help screens, including commands, news, and
unit characteristics. (This info may be written into printable files if the
interface doesn't have the screen space necessary.)
'/' will tell you what you are looking at on the screen.
.LP
The foregoing is sufficient to play - just jump in and go!
After a few games, it should be clear what your units can and cannot do.
The game will end automatically when your opponents are no longer capable
of winning (either they have nothing left or they have given up).
The following sections contain many boring details, and should be referred to
for answers to questions.
.SH
DEFINITIONS OF TERMS
.LP
An xconq game involves several \fIsides\fP,
each of which has a human or machine player associated with it.
Sides may be enemies, allies, or neutral with respect to each other,
often start out in a hexagonal \fIcountry\fP.
A side owns a number of \fIunits\fP,
which includes the cities and the armed forces of that side.
Units may also be \fIneutral\fP, and belong to no side
(this is different from being on a neutral side).
Units may be inside other units, in which case the one inside is in
\fIoccupant\fP, and the other is a \fItransport\fP (even if it can't move).
Units always have \fIorders\fP that they follow, even when they appear
to be under manual control.  There are also \fIstanding orders\fP that
get passed to occupants automatically.
The game is divided into a number of \fIturns\fP,
during which each side gets to move some or all of its units.
All the action happens over a \fImap\fP of a real or imaginary world that is
divided into hexagonal shapes usually called \fIhexes\fP.
Each hex has a \fIterrain\fP assumed to cover the entire hex.
In some games, hexes also have a \fIpopulace\fP belonging to some side.
Terrain on the map can produce \fIresources\fP,
which are natural items ranging from water and food to gold and weapons;
resources being carried by a unit are also called \fIsupplies\fP.
\fIScenarios\fP are predefined games that set up maps, sides, units, and
\fIwin/lose conditions\fP, which define the circumstances under which
one or more sides win or lose in the scenario.
.LP
The numbers and kinds of units, resources, and terrain are not built in;
they are defined by a historical (or ahistorical!) \fIperiod\fP.
This means that the following sections will be somewhat vague on specific
units and behaviors, since
the information applies to times ranging from Ancient Greece to Star Wars.
Later sections will describe some of 
the periods that have been developed so far;
in addition, complete online help is available on the period in effect.
.SH
THE WORLD
.PP
.I
Geography defines the background to conflict.  Gold mines are usually in the
mountains, far from the sea.  Islands tend to be left alone, unless they
are on a direct path somewhere else.  A seacoast town can be strategically
useless if its approach is through shallow water.  Attention to terrain
and its effect on one's units can make the difference between
winning and losing.
.P
.LP
The world map on which you play is a cylinder of variable height and diameter.
Although it is always displayed as a rectangle, you can actually
circumnavigate the world.  The most northerly and southerly
rows of hexes are out of bounds.
Sizes can range from 20x20 "quicky" maps to the 
Earth at 1 degree resolution between
60 north and 60 south - no less than 360 by 120 hexes! 
When starting up, you have the
choice of several maps of real areas, depending on the period, or by
default you get a randomly-generated 60x30 map. You can get other sizes
from about 10x10 up to whatever your machine's memory can hold,
by using the \fB-M\fP option on the command line.
The \fB-m\fP command line option loads a named map,
and the \fB-x\fP option may also offer a menus of maps to use.
Predefined maps usually have their own documentation, which is displayed
on one of the help screens.
.LP
Each individual hex of the world contains one kind of terrain,
which is assumed to more-or-less cover the entire hex.
The exact set of terrains depends on the historical period; the set
below is from the standard period, and is actually shared by many
periods.
Monochrome \fIxconq\fP uses icons for each type of terrain, which cannot
possibly be described verbally, so use the help commands to decipher them.
.IP
Sea (dark blue) is assumed to be deep enough for any ship.  Armies can't
walk on water.
.IP
Shallows (light blue or cyan) include shoals, reefs, rivers, and any
other sort of shallow water.  They restrict movement of ships and
may entirely prevent passage of the largest ships.
.IP
Plains (light green) are generally flat and hospitable areas.
They usually offer no impediments to movement.
.IP
Forest (dark green) is dense forest or jungle, and may hinder movement for
some kinds of units.
.IP
Swamps (gray) are half water and half land, and impassable to almost
everybody.
.IP
Desert (yellow) ranges from Saharan sands to Sonoran cacti.  It is always
inhospitable but fast to move through - think of armor in North Africa.
.IP
Mountains (brown) are relatively barren and at higher elevation, thus are
also inhospitable to troops.
.IP
Ice (white) is deep snow, ice, and glaciers.  Only specially equipped
ground units can pass over it, although most aircraft can fly over.
.IP
Vacuum (black) is outer space, included for the purpose of doing futuristic
periods.
.LP
Each hex is adjacent to six others, and there is no special border to cross.
By default, hexes represent areas about 100 km on a side, although many
maps have larger or smaller scales.
.LP
Randomly generated maps vary in their "roughness", and in the percentages
of each kind of terrain.  These properties also depend on the period, and
it is worthwhile to have a general idea of the values.  Percentage coverage
is simple (for instance, the earth is 70% covered by water), but roughness
is more subtle; essentially the "jagginess" of the terrain.  Very rough
terrain has lots of sharp peaks and small islands, while smooth terrain
has large flat continents.
.LP
SIDES
.PP
.I
Politics provides the context for war; the war being a result of failed
policy.  The leader of a country is faced with the problem of achieving
certain goals, either externally- or self-imposed.  Diplomacy can sometimes
accomplish the desired outcome, and is much cheaper to boot.  When it fails,
one country or another declares war, and any alliances immediately broaden
its scope.  Declaring peace again is much more difficult...
.R
.LP
Sides in the game can be allies or enemies in various combinations.
Any two sides can form a formal alliance; human players do it by sending
the message "alliance" to each other using the message command (see below).
Machine players
are "aware" of their relative incompetence, and will usually ally with each
other (except in the case of a machine player attached to a display, so as
to facilitate debugging).
Players may become neutral or declare war by sending the messages "neutral"
and "war" to another side.
Scenarios may sometimes set up particular patterns of alliances, although
there is nothing to prevent the players from changing them around during
the game.
If all the sides left in a game are allied, then it automatically ends.
.LP
Some displays distinguish alignments by using the same colors for
allies as for yourself, while painting neutrals and enemies in distinctive
colors.  For others, you just have to remember who is on whose side.
.LP
Names of sides come from a scenario or are randomly generated from a list
of names, depending on period.  If the period doesn't define any names for
sides, then the list will be 100+ contemporary names (including Botswanans
and Peruvians).
Players may also rename themselves, using a command (see below).
Since it is usually hard
to remember which player has which name, many mentions of sides include
the display that the side is using (or nothing if a machine player), or
sometimes the number of the side (especially for input).
.LP
When a side loses, for whatever reason, units are either destroyed or made
neutral (depending on unit and period).  In the standard period, infantry
is destroyed, while cities become neutral (thus easy pickings for the
remaining players who get to them the quickest).
.LP
Informal
alliances frequently happen in games involving more than two people, so
I have a few words of advice.  First, an alliance between two of the
players is almost certain in a three-person game, and inevitably
results in the "odd man out" being quickly defeated.  In four-person
games, the alliances should be decided after looking at the map via "-v",
so that one pair is not hopelessly separated.  Five or more players is
going to be a free-for-all of formal and informal alliances.
Some scenarios are designed with a particular number of players in mind;
hopefully they will also have some natural balance.
.SH
UNITS
.PP
.I
War is based on the application of force to the other side, using
whatever is available; from spears and arrows to the high-tech equipment
available as a significant fraction of a nation's GNP.
.P
.LP
Units can be almost anything: armies, balloons, triremes, cavalry,
battleships, bridges, headquarters, cities.
Units move around, attack other units, produce resources, 
and build more units, among
other things.  Individual units occupy entire hexes, and no other unit
can enter that hex unless it can occupy or be occupied by the unit already
there.
.LP
Only some kinds of units can build other units,
at a rate depending on the period, the
unit being built, and the unit doing the building.
The first unit that is produced takes
somewhat longer, and the very first unit built by a side can take even
longer (research and development time),
but then succeeding units come out at a constant rate.
There is no memory about production, so switching to a different type then
switching back still incurs the extra startup time.
Most units that do building will do it all the time, and only stop when
explicitly directed to (such as cities), while others need to be directed
to build, and cannot move while doing so (such as engineers building a base).
.LP
Once created, a unit moves according to its orders, and subject to various
constraints - armies can't swim, ships can't walk, etc.
Units can sometimes be disbanded with a command (depending on the period),
by losing them
in battle, by running out of supplies, by being taken prisoner when a unit
is captured, or by garrisoning a captured unit.
.LP
Every unit starts out with a
number of \fIhit points\fP representing its strength,
and possibly supplies of
food, fuel etc that it carries around.
Supplies are used up by movement, combat, and by just existing, and are
created by production on certain terrain types, or by transference from
some other unit.
.LP
There is only one situation under which several units
can be in the same hex at once;
if one is a transport of some sort and the others are its passengers
or occupants.
The notion of "transport" and "occupant" is general, and covers
fighters on carriers, ships in port, bombs in bombers, and troops being
led by a general.
Occupants board by moving
into the hex occupied by the transport, but will refuse to go if the
transport is full or can't carry that type of unit.  Getting on board
takes a number of moves of that unit; if there are any left, it may move
off or take some other action.
Transports can also move over a occupant to take it on, but only if the
transport can move on the terrain that the occupant is on.
Occupants always move with the
transport (that's what transporting is all about!),
but may leave at any time if possible, either onto a valid terrain
or onto another transport.
To debark, just
move the unit off (the cursor indicates that the occupant
and not the transport is to be moved).  Usually, you will want to put the
occupant on sentry duty while moving the transport, and so must wake the
occupants up before they can be moved again.
.SH
THE GAME
.LP
Games may be predefined scenarios, which define the map, sides, and units,
or they may be randomly generated.  If randomly generated,
depending on the period, you start with either a country-full of units
or just one, which may or may not have its production defined already.
If you start with one, the period may also define some neutral units
in your country, which should be captured as quickly as possible.
.LP
Sometimes the map will be too small or have the wrong terrain, and then
\fIxconq\fP will fail at placement and exit instantly.  There is not much
you can do at that point except to try again or relax the constraints,
perhaps by reducing the number of sides or increasing the map size.
(This can also be fixed by altering the period - see the customization
document for details.)  Since there is a lot of randomness in placement,
second tries are frequently successful, although tenth tries usually indicate
a real problem.
.LP
A turn consists of several phases, although only one actually involves
player interaction:
.IP Spy\ Phase 5
Leakage of information from one side to another.
.IP Disaster\ Phase 5
Revolts, surrenders, attrition, and accidents.
.IP Build\ Phase 5
Construction of new units and repair of damaged ones.
.IP Supply\ Phase 5
Production and distribution of resources.
.IP Movement\ Phase 5
Automatic and manual movement of all units.
.IP Consumption\ Phase 5
Details relating to supply usage during movement.
.LP
During the movement phase,
the program iterates through all units, prompting each side to give
orders to any unit that is awake or becomes awake during the course of
its move.  One consequence is that you will not have a chance to change
orders, look around, or do anything else 
if no unit produces a unit and no units
wake up.  This speeds playing but can be annoying if you get overrun and
lose without ever getting a chance to respond (but do you deserve anything
else for pursuing a "hands-off" management strategy?).  Sides that
lose are automatically cut out of the game.  Since one additional iteration
is needed to verify that somebody lost, the final winner will have to go
through an entire turn before the game will exit (doing the sentry command
on everything is easy and quick).
.LP
The game ends when the win/lose conditions have been met; these vary
from scenario to scenario.
For a randomly-generated game, the end comes when no mutual enemies are left,
whether by elimination or by peace.
Usually this means
that only one side is left alive, but multiple machine players (not
associated with displays - the usual case) are always allied, and thus
may win as a group.  This also means that a single member of the alliance
will not resign until the position of the whole alliance is hopeless;
after all, the WWII Allies included several brigades of Polish troops
after Poland was overrun.
.LP
The last player must type a key to close down the
windows (this is so that they will stay up for everybody to look at).
When the game closes down, the winners (if any) will be listed.
If the STATISTICS option has been set by the installer, \fIxconq\fP
will write a file "stats.xconq" into the current directory.  This file
summarizes some crucial statistics concerning combat performance, losses,
and other miscellany.  It is quite useful for rationalizing your
humiliating defeat!
.SH
DISASTER PHASE
.PP
.I
War is inherently random.  Both military and civilian units desert,
get diseases, have accidents, defect, and surrender without a struggle.
These effects cannot be eliminated completely,
but can be reduced by keeping one's forces out of hazardous situations
and by keeping morale up.
.P
.LP
Three types of disasters can befall a unit during the disaster phase:
revolt/surrender, attrition, and accidents.
.LP
Revolts and surrenders are really the same sorts of occurrence; a unit
changes sides spontaneously, perhaps to neutrality, perhaps to the side
of a nearby enemy unit.  During every disaster phase, each unit makes
a revolt check.  The revolt chance is a hundredth percentage.
When a unit revolts, it changes to its original side (whatever the
unit started out as - i.e. your initial units will never revolt).
Occupants will either change over or be killed.
Any construction will be cancelled, unless the scenario is one in which
construction changes are not allowed.
.LP
Surrender only occurs if a unit is
capable of capture is present.  The capturing unit does not move.
Occupants of the surrendering unit also change over or die.
Chance of surrender is increased by low unit morale.
.LP
The chance of surrender can be greatly increased (depending on period)
by surrounding the unit completely.
This includes naval units
for any sea hexes.  One of the surrounding units must be capable (even
if only a small chance) of capturing the unit by direct attack.
The siege is only in effect
in those turns where the unit is completely surrounded.
When the unit surrenders, one of the "capture-capable" units will be randomly
picked to accept the surrender, and things happen as for a direct assault
(described below).  Note that if several sides are surrounding the same
unit, the selection is still random from among those sides, as long as the
side is an enemy.
.LP
Attrition is a "slow death" process applicable primarily to multi-hp
units.  It takes away some number of hit points
each time it occurs, and kills units only if they have no points left.
Attrition is also specified in hundredths of a percent,
and depends on unit type and terrain type.
Morale drops by 1 when attrition occurs.  A message will be displayed as well.
.LP
Finally, there is a chance for an accident to destroy a unit instantly and
totally.  Like attrition, this depends on both unit and terrain type, and
is measured in hundredths of a percent.  If the accident occurs, the unit
is killed along with all occupants.  A message will be displayed.
.SH
BUILD PHASE
.PP
.I
Sustained efforts in a war depend vitally on the replacement and
repair of forces destroyed or damaged in combat.  In total war,
the production base constitutes a chief strategic target, to be
isolated or destroyed if possible.  Repair of units is also significant
since a battle may result only in damage, but be successful nevertheless
if units must retire (as a cheaper alternative to new production).
Historically, battle damage has resulted in the termination of an
entire campaign.
.P
.LP
During the build phase, units construct new units and repair damaged
occupants or transports (or themselves).
.LP
Construction is straightforward; the schedule is decremented once/turn.
When it has counted down to zero, the unit is created, and placed either
as an occupant of the builder, or the builder is made to occupy the new
unit.  If neither alternative works (perhaps because the builder is full
already), then completion is postponed, and attempted on the next turn.
This will be repeated indefinitely.  If the new unit cannot be placed at
all, it is thrown away.  If the period specifies that the builder is to
guard the new unit, then the builder will be assigned to garrison the new
unit, and is destroyed.
.LP
Repair happens automatically if the damaged unit contains or is
contained by another unit capable of repairing, or if the unit can
repair itself.  The repair rate is depends on both the repairer and
repairee, and can happen no faster than one hp/turn.
.SH
SUPPLY PHASE
.PP
.I
The Allies floated to victory on a sea of oil.  -- LORD CURZON.
.P
.LP
Resources themselves are basically inanimate material that come in varying
amounts and are governed by conservation laws.  They can be produced,
moved around, and consumed during various activities.
Resources originate either as supplies carried by units at the outset,
or more typically, through production by units.  Production rate
depends on unit, resource, and terrain types, and is unaffected by
side changes, combat, or anything else.  Produced resources go into
the producing unit's storage.
.LP
Excess production is discarded, unless it can be unloaded into the
producer's occupying units, or distributed to nearby units via
\fIsupply lines\fP.  Supply lines automatically exist between units
that are close enough (as decreed by the period), and there is no
need for explicit manipulation.
.LP
Units consume their supplies, both in the course of existence,
and by motion/combat.
The rate depends on period and unit type; it consists of an overhead
consumed each turn without fail, and consumption for each hex of movement.
The total is a max, not a sum, since units with a constant consumption
rate are not likely to need additional supplies to move (consider foot
soldiers who eat as much sitting around as they do walking).
Supplies may also be consumed for production and repair, again depending
on period and unit types, but this consumption happens during the build phase.
Consumption is not affected by the situation of the consuming unit;
armies in troop transports eat just as much as when in the field.
.LP
Supply line length depends on the period and the units on both ends,
but is not affected by the intervening terrain.
Supply redistribution is managed by logistics experts, who are ignorant
of the war effort and seek only to even everything out.
The redistribution method is rather adhoc; units try to get rid of all
their excess supply, and try to take up supply from other units within
supply range.  Each direction is controlled independently, so for instance
airplanes can get automatically refueled from a nearby city, but not from
each other.  No unit will transfer all of its supply via supply lines.
Normally units in the same hex can exchange supplies, but some periods
can disable this behavior, so that explicit transfer using the give and
take commands is always necessary.
.SH
MOVEMENT AND COMBAT
.LP
The movement phase is the one in which all the action happens.
At its outset, the phase computes the number of moves available to
each unit.  This value is essentially the maximum of the unit's moves
on each type of terrain.  The movement phase continues until all these
moves have been expended in some way or another.
.LP
Some periods may define a small chance of random movement, in which the
unit moving actually goes in some other direction than that intended.
This is a potentially dangerous occurrence, since the unit will be
destroyed if the hex is impassable or contains another unit (whether
or not the other unit can take the moving one as an occupant).
.LP
All combat occurs during the movement phase.
Battles happen when one unit attempts to move onto a hex occupied by
an unfriendly unit.
In most periods, each unit attacks the other equally well, but if
"counterattack" is not enabled, then the defender just has to sit and
take the punishment.
The outcome
is determined independently for each unit, based on a probability table;
this means that both draws and mutual damage/destruction are possible.
The odds are the same whether a unit is attacking or being attacked.
Ammunition may be expended by each unit in each combat - if the ammo is
gone, then the attacker will not attack and the defender cannot defend itself.
The results are announced both by a message and by some flashes on the
screen (the size of the flash corresponds to damage seriousness).
Damage is assessed using hit points, and if the hit points are zero, the
unit is destroyed, along with any occupants.  Typically armies have only
one hit point each, so they are destroyed if hit.
Units with multiple hit points may be \fIcrippled\fP if their hit points
drop below a period-specified level.  Crippled units move more slowly
(in proportion to their damage), have reduced transport volume, cannot
repair anything, and do not make progress on any construction.
The final outcome of combat
depends on whether the defender was destroyed.  If so, the attacker will
move into the defender's position (if possible), otherwise no movement
will happen.
.LP
If a unit
is hit sufficiently hard, that is
considered a "nuke" and you get more spectacular visual effects, plus
the hex is converted into desert or something else desolate.
.LP
Some units are capable of capturing other units, with a probability depending
on the types of both units involved.  If the capture attempt is successful,
the capturer will move into the hex if possible, either as occupant or
transport.  In some periods, the capturer may be all or partially disbanded,
to serve as guards.
The regular attack as described above always happens first.
.SH
ORDERS
.PP
.I
A perennial feature of the highest level of command is its inherent
complexity.  Although the use of subordinates reduces the bewiderment
somewhat, the commander-in-chief must still keep in mind hundreds of
apparently unrelated facts; the state of the weather, the past performance
of units, the current goals of the war, and many other things.
It is very important that lower-level units be able to operate on their
own as much as possible.
.R
.LP
Although units have been said to "move", in actuality they follow orders,
some kinds of which specify movement.  When you are moving a unit
hex-by-hex, it is following the order "Awake",
which just means that every turn it asks what to do next.
There are many kinds of orders.  Some, such as movement in a given direction,
or to a given place, take parameters, but all take a repetition, which
tells for how many turns the unit will carry out the order.
(For some orders, the repetition is not particularly meaningful,
and is ignored.)
Repetition is always specified as a prefix numeric argument to commands.
.LP
Orders that a unit can do include:
.IP Awake 10
ask for a movement or other command.
.IP Sentry 10
sit at the present location as long as the repetition says.
.IP MoveDir 10
move in the given direction.
.IP MoveTo 10
try to move to the given location.
.IP FollowCoast 10
attempt to follow a coastline.
.IP FollowLeader 10
move towards another given unit (which must be one of your own).
.IP Patrol 10
go back and forth between two points.
.IP Embark 10
embark on the first available transport.  If no transport is
available, it will just wait.  Once it embarks, the remainder of its
time is changed to doing sentry duty.
.LP
Most movement commands just give these orders to the unit currently being
prompted about.  In addition, a unit may be given "standing orders", which
will be passed to any unit of a particular type entering or produced in that
unit.  This is useful for a variety of purposes, such as staging fighter
planes up to the front lines or sending ships out on standard patrols.
.so xconq2.ms
