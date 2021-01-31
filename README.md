# Phantcli - ncurses client for Phantasia 4

This is an alternate client for Phantasia 4 <https://www.phantasia4.net> using ncurses.

## Building
So far, I have only tried this on Linux. There is no autoconf or meson at the moment; after cloning the repository, just go into the src directory and run make. You will need ncurses development headers to be installed. Optionally, copy the phantcli binary into a directory on your path (ie, /usr/local/bin).

## Running
By default, the client will connect to phantasia4.net on port 43302. This can be changed via the -h and -p command line options.

Much of the interface is menu-driven and should be self-explanatory, but a few things need explaining.  
When there is only one button available (typically "More"), the client will just print something like, "--More--". At that point, pressing spacebar will advance the game.  
For the main menu (when not fighting a monster, inside a trading post, etc), it is possible to move, in addition to selecting one of the presented options. The keys to do this are as follows and will be familiar to anyone who has played a Roguelike:
* y: move northwest
* k: move north
* u: move northeast
* h: move west
* . or spacebar: rest.
* l: move east.
* b: move southwest.
* j: move south.
* n: move southeast.
