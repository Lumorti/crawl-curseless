It's a fork of DCSS, but with the curses.h/windows.h dependency removed, so it renders all the text to stdout and takes the input from stdin. This makes it easier to integrate with other projects, specifically when dealing with pipes. Although it's basically unplayable directly.

Other changes:
 - full description now shows friendly monsters if the filter matches
 - full description now shows clouds if the filter matches
 - we refresh the screen more often (e.g. after level up)
 - we optimize with -O3 and -march=native regardless
