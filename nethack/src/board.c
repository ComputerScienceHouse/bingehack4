/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <time.h>

struct gamewin *gw;
struct win_menu *mdat;
nh_bool done;
struct nh_board_entry *entries;
int listlen, maxplayersize;

static void
board_display_curses_ui(struct nh_menuitem *items, int icount)
{
    int startx, starty, x1, x2, y1, y2;

    if (gw != NULL) {
        delwin(mdat->content);
        delwin(gw->win);
        delete_gamewin(gw);
    }

    gw = alloc_gamewin(sizeof (struct win_menu));
    gw->draw = draw_menu;
    gw->resize = resize_menu;
    mdat = (struct win_menu *)gw->extra;
    mdat->items = items;
    mdat->icount = icount;
    mdat->title = "Binge Board";
    mdat->selected = calloc(icount, sizeof (nh_bool));
    mdat->x1 = 0;
    mdat->y1 = 0;
    mdat->x2 = -1;
    mdat->y2 = -1;

    x1 = 0;
    y1 = 0;
    x2 = COLS;
    y2 = LINES;

    assign_menu_accelerators(mdat);
    layout_menu(gw);

    starty = (y2 - y1 - mdat->height) / 2 + y1;
    startx = (x2 - x1 - mdat->width) / 2 + x1;

    gw->win = newwin(mdat->height, mdat->width, starty, startx);
    keypad(gw->win, TRUE);
    wattron(gw->win, FRAME_ATTRS);
    box(gw->win, 0, 0);
    wattroff(gw->win, FRAME_ATTRS);
    mdat->content =
        derwin(gw->win, mdat->innerheight, mdat->innerwidth,
               mdat->frameheight - 1, 2);
    leaveok(gw->win, TRUE);
    leaveok(mdat->content, TRUE);
    wtimeout(gw->win, 500);
}

static void
board_add_entry(struct nh_board_entry *entry, struct nh_menuitem **items,
                int *size, int *icount, int maxwidth, int maxplayerwidth,
                enum nh_menuitem_role txtrole)
{
    char fmtline[BUFSZ], line[BUFSZ], name[BUFSZ], hptxt[BUFSZ], entxt[BUFSZ];
    struct tm tm;
    time_t t, now;
    char *inactive;

    now = time(NULL);

    strptime(entry->lastactive, "%Y-%m-%d %H:%M:%S.000000%z", &tm);
    t = mktime(&tm);

    if(difftime(now, t) > 30) { //Marks a player inactive after 30 seconds
        inactive = "I";
    } else {
        inactive = "";
    }

    sprintf(name, "%s the %s %s %s %s", entry->name, entry->plalign,
            entry->plgend, entry->plrace, entry->plrole);
    sprintf(hptxt, "%d/%d", entry->hp, entry->hpmax);
    sprintf(entxt, "%d/%d", entry->en, entry->enmax);
    sprintf(fmtline, "%%1s  %%-%ds | %%-12s | %%-7s %%-7s | %%3d %%3d %%3d %%3d %%3d %%3d | %%5d | %%5d | %%5d",
            maxplayerwidth);
    sprintf(line, fmtline, inactive, name,
            entry->leveldesc, hptxt, entxt, entry->wi, entry->in, entry->st,
            entry->dx, entry->co, entry->ch, entry->moves, entry->depth,
            entry->level);
    add_menu_txt(*items, *size, *icount, line, txtrole);
}

static int
get_max_playerdesc(struct nh_board_entry *entries, int listlen)
{
    int i, tmpsize, maxsize = 0;
    struct nh_board_entry *tmpentry;
    for(i = 0; i < listlen; i++) {
        tmpentry = (entries + i);
        tmpsize = strlen(tmpentry->name) //Length of the player's name
                    + 5                  //Length of " the "
                    + strlen(tmpentry->plalign) //Length of their alignment
                    + 1                  //Space
                    + strlen(tmpentry->plgend)
                    + 1                  //Space
                    + strlen(tmpentry->plrace)
                    + 1                  //Space
                    + strlen(tmpentry->plrole);
        if(maxsize < tmpsize) {
            maxsize = tmpsize;
        }
    }
    return maxsize;
}

static void
handle_key(int key)
{
    switch (key) {
    case KEY_ESC:
    case 'q':
        done = TRUE;
        draw_menu(gw);
        break;
    default:
        break;
    }
}

static void
update_board(struct nh_menuitem **items, char *time)
{
    struct nh_board_entry *new_entries;
    int i, new_listlen, new_maxplayersize;
    int icount, size;
    char fmtline[BUFSZ], line[BUFSZ];

    new_entries = nh_get_board_entries(time, &new_listlen);

    icount = 0;
    size = 4 + listlen * 2;     /* maximum length, only required if every item
                                   wraps */

    free((*items));
    (*items) = malloc(size * sizeof (struct nh_menuitem));

    if(new_listlen == 0) {
        new_maxplayersize = 0;
        add_menu_txt((*items), size, icount, "No one seems to be playing right now.", MI_TEXT);
    } else {
        new_maxplayersize = get_max_playerdesc(new_entries, new_listlen);

        sprintf(fmtline, "   %%-%ds | Location     | Health  Energy  | Wis Int Str Dex Con Cha | Moves | Depth | Level",
                new_maxplayersize);
        sprintf(line, fmtline, "Player");
        add_menu_txt((*items), size, icount, line, MI_HEADING);

        for (i = 0; i < new_listlen; i++) {
            enum nh_menuitem_role txtrole;
            if(i < listlen && entries[i].moves != new_entries[i].moves) {
                txtrole = MI_HEADING;
            } else {
                txtrole = MI_TEXT;
            }
            board_add_entry(&new_entries[i], items, &size, &icount,
                             min(COLS, BUFSZ) - 4, new_maxplayersize, txtrole);
        }
        add_menu_txt((*items), size, icount, "", MI_TEXT);
    }
    if(new_maxplayersize < maxplayersize || new_listlen < listlen) {
        clear();
        refresh();
    }

    board_display_curses_ui((*items), icount);

    entries = new_entries;
    listlen = new_listlen;
    maxplayersize = new_maxplayersize;
}

void
show_bingeboard()
{
    struct nh_menuitem *items;
    int prevcurs, key;
    struct tm *tm;
    time_t t;
    char timestr[100];

    gw = NULL;
    mdat = NULL;
    done = FALSE;
    entries = NULL;
    listlen = 0;
    maxplayersize = 0;

    t = time(NULL);
    tm = localtime(&t);
    strftime(timestr, sizeof(timestr), "%Y-%m-%d %H:%M:%S.000000%z", tm);

    prevcurs = curs_set(0);
    items = NULL;

    /* show the score list on a blank screen */
    clear();
    refresh();

    while (!done) {
        update_board(&items, timestr);
        draw_menu(gw);
        key = nh_wgetch(gw->win);
        handle_key(key);
    }

    delwin(mdat->content);
    delwin(gw->win);
    free(mdat->selected);
    delete_gamewin(gw);
    free(items);

    redraw_game_windows();
    curs_set(prevcurs);
}
