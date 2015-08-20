/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* NetHack may be freely redistributed.  See license for details. */

#include "nhcurses.h"
#include <time.h>

static struct gamewin *gw = NULL;
static struct win_menu *mdat = NULL;
static nh_bool done = false;
static struct nh_board_entry *entries = NULL;
static int listlen = 0, maxplayersize = 0;

static void
board_display_curses_ui(struct nh_menuitem *items, int icount)
{
    char errmsg[256] = "\0";
    int startx = 0, starty = 0;

    if (gw != NULL) {
        free(mdat->selected);
        delwin(mdat->content);
        delwin(gw->win);
        delete_gamewin(gw);
    }

    gw = alloc_gamewin(sizeof (struct win_menu));
    if(!gw) {
        strcpy(errmsg, "alloc_gamewin: Failed to allocate game window\n");
        goto alloc_gamewin_fail;
    }
    gw->draw = draw_menu;
    gw->resize = resize_menu;
    mdat = (struct win_menu *)gw->extra;
    mdat->items = items;
    mdat->icount = icount;
    mdat->title = "Binge Board";
    mdat->selected = calloc(icount, sizeof (nh_bool));
    if (!mdat->selected) {
        strcpy(errmsg, "calloc: Couldn't allocate memory for mdat->selected\n");
        goto alloc_selected_fail;
    }
    mdat->x1 = 0;
    mdat->y1 = 0;
    mdat->x2 = -1;
    mdat->y2 = -1;

    assign_menu_accelerators(mdat);
    layout_menu(gw);

    startx = (COLS - mdat->width) / 2;
    starty = (LINES - mdat->height) / 2;

    gw->win = newwin(mdat->height, mdat->width, starty, startx);
    if (gw->win == NULL) {
        strcpy(errmsg, "newwin: Failed to allocate a new window\n");
        goto alloc_newwin_fail;
    }
    if (keypad(gw->win, TRUE) == ERR) {
        strcpy(errmsg, "keypad: Failed to set keypad to true\n");
        goto keypad_fail;
    }
    if (wattron(gw->win, FRAME_ATTRS) == ERR) {
        strcpy(errmsg, "wattron: failed to set attributes on game window\n");
        goto wattron_fail;
    }
    box(gw->win, 0, 0);
    if (wattroff(gw->win, FRAME_ATTRS) == ERR) {
        strcpy(errmsg, "box: failed to set box on game window\n");
        goto box_fail;
    }
    mdat->content =
        derwin(gw->win, mdat->innerheight, mdat->innerwidth,
               mdat->frameheight - 1, 2);
    if (mdat->content == NULL) {
        strcpy(errmsg, "derwin: failed allocate sub window\n");
        goto derwin_fail;
    }
    if (leaveok(gw->win, TRUE) == ERR) {
        strcpy(errmsg, "leaveok: failed to set leaveok on game window\n");
        goto leaveok_gwwin_fail;
    }
    if (leaveok(mdat->content, TRUE) == ERR) {
        strcpy(errmsg, "leaveok: failed to set leaveok on game content\n");
        goto leaveok_mdatcont_fail;
    }
    wtimeout(gw->win, 500);

    return;

leaveok_mdatcont_fail:
leaveok_gwwin_fail:
    delwin(mdat->content);
derwin_fail:
box_fail:
wattron_fail:
keypad_fail:
    delwin(gw->win);
alloc_newwin_fail:
    free(mdat->selected);
alloc_selected_fail:
    delete_gamewin(gw);
alloc_gamewin_fail:
    endwin();
    fprintf(stderr, errmsg);
    exit(EXIT_FAILURE);
}

static void
board_add_entry(struct nh_board_entry *entry, struct nh_menuitem **items,
                int *size, int *icount, int maxplayerwidth,
                enum nh_menuitem_role txtrole)
{
    char fmtline[BUFSZ] = "\0", line[BUFSZ] = "\0", name[BUFSZ] = "\0",
         hptxt[BUFSZ] = "\0", entxt[BUFSZ] = "\0";
    struct tm tm = { 0 };
    time_t t = time(NULL), now = time(NULL);
    char *inactive = NULL;

    strptime(entry->lastactive, "%Y-%m-%d %H:%M:%S.000000%z", &tm);
    t = mktime(&tm);

    if(difftime(now, t) > 30) { //Marks a player inactive after 30 seconds
        inactive = "I";
    } else {
        inactive = "";
    }

    snprintf(name, BUFSZ, "%s the %s %s %s %s", entry->name, entry->plalign,
             entry->plgend, entry->plrace, entry->plrole);
    snprintf(hptxt, 7, "%d/%d", entry->hp, entry->hpmax);
    snprintf(entxt, 7, "%d/%d", entry->en, entry->enmax);
    snprintf(fmtline, BUFSZ, "%%1s  %%-%ds | %%-12s | %%-7s %%-7s | %%3d %%3d %%3d %%3d %%3d %%3d | %%5d | %%5d | %%5d",
             maxplayerwidth);
    snprintf(line, BUFSZ, fmtline, inactive, name,
            entry->leveldesc, hptxt, entxt, entry->wi, entry->in, entry->st,
            entry->dx, entry->co, entry->ch, entry->moves, entry->depth,
            entry->level);
    add_menu_txt(*items, *size, *icount, line, txtrole);
}

static int
get_max_playerdesc(struct nh_board_entry *entries, int listlen)
{
    int i = 0, tmpsize = 0, maxsize = 0;
    struct nh_board_entry *tmpentry = NULL;
    for(i = 0; i < listlen; i++) {
        tmpentry = (entries + i);
        tmpsize = strlen(tmpentry->name)
                    + strlen(" the ")
                    + strlen(tmpentry->plalign)
                    + strlen(" ")
                    + strlen(tmpentry->plgend)
                    + strlen(" ")
                    + strlen(tmpentry->plrace)
                    + strlen(" ")
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
    struct nh_board_entry *new_entries = NULL;
    int i = 0, new_listlen = 0, new_maxplayersize = 0;
    int icount = 0, size = 0;
    char fmtline[BUFSZ] = "\0", line[BUFSZ] = "\0";

    new_entries = nh_get_board_entries(time, &new_listlen);

    icount = 0;
    size = 4 + listlen * 2;     /* maximum length, only required if every item
                                   wraps */

    free((*items));
    (*items) = calloc(size, sizeof (struct nh_menuitem));
    if ((*items) == NULL) {
        return;
    }

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
                             new_maxplayersize, txtrole);
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
    struct nh_menuitem *items = NULL;
    int prevcurs = 0, key = 0;
    struct tm *tm = NULL;
    time_t t = time(NULL);
    char timestr[100] = "\0";

    gw = NULL;
    mdat = NULL;
    done = FALSE;
    entries = NULL;
    listlen = 0;
    maxplayersize = 0;

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
