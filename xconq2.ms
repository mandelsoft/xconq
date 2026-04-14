.SH
THE DISPLAY
.PP
.I
All warfare is based on deception.  -- SUN TZU
.P
.LP
When a game is started up, it opens a
number of windows, of which the most important is the area map (which
therefore gets the largest window).
Above the area map are several windows for status
and notifications, and next to those is a turn counter and a list of all
sides in the game. 
The middle right side has a list of all unit types, used for statistics
display, while the lower right-hand corner has a map of the world
(if the display is sufficiently large).
.LP
All of the \fIxconq\fP windows
are actually subwindows of a main window with
a patterned background you can see here and there.  You can iconify and move
the main window, and the subwindows will keep their relative positions.
The largest of these is the map, which is a (typically) 30x30 section of the
world in full detail.  The view is scrolled around as necessary (remember
that the world is cylindrical).
.LP
To the right and down, you see a map of the whole world.  This view is like
the close-in map, but units and units are rendered as solid blobs, since
the world is too large to permit any detail.  To assist in matching up the
two different views, the world map includes an outline box indicating
the position of the close-in view.
.LP
Three text displays are stacked at the top of the screen.  The uppermost
is about ten lines of notices about various occurrences, each prefixed
by the number of the turn in which it was issued.  The display scrolls.
The next four lines are an information window that summarizes the status of
the unit or unit at the current cursor position.  It can display info about
enemy things also, but of course the amount of information is less.
Finally there is a one-line
prompt window just above the map, in which all questions and prompts appear.
.LP
The list of sides playing appears in the upper right corner.  Sides that have
already lost appear with a line through them,
while the currently moving side has a "*" next to it.
Your own side name is highlighted or inverted.
The name of the side, its host (if any), and the number of that side are
shown.  On color displays, the color of the number indicates the alignment
of that side.
.LP
The X interface
draws the area map as a number of hexagonal shapes with icons for
units superimposed.
Unknown territory is black.
Your own possessions appear in black,
neutral units in gray, and all enemies in red.  If there is more than
one enemy side, they are distinguished by the number of their side in
the upper right corner of the icon.  Not all enemy units
will be visible; the chance of seeing one may be very low, or depend
on viewing with the right type of unit.
Monochrome screens display enemies and
neutrals as inverted from your own appearance, and all enemy units/units
have numbers, to distinguish them from the neutral units.
.LP
The curses interface displays each hex as two characters side-by-side.
Terrain is a character representing the terrain, as are units.  The second
character in a hex is either the side number of an enemy, or an apostrophe
for neutrals.
.LP
As mentioned previously, the view is a record of what has already been
seen, but is not updated except in the immediate vicinity of your own
units and units.  In multi-human games, all screens will be kept
up-to-date simultaneously, so that persons waiting for their turns
can see enemy units moving around, units change hands, and so forth.
The previous view of the hex is also remembered and will be described
by moving the cursor to a hex in survey mode.
.SH
INPUT
.LP
Input may be supplied both from the mouse and the keyboard.
Moving the mouse cursor to a screen position and clicking either button
has the effect of issuing MoveTo orders to the current unit, which will
be carried out until successful.  There are two exceptions.  The first is
that if the mouse is on the unit itself, the unit sits where it is until
the next turn (same as the ' ' command below).  If the desired new
position is adjacent, the unit will unconditionally move there - this is
useful for attacking enemies.  At present, there is no special meaning
attached to particular mouse buttons.
.LP
As an alternative to using the mouse,
the standard directions (h = West,
l = East, y = NW, u = NE, b = SW, n = SE) can be used to specify movement.
Uppercase versions of these makes the unit move forever in that direction.
Letter directions are really only of use when the mouse fails, or for diehard
Unix game players for whom the direction keys have been permanently wired
in the brain!  Note that in a hexagonal system, 'j' and 'k' are not
meaningful.
.LP
Any command may be prefixed by a single numeric argument (which may be
positive or negative).
Not all commands will
use this number, while others need a number to know how often to repeat
an order, or perhaps for some other reason.
The "current unit" is the one
being prompted about, while the "main unit" is the one occupying the
hex itself (as opposed to its occupants).
.LP
Commands to give units orders typically default to a repetition of 100 turns
(In some cases, this is meaningless, as in moving to a place):
.IP s 6
Sentry; the current unit "goes to sleep",
only wakes up by explicit command or when an enemy pops into view
(the enemy unit won't necessarily be adjacent, if the unit on sentry
duty can see far-off hexes).
.IP w 6
Wake up; the unit's orders will be erased
(whatever they were) and it will ask about its next order (not
always immediately).  This command interprets an argument as a radius for
the effect of waking up; for instance, the default of 0 means to wake only
the unit itself, 1 means to wake up adjacent units as well,
15 will wake up a screenful of units, and 999 will usually wake all units.
.IP W 6
Wake all; both the unit and all its occupants will be woken up, as
well as all of their occupants, recursively.
The command is otherwise identical to 'w'.
.IP Space 6
Sit; unit goes on sentry duty for exactly one turn, and will ask for
a move in the next turn.  Useful for waiting one or two turns.
.IP r 6
Return; unit returns to nearest transport by shortest route.
It will not return to transports with no room or no supplies.
Most useful for aircraft, but works with any unit.
.IP m 6
Move to a position; this is equivalent to mouse clicking, but can be used
with mouse-less interfaces or to move further than one screen width.  You will
be prompted to do movement commands (either mouse or keyboard), then can use
the space bar to designate the final destination.
.IP f 6
Follow leader; follow another unit.
The program will ask you to designate a unit to be followed, which must be
one of your own.  The interaction is identical with that for 'm'.
Each turn the unit either attempts to move
towards its given leader, and sits if it is within a couple hexes of
the leader.  Units will not follow themselves.
.IP F 6
Follow coast; follow a coast line.  This can be applied to any unit, although
the unit will immediately wake up again if it is not next to some sort of
terrain that it cannot move into.
The command will prompt for a standard direction to decide how to start, then
a contour-following algorithm will continually try to keep the unit adjacent
to impassable terrain (thus armor might use this command to go around a
mountain range or forest).  Because the terrain is in discrete hexes,
it is possible for the unit to get confused, but that's life.
.IP Z 6
Patrol; set the current unit to go back and forth between two points.
One point is the unit's current position, and the other will be prompted
for.  The roundtrip will be repeated for the number of times designated
by the command's argument.
.LP
Commands for modes.  There are only two modes defined at present:
move mode (the default) and survey mode.
Most commands work the same in both modes.
.IP z 6
Survey mode; toggle into/out of survey mode.  In survey mode, movement pushes a
cursor and allows you to look at things.  The other commands are still
available; for instance, you can 
give a unit new orders or to set unit production.
.LP
Commands for general manipulation of units:
.IP d 6
Delay move; unit's movement is delayed until all your other units have been
moved, then it will be prompted for again.  Useful in crowded situations.
Delay can be used on any number of units any number of times during a turn.
.IP P 6
Set unit production; will ask for a type of unit (if a choice possible) 
and then schedule construction
for a unit of that type.
Any partial production will be discarded.
.IP ^P 6
Set unit production automatically; will use the machine algorithm to
determine the product.  
.IP I 6
Idle;  cancel production for the given unit and leave it idle for a while
Default is 100 turns, argument to command overrides.
.IP C 6
Call unit by name; prompts for a string by which current unit
will be referred to.
If string is empty, unit name will be removed.
If this command is done when the cursor is on an empty hex, the string will
become the new name of the whole side instead.
.IP D 6
Disband; unit disbands and disappears from game.
Not all units can be disbanded; for instance, most periods will not allow
the voluntary destruction of a city.  If an occupant is disbanded, then its
transport will get any available resources, both those held as supplies and
any used in making the disbanded unit (possibly not all, depending on the
period's "efficiency" parameter).
.IP a 6
Cycle through occupants; this can only be used in survey mode, and allows
examination of each occupant and its suboccupants.  The order of traversal
is depth-first, and cycles through all units in the hex repeatedly.
.IP x 6
Mark a unit; used with embarkation, below.  Only one unit is ever marked at
any one time.
.IP e 6
Embark; put the current unit onto a random transport in the same hex.
This is useful
when you don't want to move transport or unit out just for the
purpose of boarding.
If the marked unit is in the same hex,
it will be used as the transport.  May given argument to have unit
wait that many turns for a suitable transport.  Once it is on board,
it goes to sentry duty for the remainder of its time.
.IP g 6
Give supplies; transfer all types of supplies from the current unit to its
transport, if there is one.  The default is to try to fill up the transport
if possible.  If an argument is supplied, it means to transfer exactly that
quantity of each resource type.  If the current unit is low on some type,
then it will transfer half of what was requested.  (Repeating the command
transfers half again, and so forth.)
.IP t 6
Take supplies; transfer all types of supplies to the current unit from its
transport, if there is one.  The default is to try to fill up the current
unit, or to interpret the argument as the quantity to take.  If the transport
is low, then it will only transfer half of what was requested.
.IP O 6
Set standing orders;  will ask for type of unit to which standing orders
will apply, then goes into a "teach mode"; the next input will saved as
an order rather than being applied to some unit.
When any unit of the appropriate type enters the unit with the standing order,
it will be given those
orders and carry them out.  There is no way to cancel standing orders
at present, but occupants can be set to wake up during entry.  A unit
receiving a standing order will automatically leave automatic mode,
unless the standing order puts it in automatic mode.  All standing
orders are displayed before the unit type is prompted for.  If no
change is desired, then exit the command by typing the escape key.
.IP G 6
Give unit;  give the current unit to the side specified by the argument.
If the side is invalid, then the unit is made neutral.  Not all unit types
can be given away.
.IP ^A 6
Automate unit;  allows the machine to control this unit until it is
explicitly woken by the user.  New orders are only given after the
current orders have expired, allowing the user to give order in survey
mode.  A standing order can be left to automate a unit.  Such an order
may be combined with another order, but must be left as the second
order for the same unit type.  If both a normal order and an automate
order are present, the regular order is carried out and then new
orders will be given by the machine.
.LP
Commands for side manipulation:
.IP c 6
Center; the list of units is sorted so that the one at the current cursor
will move first, and others move in concentric circles outward.  This is
useful for concentrating on one particular area and reducing the amount of
map redrawing.
.IP M 6
Message; send a message to another side.  The side is specified by giving its
number as a prefix to the command; if the number is not the number of a side,
then your message will be broadcast to all sides (including yourself).
You may type in a message up to the
length of the prompt window.  Backspacing is available.
When a newline is entered, the message is sent immediately to
the destination.
Specially recognized messages must be typed exactly, with no other words
or characters in the message:
.IP war 12
Declare war.  Only one side need do this.  This involves all allies on both
sides immediately.
.IP neutral 12
Declare neutrality.  Both sides must send this message to each other.
.IP alliance 12
If two sides send this message to each other, they become formal allies.
The display changes to reflect this, as do things like wakeups, etc.
All sides in the game will hear about the alliance.
.IP briefing 12
Sends the view of all of your units to the other side.  Useful for allies,
as well as to convince a victim that further resistance is hopeless.
Of course, the victim's position might not be so hopeless after all, in
which case you've given away all your secrets!
.IP . 6
Recenter; put the hex with the cursor in it in the center of the screen.
.IP "^F" 6
Follow Action; toggle look mode.  Look mode puts any changes in the
view on the screen when it is not your turn.  The mode window will be
followed by a "L" in look mode.
.LP
Commands for game control.
.IP X 6
Resign; resign from the game, (asks for confirmation first).
The effect is the same as losing.
.IP Q 6
Quit; terminate the game for everybody (asks for confirmation first).
Note that although this can be used even in multi-human games, applying it
without prior consent of the other players is definitely anti-social!
.IP S 6
Save game; record the game state into a file and exit
(asks for confirmation first).
The saved game is ASCII and unprotected,
so it's possible to "checkpoint" games and engage in other kinds of cheating.
The game exits once it has been saved.  To restart, start up the program
without any command line arguments, and in the directory where the save
file is located.  If players are specified on the command line, then they
override the saved player data.  This is one way to switch sides; for instance,
saving from "xconq" and restarting with "xconq -r -e 1 $DISPLAY" has the
effect of you switching sides with the machine.
.IP A 6
Add player; add a new player to the game (not implemented yet).
.IP o 6
Options; set various options.  Each option is a single character.
Options at present include:
.IP g 12
Graph; toggle between text and bar graph displays about the current unit's
supplies, hit points, etc.
.IP d 12
Display mode; cycle between four different color display techniques.
The curses interface also has two display modes (one or two terrain chars
per hex), but you still have to cycle between four modes.
.IP i 12
Invert; invert foreground and background colors everywhere (monochrome
only).
.IP w 12
Width;  set the width of the map display to be the value of the argument.
This is measured in hexes.
.IP h 12
Height;  set the height of the map display to be the value of the argument.
This is measured in hexes.
.IP n 12
Notices;  set the number of notice lines at the top of the screen.
.IP r 12
Robot; convert yourself into a machine player.  This asks for confirmation.
If there are no other human players,
^C is re-enabled, so you can terminate the program.  This may be
reversed by typing Escape. (Not functional in curses version.)
.IP m 12
Monochrome; This has the bizarre behavior
of converting a color display into its monochrome equivalent.
Actually intended for debugging, but pretty flashy if you're bored.
.IP b 12
Start Beep Time; A beep will sound at the start of your turn if it is
more than this many seconds since your last turn.
.LP
Information commands.
.IP ? 6
General help; show a sequence of help screens, starting with a list
of commands, then a display of icons, then any news, then general
info about the period, then the characteristics
of each unit (as for '=' below).  You may page back and forth through the
screens.
This general help is available in both move and survey modes.
Some specialized prompts (such as for unit type) will
also recognize '?', but will only supply more details about possible answers
to the prompt.
.IP / 6
Identify; display a short phrase indicating what is being seen
in the hex at the current cursor position.  This works in all modes,
and is useful for deciphering unusual colors or icons.
.IP = 6
List the characteristics of a type of unit.  It will prompt for the type,
then format all the period-specific details into a semi-readable summary.
To get a hardcopy of this, use 'p'.
.IP p 6
Print; dump all the characteristics of all unit types into a file
"parms.xconq".
This file may be printed, and is very useful for learning about a period.
It will include designer's notes about the period which cannot otherwise
be obtained.
Also print the current view, and a list of the commands.
.IP v 6
View current unit; display a flash that should be bright enough to
catch the eye and make it easier to see where the current unit is.
.IP V 6
Version; display the current version and other useless information.
Be sure to include the version number when reporting bugs.
.IP "^R, ^L" 6
Redraw the screen.  Redrawing happens automatically most of the time.
Keep in mind that \fIxconq\fP is a single program, despite opening multiple
screens, and attempts to redraw may be ignored for awhile.  An
argument to redraw will erase the views of mobile units that have not
been seen in as many turns as the argument.  ^R defaults to erasing
units we haven't seen in 10 turns.
.IP "T, ^T" 6
Find a unit.  T always prompts for a unit type while ^T will use the
current unit type, if any.  This will find a unit of the given type,
or the next for ^T.  An argument to the command gives the number of
a particular unit.  With no argument, ^T can be used to cycle through
all units of one type.
.IP # 6
Determine distance; will prompt for another location and will print
the distance to that location.
.LP
Additional commands are available for building scenarios, and are described
in the customization document.
.SH
PERIOD HELP
.LP
The help screens describing unit characteristics include an enormous amount
of information.  In fact, a period that utilized the full range of capabilities
would be too complicated to play, even as a computer game.
As a result, the help screen are rather compact and cryptic.
For any single unit, there are three tables, summarizing the unit's
relationships with resource, terrain, and other unit types.
The numbers in parentheses are default values that fill in
any blank entries.
.LP
Resources:
.IP ToBui 8
Amount of resource needed to build the unit.
.IP Prod 8
Amount produced each turn under best conditions.
.IP Store 8
Amount that can be carried around.
.IP Eats 8
Minimum amount consumed during a turn.
.IP ToMov 8
Amount consumed by moving one hex.
.IP Hits 8
Amount needed to hit another unit.
.IP HitBy 8
Amount needed to be hit by another unit.
.LP
Terrain:
.IP Slowed 8
Move penalty for entering hex with the given terrain type.
Default is negative, which prevents movement entirely.
0 means can move in at maximum speed.
.IP Rand% 8
Chance (in hundredths of a percent) to move randomly in
the terrain.
.IP Hide% 8
Increased difficulty for others to see unit in this terrain.
.IP Defn% 8
Increased difficulty for others to hit unit in this terrain.
.IP Prod% 8
Productivity of this terrain for resource production.
.IP Attr% 8
Chance (in hundredths of a percent) for attition to occur.
.IP Acdn% 8
Chance (in hundredths of a percent) for an accident to occur.
.LP
Other units:
.IP Hit% 8
Chance to hit a unit of that type.
.IP Damg 8
Number of hit points of damage done when hit successful.
.IP Cap% 8
Chance to capture unit.
.IP Guard 8
1 if capturing unit converted into garrison.
.IP Pro% 8
Percentage of hit that is prevented from hitting the
unit type that occupies, or decrease in chance of hit
on unit type transporting this unit.
.IP Holds 8
Number of units that can be carried.
.IP Enter 8
Extra moves consumed by entering the transport type.
.IP Leave 8
Extra moves consumed by leaving the transport type.
.IP Mob% 8
Transport mobility when carrying unit type.
.IP Bridg 8
1 if can attack unit type across impassable terrain.
.IP Build 8
Basic construction time for the unit type.
.IP Fix 8
Time to repair one hit point of damage to the unit type.
.LP
The customization document has additional explanation for
some of these (rather obscure) parameters.
.SH
HINTS
.PP
.I
Generally in war the best policy is to take a state intact; to ruin it
is inferior to this.  -- SUN TZU
.PP
.I
Attack where he is unprepared;
sally out when he does not expect you.  -- SUN TZU
.PP
.I
There has never been a protracted war from which a country has
benefited. -- SUN TZU
.P
.LP
The works of Sun Tzu and Clausewitz say nearly all there is to
be said on strategy in general.
\fIXconq\fP strategy is fairly close to real strategy.
.LP
The most important consideration is to conceal your own
forces and movements as much as possible.  Decoys and feints are worthwhile
if they don't draw critcial strength away.
.LP
Secondly, don't rush to attack with weak forces.  Especially over long
distances, the defender has the advantage.  Wait until you have assembled
enough to take and hold a piece of territory, then allow some extra, just
in case.
.LP
Make a plan, and have some contingency plans ready as well.
.LP
Be ready to take advantage of opportunities.
.SH
PERIODS
.LP
\fIXconq\fP starts with one period compiled into it.  It can also read
and interpret other periods.  Typically the installer will have built
in the period called "standard", for which the description is included
below.
Other periods include Napoleonic times, Ancient Greece, a somewhat silly
futuristic period, an even sillier "Star Wars" period, whose sole reason
for existence is to watch death stars blast cities, a "flattop" period
featuring carriers, some simulations of board games, and more.
.LP
The standard period
represents units of about 1945, from infantry to atomic bombs.  This is
the most familiar, which makes it easier to play, but also more controversial,
since historians have many conflicting theories about which kinds of
units were most effective.  This set has been most influenced by other
empire games (thus the greater variety of ships),
and the numbers have been honed by extensive playing experience at Utah.
.IP
Infantry.  The infantry division is the slowest of units, but it can go almost
anywhere.  It is also quick to produce.  Infantry is the staple of
campaigns - a little boring perhaps, but essential to success.
.IP
Armor.  The armor division is highly mobile and hits hard.  Unfortunately,
it is limited to operating in open terrain - plains and desert.  It also
takes longer to produce.  Armor can last twice as long in the 
desert as infantry.  Both armor and infantry can
assault and capture units; they are the only units that can do so.
.IP
Fighters.  A fighter is a squadron or wing of high-speed armed aircraft.
Their fuel supply can be gotten only at units, towns, and bases, so they
must continually be taking off and landing.  Fighters are not too effective
against ground units or ships, but they eat bombers for lunch.  Fighters
are very good for reconnaisance - important in a game where you can't always
see the enemy moving!
.IP
Bomber groups.  Bombers are very powerful, being capable of destroying any
unit.  Attrition rate in such activities is high,
so they're not a shortcut to victory!  Bomber performance against
other units is less impressive, and of course fighters love to munch on them.
Bomber range is greater, but they are slower, taking several turns before they
must land.  They are also a last-ditch method to escape from a continent
if no seaports are available.
.IP
Destroyers.  Destroyers are fast small ships for both exploration and
anti-submarine activities.
.IP
Submarines.  The favorite food of submarines is of course merchant shipping
and troopships, and they can sink troop transports with one blow.
Subs can't be seen by the other side,  although their
presence might be suspected.  Subs are always highly vulnerable to attack
by bombers or even fighters.
.IP
Troop transports.
This is how ground units get across the sea.  They can defend
themselves against ships and aircraft, but are basically vulnerable.
They're not very fast either.
.IP
Aircraft carriers.  Compensates for the fighter's limited range by providing
a portable airport.  Carriers themselves are sitting ducks, particularly
with respect to aircraft.  Fighter patrols are mandatory.
.IP
Battleships.  The aptly named "Dread Naught" has little to fear from other
units of this period.  Subs may sink them with enough effort, and a group
of bombers and fighters are also deadly, but with eight hit points to start,
a battleship can usually survive long enough to escape.
Battleships are very effective against units and armies, at least the
ones on the coast.
.IP
Atomic bombs.  The Final Solution; but they are not easy to use.  A bomb
takes a long time to produce, moves very slowly by itself, is easily
destroyed by other units, and reduces the range of bombers that
carry it.
The plus side is instant destruction for any unit of any size!
.IP
Bases.  To simplify matters, this can serve as a camp, airbase, and port.
Bases cannot build units, although they can repair some damage.
.IP
Towns.  Towns are the staple of territory.  They produce units at the
same rate as cities, but are easier to capture.
.IP
Cities.  Cities are very large, powerful, and well defended.  They are
basically capital cities, or something in a comparable range.  (New York
and San Francisco are cities, Salt Lake City and San Antonio are towns.)
A city is worth five towns, territory-wise.
.LP
Current thinking about optimal strategy for this period differs.  In general,
blitzkrieg works, and can win the game in a hurry.  The problem is to
muster enough force before striking.  One full troop transport is not
enough; the invasion will melt away like ice cream on a hot sidewalk, unless
reinforcements (either air or land) show up quickly.  Air cover is very
important.  While building up an invasion force, airborne assaults using
bombers and infantry can provide useful diversions, although it can be
wasteful of bombers.  Human vs human games on the default map generally
last about 100 turns, usually not enough time or units to build atomic
bombs or battleships, and not a big enough map to really need carriers
(although bases for staging are quite useful.)
.SH
CHANGES FROM VERSION 1
.LP
Aside from the significant changes (hexes, simultaneity, period descriptions),
there are a number of smaller changes that will affect experienced players:
.IP
The command to construct a base is now the same as the general build
command \fB'P'\fP (since bases are units like any other).
.IP
The random movement command is gone.
.IP
The disband command is now \fB'D'\fP instead of \fB'd'\fP.
.LP
There are probably others I have forgotten about.
.SH
ACKNOWLEDGMENTS
.LP
Special thanks must go to Eric Muehle, now at Martin-Marietta, who has been
a tireless source of ideas, advice, and playtesting.
Mohammad Pourheidari, Bob Kessler, Kevin Deford, Spencer Thomas, Dan Reading,
Mark Bradakis, Grant Weiler, Jed Krohnfeldt, Sandra Loosemore, Jimmy
Miklavcic, Tim Moore, and others
at Utah have also endured
initial playtesting, with the apparently endless bugs and
misfeatures.  Thanks also to Leigh Stoller, who suggested using X,
and to Harold Carr, who suggested the postfix language for period
descriptions.
.LP
Since the first release, many many \fIxconq\fP 
players around the net have sent
in literally hundreds of suggestions, fixes, and improvements.
Significant contributors include Jim Anderson at CMU; Jay Scott at Swarthmore,
who designed the "future" period; John Tonry at MIT, who supplied a great
map derived from JPL data; Kurt Hoyt at SEI, who did an X11 port; Julian
Onions at Nottingham;  Ravi Subrahmanyam at MCNC;  and Joel Rives at Georgia
Tech, who is working on a large period.
Chris Peterson at MIT and Tim Moore at Utah have been essential
to the construction and debugging of the X11 interface to version 5.
In addition, A.G. Hirai, Jeff Kelley, John Shovic, Dave Pare, Michael
Lounsbery, Josh Siegel, Fred Douglis, Cimarron Taylor,
and Rick Ledoux have shared
a number of good ideas, although not all of them made it into this version.
