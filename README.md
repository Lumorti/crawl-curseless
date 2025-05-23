It's a fork of DCSS, but with the curses.h/windows.h dependency removed, so it renders all the text to stdout and takes the input from stdin. This makes it easier to integrate with other projects, specifically when dealing with pipes. Although it's basically unplayable directly. Also a bunch of other tweaks to make processing this output easier.

Full changelist:
 - removed curses dependency, now reads/writes to std::cout
 - full description now shows friendly monsters
 - full description now shows clouds
 - check to revive in wizard mode removed
 - we refresh the screen more often (e.g. after level up)
 - we optimize with -O3 and -march=native 

Also, this also includes a http server version, so that it can be played on Quest 2/3 with the game running on a seperate device. Note that the server is just acting a local http server sending the terminal output of crawl-curseless and recieving the player's inputs.

The server uses [this](https://github.com/yhirose/cpp-httplib) library, licensed under MIT.
