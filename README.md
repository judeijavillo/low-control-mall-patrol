# Low Control Mall Patrol 
Team Members: Faris, Tony, Caroline, Brian, Jude, Bryce, Matt, Alina, Sh'yee

Gameplay Prototype Release:
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
