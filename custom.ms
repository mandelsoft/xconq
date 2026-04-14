.TL
CUSTOMIZING XCONQ (Made Somewhat Simple)
.AU
Stan Shebs
.AI
Department of Computer Science
University of Utah
.AB
For advanced players who desire new challenges, \fIxconq\fP provides several
customization mechanisms.
It is possible to build new maps, scenarios, and historical periods.
This document describes the customization process in excruciating detail.
The reader is assumed to be familiar with \fIxconq\fP and its basic
documentation.
.AE
.SH
INTRODUCTION
.LP
\fIXconq\fP has been designed to facilitate the construction of new maps,
scenarios, and historical periods.
All these additions happen through a single mechanism, the \fImapfile\fP,
which comes in several parts, and may also load other mapfiles.
Saved games are very detailed mapfiles; less detailed mapfiles are more
suitable as scenarios.  Maps and period descriptions are mapfiles with
most sections missing.
.LP
Mapfiles are created either by text editing,
by \fIxconq\fP, or by a combination of both.  
Since they are mostly read by variations on C's \fIscanf\fP function,
the format is rather rigid; numbers may not be omitted, no blank lines
are allowed (but whitespace between numbers on a line is OK),
and mistakes will not always be recognized.
The one general exception is the period description, which is read by
a lexical analyzer, which allows free-format input.  Other more specific
exceptions will be noted below.
.SH
MAPFILES
.LP
A mapfile is a normal ASCII text file.  It begins with a header line of the
form
.IP
\fBXconq \fIn [+-][+-][+-][+-][+-][+-] description[;]\fR
.LP
where \fIn\fP
is the number of additional mapfiles to be loaded (up to 16 or so in all),
each + or -
indicates the presence or absence of a particular section, and the
\fIdescription\fP
is free-form text (remainder of the line) describing the mapfile.
Note that that all reading/writing in the mapfile is case-sensitive,
including the header words.
The sections are for version, period, map, globals, sides, and units,
in that order.  Most sections vary in size, and some vary also in
the detail of the information they contain.  Most include extension
flags, which are reserved for future expansion of mapfiles.
.LP
Additional notes may be included by terminating the \fIdescription\fP
with a semicolon (with NO whitespace following).
In such a case, the following text lines
will skipped over until a period "." is seen by itself on a line.
.LP
Following the header are the names of the \fIn\fP
mapfiles, one to a line.
Each will be loaded into \fIxconq\fP before the rest of the current mapfile.
This process is recursive (at least in theory).  Any conflicts are
resolved by simple overwriting.  Note that this presents the possibility
of catastrophic conflicts if the designer is not careful to, say, match
units and periods.  A file with a given name will be loaded only once;
a second attempt will result in a warning message, and the program will
continue on.
.LP
The version section is quite simple, since it is one line;
exactly the version
string displayed when \fIxconq\fP starts up.  This is typically only needed for
saved games, but might be convenient while \fIxconq\fP versions are changing
incompatibly.
When this section is loaded, the version is merely compared with the
internal version number, and \fIxconq\fP fails if they do not match.
.LP
The period section is a program in a simple postfix language,
which when executed fills in the period description.
.LP
The map section includes the world dimensions and terrain.
It may also include information about countries at some future date.
.LP
The globals section includes any global values, such as the current turn,
and also the win/lose conditions for a scenario.
.LP
The sides section defines the sides in a game.  Required info is little
more than the name of the side, but a detailed section may also include
the area of the map seen by that side, and other data.
.LP
Finally, the units sections lists a number of units in the game.
Again, detail can range from type, name, and position to complete
details on its orders.
.LP
Each of these sections is entirely optional - \fIxconq\fP can synthesize its
own replacements for each.  For instance, a mapfile could specify a map
and two sides, or a period, or an entire game, by including the appropriate
sections.
The sections do interact somewhat, with dependencies as follows:
.IP
Maps, win/lose conditions in globals,
detailed sides, and units depend on period.
.IP
Detailed and/or non-neutral units depend on sides.
.LP
The following sections discuss each part of a mapfile in more detail.
The version secton
has already been covered sufficiently.
Since the period defines what may appear in all the other sections,
I shall begin with it.
.so period.ms
.SH
MAPS
.LP
Maps alone are the easiest thing to customize.  You can either do ordinary
text editing to acquire the map data, then add the headers, or use the
online terrain editing capability of \fIxconq\fP and save the terrain alone.
.LP
The map header has the form
.IP
\fBMap \fIwidth height scale seen extension\fR
.LP
where \fIwidth\fP and \fIheight\fP are the size of the map,
while the \fIscale\fP is
the width of a single hex in km (measured between parallel faces).
The \fIseen\fP flag is \fB0\fP if the map is not known to the players,
and \fB1\fP if it is (this is like using \fB"-v"\fP on the command line).
The \fIextension\fP
is an extension flag, with a value of \fB0\fP for the normal map.
Other values are reserved for extensions.
.LP
The preferred way to build a map is to use the \fB"-B"\fP
option of \fIxconq\fP.
This enables the space bar to "paint" terrain onto the map.
The "brush" is defined by using the terrain characters as commands
(they will override any normal commands on the same letters).
The argument is also useful - if positive, it is the radius of the
area to be painted with the given terrain type, if negative,
it is the length of a horizontal bar.  Thus, in the standard period,
the sequence \fB"999. "\fP will convert all but the largest maps into open sea.
To save the map by itself, answer with the string "m" when "saving the game"
(described in more detail below).
.LP
The other way to build is by using a text editor and following the format
exactly.
The terrain data is a number of long lines of characters.  The length of
the lines must be exactly the width of the map.  The characters for
terrain types are defined by the period.
The coordinate system is Cartesian oblique, with the y axis tilted to form
a 60-degree angle with the x axis.  Thus, landforms in the mapfile should
appear to be leaning to the left, if they are to appear upright during play.
.LP
Maps should have some distinguishing terrain
on the northern and southern edges;  also, remember that the default
map shape wraps around in the east-west direction, so landforms should
match up.  If the wraparound is undesirable, a vertical stripe of
some otherwise-unused type (such as outer space) is useful to block movement.
.LP
Run-length encoding is also available.  It is flagged by a numeric digit,
followed by any number of digits, followed by a terrain character.  The
terrain character will be replicated the number of times specified by the
digit string.  The encoding may be freely intermixed with normal terrain,
but cannot extend over line boundaries.
.SH
GLOBALS
.LP
Globals include a number of values affecting an entire game.  The header
is in the following form:
.IP
\fBGlobals \fIgametime endtime setproduct leavemap numconds extension\fR
.LP
where \fIgametime\fP is the elapsed turns, \fIendtime\fP is the last turn
of the game, \fIsetproduct\fP controls whether unit construction is changeable,
\fIleavemap\fP allows units to leave by the map edges,
and \fIextension\fP is reserved for extensions (is normally \fB0\fP).
.LP
\fInumconds\fP specifies the number of winning/losing conditions.
Conditions are evaluated at the end of each turn (along with other
ways to lose, such as losing all of one's units).
A win condition results in the side achieving it winning
while all others lose.
A losing condition knocks the side out of the game, and the other sides
continue normally.
Multiple win and lose conditions
act as disjunctions - when one is satisfied, something will happen.
There is a limit of about 10 conditions or so.
All conditions include a starting and ending time for
when they are in effect, as well as the number of a side to which it
applies, or \fB-1\fP it is to be applied to all sides equally.
.LP
The first line of a condition is the same for all types:
.IP
\fI win/lose type start end side(s)\fR
.LP
where \fIwin/lose\fP is \fB1\fP if the condition is for winning
and \fB0\fP for losing,
\fIstart\fP and \fIend\fP are the starting and ending turns for testing
the condition, \fIside(s)\fP is the number of a side to which the condition
applies (or \fB-1\fP if it applies to any side), 
and \fItype\fP is the type of condition:
.IP \fB0\fP
Territory; next line is one number, the amount of territory
(above if to win, below if to lose).
.IP \fB1\fP
Number of units; next line is list of numbers, one for each unit type.
To win, must possess at least that many of each type.
To lose, must be at or below that many for \fIall\fP unit types
simultaneously
(the default losing condition is this one, with all zeros).
To make this condition apply to only one unit type, set all the other
numbers to very low (for win condition) or very high (for lose condition)
values, outside the range normally occurring during a game.
.IP \fB2\fP
Quantity of resources; next line is a list of numbers, one for each
resource type.  Winning and losing is same as for unit types.
.IP \fB3\fP
Possession; next line is three numbers,
the first two representing the coordinates of a hex, and the third
the number of a unit type (or \fB-1\fP to indicate all unit types).
To win, must have a unit of the specified type on the hex.
To lose, do not have a unit on that hex.
.LP
Each condition has a sort of dual identity, since it is interpreted
slightly differently, depending on whether it is flagged for winning
or losing.  The logic is a little twisted perhaps - special conditions
should have all cases tested carefully before a scenario is released.
.SH
SIDES
.LP
Sides need relatively little information stored about them, particularly
for a scenario.  The header is simple:
.IP
\fBSides \fInumsides detail extension\fR
.LP
where
.I numsides
is the number of sides recorded, and
.I detail
is the level of detail that was recorded.
.LP
The side's name can be set using the naming or \fBC\fPall command, when
the cursor is not over a unit, or if an argument is supplied (this will be
the number of a side, and can be used in build mode to change the names
of other sides).
.LP
The exact contents of each level of detail are as follows:
.IP \fB1\fP
Name of side only is saved.  This is sufficient for many scenarios.
.IP \fB2\fP
Name, attributes, and political status and production counts (used for
numbering unnamed units) saved.
.IP \fB3\fP
All of above, plus view of explored part of world.
.IP \fB4\fP
All of above, plus hosts and player types, plus statistics if used.
.IP \fB9\fP
All possible data.  This is the level used for saved games.
(At the moment, this is equivalent to level 4, but may include more data
in the future.)
.LP
The data for each level begins on a new line, and some levels need several
lines (such as the view data).  The best way to study the layout is to
examine a saved game.
.SH
UNITS
.LP
Units have quite a few attributes, nearly all of which must be saved.
The header is as simple as for sides:
.IP
\fBUnits \fInumunits detail extension\fR
.LP
where \fInumunits\fP is the number of units saved, \fIdetail\fP
is the level of detail that was recorded, and of course \fIextension\fP
is the usual extension with only a value of \fB0\fP.
There are several levels of detail:
.IP \fB1\fP
Type, name, and position, and side.
The re-created units will be fully supplied and awake.
Neutral units have a side of -1.
Units capable of construction will be idled, so there should be at
least one movable unit present.
.IP \fB2\fP
All of above, plus scalar attributes and supply amounts, all on one line.
(Look at a saved game or the source code to interpret the numbers.)
.IP \fB3\fP
All of above, plus orders and standing orders if defined.
.IP \fB9\fP
All possible data.  This is the level used for saved games.
(Same as level 3 at present.)
.LP
Build mode offers a number of way to manipulate units on any side,
as described in the next section.
.SH
SCENARIOS
.LP
The \fB-B\fP command line option starts up \fIxconq\fP in \fIbuild mode\fP,
where many
additional actions are possible, all oriented towards the editing of
game state.  All commands can be performed on any side's units, and
none of the machine players will move (this can be toggled by using the
quit command 'Q', then saying 'n' to the confirmation question).  Build mode
allows some other acts:
.IP
Move any unit anywhere instantly, using the moveto command 'm'.
.IP
Create any unit anywhere, using '\\' (prompts for unit type, argument
specifies side, defaults to neutral).
.IP
Modify terrain anywhere, as described under the map section.
.IP
See all units everywhere, using 'V'
.IP
Do anything to any unit as if it were your own, using the usual commands.
.LP
Once all desired changes have been made, you may wish to allow machine
players to move a little, just to randomize things a bit.  Then use the
normal game save command.  In build mode, you will be asked to enter a
string indicating sections and levels of detail.  The string should contain
a character for each section you want written out - 'g' for globals, 'u'
for units, and so forth.  The letters 'u' and 's' must each be followed by
a digit indicating the level of detail desired, as described earlier.
The default string is \fB"ms1u1"\fP, which will write a mapfile with the map,
side names, and unit types/names/positions/sides, which is sufficient
for many interesting scenarios.
The file that will be written is always called \fB"random.scn"\fP,
is written into the current directory, and should be renamed as desired.
After sufficient playtesting, it may be added to the scenario menu, just
by adding its name to the file \fB"mapfiles"\fP in the library directory.
.LP
Scenario construction is not for everybody.  Since the processes are
semi-internal, the error-checking is not as extensive.  For instance,
you can load a submarine with battleships as passengers.  There are
also more subtle questions of balance, which are usually revealed by
repeated play of the scenario.  As a rule, the lower levels of detail
are safer to use - level 1 details for sides and units are often
simple enough to be typed in or edited by hand.
.SH
EXAMPLES
.LP
A period description with only infantry and cities:
.DS
Xconq 0 -+---- Very simple test period
Period 0

"generic" period-name

"standard" font

"i" "infantry" "moves around" utype
"@" "city" "makes infantry units" utype

"+" "land" "green" ttype  ; must always have at least one terrain type

1 @ in-country
100 land @ favored

@ first-unit
i first-product

1 i @ make
true @ maker
1 i @ capacity  ; makers need to be able to hold or be held by products

1 i speed

0 t* i moves

begin{notes}
This is just a test.
Kids, don't try this at home!
end{notes}

end
.DE
A tiny map with two cities, including an "empire" period description to ensure
meaningful city definitions:
.DS
Xconq 1 --+--+ Tiny map;
just an example...
.
empire.per
Map 9 5 100 1 0
:........
:.+++++..
:..+^^^..
:...+~~..
:........
Units 2 1 0
@ New Cork 2,3 -1
@ Old York 4,1 -1
.DE
.SH
LOSSAGES
.LP
Several words are marked NIY, meaning that although they can be used,
the code does not actually take them into account.
