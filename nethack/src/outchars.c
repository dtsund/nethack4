/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2014-10-05 */
/* Copyright (c) Daniel Thaler, 2011 */
/* NetHack may be freely redistributed.  See license for details. */

/* NOTE: This file is utf-8 encoded; saving with a non utf-8 aware editor WILL
 * damage some symbols */

#include "nhcurses.h"
#include "tilesequence.h"
#include <ctype.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>


static int corpse_id, vcdoor_id, hcdoor_id, ndoor_id;
static int upstair_id, upladder_id, upsstair_id;
static int dnstair_id, dnladder_id, dnsstair_id;
static int mportal_id, vibsquare_id;
static int room_id, darkroom_id, corr_id, litcorr_id;
struct curses_drawing_info *default_drawing, *cur_drawing;
int curses_level_display_mode;
static struct curses_drawing_info *unicode_drawing, *rogue_drawing;

char *tiletable;
int tiletable_len;
nh_bool tiletable_is_cchar = 1;

/* Additionaly glyphs not present in the NetHack core but that we want
 * to include to allow for customizability. */
static struct curses_symdef extra_bg_syms[] = {
    {"darkroom", CLR_DARK_GRAY, {0x00B7, 0}, 0},   /* · centered dot */
    {"litcorr", CLR_WHITE, {0x2592, 0}, 0},    /* ▒ medium shading */
};

static struct curses_symdef default_ovr[] = {
    {"boulder", -1, {0x0030, 0}, '0'}   /* backtick is the worst for boulders */
};

static struct curses_symdef rogue_graphics_ovr[] = {
    {"vodoor", -1, {0x002B, 0}, '+'},
    {"hodoor", -1, {0x002B, 0}, '+'},
    {"ndoor", -1, {0x002B, 0}, '+'},
    {"upstair", -1, {0x0025, 0}, '%'},
    {"dnstair", -1, {0x0025, 0}, '%'},
    {"upsstair", -1, {0x0025, 0}, '%'},
    {"dnsstair", -1, {0x0025, 0}, '%'},

    {"gold piece", -1, {0x002A, 0}, '*'},

    {"corpse", -1, {0x003A, 0}, ':'},   /* the 2 most common food items... */
    {"food ration", -1, {0x003A, 0}, ':'}
};

/* Warning: Please restrict these to Unicode characters that exist on
   Windows Glyph List 4. (Windows is the worst out of the major operating
   systems at rendering fonts.) They should also have an exact or close
   approximation on code page 437. */
static struct curses_symdef unicode_graphics_ovr[] = {
    /* bg */
    {"vwall", -1, {0x2502, 0}, 0},      /* │ vertical rule */
    {"hwall", -1, {0x2500, 0}, 0},      /* ─ horizontal rule */
    {"tlcorn", -1, {0x250C, 0}, 0},     /* ┌ top left corner */
    {"trcorn", -1, {0x2510, 0}, 0},     /* ┐ top right corner */
    {"blcorn", -1, {0x2514, 0}, 0},     /* └ bottom left */
    {"brcorn", -1, {0x2518, 0}, 0},     /* ┘ bottom right */
    {"crwall", -1, {0x253C, 0}, 0},     /* ┼ cross */
    {"tuwall", -1, {0x2534, 0}, 0},     /* T up */
    {"tdwall", -1, {0x252C, 0}, 0},     /* T down */
    {"tlwall", -1, {0x2524, 0}, 0},     /* T left */
    {"trwall", -1, {0x251C, 0}, 0},     /* T right */
    {"ndoor", -1, {0x00B7, 0}, 0},      /* · centered dot */
    {"vodoor", -1, {0x25A0, 0}, 0},     /* ■ solid block */
    {"hodoor", -1, {0x25A0, 0}, 0},     /* ■ solid block */
    {"bars", -1, {0x2261, 0}, 0},       /* ≡ equivalence symbol */
    {"fountain", -1, {0x2320, 0}, 0},   /* ⌠ top half of integral */
    {"room", -1, {0x00B7, 0}, 0},       /* · centered dot */
    {"corr", -1, {0x2591, 0}, 0},       /* ░ light shading */
    {"altar", -1, {0x03A9, 0}, 0},      /* Ω GREEK CAPITAL LETTER OMEGA */
    {"ice", -1, {0x00B7, 0}, 0},        /* · centered dot */
    {"vodbridge", -1, {0x00B7, 0}, 0},  /* · centered dot */
    {"hodbridge", -1, {0x00B7, 0}, 0},  /* · centered dot */

    /* zap */
    {"zap_v", -1, {0x2502, 0}, 0},      /* │ vertical rule */
    {"zap_h", -1, {0x2500, 0}, 0},      /* ─ horizontal rule */

    /* swallow */
    {"swallow_top_c", -1, {0x2500, 0}, 0},      /* ─ horizontal rule */
    {"swallow_mid_l", -1, {0x2502, 0}, 0},      /* │ vertical rule */
    {"swallow_mid_r", -1, {0x2502, 0}, 0},      /* │ vertical rule */
    {"swallow_bot_c", -1, {0x2500, 0}, 0},      /* ─ horizontal rule */

    /* explosion */
    {"exp_top_c", -1, {0x2500, 0}, 0},  /* ─ horizontal rule */
    {"exp_mid_l", -1, {0x2502, 0}, 0},  /* │ vertical rule */
    {"exp_mid_r", -1, {0x2502, 0}, 0},  /* │ vertical rule */
    {"exp_bot_c", -1, {0x2500, 0}, 0},  /* ─ horizontal rule */

    /* traps */
    {"web", -1, {0x0256C, 0}, 0},       /* ╬ double cross */

    {"pool", -1, {0x2248, 0}, 0},       /* ≈ double tilde */
    {"lava", -1, {0x2248, 0}, 0},       /* ≈ double tilde */
    {"water", -1, {0x2248, 0}, 0},      /* ≈ double tilde */
    {"tree", -1, {0x00b1, 0}, 0},       /* ± plus-or-minus sign */

    /* objects */
    {"boulder", -1, {0x0030, 0}, 0},    /* 0 zero */
};

#define listlen(list) (sizeof(list)/sizeof(list[0]))

static void print_tile_number(WINDOW *, int, unsigned long long);

static void
add_sym_list(struct curses_symdef **list, int len,
             const struct curses_symdef *adtnl, int num)
{
    int i;

    struct curses_symdef *newlist =
        malloc((len + num) * sizeof (struct curses_symdef));

    for (i = 0; i < len; i++)
        newlist[i] = (*list)[i];
    for (i = 0; i < num; i++) {
        newlist[i + len] = adtnl[i];
        /* We need to reallocate these because they will be freed */
        const char *name = newlist[i + len].symname;
        char *str = malloc(strlen(name) + 1);
        strcpy(str, name);
        newlist[i + len].symname = str;
    }

    free(*list);
    *list = newlist;
}


static void
add_extra_syms(struct curses_drawing_info *di)
{
    int num_bg = listlen(extra_bg_syms);

    add_sym_list(&di->bgelements, di->num_bgelements, extra_bg_syms, num_bg);

    di->num_bgelements += num_bg;
}


static nh_bool
apply_override_list(struct curses_symdef *list, int len,
                    const struct curses_symdef *ovr, nh_bool cust)
{
    int i;

    for (i = 0; i < len; i++)
        if (!strcmp(list[i].symname, ovr->symname)) {
            if (ovr->unichar[0])
                memcpy(list[i].unichar, ovr->unichar,
                       sizeof (wchar_t) * CCHARW_MAX);
            if (ovr->ch)
                list[i].ch = ovr->ch;
            if (ovr->color != -1)
                list[i].color = ovr->color;
            list[i].custom = cust;
            return TRUE;
        }
    return FALSE;
}


static void
apply_override(struct curses_drawing_info *di, const struct curses_symdef *ovr,
               int olen, nh_bool cust)
{
    int i;
    nh_bool ok;

    for (i = 0; i < olen; i++) {
        ok = FALSE;
        /* the override will effect exactly one of the symbol lists */
        ok |=
            apply_override_list(di->bgelements, di->num_bgelements, &ovr[i],
                                cust);
        ok |= apply_override_list(di->traps, di->num_traps, &ovr[i], cust);
        ok |= apply_override_list(di->objects, di->num_objects, &ovr[i], cust);
        ok |=
            apply_override_list(di->monsters, di->num_monsters, &ovr[i], cust);
        ok |=
            apply_override_list(di->warnings, di->num_warnings, &ovr[i], cust);
        ok |= apply_override_list(di->invis, 1, &ovr[i], cust);
        ok |= apply_override_list(di->effects, di->num_effects, &ovr[i], cust);
        ok |=
            apply_override_list(di->expltypes, di->num_expltypes, &ovr[i],
                                cust);
        ok |= apply_override_list(di->explsyms, NUMEXPCHARS, &ovr[i], cust);
        ok |=
            apply_override_list(di->zaptypes, di->num_zaptypes, &ovr[i], cust);
        ok |= apply_override_list(di->zapsyms, NUMZAPCHARS, &ovr[i], cust);
        ok |=
            apply_override_list(di->swallowsyms, NUMSWALLOWCHARS, &ovr[i],
                                cust);

        if (!ok)
            fprintf(stdout, "sym override %s could not be applied\n",
                    ovr[i].symname);
    }
}


static struct curses_symdef *
load_nh_symarray(const struct nh_symdef *src, int len)
{
    int i;
    struct curses_symdef *copy = malloc(len * sizeof (struct curses_symdef));

    memset(copy, 0, len * sizeof (struct curses_symdef));

    for (i = 0; i < len; i++) {
        copy[i].symname = strdup(src[i].symname);
        copy[i].ch = src[i].ch;
        copy[i].color = src[i].color;

        /* this works because ASCII 0x?? (for ?? < 128) == Unicode U+00?? */
        copy[i].unichar[0] = (wchar_t) src[i].ch;
    }

    return copy;
}


static struct curses_drawing_info *
load_nh_drawing_info(const struct nh_drawing_info *orig)
{
    struct curses_drawing_info *copy =
        malloc(sizeof (struct curses_drawing_info));

    copy->num_bgelements = orig->num_bgelements;
    copy->num_traps = orig->num_traps;
    copy->num_objects = orig->num_objects;
    copy->num_monsters = orig->num_monsters;
    copy->num_warnings = orig->num_warnings;
    copy->num_expltypes = orig->num_expltypes;
    copy->num_zaptypes = orig->num_zaptypes;
    copy->num_effects = orig->num_effects;
    copy->bg_feature_offset = orig->bg_feature_offset;

    copy->bgelements = load_nh_symarray(orig->bgelements, orig->num_bgelements);
    copy->traps = load_nh_symarray(orig->traps, orig->num_traps);
    copy->objects = load_nh_symarray(orig->objects, orig->num_objects);
    copy->monsters = load_nh_symarray(orig->monsters, orig->num_monsters);
    copy->warnings = load_nh_symarray(orig->warnings, orig->num_warnings);
    copy->invis = load_nh_symarray(orig->invis, 1);
    copy->effects = load_nh_symarray(orig->effects, orig->num_effects);
    copy->expltypes = load_nh_symarray(orig->expltypes, orig->num_expltypes);
    copy->explsyms = load_nh_symarray(orig->explsyms, NUMEXPCHARS);
    copy->zaptypes = load_nh_symarray(orig->zaptypes, orig->num_zaptypes);
    copy->zapsyms = load_nh_symarray(orig->zapsyms, NUMZAPCHARS);
    copy->swallowsyms = load_nh_symarray(orig->swallowsyms, NUMSWALLOWCHARS);

    return copy;
}


static void
read_sym_line(char *line)
{
    struct curses_symdef ovr;
    char symname[64];
    char *bp;
    int unichar;

    if (!strlen(line) || line[0] != '!' || line[1] != '"')
        return;

    line++;     /* skip the ! */
    memset(&ovr, 0, sizeof (struct curses_symdef));

    /* line format: "symbol name" color unicode [combining marks] */
    bp = &line[1];
    while (*bp && *bp != '"')
        bp++;   /* find the end of the symname */
    strncpy(symname, &line[1], bp - &line[1]);
    symname[bp - &line[1]] = '\0';
    ovr.symname = symname;
    bp++;       /* go past the " at the end of the symname */

    while (*bp && isspace(*bp))
        bp++;   /* find the start of the next value */
    sscanf(bp, "%d", &ovr.color);

    while (*bp && !isspace(*bp))
        bp++;   /* go past the previous value */
    sscanf(bp, "%x", &unichar);
    ovr.unichar[0] = (wchar_t) unichar;

    apply_override(unicode_drawing, &ovr, 1, TRUE);
}


static void
read_unisym_config(void)
{
    fnchar filename[BUFSZ];
    char *line;
    int fd, size;

    filename[0] = '\0';
    if (ui_flags.connection_only || !get_gamedir(CONFIG_DIR, filename))
        return;
    fnncat(filename, FN("unicode.conf"), BUFSZ - fnlen(filename) - 1);

    fd = sys_open(filename, O_RDONLY, 0);
    if (fd == -1)
        return;

    size = lseek(fd, 0, SEEK_END);
    lseek(fd, 0, SEEK_SET);

    char data[size + 1];
    int dptr = 0, rrv;
    while (dptr < size) {
        errno = 0;
        rrv = read(fd, data + dptr, size - dptr);
        if (rrv < 0 && errno != EINTR) {
            curses_msgwin("Could not read unicode.conf!", krc_notification);
            return;
        }
        dptr += rrv;
    }
    data[size] = '\0';
    close(fd);

    line = strtok(data, "\r\n");
    while (line) {
        read_sym_line(line);

        line = strtok(NULL, "\r\n");
    }
}


static nh_bool
write_symlist(int fd, const struct curses_symdef *list, int len)
{
    char buf[BUFSZ];
    int i;

    for (i = 0; i < len; i++) {
        snprintf(buf, ARRAY_SIZE(buf), "%c\"%s\"\t%d\t%04x\n", list[i].custom ? '!' : '#',
                list[i].symname, list[i].color, (int)list[i].unichar[0]);
        
        if (write(fd, buf, strlen(buf)) < 0) {
            curses_msgwin("Could not write unicode.conf.", krc_notification);
            return 0;
        }
    }
    return 1;
}

static const char uniconf_header[] =
    "# Unicode symbol configuration for NetHack\n"
    "# Lines that begin with '#' are commented out.\n"
    "# Change the '#' to an '!' to activate a line.\n";

static void
write_unisym_config(void)
{
    fnchar filename[BUFSZ];
    int fd;

    filename[0] = '\0';
    if (ui_flags.connection_only || !get_gamedir(CONFIG_DIR, filename))
        return;
    fnncat(filename, FN("unicode.conf"), BUFSZ - fnlen(filename) - 1);

    fd = sys_open(filename, O_TRUNC | O_CREAT | O_RDWR, 0660);
    if (fd == -1)
        return;

    if (write(fd, uniconf_header, strlen(uniconf_header)) < 0) {
        curses_msgwin("Could not write unicode.conf.", krc_notification);
        return;
    }
    /* write_symlist returns 0 on error; thus && has the right semantics to run
       until we get an error */
    (void)(
        write_symlist(fd, unicode_drawing->bgelements,
                  unicode_drawing->num_bgelements) &&
        write_symlist(fd, unicode_drawing->traps, unicode_drawing->num_traps) &&
        write_symlist(fd, unicode_drawing->objects,
                      unicode_drawing->num_objects) &&
        write_symlist(fd, unicode_drawing->monsters,
                      unicode_drawing->num_monsters) &&
        write_symlist(fd, unicode_drawing->warnings,
                      unicode_drawing->num_warnings) &&
        write_symlist(fd, unicode_drawing->invis, 1) &&
        write_symlist(fd, unicode_drawing->effects,
                      unicode_drawing->num_effects) &&
        write_symlist(fd, unicode_drawing->expltypes,
                      unicode_drawing->num_expltypes) &&
        write_symlist(fd, unicode_drawing->explsyms, NUMEXPCHARS) &&
        write_symlist(fd, unicode_drawing->zaptypes,
                      unicode_drawing->num_zaptypes) &&
        write_symlist(fd, unicode_drawing->zapsyms, NUMZAPCHARS) &&
        write_symlist(fd, unicode_drawing->swallowsyms, NUMSWALLOWCHARS));

    close(fd);
}


void
init_displaychars(void)
{
    int i;
    struct nh_drawing_info *dinfo = nh_get_drawing_info();

    default_drawing = load_nh_drawing_info(dinfo);
    unicode_drawing = load_nh_drawing_info(dinfo);
    rogue_drawing = load_nh_drawing_info(dinfo);

    add_extra_syms(default_drawing);
    add_extra_syms(unicode_drawing);
    add_extra_syms(rogue_drawing);

    apply_override(default_drawing, default_ovr, ARRAY_SIZE(default_ovr),
                   FALSE);
    apply_override(unicode_drawing, unicode_graphics_ovr,
                   ARRAY_SIZE(unicode_graphics_ovr), FALSE);
    apply_override(rogue_drawing, rogue_graphics_ovr,
                   ARRAY_SIZE(rogue_graphics_ovr), FALSE);

    read_unisym_config();

    cur_drawing = default_drawing;

    /* find objects that need special treatment */
    for (i = 0; i < cur_drawing->num_objects; i++) {
        if (!strcmp("corpse", cur_drawing->objects[i].symname))
            corpse_id = i;
    }
    for (i = 0; i < cur_drawing->num_bgelements; i++) {
        if (!strcmp("vcdoor", cur_drawing->bgelements[i].symname))
            vcdoor_id = i;
        if (!strcmp("hcdoor", cur_drawing->bgelements[i].symname))
            hcdoor_id = i;
        if (!strcmp("ndoor", cur_drawing->bgelements[i].symname))
            ndoor_id = i;
        if (!strcmp("upstair", cur_drawing->bgelements[i].symname))
            upstair_id = i;
        if (!strcmp("upladder", cur_drawing->bgelements[i].symname))
            upladder_id = i;
        if (!strcmp("upsstair", cur_drawing->bgelements[i].symname))
            upsstair_id = i;
        if (!strcmp("dnstair", cur_drawing->bgelements[i].symname))
            dnstair_id = i;
        if (!strcmp("dnladder", cur_drawing->bgelements[i].symname))
            dnladder_id = i;
        if (!strcmp("dnsstair", cur_drawing->bgelements[i].symname))
            dnsstair_id = i;
        if (!strcmp("room", cur_drawing->bgelements[i].symname))
            room_id = i;
        if (!strcmp("corr", cur_drawing->bgelements[i].symname))
            corr_id = i;
        if (!strcmp("darkroom", cur_drawing->bgelements[i].symname))
            darkroom_id = i;
        if (!strcmp("litcorr", cur_drawing->bgelements[i].symname))
            litcorr_id = i;
    }
    for (i = 0; i < cur_drawing->num_traps; i++) {
        if (!strcmp("magic portal", cur_drawing->traps[i].symname))
            mportal_id = i;
        if (!strcmp("vibrating square", cur_drawing->traps[i].symname))
            vibsquare_id = i;
    }

    /* options are parsed before display is initialized, so redo switch */
    switch_graphics(settings.graphics);
}


static void
free_symarray(struct curses_symdef *array, int len)
{
    int i;

    for (i = 0; i < len; i++)
        free((char *)array[i].symname);

    free(array);
}


static void
free_drawing_info(struct curses_drawing_info *di)
{
    free_symarray(di->bgelements, di->num_bgelements);
    free_symarray(di->traps, di->num_traps);
    free_symarray(di->objects, di->num_objects);
    free_symarray(di->monsters, di->num_monsters);
    free_symarray(di->warnings, di->num_warnings);
    free_symarray(di->invis, 1);
    free_symarray(di->effects, di->num_effects);
    free_symarray(di->expltypes, di->num_expltypes);
    free_symarray(di->explsyms, NUMEXPCHARS);
    free_symarray(di->zaptypes, di->num_zaptypes);
    free_symarray(di->zapsyms, NUMZAPCHARS);
    free_symarray(di->swallowsyms, NUMSWALLOWCHARS);

    free(di);
}


void
free_displaychars(void)
{
    write_unisym_config();

    free_drawing_info(default_drawing);
    free_drawing_info(unicode_drawing);
    free_drawing_info(rogue_drawing);

    default_drawing = rogue_drawing = NULL;
}


void
print_low_priority_brandings(WINDOW *win, struct nh_dbuf_entry *dbe)
{
    enum nhcurses_brandings branding = nhcurses_no_branding;
    if (dbe->bg == vcdoor_id || dbe->bg == hcdoor_id) {
        if (dbe->branding & NH_BRANDING_LOCKED)
            branding = nhcurses_genbranding_locked;
        else if (dbe->branding & NH_BRANDING_UNLOCKED)
            branding = nhcurses_genbranding_unlocked;
    }
    if (settings.floorcolor) {
        if ((dbe->bg == room_id || dbe->bg == corr_id ||
             dbe->bg == litcorr_id || dbe->bg == darkroom_id)
            && dbe->branding & NH_BRANDING_STEPPED)
            branding = nhcurses_genbranding_stepped;
    }
    if (branding != nhcurses_no_branding) {
        print_tile_number(win, TILESEQ_GENBRAND_OFF +
                          branding - nhcurses_genbranding_first, 0);
    }
}

void
print_high_priority_brandings(WINDOW *win, struct nh_dbuf_entry *dbe)
{
    enum nhcurses_brandings branding = nhcurses_no_branding;
    if ((dbe->monflags & MON_TAME) && settings.hilite_pet)
        branding = nhcurses_monbranding_tame;
    if ((dbe->monflags & MON_PEACEFUL) && settings.hilite_pet)
        branding = nhcurses_monbranding_peaceful;

    if (branding != nhcurses_no_branding) {
        print_tile_number(win, TILESEQ_MONBRAND_OFF +
                          branding - nhcurses_monbranding_first, 0);
    }

    if (dbe->trap || (dbe->branding & NH_BRANDING_TRAPPED))
        print_tile_number(win, TILESEQ_GENBRAND_OFF +
                          nhcurses_genbranding_trapped -
                          nhcurses_genbranding_first, 0);
}

/* What is the bottom-most, opaque background of a map square? */
static enum {
    fb_room,
    fb_corr,
}
furthest_background(const struct nh_dbuf_entry *dbe)
{
    boolean room = dbe->bg != corr_id;

    return room ? fb_room : fb_corr;
}

int
mapglyph(struct nh_dbuf_entry *dbe, struct curses_symdef *syms, int *bg_color)
{
    int id, count = 0;
    unsigned long long substitution = dbe_substitution(dbe);

    if (dbe->effect) {
        id = NH_EFFECT_ID(dbe->effect);

        switch (NH_EFFECT_TYPE(dbe->effect)) {
        case E_EXPLOSION:
            syms[0] = cur_drawing->explsyms[id % NUMEXPCHARS];
            syms[0].color = cur_drawing->expltypes[id / NUMEXPCHARS].color;
            break;

        case E_SWALLOW:
            syms[0] = cur_drawing->swallowsyms[id & 0x7];
            syms[0].color = cur_drawing->monsters[id >> 3].color;
            break;

        case E_ZAP:
            syms[0] = cur_drawing->zapsyms[id & 0x3];
            syms[0].color = cur_drawing->zaptypes[id >> 2].color;
            break;

        case E_MISC:
            syms[0] = cur_drawing->effects[id];
            syms[0].color = cur_drawing->effects[id].color;
            break;
        }

        return 1;       /* we don't want to show other glyphs under effects */
    }

    if (dbe->invis)
        syms[count++] = cur_drawing->invis[0];

    else if (dbe->mon) {
        if (dbe->mon > cur_drawing->num_monsters &&
            (dbe->monflags & MON_WARNING)) {
            id = dbe->mon - 1 - cur_drawing->num_monsters;
            syms[count++] = cur_drawing->warnings[id];
        } else {
            id = dbe->mon - 1;
            syms[count++] = cur_drawing->monsters[id];
        }
    }

    if (dbe->obj) {
        id = dbe->obj - 1;
        if (id == corpse_id) {
            syms[count] = cur_drawing->objects[id];
            syms[count].color = cur_drawing->monsters[dbe->obj_mn - 1].color;
            count++;
        } else
            syms[count++] = cur_drawing->objects[id];
    }

    if (dbe->trap) {
        id = dbe->trap - 1;
        syms[count++] = cur_drawing->traps[id];
        if (settings.bgbranding) {
            if (dbe->trap == mportal_id + 1 || dbe->trap == vibsquare_id + 1)
                *bg_color = CLR_RED;
            else
                *bg_color = CLR_CYAN;
        }
    }

    /* omit the background symbol from the list if it is boring */
    if (count == 0 || dbe->bg >= cur_drawing->bg_feature_offset) {
        syms[count++] = cur_drawing->bgelements[dbe->bg];

        /* overrides for branding; note that although we're told whether open
           doors are locked/unlocked, it doesn't make much sense to display
           that */
        if (dbe->bg == vcdoor_id || dbe->bg == hcdoor_id) {
            if (dbe->branding & NH_BRANDING_TRAPPED)
                syms[count - 1].color = CLR_CYAN;
            else if (dbe->branding & NH_BRANDING_LOCKED)
                syms[count - 1].color = CLR_RED;
            else if (dbe->branding & NH_BRANDING_UNLOCKED)
                syms[count - 1].color = CLR_GREEN;
        }

        /* Implement lighting display. We override the background of dark room
           and light corridor tiles. */
        int stepped_color = CLR_BROWN;

        if (furthest_background(dbe) == fb_room &&
            substitution & NHCURSES_SUB_UNLIT) {
            if (dbe->bg == room_id)
                syms[count-1] = cur_drawing->bgelements[darkroom_id];
            stepped_color = CLR_BLUE;
        } else if (furthest_background(dbe) == fb_corr &&
                   substitution & NHCURSES_SUB_LIT) {
            if (dbe->bg == corr_id)
                syms[count-1] = cur_drawing->bgelements[litcorr_id];
        }

        /* Override darkroom for stepped-on squares, so the player can see
           where they stepped. */
        if (settings.floorcolor && (dbe->branding & NH_BRANDING_STEPPED) &&
            (dbe->bg == room_id || dbe->bg == ndoor_id || dbe->bg == corr_id))
                syms[count - 1].color = stepped_color;

        if (dbe->bg == upstair_id || dbe->bg == dnstair_id ||
            dbe->bg == upladder_id || dbe->bg == dnladder_id ||
            dbe->bg == upsstair_id || dbe->bg == dnsstair_id) {
            if (settings.bgbranding)
                *bg_color = CLR_RED;
        }
    }

    return count;       /* count <= 4 */
}


void
set_rogue_level(nh_bool enable)
{
    if (enable)
        cur_drawing = rogue_drawing;
    else
        switch_graphics(settings.graphics);
}


void
curses_notify_level_changed(int dmode)
{
    set_rogue_level(dmode == LDM_ROGUE);
    mark_mapwin_for_full_refresh();
    curses_level_display_mode = dmode;
}


unsigned long long
dbe_substitution(struct nh_dbuf_entry *dbe)
{
    unsigned long long s = NHCURSES_SUB_LDM(curses_level_display_mode);

    /* TODO: Do we want this behaviour (that approximates 3.4.3 behaviour) for
       the "lit" substitution? Do we want it to be customizable?

       Another option is to have multiple substitutions, but that's starting to
       get silly. */
    short lit_branding = dbe->bg == corr_id ?
        (NH_BRANDING_LIT | NH_BRANDING_TEMP_LIT) :
        (NH_BRANDING_LIT | NH_BRANDING_TEMP_LIT | NH_BRANDING_SEEN);

    s |= (dbe->branding & lit_branding) ? NHCURSES_SUB_LIT : NHCURSES_SUB_UNLIT;

    /* TODO: Determine which Quest this tile belongs to (if any), and
       race/gender of a player-monster on the tile */

    return s;
}

void
switch_graphics(enum nh_text_mode mode)
{
    switch (mode) {
    case ASCII_GRAPHICS:
        cur_drawing = default_drawing;
        break;

    default: /* tiles */
    case UNICODE_GRAPHICS:
        cur_drawing = unicode_drawing;
        break;
    }
}


void
print_sym(WINDOW * win, struct curses_symdef *sym, int extra_attrs, int bgcolor)
{
    int attr;
    cchar_t uni_out;

    /* nethack color index -> curses color */
    attr = A_NORMAL | extra_attrs;
    attr |= curses_color_attr(sym->color & 0x1F, bgcolor);
    if (sym->color & 0x20)
        attr |= A_UNDERLINE;
    if (sym->color & 0x40 && settings.use_inverse && !bgcolor) {
        attr |= A_REVERSE;
        attr &= ~A_BOLD;
    }

    int color = PAIR_NUMBER(attr);

    setcchar(&uni_out, sym->unichar, attr, color, NULL);
    wadd_wch(win, &uni_out);
}

static inline unsigned long
get_tt_number(int tt_offset)
{
    unsigned long l = 0;
    l += (unsigned long)(unsigned char)tiletable[tt_offset + 0] << 0;
    l += (unsigned long)(unsigned char)tiletable[tt_offset + 1] << 8;
    l += (unsigned long)(unsigned char)tiletable[tt_offset + 2] << 16;
    l += (unsigned long)(unsigned char)tiletable[tt_offset + 3] << 24;
    return l;
}

static void
print_tile_number(WINDOW *win, int tileno, unsigned long long substitutions)
{
    /* Find the tile in question in the tile table. The rules:
       - The tile numbers must be equal;
       - The substitutions of the tile in the table must be a subset of
         the substitutions of the tile we want to draw;
       - If multiple tiles fit these constraints, draw the one with the
         numerically largest substitution number.

       The tile table itself is formatted as follows:
       4 bytes: tile number
       8 bytes: substiutitions
       4 bytes: image index (i.e. wset_tiles_tile argument)

       All these numbers are little-endian, regardless of the endianness of the
       system we're running on.

       The tile table is sorted by number then substitutions, so we can use a
       binary search. */

    int ttelements = tiletable_len / 16;
    int low = 0, high = ttelements;

    if (ttelements == 0)
        return; /* no tile table */

    /* Invariant: tiles not in low .. high inclusive are definitely not the
       tile we're looking for */
    while (low < high) {
        int pivot = (low + high) / 2;
        int table_tileno = get_tt_number(pivot * 16);

        if (table_tileno < tileno)
            low = pivot + 1;
        else if (table_tileno > tileno)
            high = pivot - 1;
        else {
            /* We're somewhere in the section for this tile. Find its bounds. */
            low = high = pivot;
            while (low >= 0 && get_tt_number(low * 16) == tileno)
                low--;
            low++;
            while (high < ttelements && get_tt_number(high * 16) == tileno)
                high++;
            high--;

            for (pivot = high; pivot >= low; pivot--) {
                unsigned long long table_substitutions =
                    ((unsigned long long)get_tt_number(pivot * 16 + 4)) +
                    ((unsigned long long)get_tt_number(pivot * 16 + 8) << 32);

                if ((table_substitutions & substitutions) ==
                    table_substitutions) {
                    low = pivot;
                    break;
                }
            }
            break;
        }
    }

    /* can happen if the tileset is missing high-numbered tiles */
    if (low >= ttelements)
        low = ttelements - 1;

    wset_tiles_tile(win, get_tt_number(low * 16 + 12));
}

void
print_tile(WINDOW *win, struct curses_symdef *api_name, 
           struct curses_symdef *api_type, int offset,
           unsigned long long substitutions)
{
    int tileno = tileno_from_api_name(
        api_name->symname, api_type ? api_type->symname : NULL, offset);
    /* TODO: better rendition for missing tiles than just using the unexplored
       area tile */
    if (tileno == TILESEQ_INVALID_OFF) tileno = 0;

    print_tile_number(win, tileno, substitutions);
}

const char *const furthest_backgrounds[] = {
    [fb_room] = "the floor of a room",
    [fb_corr] = "corridor",
};

static int furthest_background_tileno[sizeof furthest_backgrounds /
                                      sizeof *furthest_backgrounds];
static nh_bool furthest_background_tileno_needs_initializing = 1;

void
print_background_tile(WINDOW *win, struct nh_dbuf_entry *dbe)
{
    unsigned long long substitutions = dbe_substitution(dbe);
    if (furthest_background_tileno_needs_initializing) {
        int i;
        for (i = 0; i < sizeof furthest_backgrounds /
                 sizeof *furthest_backgrounds; i++) {
            furthest_background_tileno[i] =
                tileno_from_name(furthest_backgrounds[i], TILESEQ_CMAP_OFF);
        }
        furthest_background_tileno_needs_initializing = 0;
    }

    print_tile_number(win, furthest_background_tileno[furthest_background(dbe)],
                      substitutions);
    if (dbe->bg != room_id && dbe->bg != corr_id)
        print_tile(win, cur_drawing->bgelements + dbe->bg,
                   NULL, TILESEQ_CMAP_OFF, substitutions);
}

/* outchars.c */
