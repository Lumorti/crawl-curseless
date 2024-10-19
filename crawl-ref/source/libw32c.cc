/**
 * @file
 * @brief Functions for unix and curses support
**/

/* Emulation of ancient Borland conio.
   Some of these are inspired by/stolen from the Linux-conio package
   by Mental EXPlotion. Hope you guys don't mind.
   The colour exchange system is perhaps a little overkill, but I wanted
   it to be general to support future changes.. The only thing not
   supported properly is black on black (used by curses for "normal" mode)
   and white on white (used by me for "bright black" (darkgrey) on black

   Jan 1998 Svante Gerhard <svante@algonet.se>                          */

#include "AppHdr.h"

#define _LIBUNIX_IMPLEMENTATION
#include "libunix.h"

#include <iostream>
#include <cassert>
#include <cctype>
#include <clocale>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <cwchar>
//#include <langinfo.h>
//#include <term.h>
//#include <termios.h>
#include <unistd.h>

#include "colour.h"
#include "cio.h"
#include "crash.h"
#include "libutil.h"
#include "state.h"
#include "tiles-build-specific.h"
#include "unicode.h"
#include "version.h"
#include "view.h"
#include "ui.h"

//static struct termios def_term;
//static struct termios game_term;

#ifdef USE_UNIX_SIGNALS
#include <signal.h>
#endif

#include <time.h>

// replace definitions from curses.h; not needed outside this file
#define HEADLESS_LINES 24
#define HEADLESS_COLS 80

// TODO1
#define cchar_t char32_t
std::vector<std::vector<cchar_t>> screenArray;
int curX = 0;
int curY = 0;

// for some reason we use 1 indexing internally
//static int headless_x = 1;
//static int headless_y = 1;

// Its best if curses comes at the end (name conflicts with Solaris). -- bwr
//#ifndef CURSES_INCLUDE_FILE
    //#ifndef _XOPEN_SOURCE_EXTENDED
    //#define _XOPEN_SOURCE_EXTENDED
    //#endif

    //#include <curses.h>
//#else
    //#include CURSES_INCLUDE_FILE
//#endif

static bool _headless_mode = false;
bool in_headless_mode() { return _headless_mode; }
void enter_headless_mode() { _headless_mode = true; }

// Globals holding current text/backg. colours
// Note that these are internal colours, *not* curses colors.
/** @brief The current foreground @em colour. */
//static COLOURS FG_COL = LIGHTGREY;

/** @brief The default foreground @em colour. */
//static COLOURS FG_COL_DEFAULT = LIGHTGREY;

/** @brief The current background @em colour. */
//static COLOURS BG_COL = BLACK;

/** @brief The default background @em colour. */
//static COLOURS BG_COL_DEFAULT = BLACK;

struct curses_style
{
    //attr_t attr;
    short color_pair;
};

/**
 * @brief Can extended colors be used directly instead of character attributes?
 *
 * @return
 *  True if extended colors can be used in lieu of attributes, false otherwise.
 */
static bool curs_can_use_extended_colors();

/**
 * @brief Write a complex curses character to specified screen location.
 *
 * There are two additional effects of this function:
 *  - The screen's character attributes are set to those contained in @p ch.
 *  - The cursor is moved to the passed coordinate.
 *
 * @param y
 *  The y-component of the location to draw @p ch.
 * @param x
 *  The x-component of the location to draw @p ch.
 * @param ch
 *  The complex character to render.
 */
static void write_char_at(int y, int x, const cchar_t &ch);

/**
 * @brief Terminal default aware version of pair_safe.
 *
 * @param pair
 *   Pair identifier
 * @param f
 *   Foreground colour
 * @param b
 *   Background colour
 */
//static void init_pair_safe(short pair, short f, short b);

static bool cursor_is_enabled = true;

void set_mouse_enabled(bool enabled)
{
    return;
    enabled = !enabled;
#ifdef NCURSES_MOUSE_VERSION
    if (_headless_mode)
        return;
    const int mask = enabled ? ALL_MOUSE_EVENTS | REPORT_MOUSE_POSITION : 0;
    mmask_t oldmask = 0;
    mousemask(mask, &oldmask);
#endif
}

#ifdef NCURSES_MOUSE_VERSION
static int proc_mouse_event(int c, const MEVENT *me)
{
    UNUSED(c);

    crawl_view.mousep.x = me->x + 1;
    crawl_view.mousep.y = me->y + 1;

    if (!crawl_state.mouse_enabled)
        return CK_MOUSE_MOVE;

    c_mouse_event cme(crawl_view.mousep);
    if (me->bstate & BUTTON1_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON1;
    else if (me->bstate & BUTTON1_DOUBLE_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON1_DBL;
    else if (me->bstate & BUTTON2_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON2;
    else if (me->bstate & BUTTON2_DOUBLE_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON2_DBL;
    else if (me->bstate & BUTTON3_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON3;
    else if (me->bstate & BUTTON3_DOUBLE_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON3_DBL;
    else if (me->bstate & BUTTON4_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON4;
    else if (me->bstate & BUTTON4_DOUBLE_CLICKED)
        cme.bstate |= c_mouse_event::BUTTON4_DBL;

    if (cme)
    {
        new_mouse_event(cme);
        return CK_MOUSE_CLICK;
    }

    return CK_MOUSE_MOVE;
}
#endif

//static int pending = 0;

static int _get_key_from_curses()
{
//#ifdef WATCHDOG
    //// If we have (or wait for) actual keyboard input, it's not an infinite
    //// loop.
    //watchdog();
//#endif

    //if (pending)
    //{
        //int c = pending;
        //pending = 0;
        //return c;
    //}

    //wint_t c;

//#ifdef USE_TILE_WEB
    //refresh();

    //tiles.redraw();
    //tiles.await_input(c, true);

    //if (c != 0)
        //return c;
//#endif

    //switch (get_wch(&c))
    //{
    //case ERR:
        //// getch() returns -1 on EOF, convert that into an Escape. Evil hack,
        //// but the alternative is to explicitly check for -1 everywhere where
        //// we might otherwise spin in a tight keyboard input loop.
        //return ESCAPE;
    //case OK:
        //// a normal (printable) key
        //return c;
    //}

    //return -c;

    std::string str;
    std::getline(std::cin, str);
    //switch (str)
    //{
        //case "up":
            //return CK_UP;
        //case "left":
            //return CK_LEFT;
        //case "down":
            //return CK_DOWN;
        //case "right":
            //return CK_RIGHT;
        //case "backspace":
            //return CK_BKSP;
        //case "delete":
            //return CK_DELETE;
        //case "space":
            //return CK_SPACE;
        //case "escape":
            //return ESCAPE;
        //case "enter":
            //return CK
        //default:
            //return str[0];
    //}
    if (str == "up") {
        return CK_UP;
    } else if (str == "left") {
        return CK_LEFT;
    } else if (str == "down") {
        return CK_DOWN;
    } else if (str == "right") {
        return CK_RIGHT;
    } else if (str == "backspace") {
        return CK_BKSP;
    } else if (str == "delete") {
        return CK_DELETE;
    } else if (str == "escape") {
        return ESCAPE;
    } else if (str == "enter") {
        return CK_ENTER;
    } else if (str == "exit") {
        return CONTROL('S');
    } else {
        return str[0];
    }

}

#if defined(KEY_RESIZE) || defined(USE_UNIX_SIGNALS)
static void unix_handle_resize_event(int event)
{
    event = event;
    crawl_state.last_winch = time(0);
    if (crawl_state.waiting_for_command)
        handle_terminal_resize();
    else
        crawl_state.terminal_resized = true;
}
#endif

static bool getch_returns_resizes;
void set_getch_returns_resizes(bool rr)
{
    getch_returns_resizes = rr;
}

//static int _headless_getchk()
//{
//#ifdef WATCHDOG
    //// If we have (or wait for) actual keyboard input, it's not an infinite
    //// loop.
    //watchdog();
//#endif

    //if (pending)
    //{
        //int c = pending;
        //pending = 0;
        //return c;
    //}


//#ifdef USE_TILE_WEB
    //wint_t c;
    //tiles.redraw();
    //tiles.await_input(c, true);

    //if (c != 0)
        //return c;
//#endif

    //return ESCAPE; // TODO: ??
//}

//static int _headless_getch_ck()
//{
    //int c;
    //do
    //{
        //c = _headless_getchk();
        //// TODO: release?
        //// XX this should possibly sleep
    //} while (
             //((c == CK_MOUSE_MOVE || c == CK_MOUSE_CLICK)
                 //&& !crawl_state.mouse_enabled));

    //return c;
//}

int getch_ck()
{
    int c = _get_key_from_curses();
    return c;

//#ifdef KEY_RESIZE
        //case KEY_RESIZE:    return CK_RESIZE;
//#endif

    //if (_headless_mode)
        //return _headless_getch_ck();

    //while (true)
    //{
        //int c = _get_key_from_curses();

//#ifdef NCURSES_MOUSE_VERSION
        //if (c == -KEY_MOUSE)
        //{
            //MEVENT me;
            //getmouse(&me);
            //c = proc_mouse_event(c, &me);

            //if (!crawl_state.mouse_enabled)
                //continue;
        //}
//#endif

//#ifdef KEY_RESIZE
        //if (c == -KEY_RESIZE)
        //{
            //unix_handle_resize_event();

            //// XXX: Before ncurses get_wch() returns KEY_RESIZE, it
            //// updates LINES and COLS to the new window size. The resize
            //// handler will only redraw the whole screen if the main view
            //// is being shown, for slightly insane reasons, which results
            //// in crawl_view.termsz being out of sync.
            ////
            //// This causes crashiness: e.g. in a menu, make the window taller,
            //// then scroll down one line. To fix this, we always sync termsz:
            //crawl_view.init_geometry();

            //if (!getch_returns_resizes)
                //continue;
        //}
//#endif

        //switch (-c)
        //{
        //case 127:
        //// 127 is ASCII DEL, which some terminals (all mac, some linux) use for
        //// the backspace key. ncurses does not typically map this to
        //// KEY_BACKSPACE (though this may depend on TERM settings?). '\b' (^H)
        //// in contrast should be handled automatically. Note that ASCII DEL
        //// is distinct from the standard esc code for del, esc[3~, which
        //// reliably does get mapped to KEY_DC by ncurses. Some background:
        ////     https://invisible-island.net/xterm/xterm.faq.html#xterm_erase
        //// (I've never found documentation for the mac situation.)
        //case KEY_BACKSPACE: return CK_BKSP;
        //case KEY_IC:        return CK_INSERT;
        //case KEY_DC:        return CK_DELETE;
        //case KEY_HOME:      return CK_HOME;
        //case KEY_END:       return CK_END;
        //case KEY_PPAGE:     return CK_PGUP;
        //case KEY_NPAGE:     return CK_PGDN;
        //case KEY_UP:        return CK_UP;
        //case KEY_DOWN:      return CK_DOWN;
        //case KEY_LEFT:      return CK_LEFT;
        //case KEY_RIGHT:     return CK_RIGHT;
        //case KEY_BEG:       return CK_CLEAR;

        //case KEY_BTAB:      return CK_SHIFT_TAB;
        //case KEY_SDC:       return CK_SHIFT_DELETE;
        //case KEY_SHOME:     return CK_SHIFT_HOME;
        //case KEY_SEND:      return CK_SHIFT_END;
        //case KEY_SPREVIOUS: return CK_SHIFT_PGUP;
        //case KEY_SNEXT:     return CK_SHIFT_PGDN;
        //case KEY_SR:        return CK_SHIFT_UP;
        //case KEY_SF:        return CK_SHIFT_DOWN;
        //case KEY_SLEFT:     return CK_SHIFT_LEFT;
        //case KEY_SRIGHT:    return CK_SHIFT_RIGHT;

        //case KEY_A1:        return CK_NUMPAD_7;
        //case KEY_A3:        return CK_NUMPAD_9;
        //case KEY_B2:        return CK_NUMPAD_5;
        //case KEY_C1:        return CK_NUMPAD_1;
        //case KEY_C3:        return CK_NUMPAD_3;

//#ifdef KEY_RESIZE
        //case KEY_RESIZE:    return CK_RESIZE;
//#endif

        //// Undocumented ncurses control keycodes, here be dragons!!!
        //case 515:           return CK_CTRL_DELETE; // Mac
        //case 526:           return CK_CTRL_DELETE; // Linux
        //case 542:           return CK_CTRL_HOME;
        //case 537:           return CK_CTRL_END;
        //case 562:           return CK_CTRL_PGUP;
        //case 557:           return CK_CTRL_PGDN;
        //case 573:           return CK_CTRL_UP;
        //case 532:           return CK_CTRL_DOWN;
        //case 552:           return CK_CTRL_LEFT;
        //case 567:           return CK_CTRL_RIGHT;

        //default:            return c;
        //}
    //}
}

//static void unix_handle_terminal_resize()
//{
    //console_shutdown();
    //console_startup();
//}

//static void unixcurses_defkeys()
//{
//#ifdef NCURSES_VERSION
    //// To debug these on a specific terminal, you can use `cat -v` to see what
    //// escape codes are being printed. To some degree it's better to let ncurses
    //// do what it can rather than hard-coding things, but that doesn't always
    //// work.
    //// cool trick: `printf '\033[?1061h\033='; cat -v` initializes application
    //// mode if the terminal supports it. (For some terminals, it may need to
    //// be explicitly allowed, or enable via numlock.)

    //// keypad 0-9 (only if the "application mode" was successfully initialised)
    //define_key("\033Op", 1000);
    //define_key("\033Oq", 1001);
    //define_key("\033Or", 1002);
    //define_key("\033Os", 1003);
    //define_key("\033Ot", 1004);
    //define_key("\033Ou", 1005);
    //define_key("\033Ov", 1006);
    //define_key("\033Ow", 1007);
    //define_key("\033Ox", 1008);
    //define_key("\033Oy", 1009);

    //// non-arrow keypad keys (for macros)
    //define_key("\033OM", 1010); // keypad enter

    //// TODO: I don't know under what context these four are mapped to numpad
    //// keys, but they are *much* more commonly used for F1-F4. So don't
    //// unconditionally define these. But these mappings have been around for
    //// a while, so I'm hesitant to remove them...
//#define check_define_key(s, n) if (!key_defined(s)) define_key(s, n)
    //check_define_key("\033OP", 1011); // NumLock
    //check_define_key("\033OQ", 1012); // /
    //check_define_key("\033OR", 1013); // *
    //check_define_key("\033OS", 1014); // -

    //// TODO: these could probably use further verification on linux
    //// notes:
    //// * this code doesn't like to map multiple esc sequences to the same
    ////   keycode. However, doing so works fine on my testing on mac, on
    ////   current ncurses. Why would this be bad?
    //// * mac Terminal.app even in application mode does not shift =[>
    //// * historically, several comments here were wrong on my testing, but
    ////   they could be right somewhere. The current key descriptions are
    ////   accurate as far as I can tell.
    //define_key("\033Oj", 1015); // *
    //define_key("\033Ok", 1016); // + (probably everything else except mac terminal)
    //define_key("\033Ol", 1017); // + (mac terminal application mode)
    //define_key("\033Om", 1018); // -
    //define_key("\033On", 1019); // .
    //define_key("\033Oo", 1012); // / (may conflict with the above define?)
    //define_key("\033OX", 1021); // =, at least on mac console

//# ifdef TARGET_OS_MACOSX
    //// force some mappings for function keys that work on mac Terminal.app with
    //// the default TERM value.

    //// The following seem to be the rxvt escape codes, even
    //// though Terminal.app defaults to xterm-256color.
    //// TODO: would it be harmful to force this unconditionally?
    //check_define_key("\033[25~", 277); // F13
    //check_define_key("\033[26~", 278); // F14
    //check_define_key("\033[28~", 279); // F15
    //check_define_key("\033[29~", 280); // F16
    //check_define_key("\033[31~", 281); // F17
    //check_define_key("\033[32~", 282); // F18
    //check_define_key("\033[33~", 283); // F19, highest key on a magic keyboard

    //// not sure exactly what's up with these, but they exist by default:
    //// ctrl bindings do too, but they are intercepted by macos
    //check_define_key("\033b", -(CK_LEFT + CK_ALT_BASE));
    //check_define_key("\033f", -(CK_RIGHT + CK_ALT_BASE));
    //// (sadly, only left and right have modifiers by default on Terminal.app)
//# endif
//#undef check_define_key

//#endif
//}

// Certain terminals support vt100 keypad application mode only after some
// extra goading.
#define KPADAPP "\033[?1051l\033[?1052l\033[?1060l\033[?1061h"
#define KPADCUR "\033[?1051l\033[?1052l\033[?1060l\033[?1061l"

//static void _headless_startup()
//{
    //// override the default behavior for SIGINT set in libutil.cc:init_signals.
    //// TODO: windows ctrl-c? should be able to add a handler on top of
    //// libutil.cc:console_handler
//#if defined(USE_UNIX_SIGNALS) && defined(SIGINT)
    //signal(SIGINT, handle_hangup);
//#endif

//#ifdef USE_TILE_WEB
    //tiles.resize();
//#endif
//}

void console_startup()
{
    // Init the screen
    screenArray = std::vector<std::vector<cchar_t>>(HEADLESS_LINES, std::vector<cchar_t>(HEADLESS_COLS, ' '));
#if defined(USE_UNIX_SIGNALS) && defined(SIGINT)
    //signal(SIGINT, handle_hangup);
#endif
#ifdef USE_UNIX_SIGNALS
# ifndef KEY_RESIZE
    //signal(SIGWINCH, unix_handle_resize_event);
# endif
#endif
    crawl_view.init_geometry();
    ui::resize(crawl_view.termsz.x, crawl_view.termsz.y);
    return;

    //if (_headless_mode)
    //{
        //_headless_startup();
        //return;
    //}
    //termio_init();

//#ifdef CURSES_USE_KEYPAD
    //// If hardening is enabled (default on recent distributions), glibc
    //// declares write() with __attribute__((warn_unused_result)) which not
    //// only spams when not relevant, but cannot even be selectively hushed
    //// by (void) casts like all other such warnings.
    //// "if ();" is an unsightly hack...
    //if (write(1, KPADAPP, strlen(KPADAPP))) {};
//#endif

//#ifdef USE_UNIX_SIGNALS
//# ifndef KEY_RESIZE
    //signal(SIGWINCH, unix_handle_resize_event);
//# endif
//#endif

    //initscr();
    //raw();
    //noecho();

    //nonl();
    //intrflush(stdscr, FALSE);
//#ifdef CURSES_USE_KEYPAD
    //keypad(stdscr, TRUE);

//# ifdef CURSES_SET_ESCDELAY
//#  ifdef NCURSES_REENTRANT
    //set_escdelay(CURSES_SET_ESCDELAY);
//#  else
    //ESCDELAY = CURSES_SET_ESCDELAY;
//#  endif
//# endif
//#endif

    //meta(stdscr, TRUE);
    //unixcurses_defkeys();
    //start_color();

    //setup_colour_pairs();
    //// Since it may swap pairs, set default colors *after* setting up all pairs.
    //curs_set_default_colors();

    //scrollok(stdscr, FALSE);

    //// Must call refresh() for ncurses to update COLS and LINES.
    //refresh();
    //crawl_view.init_geometry();

    //// TODO: how does this relate to what tiles.resize does?
    //ui::resize(crawl_view.termsz.x, crawl_view.termsz.y);

//#ifdef USE_TILE_WEB
    //tiles.resize();
//#endif
}

void console_shutdown()
{
    //if (_headless_mode)
        //return;

    //// resetty();
    //endwin();

    //tcsetattr(0, TCSAFLUSH, &def_term);
//#ifdef CURSES_USE_KEYPAD
    //// "if ();" to avoid undisableable spurious warning.
    //if (write(1, KPADCUR, strlen(KPADCUR))) {};
//#endif

#ifdef USE_UNIX_SIGNALS
# ifndef KEY_RESIZE
    //signal(SIGWINCH, SIG_DFL);
# endif
#endif
}

void w32_insert_escape()
{
}

void cprintf(const char *format, ...)
{
    char buffer[2048];          // One full screen if no control seq...

    va_list argp;

    va_start(argp, format);
    vsnprintf(buffer, sizeof(buffer), format, argp);
    va_end(argp);

    char32_t c;
    char *bp = buffer;
    while (int s = utf8towc(&c, bp))
    {
        bp += s;
        // headless check handled in putwch
        putwch(c);
    }
}

void putwch(char32_t chr)
{
    //wchar_t c = chr;
    //curX += c ? wcwidth(chr) : 0;
    screenArray[curY][curX] = chr;
    curX++;
    if (curX >= HEADLESS_COLS)
    {
        curY++;
        curX = curX - HEADLESS_COLS;
    }
    if (curX >= HEADLESS_COLS && curY >= HEADLESS_LINES)
    {
        curX = int(HEADLESS_COLS)-1;
        curY = int(HEADLESS_LINES)-1;
    }
    //std::cout << curX << " " << curY << " " << char(chr) << std::endl;
    
    //wchar_t c = chr; // ??
    //if (_headless_mode)
    //{
        //// simulate cursor movement and wrapping
        //headless_x += c ? wcwidth(chr) : 0;
        //if (headless_x >= HEADLESS_COLS && headless_y >= HEADLESS_LINES)
        //{
            //headless_x = HEADLESS_COLS;
            //headless_y = HEADLESS_LINES;
        //}
        //else if (headless_x > HEADLESS_COLS)
        //{
            //headless_y++;
            //headless_x = headless_x - HEADLESS_COLS;
        //}
    //}
    //else
    //{
        //if (!c)
            //c = ' ';
        //// TODO: recognize unsupported characters and try to transliterate
        //addnwstr(&c, 1);
    //}

//#ifdef USE_TILE_WEB
    //char32_t buf[2];
    //buf[0] = chr;
    //buf[1] = 0;
    //tiles.put_ucs_string(buf);
//#endif
}

void puttext(int x1, int y1, const crawl_view_buffer &vbuf)
{
    const screen_cell_t *cell = vbuf;
    const coord_def size = vbuf.size();
    for (int y = 0; y < size.y; ++y)
    {
        cgotoxy(x1, y1 + y);
        for (int x = 0; x < size.x; ++x)
        {
            // headless check handled in putwch, which this calls
            put_colour_ch(cell->colour, cell->glyph);
            cell++;
        }
    }
}

// These next four are front functions so that we can reduce
// the amount of curses special code that occurs outside this
// this file. This is good, since there are some issues with
// name space collisions between curses macros and the standard
// C++ string class.  -- bwr
void update_screen()
{
    for (int y = 0; y < HEADLESS_LINES; ++y)
    {
        for (int x = 0; x < HEADLESS_COLS; ++x)
        {
            std::cout << char(screenArray[y][x]);
        }
        std::cout << std::endl;
    }
    
    // In objstat, headless, and similar modes, there might not be a screen to update.
    //if (stdscr)
    //{
        //// Refreshing the default colors helps keep colors synced in ttyrecs.
        //curs_set_default_colors();
        //refresh();
    //}

//#ifdef USE_TILE_WEB
    //tiles.set_need_redraw();
//#endif
}

void clear_to_end_of_line()
{
    for (int x = curX; x < get_number_of_cols(); ++x)
    {
        screenArray[curY][x] = ' ';
    }
    //if (!_headless_mode)
    //{
        //textcolour(LIGHTGREY);
        //textbackground(BLACK);
        //clrtoeol(); // shouldn't move cursor pos
    //}

//#ifdef USE_TILE_WEB
    //tiles.clear_to_end_of_line();
//#endif
}

int get_number_of_lines()
{
    return HEADLESS_LINES;
    //if (_headless_mode)
        //return HEADLESS_LINES;
    //else
        //return LINES;
}

int get_number_of_cols()
{
    return HEADLESS_COLS;
    //if (_headless_mode)
        //return HEADLESS_COLS;
    //else
        //return COLS;
}

int num_to_lines(int num)
{
    return num;
}

#ifdef DGAMELAUNCH
static bool _suppress_dgl_clrscr = false;

// TODO: this is not an ideal way to solve this problem. An alternative might
// be to queue dgl clrscr and only send them at the same time as an actual
// refresh?
suppress_dgl_clrscr::suppress_dgl_clrscr()
    : prev(_suppress_dgl_clrscr)
{
    _suppress_dgl_clrscr = true;
}

suppress_dgl_clrscr::~suppress_dgl_clrscr()
{
    _suppress_dgl_clrscr = prev;
}
#endif

void clrscr_sys()
{
    for (int y = 0; y < HEADLESS_LINES; ++y)
    {
        for (int x = 0; x < HEADLESS_COLS; ++x)
        {
            screenArray[y][x] = ' ';
        }
    }
    curX = 0;
    curY = 0;

    //if (_headless_mode)
    //{
        //headless_x = 1;
        //headless_y = 1;
        //return;
    //}

    //textcolour(LIGHTGREY);
    //textbackground(BLACK);
    //clear();
//#ifdef DGAMELAUNCH
    //if (!_suppress_dgl_clrscr)
    //{
        //printf("%s", DGL_CLEAR_SCREEN);
        //fflush(stdout);
    //}
//#endif

}

void set_cursor_enabled(bool enabled)
{
    enabled = enabled;
    //curs_set(cursor_is_enabled = enabled);
//#ifdef USE_TILE_WEB
    //tiles.set_text_cursor(enabled);
//#endif
}

bool is_cursor_enabled()
{
    return cursor_is_enabled;
}

static inline unsigned get_highlight(int col)
{
    return ((col & COLFLAG_UNUSUAL_MASK) == COLFLAG_UNUSUAL_MASK) ?
                                              Options.unusual_highlight :
           (col & COLFLAG_FRIENDLY_MONSTER) ? Options.friend_highlight :
           (col & COLFLAG_NEUTRAL_MONSTER)  ? Options.neutral_highlight :
           (col & COLFLAG_ITEM_HEAP)        ? Options.heap_highlight :
           (col & COLFLAG_WILLSTAB)         ? Options.stab_highlight :
           (col & COLFLAG_MAYSTAB)          ? Options.may_stab_highlight :
           (col & COLFLAG_FEATURE_ITEM)     ? Options.feature_item_highlight :
           (col & COLFLAG_TRAP_ITEM)        ? Options.trap_item_highlight :
           (col & COLFLAG_REVERSE)          ? unsigned{CHATTR_REVERSE}
                                            : unsigned{CHATTR_NORMAL};
}

// see declaration
static bool curs_can_use_extended_colors()
{
    //return Options.allow_extended_colours && COLORS >= NUM_TERM_COLOURS;
    return false;
}

lib_display_info::lib_display_info()
    : type(CRAWL_BUILD_NAME),
    term("curseless"),
    fg_colors(
        (curs_can_use_extended_colors()
                || Options.bold_brightens_foreground != false)
        ? 16 : 8),
    bg_colors(
        (curs_can_use_extended_colors() || Options.blink_brightens_background)
        ? 16 : 8)
{
}

void textcolour(int col)
{
    col = 1 - col;
    //if (!_headless_mode)
    //{
        //const auto style = curs_attr_fg(col);
        //attr_set(style.attr, style.color_pair, nullptr);
    //}

//#ifdef USE_TILE_WEB
    //tiles.textcolour(col);
//#endif
}

COLOURS default_hover_colour()
{
    // DARKGREY backgrounds go to black with 8 colors. I think this is
    // generally what we want, rather than applying a workaround (like the
    // DARKGREY -> BLUE foreground trick), but this means that using a
    // DARKGREY hover, which arguably looks better in 16colors, won't work.
    // DARKGREY is also not safe under bold_brightens_foreground, since a
    // terminal that supports this won't necessarily handle a bold background.

    // n.b. if your menu uses just one color, and you set that as the hover,
    // you will get automatic color inversion. That is generally the safest
    // option where possible.
    return DARKGREY;
    //return (curs_can_use_extended_colors() || Options.blink_brightens_background)
                 //? DARKGREY : BLUE;
}

void textbackground(int col)
{
    col = 1 - col;
    //if (!_headless_mode)
    //{
        //const auto style = curs_attr_bg(col);
        //attr_set(style.attr, style.color_pair, nullptr);
    //}

//#ifdef USE_TILE_WEB
    //tiles.textbackground(col);
//#endif
}

void gotoxy_sys(int x, int y)
{
    curX = x-1;
    curY = y-1;

    //if (_headless_mode)
    //{
        //headless_x = x;
        //headless_y = y;
    //}
    //else
        //move(y - 1, x - 1);
}

static inline cchar_t character_at(int y, int x)
{
    return screenArray[y][x];
    //cchar_t c;
    //// (void) is to hush an incorrect clang warning.
    //(void)mvin_wch(y, x, &c);
    //return c;
}

/**
 * @internal
 * Writing out a cchar_t to the screen using one of the add_wch functions does
 * not necessarily set the character attributes contained within.
 *
 * So, explicitly set the screen attributes to those contained within the passed
 * cchar_t before writing it to the screen. This *should* guarantee that that
 * all attributes within the cchar_t (and only those within the cchar_t) are
 * actually set.
 */
static void write_char_at(int y, int x, const cchar_t &ch)
{
    curX = x;
    curY = y;
    screenArray[y][x] = ch;

    //attr_t attr = 0;
    //short color_pair = 0;
    //wchar_t *wch = nullptr;

    //// Make sure to allocate enough space for the characters.
    //int chars_to_allocate = getcchar(&ch, nullptr, &attr, &color_pair, nullptr);
    //if (chars_to_allocate > 0)
        //wch = new wchar_t[chars_to_allocate];

    //// Good to go. Grab the color / attr info.
    ////getcchar(&ch, wch, &attr, &color_pair, nullptr);

    //// Clean up.
    //if (chars_to_allocate > 0)
        //delete [] wch;

    //attr_set(attr, color_pair, nullptr);
    //mvadd_wchnstr(y, x, &ch, 1);
}

void fakecursorxy(int x, int y)
{
    //if (_headless_mode)
    //{
        //gotoxy_sys(x, y);
        //set_cursor_region(GOTO_CRT);
        //return;
    //}

    int x_curses = x - 1;
    int y_curses = y - 1;

    cchar_t c = character_at(y_curses, x_curses);
    //flip_colour(c);
    write_char_at(y_curses, x_curses, c);
    //// the above still results in changes to the return values for wherex and
    //// wherey, so set the cursor region to ensure that the cursor position is
    //// valid after this call. (This also matches the behavior of real cursorxy.)
    set_cursor_region(GOTO_CRT);
}

int wherex()
{
    return curX+1;
    //if (_headless_mode)
        //return headless_x;
    //else
        //return getcurx(stdscr) + 1;
}

int wherey()
{
    return curY+1;
    //if (_headless_mode)
        //return headless_y;
    //else
        //return getcury(stdscr) + 1;
}

void delay(unsigned int time)
{
    if (crawl_state.disables[DIS_DELAY])
        return;

#ifdef USE_TILE_WEB
    tiles.redraw();
    if (time)
    {
        tiles.send_message("{\"msg\":\"delay\",\"t\":%d}", time);
        tiles.flush_messages();
    }
#endif

    //refresh();
    if (time)
        usleep(time * 1000);
}

/* This is Juho Snellman's modified kbhit, to work with macros */
bool kbhit()
{
    return false;
}
