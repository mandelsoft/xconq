.SH
PERIODS
.LP
Historical periods are the most complicated way to customize xconq,
and can be difficult to get right.
There are many hundreds of numbers,
each of which must be in balance with every other.  At best, a mistake will
result in a period whose winning strategies are quite simple;  at worst,
\fIxconq\fP will hang or core dump.
(\fIxconq\fP will make some efforts to check the numbers.)
Although most historical periods are defined 
in the period sections of mapfiles,
one period is compiled into \fIxconq\fP
(usually a version of WW II, since it is most familiar).
Compiling other periods (using the utility \fIper2c\fP) is straightforward,
and there is no problem with making unusual default periods for the
program, but in practice, periods do not take very long to read.
.LP
The header of the period section has the form
.IP
\fBPeriod \fIextension\fR
.LP
where \fIextension\fP is a flag indicating whether the period info
is to replace (= 0) or add to (= 1) any existing period info.
.LP
The remainder of the description is in a postfix language similar
to Forth or Postscript(tm), but even simpler.
\fIxconq\fP includes an interpreter
for this language, and program execution results in a filled-in period
description.  There is a compiler, called \fIper2c\fP; it is used
to produce a C version of the period definition which is then
compiled into \fIxconq\fP.
.LP
The objects of this language are quite simple.  There are only integers,
strings, and vectors of integers.  Integers and strings have direct
representation as tokens, as do symbols, while integer vectors must be
constructed using square brackets or one of the predefined vector-returning
words.  Tokens are always separated by whitespace, which means whitespace
may \fInever\fP be part of a token (backslash may be used for this purpose
at some future date).
Thus, the syntax for a token is fairly simple:
.IP
token -> [+-]*[0-9][0-9]*[%]* | "[~"]*" | [~"+\-0-9].*
.LP
In other words,
numbers are digits optionally preceded by +/- and followed by %,
while strings are anything enclosed in double quotes, and symbols are
anything else that is not white space.  A semicolon is the comment character;
the characters from the semicolon to the end of the line are discarded.
Numbers are limited to the typical range for signed shorts; -32768 to 32767.
The percent sign % is just for readability (since many numbers are
percentages), and its presence or omission has no effect on anything.
.LP
Numbers and strings merely get pushed on the stack, while symbols are
assumed to be defined words.  The reader looks them up in the dictionary
and executes the associated C function, if one is defined (otherwise, the
symbol gets pushed).  Nearly all words take a fixed number of arguments
from the stack and push nothing, although there are some exceptions.
The program is terminated by the word \fBend\fP.
.LP
Integer vectors are useful for filling in large parts of arrays.
They are individual objects, built using the words \fB[\fP and \fB]\fP.
.LP
Usage of these depends on the word, but typically words with two or more
arguments may include two vectors among them.  For instance, the line
.IP
\fB [ 1 1 2 8 ]  [ i a b B ]  hp \fP
.LP
sets the hit points for four types of units;
\fB1\fP for unit type \fBi\fP, \fB8\fP for unit type \fBB\fP, and so forth.
.LP
The words below generally follow some extremely regular patterns.
Most one-argument words define things for the entire period.
Most two-argument words define attributes of units or resources, and
may be thought of one-dimensional array operations, while three-argument
words usually fill in two-dimensional arrays.
In the two-argument and
three-argument cases, the first argument (deepest in the stack) is the
value, while second and third arguments are indices.
At present, there are no words with four or more arguments.
.LP
One-dimensional operations allow three cases of the four possibilities for
vectors and scalars: scalar value and index(set), 
scalar value/vector index (fill), vector value and index (mapping).
Two-dimensional operations offer six of the eight possible combinations
(encoded here with S for scalar and V for vector,
in the order value,index,index),
disallowing only a vector value with either two scalar or two vector
indices (VSS and VVV), since these combinations are not very meaningful.
The six other combinations are SSS (element set), SVS and SSV (row fill),
SVV (array fill), VVS and VSV (row mapping).  All of these combinations
are useful - see existing periods for examples of their use.
.LP
The following descriptions cover all the predefined words of the period
language.  Most words correspond to period attributes, and
thus have an associated default value; when not explicitly mentioned, the
default is \fB0\fP.
For arguments of type \fIbool\fP, both \fB1\fP and \fB0\fP or \fBtrue\fP
and \fBfalse\fP are valid (an argument characterized as \fIbool\fP may
still be a vector of 1s and 0s).  Arguments of type \fIn\fP, \fIn%\fP,
and \fIn.01%\fP are all just integers.
.LP
First, there are some useful words not specific to period
definition.
.IP \fBtrue\fR 5
.IP \fBfalse\fR 5
Pushes a \fB1\fP and a \fB0\fP on the stack, respectively.
.IP \fB[\fR 5
Marks the beginning of a vector.  The following objects must all evaluate
to numbers only.
.IP \fB]\fR 5
Marks the end of a vector.  All numbers going back to the last \fB[\fP
are popped and collected into a vector, which is pushed back on the stack.
.IP \fIvalue\ name\ \fBdefine\fR 5
Define the string \fIname\fP to be a word
that pushes the object \fIvalue\fP onto the
stack.  This is especially useful for assigning names to vectors, for
instance a vector of all cities or all ships.
.IP \fBprint\fP 5
Print the current contents of the stack onto standard output, surrounded
by /* */.  Very useful for debugging!
.LP
The global period definition words.
.IP \fIstring\ \fBperiod-name\fR 5
Defines the name of the period that will be displayed on startup.
Defaults to \fB"unspecified"\fP.
.IP \fIstring\ \fBfont-name\fR 5
Defines the name of the font that will be used.  This is not needed
if bitmaps are being used instead.  Defaults to the name of the text font.
See below for a discussion of bitmaps and fonts.
.LP
The words to define new kinds of units, resources, and terrain all add
new words that push the number of their unit,
resource, or terrain type onto the stack.  The numbering is in order;
the first of each type will be numbered \fB0\fP.  Most of the period
definition words use these values as indices when filling up arrays
of numbers.
.IP \fIchar\ name\ string\ \fButype\fR 5
Define a type of unit.
The arguments are unit character (a string of length one),
full name, and a
one-line help string.  The unit may thereafter be referred to by either
its character (as a one-char symbol) or by its name.
Things will not work if a blank or any other special characters (such as plus
and minus signs) are used as a unit character.  Mixing terrain characters
and unit characters is also a bad idea, since both may be used by a
graphical display.
At present, up to 30 or so unit types may be defined.
.IP \fIchar\ name\ help\ \fBrtype\fR 5
Define a type of resource, by both name and character, and supply a help
string for it.  The characters must be distinct from unit characters,
otherwise this word is identical to \fButype\fP.
.IP \fIchar\ name\ color\ \fBttype\fR 5
Define a type of terrain.  Unlike the other two definers, the terrain
character does \fInot\fP become a new word, although the name still does.
The character should be defined in the basic \fIxconq\fP font (the one named
\fBxconq.onx\fP, in the X10 interface), if the
period is to be used with an interface that needs it (monochrome X only,
at this writing).  The color may be either approximate ("brown") or exact
("#334455"), but in any case, the interpretation is up to the interface.
.IP \fBu*\fR 5
Push a vector of all unit types in order.
.IP \fBr*\fR 5
Push a vector of all resource types in order.
.IP \fBt*\fR 5
Push a vector of all terrain types in order.
.IP \fBnothing\fR 5
Push the index of a non-unit onto the stack.  This word cannot be used for
unit attributes, but it is useful for certain attributes whose values are
unit types (\fBfirst-unit\fP, \fBfirst-product\fP).
Use of this as an index with any word that sets unit attributes is
guaranteed to cause nasty coredumps...
.IP \fIstring\ unit\ \fBicon-name\fR 5
Set the name of the icon for the given unit type.  If undefined, then
the unit's character will be used in whichever font is being used for
unit icons (which may even be the text font - works but not too attractive).
Definition of an icon overrides any character in any font.
As with the terrain, the exact interpretation of this word is up to
the graphics interface - for instance, the curses interface ignores
both the \fBfont-name\fP and \fBicon-name\fP words entirely.
In X, the name is the name of a file in the usual bitmap format, as
produced by the \fIbitmap\fP program.  The actual file name is produced
by appending \fB".b"\fP for X10 bitmaps and \fB".b11"\fP
for X11 bitmaps.  The X11 interface will also look for X10 bitmap files.
.IP \fIterrain\ \fBdefault-terrain\fR 5
Set the type of terrain to be substituted while reading a map
with incomprehensible terrain characters.  Occasionally useful,
if the set of terrain types is a subset of the standard set.
.LP
Initialization characteristics of a period are used only during synthesis
of maps, sides, and units,
which happens when they are not supplied from a mapfile.
Periods that are used only with complete scenarios
need not use any of these words.
.IP \fIn%\ terrain\ \fBmin-alt\fR 5
Set the minimum percentile of terrain elevations that result in the given
terrain type.  Together with the other three words following, it is possible
to subdivide all the possible altitudes and moisture levels into different
kinds of terrain.  For instance, desert in the standard period ranges from
sea level (\fB70 desert min-alt\fP)
to high elevations (\fB93 desert max-alt\fP) but only
in the lowest percentiles of moisture (\fB0 desert min-wet\fP,
\fB20 desert max-wet\fP).  It is important that all percentiles be assigned
to some terrain type, or the map generator will complain; when designing
terrain combinations, it is helpful to make a graph with altitude percentiles
0-100 on one axis and moisture percentiles on the other.
.IP \fIn%\ terrain\ \fBmax-alt\fR 5
As for \fBmin-alt\fP, but set the maximum altitude percentile.
Defaults to \fB100\fP.
.IP \fIn%\ terrain\ \fBmin-wet\fR 5
.IP \fIn%\ terrain\ \fBmax-wet\fR 5
Set the minimum and maximum percentiles
of world moisture levels that result in the given
terrain type.  Deserts should range in the low percentiles (0-20), while
rain forests should be high (90-100).
The minimum defaults to \fB0\fP, while the maximum is \fB100\fP.
.IP \fIn%\ \fBalt-roughness\fR 5
Set a rather mysterious number that controls whether a random map tends
toward large continents or archipelagos of small islands.  It must range
between \fB0\fP and \fB100\fP.  Altitude roughness of \fB0\fP will result
in a map with one large continent, while \fB100\fP produces dozens of tiny
random islands, and drastic altitude variations from one hex to the next.
Defaults to \fB80\fP.
.IP \fIn%\ \fBwet-roughness\fR 5
Set the "moisture roughness", which is like altitude roughness, but affects
the distribution of wet and dry areas.  Defaults to \fB70\fP.
.IP \fIterrain\ \fBedge-terrain\fR 5
Set the type of terrain to fill in on the northern and southern edges of
a map.  Best user-friendliness is to have a type that is scarce elsewhere,
so the edges are not mistaken for normal terrain.
.IP \fIdistance\ \fBcountry-size\fR 5
Set the radius of a randomly-placed country, in hexes.
The country is always hexagonal in shape, and the center hex is not counted
in the radius.  The radius should be sufficient to accommodate all the
initial units, without crowding them.  For radius r, the number of hexes
is 3/4*(2r+1)**2.  Keep in mind that terrain may reduce
the number of available hexes even further.
If one type of unit can occupy another, then they are free to be placed in
the same hex.
Defaults to \fB3\fP.
.IP \fIdistance\ \fBcountry-min-distance\fR 5
.IP \fIdistance\ \fBcountry-max-distance\fR 5
Set the minimum and maximum
distances of country centers from each other, in hexes.
These values are sometimes tricky to set properly.
If too small, countries will mostly overlap;
if too large, then attempts to use
small maps will fail; if too close to each other, placements can also fail.
Default to \fB7\fP and \fB60\fP hexes, respectively.
.IP \fIunit\ \fBfirst-unit\fR 5
Set the type of unit that player will be started off in.
Setting this to \fBnothing\fP has the effect of giving every unit in
the country to the player at the outset.  Production will not be
set automatically, so this is not recommended for novices, who tend
to find large numbers of units confusing at the outset.
Defaults to \fBnothing\fP.
.IP \fIunit\ \fBfirst-product\fR 5
Set the type of unit that will be built automatically first.
A "cheap" (quick to build) type is usually best,
although interesting situations could result from, say, the
automatic production of one atomic bomb at the outset (note that all sides
start out in exactly the same way).
Defaults to \fBnothing\fP.
.IP \fIn\ unit\ \fBin-country\fR 5
Set the number of units of the given type in a player's country.  These
units are randomly scattered, with some bias towards the middle of the
country, and subject to terrain limitations via the \fBfavored\fP parameter
(see below).
.IP \fIn\ unit\ \fBdensity\fR 5
Set the total number of units appearing throughout the map, at the rate
of one per ten thousand hexes.  The numbers of units appearing in countries
is subtracted first,
so that the final density of units is independent of the
number of players.  If this value is nonzero, then at least one unit will
appear on the map, even if the map is very small (i.e. the calculation of
numbers rounds up not down).  Units not assigned to countries to meet 
the quota set by \fBin-country\fP always become neutral.
.IP \fIbool\ unit\ \fBnamed\fR 5
Set a type of unit to get a random name during initialization.  The
names are usually the names of towns,
so this flag should be used judiciously
(A battleship named "Wankers Corner" is only briefly amusing!).
The value can also be set to \fB2\fP, in which case the unit name
will be displayed by itself, without side name or unit type name.
.IP \fIname\ \fBuname\fR 5
Define the string \fIname\fP to be a plausible name for random assignment
to an initial named unit.  The name will be included with any previously
defined, including possibly ones in the compiled-in period.
This behavior, which is unlike any other period word except for \fBsname\fP,
is intended to ensure that each period need not define its own list of
names.
.IP \fBclear-unit-names\fR 5
Reset the list of unit names.  This is appropriate if the compiled-in list
is completely wrong for the period.
.IP \fIn%\ terrain\ unit\ \fBfavored\fR 5
Set the probability of the unit being on the given type of terrain at the
outset.  The default of \fB0\fP is an absolute prohibition against placing
the unit on that type of terrain, thus every period must specify at least
one non-zero value for some terrain type and some initial unit type.
(Note that this does not preclude a unit type with no favored terrain,
but it must be able to occupy some other unit already placed.  In fact,
this is a useful way to force one initial unit to start out inside another.)
.IP
This parameter is tricky to use properly, and not very flexible.  The problem
is a widely differing favored terrains for initial units may be too
constraining to work with the typical random map.  For instance, very few
small countries will include both ice and open sea, or deserts and forests
and swamps.  Failure to find such combinations will result in games exiting
while still initializing, thus frustrating erstwhile players of the period.
Best results will be had if the favored terrains are the same for all initial
types of units, and the terrain types are common on random maps.
(At present, the most-favored, or the lowest-numbered type among
equally-favored terrains should be plentiful on the map;
this is to get around a bug.)
.IP \fIn\ \fBknown-radius\fR 5
Set the area of the world known about, in those cases where the world is not
already known.  \fIn\fP is measured in hexes,
and represents a radius which will be seen around each of the starting units.
.IP \fIbool\ unit\ \fBalready-seen\fR 5
Set the type of unit that is or is not seen at the outset.
This should usually be true of things like cities, independently of their
\fBalways-seen\fP setting.
.IP \fIn%\ resource\ unit\ \fBstockpile\fR 5
Set the percentage of capacity for the given resource that each unit will
start out with.  Defaults to \fB100\fP.
.IP \fIname\ \fBsname\fR 5
Declare the string \fIname\fP to be a plausible name for random assignment
to a side.  The name will be added to the others already defined, including
ones from the compiled-in period.
.IP \fBclear-side-names\fR 5
Reset the list of side names.  This is appropriate if the compiled-in list
is completely wrong for the period, but if used, you must supply at least
as many side names as there are possible sides (7 or so).
.IP \fIn\ terrain\ \fBinhabitants\fR
Set the number of inhabitants in each hex of a country with the given type
of terrain.  The number is relative, and at present is only treated as a
boolean value.
.IP \fIn\ terrain\ \fBindependence\fR
Set the "independence" of the inhabitants in the given type of terrain;
how they react to enemy units in terms in attrition, supply, etc. [not implemented yet]
.LP
The first phase in a turn is devoted to spying.  This is the revealing of
all or part of a side's unit positions to another randomly-selected side.
It is controlled by only two parameters.
.IP \fIn%\ \fBspy-chance\fR 5
Set the percentage chance of spying occurring on this turn.  If the chance
is low, then the player doing the spying will get a message, otherwise the
display will be silently updated.  The player spied upon is never informed.
Defaults to \fB1\fP.
.IP \fIn%\ \fBspy-quality\fR 5
Set the percentage of enemy units that will be seen when spying is
successful.  Defaults to \fB50\fP (i.e. on the average about half of the
side's units will be seen).
.LP
.IP \fIn%\ \fBleave-map\fR 5
Can the units leave the map?

.LP
The second phase in a turn determines any revolts or surrenders, attrition,
and disasters.  Since these are (usually) rare events, the probabilities
are set in one-hundredth percent increments.  Revolts happen anywhere, while
surrender happens only if enemy units are nearby.  Attrition is the loss
of single hit points, without actually destroying a unit, while disaster
is the complete destruction of the unit (both of these depend on terrain).
.LP
Note that
with 100 units in play, the lowest possible nonzero chance of 1 for a value
still results in an occurence of that sort of
disaster every 100 turns or so, so these parameters require a "light touch".
.IP \fIn.01%\ unit\ \fBrevolt\fR 5
Set the base chance for the unit to revolt spontaneously in that turn.
This chance is reduced by better morale and maybe other things.
.IP \fIn.01%\ unit\ \fBsurrender\fR 5
Set the base chance for surrender to some adjacent enemy unit.
Each enemy unit present adds to the chance by this amount.
.IP \fIn.01%\ unit\ \fBsiege\fR 5
Set the additional chance for surrender when the unit is completely
surrounded by enemy units.  This is added to the basic surrender chance.
.IP \fIn.01%\ terrain\ unit\ \fBattrition\fR 5
Set the chance of a unit losing a single hit point while in the given
terrain.
.IP \fIn\ unit\ \fBattrition-damage\fR 5
Number of hit points lost when attrition happens.  Defaults to \fB1\fP.
Note that repair is in the following phase, and 1 hp of attrition damage
might be repaired immediately, and appear not to have happened.
.IP \fIstring\ unit\ \fBattrition-message\fR 5
Set what to say when unit is hit by attrition.
Defaults to \fB"suffers attrition"\fP.
If the string is \fB""\fP, then the message will be suppressed entirely.
.IP \fIn.01%\ terrain\ unit\ \fBdisaster\fR 5
Set the chance of completely losing the unit while in the given terrain.
Accidents should be restricted to definite hazardous situations, to go along
with movement constraints - for instance, carriers in shallow water should
move more slowly and have a nonzero accident rate.
See random movement for another way to achieve similar effects.
.IP \fIstring\ unit\ \fBdisaster-message\fR 5
Set what to say when unit is lost in a disaster.
Defaults to \fB"has met with disaster"\fP.
.LP
The next phase of a turn handles creation of new units and repair of damaged
units.
Units can only be created by certain other
kinds of units, limited both by time and raw materials.  Also there are
startup and research times.
.IP \fIn\ unit2\ unit\ \fBmake\fR 5
Set the time in turns needed for a unit of type \fIunit\fP
to build one unit of type \fIunit2\fP,
assuming sufficient resources to do so.
.IP \fIbool\ unit\ \fBmaker\fR 5
Set the unit type to be a "maker".  Makers always build unless
explicitly idled, and may move while building.  If a period starts with
no movers, then it needs at least one maker in the country, who will prompt
for a unit type at the beginning of a game.
.IP \fIbool\ unit\ \fBoccupant-produce\fR 5
If true, then a unit may produce as the occupant of another unit.
Default is FALSE.  Makers ignore this flag and always produce.
.IP \fIn%\ unit\ \fBstartup\fR 5
Set the extra time needed to build the first unit, if the maker was
producing something else before.  Startup time should be higher for
high-tech or large units, for instance to represent tooling or production
pipeline startup.
.IP \fIn%\ unit\ \fBresearch\fR 5
Set the extra time needed for a side to build the very first unit of
that type.  This time is in addition to the startup time for the first unit.
Long research time is a good way to keep a unit type out of play for awhile.
.IP \fIn%\ unit2\ unit\ \fBresearch-contrib\fR 5
Percent of research on \fIunit2\fP to count towards \fIunit\fP.  All
research contributions are summed and will never do more than
eliminate research on a unit type.  Only completed research is counted
(i.e., a unit must already have been produced).
.IP \fIn\ resource\ unit\ \fBto-make\fR 5
Set the total amount of a resource type needed to build a unit.
This amount is amortized over the normal construction schedule, which
means that extra resources are consumed by startup or research times
(representing mistakes and experiments).
.IP \fIn\ unit2\ unit\ \fBrepair\fR 5
Set the time needed for the unit type \fIunit\fP
to repair \fBrepair-scale\fR hit points of damage to unit type \fIunit2\fP.
One of the two units must be able to occupy the other;
It is also legitimate for a unit to repair itself.
If the unit being repaired was crippled,
its repair will require the same kinds and amounts of resources that
were used to build it.
.IP \fIn\ \fBrepair-scale\fR 5
Set the "scale" of repairs, meaning the overall amount that individual
repair rates are relative to.  So for instance a repair-scale of 4 means
that a repair time of 2 results in the recovery of 2 hp/turn.  Defaults to 1.
.LP
The supply phase of a turn handles both the production of resources and
their distribution via supply lines.  Resource production involves a
three-dimensional array indexing unit type, resource type, and terrain
type, but supply lines are measured only by length and resource type.
Supply lines are always interrupted by enemy presence.
.IP \fIn\ resource\ unit\ \fBproduce\fR 5
Set the basic amount of each resource produced by each unit in one turn.
.IP \fIn\ terrain\ unit\ \fBproductivity\fR 5
Set the percentage productivity of a unit on a type of terrain.
This is multiplied with the basic production rate to get actual
production, so productivity of \fB0\fP completely disables production on
that terrain type, and productivity of \fB100\fP is yields the maximum rate
specified by \fBproduce\fP.
.IP \fIn\ resource\ unit\ \fBstorage\fR 5
Set the unit's capacity to carry each sort of resource.
Amount carried does not affect unit's performance.
When the value is \fB0\fP, displays for that type of unit will not mention
this resource type at all.
.IP \fIn\ resource\ unit\ \fBconsume\fR 5
Set the amount of resources consumed by the unit in a turn, even if it
doesn't move or do anything else.  This includes riding as a passenger.
This only comes into play if the unit has used less than its base
consumption while moving.  In other words, the total supply usage for
one unit in one turn is max(#moves * to-move, consume).
If the unit runs out of a resource that it must consume,
it dies due to starvation.
.IP \fIn\ resource\ unit\ \fBin-length\fR 5
.IP \fIn\ resource\ unit\ \fBout-length\fR 5
These two are used together to determine the length (in hexes) of supply lines
between units.  The given type of resource can only be transferred from
unit type A to unit type B if the distance is less than the minimum of
the in-length of B and the out-length of A.
For instance, the in-length
for a fighter's fuel might be 3 hexes, while the out-length of fuel from
a city is 4 hexes.  If the fighter's out-length is 0, then it will be
constantly supplied with fuel when within 3 hexes of a city, but will never
transfer any fuel to the city unless it actually lands there.
An in- or out-length of \fB0\fP
means that the two units must be in the same hex,
while a negative length disables the automatic transfer completely.
Long out-length
lines should be used sparingly, since the algorithm uses the out-length to
define the radius of search for units to be resupplied.  Supply lines are
not affected by terrain at present.
.IP \fIbool\ unit\fBconsume-as-occupant\fR 5
If this is true, than this type of unit does not consume any supplies
as long as it is an occupant on some transport.  This is useful for
units such as planes which always consume fuel in the air but not on
the ground.  This defaults to TRUE.
.IP \fIn%\ unit\ \fBsurvival\fR 5
Chance that a unit type can survive on no supplies.  The test is made once
per turn.
.IP \fIstring\ unit\ \fBstarve-message\fR 5
Set what to say when unit has no more of some supply to consume.
Defaults to \fB"runs out of supplies and dies"\fP.
.LP
The movement phase is the main part of a turn in \fIxconq\fP, and the only part
involving interaction with players.  All combat happens during the movement
phase.
.IP \fIn\ unit\ \fBspeed\fR 5
Set the maximum theoretical speed of a unit, in hexes/turn.
If the unit cannot move on any sort of terrain, it will never be prompted
about - thus every period should define at least one type of moving unit.
.IP \fIn\ terrain\ unit\ \fBmoves\fR 5
Set extra moves used up on each type of terrain. \fB0\fP indicates no
decrease from theoretical max, \fB2\fP indicates a move into that type
of terrain uses up 3 moves instead of 1, and \fB-1\fP indicates that
movement on that type of terrain is not possible.
Defaults to \fB-1\fP.
.IP \fIn.01%\ terrain\ unit\ \fBrandom-move\fR 5
Set the randomness of movement of a unit on the terrain.  This is different
from disaster and attrition, since it is not always fatal, and happens only
during attempts to move.  However, collisions with other units or with
impassable terrain, due to random moves, are always fatal.
.IP \fIbool\ unit\ \fBfree-move\fR 5
Set whether the unit can move even if there is insufficient movement
allowance remaining in this turn.  Defaults to \fBtrue\fP.
(Most board wargames make this false - if you don't have enough movement
points to meet the entry requirement for a hex, that's too bad.)
Can be useful to make "double movement phases", if
attack time is equal to movement allowance; a unit can only attack units
that it is adjacent to at the start of the movement phase.
.IP \fIbool\ unit\ \fBone-move\fR 5
Set whether the unit can make exactly one move before dying (appropriate
for rockets and other automatic equipment). [not implemented yet]
.IP \fIbool\ unit\ \fBjump-move\fR 5
Set whether a unit can jump over another unit to get somewhere. [not implemented yet]
.IP \fIn\ resource\ unit\ \fBto-move\fR 5
Set the amount of resource used by a unit to move one hex.
The amount taken is independent of the terrain in the hex.
If the unit is out of any movement resource, it is immobilized
until it gets more.
.LP
Transportation-related parameters.  Capacity is measured both by number
and volume of occupants.  For instance, if you wanted a transport to carry
up to 8 infantry and/or armor, but no more than 4 armor units, then capacity
for infantry should be 8 and capacity for armor 4, the volumes for each 
should be 1, while the transport hold-volume should be 8.
.IP \fIn\ unit2\ unit\ \fBcapacity\fR 5
Set the basic carrying capability of a transport type \fIunit\fP
for its occupants of type \fIunit2\fP.
.IP \fIn\ unit\ \fBhold-volume\fR 5
Set the volume capacity of a transport.  Volume measure is quite arbitrary,
and is used only
for comparison.
The default value of \fB0\fP implies infinite capacity, volume-wise.
.IP \fIn\ unit\ \fBvolume\fR 5
Set the volume of a unit.  The volume of a unit may be smaller than its
hold-volume, the code will not care about this.
.IP \fIn\ unit2\ unit\ \fBenter-time\fR 5
Number of moves needed to enter a transport.  This is a time measure;
extra supplies are not used up.
.IP \fIn\ unit2\ unit\ \fBleave-time\fR 5
Number of moves needed to leave a transport; similar to \fBenter-time\fP.
.IP \fIn%\ unit2\ unit\ \fBalter-mobility\fR 5
Set the effect of an occupant on the transport's speed as a ratio of the
transport's usual speed.  Defaults to \fB100\fP;  smaller values slow the
transport, and \fB0\fP prevents it from moving entirely.  To simplify the code,
only the effect of one (randomly chosen) type of occupant has this effect.
If a transport has two types of
occupants each of which alter its speed differently, the resulting transport
speed will be unpredictable.
The total slowdown is multiplied by the number of occupants of all types.
.LP
Seeing is an important part of \fIxconq\fP, and needs parameters to accommodate
submarines, radar installations, and Indians hiding in the woods.
The visibility of a unit and the intensity of viewing are computed separately,
and compared to get the final decision on seeing something.  This doesn't
allow for much differential between two types of units viewing a third,
but that's life.  For units seeing things at a distance, the chances are
interpolated linearly, from the best conditions (adjacent hex) to worst
(maximum range).
.IP \fIbool\ \fBall-seen\fR 5
If true, then all sides see all of each other's units.  If secrecy unneeded
(as in a board game), this will speed up the display process somewhat.
.IP \fIn%\ unit\ \fBsee-best\fR 5
Set the basic chance of one unit seeing any other,
under best possible conditions.  Defaults to \fB100\fP.
.IP \fIn\ unit\ \fBsee-range\fR 5
Set the maximum distance in hexes at which the unit can see anything.
Defaults to \fB1\fP (adjacent hexes only).
.IP \fIn%\ unit\ \fBsee-worst\fR 5
Set the chance of seeing a unit at the maximum range.
Defaults to \fB100\fP.
.IP \fIn\ unit\ \fBvisibility\fR 5
Set the basic chance of a unit to be seen.
Crippled unit is more visible, in proportion to hp loss.
Defaults to \fB100\fP.
.IP \fIn%\ terrain\ unit\ \fBconceal\fR 5
Set the percent effect of terrain on seeing the unit.  This is subtracted
from the basic chance, since it is a "concealment factor".
.IP \fIbool\ unit\ \fBalways-seen\fR 5
Declare the unit to be of a type that is always seen and up-to-date.
This applies only to units whose underlying hexes have been seen.
This is useful for units like towns,
which are unlikely to disappear secretly.
.LP
Combat is part of movement, and has its own large set of parameters.
The basic plan of combat is for attackers and defenders to hit each other,
then attackers to attempt to capture.
Success of a hit attempt depends on a number of attributes, including
chances, terrain, and the availability of the correct sort of ammo.
.IP \fIbool\ unit\ \fBmulti-part\fR 5
Set a unit to be treated as an aggregate of smaller identical units.
Affects various things. [not implemented yet]
.IP \fIn\ unit\ \fBhp\fR 5
Set the maximum number of hit points for each part of a unit.
Defaults to \fB1\fP, may never be set any lower.
.IP \fIn\ unit\ \fBcrippled\fR 5
Set the hit point level below which the unit is considered to be crippled.
Below this level, repair and construction ceases, supply production is
reduced, maximum speed starts to decrease,
and the bridging capability is disabled.
.IP \fIn%\ unit2\ unit\ \fBhit\fR 5
Base chance of a single attack by the type \fIunit\fP
hitting the defender \fIunit2\fP, assuming the resources are available.
If chance to hit is \fB0\fP, attacker cannot attack or defend itself.
.IP \fIn%\ terrain\ unit\ \fBdefense\fR 5
Set the decreased chance of hitting if the defending type
\fIunit\fP is in that terrain type.
Percentage is subtracted from base chance.
.IP \fIn%\ \fBneutrality\fR 5
Set the change in defense for neutral units.
This is subtracted from chances to hit and capture, but the \fIn%\fP
can be negative, which would make it harder to hit/capture.
.IP \fIn\ unit2\ unit\ \fBdamage\fR 5
Number of hit points that the defender \fIunit2\fP loses when hit by
its attacker \fIunit\fP.
.IP \fIn\ \fBnuke-hit\fR 5
Minimum damage for a hit to qualify as a nuclear blast and be displayed
appropriately.  Default value is \fB50\fP.
.IP \fIbool\ unit\ \fBself-destruct\fR 5
Declare that unit self-destructs when it attacks.  This eliminates some
weird messages and hit chances.
.IP \fIbool\ \fBcounterattack\fR 5
When true, combat is two-way; the initiator of an attack is also hit by
a counterattack.  Otherwise, the defender must wait to get its revenge.
Defaults to \fBtrue\fP.
.IP \fIbool\ \fBcapturemoves\fR 5
When true, a captured unit can immediately be moved.  If false, then a
captured unit can not be moved until the next turn.  Default is TRUE.
.IP \fIbool\ unit\ \fBcan-counter\fR 5
Like \fBcounterattack\fP, but applies only to particular unit types
being attacked.
Defaults to \fBtrue\fP.
.IP \fIn%\ unit2\ unit\ \fBcapture\fR 5
Set the base chance of the type \fIunit\fP capturing the defender
type \fIunit2\fP.  This is conditional
on both attacker and defender surviving initial hits, and is modified
by morale and quality of both sides.
.IP \fIbool\ unit2\ unit\ \fBbridge\fR 5
True if the unit type \fIunit\fP can capture another unit
\fIunit2\fP, even across impassable terrain.
.IP \fIn%\ unit\ \fBchanges-side\fR 5
Set chance that the given unit will change sides if captured.
This is appropriate for units that are primarily hardware or
otherwise indifferent to their fate.  Units that are captured and
do not change sides become prisoners (prisoners are not implemented yet).
.IP \fIn\ unit2\ unit\ \fBguard\fR 5
Set the number of unit hit points required to garrison or guard a captured
type \fIunit2\fP, whether or not the captured unit has changed sides
(at present, it always does).
The hit point loss is permanent.
.IP \fIn%\ unit\ \fBretreat\fR 5
Set the base chance that a unit will retreat rather than be hit.
This choice depends on ability to move into an adjacent hex and on
morale, quality, and fatigue.
.IP \fIn\ resource\ unit\ \fBhits-with\fR 5
Set the amounts of each resource used as ammo by the unit.
.IP \fIn\ resource\ unit\ \fBhit-by\fR 5
Set the amounts of each resource necessary to score a hit on the unit.
This is correlated with the previous parameter to decide if right sort
of ammo is available for an attack.
.IP \fIn\ unit2\ unit\ \fBprotect\fR 5
Set the level of protection that \fIunit\fP offers to \fIunit2\fP.
Transports protect their occupants by only letting a percentage of
hits get through.  Occupants protect their transports by reducing the
chance of a hit and increasing chance of a counterattack.
(The default of \fB0\fP implies terrible carnage if a full transport is hit.)
.IP \fIn\ unit\ \fBcombat-time\fR 5
Set the extra number of moves used by an attack.
.IP \fIstring\ unit\ \fBdestroy-message\fR 5
Set what to say when a unit is killed in combat,
as an active verb for what the destroying unit has done to its victim.
Defaults to \fB"destroys"\fP.
.LP
General characteristics are not really classifiable anywhere else.
.IP \fIn\ unit\ \fBterritory\fR 5
Set the territorial value of a unit.  Primarily used by machine players
and win/lose conditions.
.IP \fIn\ unit\ \fBmax-quality\fR 5
Set the maximum quality achievable by a unit.
.IP \fIn%\ unit\ \fBveteran\fR 5
Set the effect of one point of quality on hit and capture chances.
.IP \fIn\ unit\ \fBmax-morale\fR 5
Set the maximum morale to which a unit can rise.
.IP \fIn%\ unit\ \fBcontrol\fR 5
Set the chance of a unit obeying its orders.  Defaults to \fB100\fP.
When the unit does not obey orders, it makes a decision using the machine
players' algorithm.
.IP \fIbool\ unit\ \fBcan-disband\fR 5
Set whether a 'D' disband command can be used to get rid of a unit.
It should not be possible to disband a city, for instance, to eliminate
it as a strategic target.  Note that the default of \fB0\fP effectively
disables the disbanding command entirely.
.IP \fIn%\ \fBefficiency\fR 5
Units disbanded in a transport can have the resources used to build them
reclaimed - this parameter sets the percentage that is actually obtained.
.IP \fIbool\ unit\ \fBneutral\fR 5
Set to \fBtrue\fP if unit can exist as a neutral.
If \fBfalse\fP, then anything that
would cause the unit to become neutral (revolt, surrender of owner)
has the effect of removing it instead.  Defaults to \fBfalse\fP.
.LP
Miscellaneous words.
.IP \fIn\ \fBhostility\fR 5
Set the level of hostility exhibited by a population toward a unit
from some other side. [not implemented yet]
.IP \fBbegin{notes}\fR 5
Declare the beginning of the designer's notes.  This word kicks in a
special reader that absorbs all lines until it sees the line
"\fBend{notes}\fP".  The intervening lines are saved as period notes and
listed out in \fB"parms.xconq"\fP.
The notes should rationalize the design and discuss features
of special interest to the player.
.IP \fBend\fR 5
Marks the end of the period description.
.LP
Nearly all the elementary programming errors are checked, such as stack
over/underflow, and
as many of the period parameters as possible will be checked, although
there is plenty of room for subtle loopholes.  You should think carefully
about the consequences of each parameter, being particularly sensitive to
degenerate winning strategies.  Most common are units that are too powerful,
or that are built so quickly that they overwhelm any opposition.  The
players should always be a little "hungry"; not able to get quite as much
of units or resources as they would really like.
.LP
Although there are many interesting possibilities inherent in this
period description language, you should avoid making the period too
complex to be humanly playable.  The compiled form of the period description
can involve over 16,000 individually settable numbers, each with an expected
range of perhaps 100 distinct values.  It is clearly possible to spend many
years exploring a single set of these numbers.  For more playable and
enjoyable games, either pick a single aspect to treat in detail, or else
do all aspects in a simplified way.  Aspects could include exploration,
logistics, naval operations, "shoot-em-up", renditions of familiar board
games or even some team sports (rugby for instance).  Another thing to
keep in mind is that the introduction of a new type may have far-reaching
consequences - a new unit type will need its interactions with \fIall\fP
other unit types defined.  One approach is to introduce a new type as a
slight modification of an existing type, then to share most of the
definitions.
.LP
Something else to keep in mind is that the period parameters have been
chosen for their ability to combine in interesting ways, rather than for
obvious usefulness.  For example, past startup, the production rate for
units is constant and unending.  But suppose you want to put a limit on
the numbers of that type of unit?  One way is to define a resource that
is essential for construction of that type, let the builder have an initial
supply, but provide no way to get more of that resource.  When it runs out,
no more units!  Another trick is to motivate an activity by making it a
prerequisite to the basic builtin goal of defeating the other player.
The age of discovery worked this way.  The kings of that time weren't
interested in new lands per se;  they wanted exploitable possessions that
could be used to get gold to buy armies big enough to defeat their neighbors.
The period language could describe this situation almost exactly, by making
gold a resource obtainable only by the capture of neutral mines thinly
scattered over the map.  Be inventive!
Studying the predefined periods should reveal a number of tricks.
.LP
Completely new periods usually have a number of bugs.  The tools are
rather limited, but then most of the bugs are fairly obvious.
The \fBprint\fP word is useful for examining the stack, and a number of
errors (such as stack overflow/underflow) have messages.  Finding out
where the problem occurred requires the use of the \fIxconq\fP debugging
flag \fB-D\fP, which has the effect of listing out each period token as
it is read.  This can also be used with the period compiler, which starts
up faster; invoke it as \fB"per2c -D <newperiod.per"\fP.  The most serious
problems with periods are play balance issues.  Some can be found out by
watching a machine player, since its decisions are based on perceived values
of the units.  The most subtle bugs can only be uncovered by extensive play
interspersed with judicious alteration of parameters.  I find it useful to
play for a while, then go over all the period parameters, thus avoiding
tweaking one parameter only to find that it results in another being
inconsistent.  Parameters interact in many ways - you should keep this in
mind when experimenting.
