/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Derrick Sund, 2014-03-22 */
/* Copyright (c) Daniel Thaler, 2011.                             */
/* NetHack may be freely redistributed.  See license for details. */

#include <ctype.h>

#include "nhcurses.h"

struct msghist_entry {
    int turn;               /* for displaying in the UI */
    char *message;          /* the message; can be NULL if this slot is empty */
    nh_bool old;            /* true if the user has acted since seeing it */
    nh_bool unseen;         /* true if the user must see it but has not, yet */
    nh_bool nomerge;        /* don't merge this message with the next one */
};

static struct msghist_entry *histlines; /* circular buffer */
static int histlines_alloclen;          /* allocated length of histlines */
static int histlines_pointer;           /* next unused histline entry */
static nh_bool last_line_reserved;      /* keep the last line of msgwin blank */
static int first_unseen = -1;           /* first unseen histline entry */
static int first_new = -1;              /* first non-"old" histline entry */
static nh_bool stopmore = 0;            /* stop doing input at --More-- */

static struct msghist_entry *showlines; /* lines to be displayed; noncircular.
                                           showlines[0] is bottom message. */
static int num_showlines;               /* number of lines in the message buf */

static const char* more_text = " --More--";   /* The string to use in more
                                                 prompts */

/* Allocates space for settings.msghistory lines of message history, or adjusts
   the message history to the given amount of space if it's already been
   allocated. We actually allocate an extra line, so that histlines_pointer can
   always point to a blank line; this simplifies the code. */
void
alloc_hist_array(void)
{
    static struct msghist_entry *newhistlines;
    int i, j;

    newhistlines = calloc((settings.msghistory + 1), sizeof *newhistlines);

    if (!newhistlines)
        return;                            /* just keep using the old array */

    i = histlines_pointer + 1;
    j = 0;

    first_unseen = -1;
    first_new = -1;

    if (histlines) {
        while (i != histlines_pointer) {
            if (histlines[i].message) {
                newhistlines[j] = histlines[i];

                if (histlines[j].unseen && first_unseen == -1)
                    first_unseen = j;
                if (!histlines[j].old && first_new == -1)
                    first_new = j;

                j++;
                j %= settings.msghistory + 1;
            }
            i++;
            i %= histlines_alloclen;
        }
        free(histlines);
    }
    histlines = newhistlines;
    histlines_pointer = j;
    histlines_alloclen = settings.msghistory + 1;
}

/* Deallocate and reset state used by the messages code. */
void
cleanup_messages(void)
{
    int i;
    if (histlines)
        for (i = 0; i < histlines_alloclen; i++)
            free(histlines[i].message);            /* free(NULL) is legal */
    free(histlines);
    histlines = 0;
    histlines_alloclen = 0;
    histlines_pointer = 0;
    first_unseen = -1;
    first_new = -1;
}

/* Allocate showlines. */
void
setup_showlines(void)
{
    num_showlines = getmaxy(msgwin);
    showlines = calloc((num_showlines + 1), sizeof *showlines);
    int i;
    for (i = 0; i < num_showlines; i++) {
        showlines[i].turn = -1;
        showlines[i].old = FALSE;
        showlines[i].unseen = FALSE;
        showlines[i].nomerge = FALSE;
    }
}

/* Reallocate showlines (preserving existing messages where possible) if the 
   window gets resized. */
void
redo_showlines(void)
{
    int new_num_showlines = getmaxy(msgwin);
    static struct msghist_entry *new_showlines;
    new_showlines = calloc((new_num_showlines + 1), sizeof *new_showlines);
    int i;
    for (i = 0; i < new_num_showlines && i < num_showlines; i++) {
        new_showlines[i].turn = showlines[i].turn;
        new_showlines[i].message = showlines[i].message;
        new_showlines[i].old = showlines[i].old;
        new_showlines[i].unseen = showlines[i].unseen;
        new_showlines[i].nomerge = showlines[i].nomerge;
    }
    /* At most one of the following loops will execute at all. */
    for (; i < new_num_showlines; i++) {
        new_showlines[i].turn = -1;
        new_showlines[i].old = TRUE;
        new_showlines[i].unseen = FALSE;
        new_showlines[i].nomerge = FALSE;
    }
    for (; i < num_showlines; i++) {
        free(showlines[i].message);
    }
    free(showlines);
    showlines = new_showlines;
    num_showlines = new_num_showlines;
}

/* Appends second to the end of first, which will be reallocated larger to fit
   if necessary. The allocated length of first is stored in first_alloclen.
   *first can be NULL, in which case *first_alloclen must be 0; first itself
   must point to a valid char pointer. If second is NULL, the function returns
   without doing anything. */
static void
realloc_strcat(char **first, int *first_alloclen, char *second)
{
    int strlen_first = *first ? strlen(*first) : 0;
    int strlen_second = second ? strlen(second) : 0;

    if (!second)
        return;

    if (strlen_first + strlen_second >= *first_alloclen) {
        int first_was_null = !*first;

        *first_alloclen = ((strlen_first + strlen_second + 1) / 256)
                          * 256 + 256;
        *first = realloc(*first, *first_alloclen);

        if (first_was_null)
            **first = 0;
    }

    strcat(*first, second);
}

static void
show_msgwin(nh_bool more)
{
    werase(msgwin);
    int i;
    for (i = num_showlines - 1; i >= 0; i--) {
        wmove(msgwin, num_showlines - 1 - i, 0);
        if(!showlines[i].message)
            continue;
        char *p = showlines[i].message;
        //XXX: Do colors properly
        attr_t color_attr = showlines[i].old ?
            curses_color_attr(COLOR_BLACK, 0) :
            curses_color_attr(COLOR_WHITE, 0);
        while (*p)
            waddch(msgwin, *p++ | color_attr);
        if (i == 0 && more) {
            p = more_text;
            while (*p)
                waddch(msgwin, *p++ | curses_color_attr(COLOR_WHITE + 8, 0));
        }
    }
    wnoutrefresh(msgwin);
}

static void
mark_all_seen(nh_bool mark_old)
{
    int i;
    for (i = 0; i < num_showlines; i++) {
        showlines[i].unseen = FALSE;
        showlines[i].nomerge = TRUE;
        if(mark_old)
            showlines[i].old = TRUE;
    }
}

static void
keypress_at_more(void)
{
    int continue_looping = 1;
    /* Well, we've at least tried to give a --More--.  Any failure to see
       the currently-visible messages is the players own fault. */
    mark_all_seen(FALSE);
    if (stopmore)
        return;

    while (continue_looping) {
        switch (get_map_key(FALSE)) {
        case KEY_ESCAPE:
            stopmore = 1;
            continue_looping = 0;
            break;
        case ' ':
        case 10:
        case 13:
            continue_looping = 0;
            break;
        }
    }
}

#if 0
/* Ensure that the user has seen all the messages that they're required to see
   (via displaying them, with --More-- if necessary), finally leaving the last
   onscreen. If more is set, draw a --More-- after the last set, too. */
static void
force_seen(nh_bool more) {
    if (!layout_msgwin(0, 0, more)) {
        /* The text so far doesn't fit onto the screen. Draw it, followed by a
           --More--. */
        int offset = 1;
        while (!layout_msgwin(0, offset, 1))
            offset++;
        while (offset > 0) {
            layout_msgwin(1, offset, 1); /* sets unseen to 0 */
            keypress_at_more();
            offset -= getmaxy(msgwin);
        }
    }
    layout_msgwin(1, 0, more);
}
#endif

/* Draws messages on the screen. Any messages drawn since the last call to
   new_action() are in white; others are in blue. This routine adapts to the
   size of the message buffer.

   This never asks for user input; it's more low-level than that. When adding a
   new message line, the caller will need to arrange for --More-- to be shown
   appropriately by itself. Typically, it's used to redraw the message window
   if it hasn't changed since the last time it was drawn. */
void
draw_msgwin(void)
{
    show_msgwin(FALSE);
}


/* When called, previous messages should be blued out. Assumes that there has
   been user input with the message window visible since the last message was
   written. */
void
new_action(void)
{
    mark_all_seen(TRUE);
    draw_msgwin();
    #if 0
    int hp = first_new;
    int last_hp = hp;
    if (hp == -1)
        return;

    if (!histlines)
        alloc_hist_array();

    while (hp != histlines_pointer) {
        histlines[hp].old = 1;
        last_hp = hp;
        hp++;
        hp %= histlines_alloclen;
    }

    /* Don't merge histlines from different actions. */
    histlines[last_hp].nomerge = 1;

    first_new = -1;
    stopmore = 0;

    layout_msgwin(1, 0, 0);
    #endif
}

static void
move_lines_upward(int num_to_bump)
{
    int i;
    for (i = num_showlines - 1; i >= num_showlines - num_to_bump; i--)
        free(showlines[i].message);
    for (i = num_showlines - 1; i >= num_to_bump; i--) {
        showlines[i].message = showlines[i - num_to_bump].message;
        showlines[i].turn = showlines[i - num_to_bump].turn;
        showlines[i].old = showlines[i - num_to_bump].old;
        showlines[i].unseen = showlines[i - num_to_bump].unseen;
        showlines[i].nomerge = showlines[i - num_to_bump].nomerge;
    }
    for (; i >= 0; i--) {
        showlines[i].message = NULL;
        showlines[i].turn = -1;
        showlines[i].old = FALSE;
        showlines[i].unseen = FALSE;
        showlines[i].nomerge = FALSE;
    }
}

/* Update the showlines array with new string text from intermediate.
   Returns TRUE if we're going to need a --More-- and another pass. */
static nh_bool
update_showlines(char **intermediate, int *length, nh_bool force_more)
{
    /*
     * Each individual step in this can be ugly, but the overall logic isn't
     * terribly complicated.
     * STEP 1: Determine whether the string already present in showlines[0]
     *         (that is, the one at the bottom of the message window) should be
     *         merged with the text in intermediate.  Create a new buffer, buf,
     *         out of the combination of (possibly) the text from showlines[0]
     *         and the text from intermediate.
     * STEP 2: Wrap the buffer we got in Step 1.  Count how many showlines we
     *         can and should bump upward to make room for the new text.  If
     *         we can't make enough room to fit all of the wrapped lines from
     *         buf, make a note that we're going to need another pass/more.
     * STEP 3: Shift the showlines messages, freeing the ones that fall off the
     *         end, and put the wrapped lines in the freed slots.
     * STEP 4: Wipe out intermediate, and reconstruct it by concatenating the
     *         lines (if any exist) we couldn't fit in Step 3.
     * STEP 5: If we need another pass, strip tokens off the end of showlines[0]
     *         and shove them into the beginning of intermediate until we have
     *         room for a more prompt.
     */
    
    /* Step 1 begins here. */
    int messagelen = 0;
    nh_bool merging = FALSE;
    nh_bool need_more = force_more && showlines[0].unseen;
    if (showlines[0].message)
        messagelen = strlen(showlines[0].message);

    char buf[strlen(*intermediate) + messagelen + 3];

    if (!showlines[0].nomerge && showlines[0].message) {
        strcpy(buf, showlines[0].message);
        strcat(buf, "  ");
        strcat(buf, *intermediate);
        merging = TRUE;
    }
    else if (!showlines[0].message) {
        /* Setting merging to TRUE means showlines[0].message will be freed,
           but free(NULL) is legal. */
        strcpy(buf, *intermediate);
        merging = TRUE;
    }
    else
        strcpy(buf, *intermediate);

    /* Step 2 begins here. */
    char **wrapped_buf = NULL;
    int num_buflines = 0;
    wrap_text(getmaxx(msgwin), buf, &num_buflines, &wrapped_buf);

    /* Determine the number of entries in showlines to bump off the top and
       into the gaping maw of free().  It is bounded above by:
       1: num_buflines
       2: the number of showlines that have been seen and can legally be
          bumped. */
    int num_can_bump = 0;
    int i;
    for (i = 0; i < num_showlines; i++)
        if (!showlines[i].unseen)
            num_can_bump++;

    int num_to_bump = num_can_bump;
    if (num_to_bump >= num_buflines)
        num_to_bump = num_buflines;
    //XXX: num_to_bump is sometimes negative, particularly when quitting
    //XXX: FIX THIS
    if (merging && num_to_bump > 0)
        num_to_bump--;

    /* If we're merging, we'll need a --More-- if num_to_bump is strictly
       smaller than num_buflines - 1.
       If we're not merging, we'll need a --More-- if num_to_bump is strictly
       smaller than num_buflines. */
    if ((num_to_bump < num_buflines - 1) ||
        (!merging && num_to_bump < num_buflines))
        need_more = TRUE;

    if (merging)
        free(showlines[0].message);

    /* Step 3 begins here. */
    move_lines_upward(num_to_bump);

    if (!merging) {
        for (i = num_to_bump - 1; i >= 0; i--)
        {
            showlines[i].message =
                malloc(strlen(wrapped_buf[num_to_bump - 1 - i]) + 1);
            strcpy(showlines[i].message, wrapped_buf[num_to_bump - 1 - i]);
            showlines[i].unseen = TRUE;
            showlines[i].nomerge = FALSE;
        }
    }
    else {
        for (i = num_to_bump; i >= 0; i--)
        {
            showlines[i].message =
                malloc(strlen(wrapped_buf[num_to_bump - i]) + 1);
            strcpy(showlines[i].message, wrapped_buf[num_to_bump - i]);
            showlines[i].unseen = TRUE;
            showlines[i].nomerge = FALSE;
            showlines[i].old = FALSE;
        }
    }

    /* Step 4 begins here. */
    messagelen = strlen(*intermediate);
    strcpy(*intermediate, "");
    for (i = merging ? num_to_bump + 1 : num_to_bump; i < num_buflines; i++) {
        realloc_strcat(intermediate, length, wrapped_buf[i]);
        /* TODO: Base this on the whitespace in buf rather than trying to divine
           it from punctuation. */
        if((*intermediate)[strlen(*intermediate) - 1] == '.')
            realloc_strcat(intermediate, length, "  ");
        else
            realloc_strcat(intermediate, length, " ");
        
    }
    //XXX: The above intermediate thing might mangle whitespace somehow.
    //It'll do for prototyping.

    /* Step 5 begins here. */
    while (showlines[0].message && need_more &&
           strlen(showlines[0].message) > getmaxx(msgwin) - strlen(more_text)) {
        /* Find the last space in the current showlines[0]. */
        char *last;
        last = strrchr(showlines[0].message, ' ');
        /* If the showlines[0] string doesn't *have* any whitespace, just
           kind of split it up anyway. */
        if (!last)
            last = showlines[0].message + getmaxx(msgwin) - strlen(more_text);
        char *temp = malloc(strlen(*intermediate) + strlen(last) + 1);
        strcpy(temp, last + 1);
        strcat(temp, " ");
        strcat(temp, *intermediate);
        free(*intermediate);
        *intermediate = temp;
        *last = '\0';
    }

    free_wrap(wrapped_buf);
    return need_more;
}

/* Guarantee the player sees the current message buffer by forcing a more prompt
   if this is legal. */
static void
force_seen(void)
{
    /* dummy is just "" initially, but forcing a more in update_showlines might
       lop a few tokens off the end of showlines[0].message and put them into
       dummy.  That's why we need to call update_showlines in a loop. */
    char* dummy = malloc(1);
    int dummy_length = 1;
    strcpy(dummy, "");
    nh_bool keep_going = TRUE;
    while (keep_going) {
        keep_going = update_showlines(&dummy, &dummy_length, TRUE);
        show_msgwin(keep_going);
        if (keep_going)
            keypress_at_more();
    }
    free(dummy);
}

/* Make sure the bottom message line is empty. If this would scroll something
   off the screen, do a --More-- first if necessary. */
void
fresh_message_line(nh_bool canblock)
{
    force_seen();
    if (showlines[0].message)
        move_lines_upward(1);
    #if 0
    force_seen(0);
    last_line_reserved = 1;
    if (!layout_msgwin(0, 0, 0) && canblock) {
        layout_msgwin(1, 0, 1);
        keypress_at_more();
    }
    layout_msgwin(1, 0, 0);
    #endif
}

static void
curses_print_message_core(int turn, const char *msg, nh_bool important)
{
    
    if (!important && num_showlines == 1)
        return;

    nh_bool keep_going = TRUE;
    if (!histlines)
        alloc_hist_array();

    //XXX: I tried stack allocating this in a VLA and wound up with mysterious
    //bus errors.  Usage of intermediate is simple enough that heap is fairly
    //safe here, but it leaves a bad taste in my mouth.
    char *intermediate;
    intermediate = calloc(strlen(msg) + 1, sizeof (char));
    int intermediate_size = strlen(msg) + 1;
    strcpy(intermediate, msg);
    while (keep_going) {
        keep_going = update_showlines(&intermediate, &intermediate_size, FALSE);
        show_msgwin(keep_going);
        if (keep_going)
            keypress_at_more();
    }
    free(intermediate);
    #if 0
    struct msghist_entry *h;

    if (!histlines)
        alloc_hist_array();

    h = histlines + histlines_pointer;

    last_line_reserved = 0;

    free(h->message); /* in case there was something there */
    h->turn = turn;
    h->message = malloc(strlen(msg)+1);
    strcpy(h->message, msg);
    h->old = 0;
    h->unseen = canblock;
    h->nomerge = 0;

    if (first_new == -1)
        first_new = histlines_pointer;
    if (first_unseen == -1 && canblock)
        first_unseen = histlines_pointer;

    histlines_pointer++;
    histlines_pointer %= histlines_alloclen;

    free(histlines[histlines_pointer].message);
    histlines[histlines_pointer].message = 0;

    if (!layout_msgwin(0, 0, 0))
        force_seen(0); /* print a --More-- at the appropriate point */
    else
        layout_msgwin(1, 0, 0);
    #endif
}

/* Prints a message onto the screen, and into message history. The code will
   ensure that the user sees the message (e.g. with --More--). */
void
curses_print_message(int turn, const char *inmsg)
{
    curses_print_message_core(turn, inmsg, TRUE);
}

/* Prints a message into message history, and shows it onscreen unless there are
   more important messages to show. */
void
curses_print_message_nonblocking(int turn, const char *inmsg)
{
    curses_print_message_core(turn, inmsg, FALSE);
}

/* Ensure that the user has seen all messages printed so far, before
   continuing to run the rest of the program. */
void
pause_messages(void)
{
    stopmore = 0;
    force_seen();
}

/* Displays the message history. */
void
doprev_message(void)
{
    int i, j, curlinelen = 0, lines = 0;
    struct nh_menulist menu;
    char *curline = NULL;
    char **buf = NULL;

    init_menulist(&menu);

    if (!histlines)
        alloc_hist_array();

    i = histlines_pointer;
    do {
        if (!histlines[i].message) {
            i = (i + 1) % settings.msghistory;
            continue;
        }
        realloc_strcat(&curline, &curlinelen, histlines[i].message);
        /* This'll mean the string always has two spaces at the end, but
           wrap_text will take care of them for us. */
        realloc_strcat(&curline, &curlinelen, "  ");
        /* If either the next line is where we wrap around or the next line
           is the start of a new turn's worth of messages, quit appending.
           Otherwise, make another append pass. */
        if (!histlines[i].nomerge &&
            ((i + 1) % settings.msghistory != histlines_pointer)) {
            i = (i + 1) % settings.msghistory;
            continue;
        }
        /* Subtracting 3 is necessary to prevent curses_display_menu in
           smallterm games from eating the last part of the message here.
           Subtracting 4 allows slight indentation where appropriate. */
        wrap_text(getmaxx(msgwin) - 4, curline, &lines, &buf);
        free(curline);
        curline = NULL;
        curlinelen = 0;
        for (j = 0; j < lines; j++) {
            /* If a message wraps, very slightly indent the additional lines
               to make them obvious. */
            char tempstr[getmaxx(msgwin)];
            sprintf(tempstr, "%s%s", j == 0 ? "" : " ", buf[j]);
            add_menu_txt(&menu, tempstr, MI_TEXT);
        }
        free_wrap(buf);
        buf = NULL;
        i = (i + 1) % settings.msghistory;
    } while (i != histlines_pointer);

    curses_display_menu_core(&menu, "Previous messages:", PICK_NONE, NULL, 0, 0,
                             -1, -1, TRUE, NULL);
}

/* Given the string "input", generate a series of strings of the given maximum
   width, wrapping lines at spaces in the text. The number of lines will be
   stored into *output_count, and an array of the output lines will be stored in
   *output. The memory for both the output strings and the output array is
   obtained via malloc and should be freed when no longer needed. */
void
wrap_text(int width, const char *input, int *output_count, char ***output)
{
    const int min_width = 20, max_wrap = 20;
    int len = strlen(input);
    int input_idx, input_lidx;
    int idx, outcount;

    *output = malloc(max_wrap * sizeof (char *));
    for (idx = 0; idx < max_wrap; idx++)
        (*output)[idx] = NULL;

    input_idx = 0;
    outcount = 0;
    do {
        if (len - input_idx <= width) {
            (*output)[outcount] = strdup(input + input_idx);
            outcount++;
            break;
        }

        for (input_lidx = input_idx + width;
             !isspace(input[input_lidx]) && input_lidx - input_idx > min_width;
             input_lidx--) {}

        if (!isspace(input[input_lidx]))
            input_lidx = input_idx + width;

        (*output)[outcount] = malloc(input_lidx - input_idx + 1);
        strncpy((*output)[outcount], input + input_idx, input_lidx - input_idx);
        (*output)[outcount][input_lidx - input_idx] = '\0';
        outcount++;

        for (input_idx = input_lidx; isspace(input[input_idx]); input_idx++) {}
    } while (input[input_idx] && outcount < max_wrap);

    *output_count = outcount;
}


void
free_wrap(char **wrap_output)
{
    const int max_wrap = 20;
    int idx;

    for (idx = 0; idx < max_wrap; idx++) {
        if (!wrap_output[idx])
            break;
        free(wrap_output[idx]);
    }
    free(wrap_output);
}
