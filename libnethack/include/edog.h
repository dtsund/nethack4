/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2014-04-05 */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef EDOG_H
# define EDOG_H

# include "coord.h"

/*      various types of food, the lower, the better liked.     */

# define DOGFOOD 0
# define CADAVER 1
# define ACCFOOD 2
# define MANFOOD 3
# define APPORT  4
# define POISON  5
# define UNDEF   6
# define TABU    7

struct edog {
    unsigned int droptime;      /* moment dog dropped object */
    unsigned int dropdist;      /* dist of drpped obj from @ */
    int apport; /* amount of training */
    unsigned int whistletime;   /* last time he whistled */
    unsigned int hungrytime;    /* will get hungry at this time */
    int abuse;  /* track abuses to this pet */
    int revivals;       /* count pet deaths */
    int mhpmax_penalty; /* while starving, points reduced */
    coord ogoal;        /* previous goal location */
    unsigned killed_by_u:1;     /* you attempted to kill him */
};

# define EDOG(mon)       ((struct edog *)&(mon)->mextra[0])
# define CONST_EDOG(mon) ((const struct edog *)&(mon)->mextra[0])

#endif /* EDOG_H */

