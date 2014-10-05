/* vim:set cin ft=c sw=4 sts=4 ts=8 et ai cino=Ls\:0t0(0 : -*- mode:c;fill-column:80;tab-width:8;c-basic-offset:4;indent-tabs-mode:nil;c-file-style:"k&r" -*-*/
/* Last modified by Alex Smith, 2014-04-05 */
/* Copyright 1986, M. Stephenson                                  */
/* NetHack may be freely redistributed.  See license for details. */

#ifndef SPELL_H
# define SPELL_H

# include "global.h"

struct spell {
    short sp_id;        /* spell id (== object.otyp) */
    xchar sp_lev;       /* power level */
    int sp_know;        /* knowlege of spell */
};

enum spell_type {
    spell_cancellation,
    spell_first = spell_cancellation,
    spell_first_book = spell_cancellation,
    spell_cause_fear,
    spell_charm_monster,
    spell_clairvoyance,
    spell_create_familiar,
    spell_create_monster,
    spell_cone_of_cold,
    spell_confuse_monster,
    spell_cure_blindness,
    spell_cure_sickness,
    spell_detect_food,
    spell_detect_monsters,
    spell_detect_treasure,
    spell_detect_unseen,
    spell_dig,
    spell_drain_life,
    spell_extra_healing,
    spell_finger_of_death,
    spell_fireball,
    spell_force_bolt,
    spell_haste_self,
    spell_healing,
    spell_identify,
    spell_invisibility,
    spell_jumping,
    spell_knock,
    spell_levitation,
    spell_light,
    spell_magic_mapping,
    spell_magic_missile,
    spell_polymorph,
    spell_protection,
    spell_restore_ability,
    spell_remove_curse,
    spell_sleep,
    spell_slow_monster,
    spell_stone_to_flesh,
    spell_teleport_away,
    spell_turn_undead,
    spell_wizard_lock,
    spell_last_book = spell_wizard_lock,
    spell_enlightenment,
    spell_wishing,
    spell_striking,
    spell_make_invisible, /* permanent, targetable version */
    spell_speed_monster, /* permanent, targetable version */
    spell_probing,
    spell_fire_bolt, /* wand version, doesn't explode */
    spell_cold_bolt, /* wand version, doesn't explode */
    spell_death,
    spell_lightning_bolt, /* wand version, ranged */
    //spell_light_burst, /* broken wand of light */
    //spell_sleep_burst, /* broken wand of sleep */
    spell_psi_bolt,
    spell_first_monster = spell_psi_bolt,
    spell_cure_self,
    spell_haste_self_mon,
    spell_stun,
    spell_disappear,
    spell_drain_strength,
    spell_destroy_armor,
    spell_curse_items,
    spell_aggravate,
    spell_summon_nasties,
    spell_touch_of_death,
    spell_double_trouble,
    spell_open_wounds,
    spell_confuse,
    spell_paralyze,
    spell_blind,
    spell_summon_insects,
    spell_lightning, /* monster version, melee */
    spell_fire_pillar,
    spell_geyser,
    spell_last_monster = spell_geyser,
    spell_missile_breath,
    spell_first_breath = spell_missile_breath,
    /* spell_light_breath, */ /* Deferred feature. */
    spell_fire_breath,
    spell_cold_breath,
    spell_sleep_breath,
    spell_disintegration_breath,
    spell_lightning_breath,
    spell_poison_breath,
    spell_acid_breath,
    spell_last_breath = spell_acid_breath,
    spell_last = spell_acid_breath,
    spell_no_spell,
};

/* If you add things to this or change any orders, REMEMBER TO CHANGE
   RAY_TEXT. */
enum ray_type {
    RAY_MAGIC_MISSILE,
    RAY_FIRST = RAY_MAGIC_MISSILE,
    RAY_FIRE_BOLT,
    RAY_COLD_BOLT,
    RAY_SLEEP,
    RAY_DEATH,
    RAY_LIGHTNING,
    RAY_FIRE_BALL,
    RAY_COLD_CONE,
    RAY_DEATH_FINGER, /* Different from RAY_DEATH so the text can differ */
    RAY_MISSILE_BREATH,
    RAY_FIRE_BREATH,
    RAY_COLD_BREATH,
    RAY_SLEEP_BREATH,
    RAY_DISINT_BREATH,
    RAY_LIGHTNING_BREATH,
    RAY_POISON_BREATH,
    RAY_ACID_BREATH,
    RAY_LAST = RAY_ACID_BREATH,
};



enum spell_source {
    source_player_wand,
    source_first_player = source_player_wand,
    source_player_spell,
    source_player_breath,
    source_last_player = source_player_breath,
    source_monster_wand,
    source_first_monster = source_monster_wand,
    source_monster_spell,
    source_monster_breath,
    source_last_monster = source_monster_breath,
};

/* levels of memory destruction with a scroll of amnesia */
# define ALL_MAP          0x1
# define ALL_SPELLS       0x2

# define decrnknow(spell) spl_book[spell].sp_know--
# define spellid(spell)   spl_book[spell].sp_id
# define spellknow(spell) spl_book[spell].sp_know

#endif /* SPELL_H */

