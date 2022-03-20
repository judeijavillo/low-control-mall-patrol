# Low Control Mall Patrol 
Team Members: Faris, Tony, Caroline, Brian, Jude, Bryce, Matt, Alina, Sh'yee

## Technical Prototype Release:
Due to recent merging, this build is not guaranteed to compile on all platforms,
but it is guaranteed to compile for Apple.

When playing alone, the player starts off as the thief. As a dev command, double
tapping switches control to one of the cops. Note this command only works on
mobile. 

When playing with multiple people, the thief is randomly assigned amongst the
players, and the remaining players are the cops. 

Cop tackle is now availble by swiping in a direction as a cop. Note, tackle is
not currently being sent over the network, so victory/defeat may look pretty
jank on multiplayer. Also note that the deterministic networked physics hasn't
been added to the project yet, so multiplayer movement will likely look a little
shaky.

Debug mode is on, because we do not yet have proper textures for traps. Traps
are indicated by the debug polygons that have no textures. Traps are represented
with two polygons: the trigger area and the effect area. For the sake of
consistency, the trigger area is the smaller polygon. To activate a trap,
press spacebar as the thief while in the trigger area.

Currently, the example1 map is loaded, which has some randomly placed walls for
the sake of testing JSON loading, as well as some traps directly north of the
thief's starting position. To switch to a map that is a little more adequate 
for multiplayer gameplay, change lines 61 and 63 of LCMPConstants.h to replace 
instaces of "example1" with "example2".

The controls and objectives are the same from the previous release, with the
additional following features:
- Walls are loadable from JSON, and the map is limited to a finite space
- Double tap to switch between thief and cop control (mobile only, dev command)
- Player animations (timing is still being worked on, but animations do work)
- Automatic reset after some time after game over
- Directional indicators for the thief to see where the cops are
- Distance indicator for the cops to see how far away they are from the thief
- Cop tackle (swipe on mobile, not yet availble on desktop)
- Traps (spacebar to activate on desktop, not yet available on mobile)
- Randomization for determining who plays as the thief
- All five players are playable (but network connection and UI feedback for it 
    is still to be desired)

## Gameplay Prototype Release:
Due to recent merging, this build is not guaranteed to compile on all platforms,
but it is guaranteed to compile for Apple. For the sake of consistency, for now,
the host is always the thief and the client is always the cop.

Cop Movement:
- Mobile: Tilt the device to move
- Desktop: Arrow Keys to move

Thief Movement:
- Mobile: Drag the joystick to move
- Desktop: Arrow Keys to move

Objective:
- Thief: evade cops (timer not built in yet)
- Cops: collide with thief 

Unfortunately, resetting was causing some weird errors during the merge, so to 
play again, for now you'll need to start the application again.

Note that since collision processing came after the networking commit, there is
a chance that one player ends the game by making contact, but the other player
doesn't recognize the collision (authority issue to be addressed in the next
sprint).

The host can start the game without other players joined (i.e. the start button
should be activated), but you will always be playing as the thief.
