import curses

# ncurses extension for humble scheme

# poorman replication of the framework as needed to
# write an extension.  as defined, we have
BIT_VAR       = (1 << 14)
VAR_VOID      = 16 + BIT_VAR
VAR_NUM       = 17 + BIT_VAR
VAR_STRING    = 19 + BIT_VAR
VAR_EXTRA_MIN = 32 + BIT_VAR
VAR_LIST      = (1 << 9) + BIT_VAR
# where all values up to VAR_EXTRA_MAX are available, but
# must be configured to not collide among extensions in use
VAR_SCR = VAR_EXTRA_MIN # +9

def fargc_must_eq(fn, args, n):
    assert len(args) == n, ("%s requires %d args" % fn, n)

def fargc_must_ge(fn, args, n):
    assert len(args) >= n, ("%s requires %d args" % fn, n)

def fargt_must_eq(fn, args, i, vt):
    assert args[i][0] == vt, ("%s expected args[%d] %x got %x"
            % (fn, i, vt, args[i][0]))

# functions in this extension

def ef_nc_initscr(*args):
    fn = "nc-initscr"
    tenths = None
    if len(args) > 0:
        fargt_must_eq(fn, args, 0, VAR_NUM)
        tenths = args[0][1]
    stdscr = curses.initscr()
    curses.noecho()
    curses.curs_set(0)
    curses.start_color()
    if tenths is not None:
        curses.halfdelay(tenths)
    curses.init_pair(1, curses.COLOR_RED, curses.COLOR_BLACK)
    curses.init_pair(2, curses.COLOR_YELLOW, curses.COLOR_BLACK)
    curses.init_pair(3, curses.COLOR_GREEN, curses.COLOR_BLACK)
    curses.init_pair(4, curses.COLOR_CYAN, curses.COLOR_BLACK)
    curses.init_pair(5, curses.COLOR_BLUE, curses.COLOR_BLACK)
    curses.init_pair(6, curses.COLOR_MAGENTA, curses.COLOR_BLACK)
    curses.init_pair(7, curses.COLOR_WHITE, curses.COLOR_BLACK)
    return [VAR_SCR, stdscr]

def ef_nc_getmaxyx(*args):
    fn = "nc-getmaxyx"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_SCR)
    stdscr = args[0][1]
    y, x = stdscr.getmaxyx()
    return [VAR_LIST, [[VAR_NUM, y], [VAR_NUM, x]]]

def ef_nc_addstr(*args):
    fn = "nc-addstr"
    fargc_must_ge(fn, args, 4)
    fargt_must_eq(fn, args, 0, VAR_SCR)
    fargt_must_eq(fn, args, 1, VAR_NUM)
    fargt_must_eq(fn, args, 2, VAR_NUM)
    fargt_must_eq(fn, args, 3, VAR_STRING)
    if len(args) >= 5:
        fargt_must_eq(fn, args, 4, VAR_NUM)
        c = args[4][1]
    else:
        c = 7
    stdscr = args[0][1]
    y = args[1][1]
    x = args[2][1]
    s = args[3][1]
    stdscr.addstr(y, x, s, curses.color_pair(c))
    return [VAR_VOID]

def ef_nc_getch(*args):
    fn = "nc-getch"
    fargc_must_eq(fn, args, 1)
    fargt_must_eq(fn, args, 0, VAR_SCR)
    stdscr = args[0][1]
    c = stdscr.getch()
    if c == curses.KEY_RESIZE:
        ef_nc_endwin()
        sys.exit(1)
    return [VAR_NUM, c]

def ef_nc_endwin(*args):
    fn = "nc-endwin"
    fargc_must_eq(fn, args, 0)
    curses.endwin()
    return [VAR_VOID]

# hook for the extension loader

def load():
    return ([VAR_SCR], [
        ("nc-initscr", ef_nc_initscr),
        ("nc-getmaxyx", ef_nc_getmaxyx),
        ("nc-addstr", ef_nc_addstr),
        ("nc-getch", ef_nc_getch),
        ("nc-endwin", ef_nc_endwin)])

