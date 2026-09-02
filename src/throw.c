/*
 * throw.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include "rogue.h"
#include "throw.h"
#include "hit.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "mz_curses.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "trap.h"

static void consume_thrown_weapon(object *weapon);

void
throw(void)
{
    short wch;
    object *weapon;
    short dir, row, col;
    object *monster;
    u8 prompt[24];

    dir = (short)get_direction();
    if (dir == CANCEL) return;
    get_message(210, prompt, sizeof(prompt));
    if ((wch = (short)pack_letter((char *)prompt, WEAPON)) == CANCEL) return;
    check_message();

    if (!(weapon = get_letter_object(wch))) {
        message_id_mz(211, 0);
        return;
    }
    if ((weapon->in_use_flags & BEING_WIELDED) && weapon->is_cursed) {
        message_id_mz(85, 0);
        return;
    }
    row = rogue.row;
    col = rogue.col;
    monster = get_thrown_at_monster(weapon, dir, &row, &col);

    if (monster) {
        monster->m_flags &= ~ASLEEP;
        if (!throw_at_monster(monster, weapon)) {
            flop_weapon(weapon, row, col);
        }
    } else {
        flop_weapon(weapon, row, col);
    }
    consume_thrown_weapon(weapon);
    (void)reg_move();
}

int
throw_at_monster(object *monster, object *weapon)
{
    short damage, hit_chance;

    hit_chance = (short)get_hit_chance(weapon);
    damage = (short)get_weapon_damage(weapon);
    if (weapon->which_kind == ARROW && rogue.weapon &&
        rogue.weapon->which_kind == BOW) {
        damage += (short)get_weapon_damage(rogue.weapon);
        damage = (short)((damage * 2) / 3);
        hit_chance += hit_chance / 3;
    } else if ((weapon->in_use_flags & BEING_WIELDED) &&
               (weapon->which_kind == DAGGER ||
                weapon->which_kind == SHURIKEN ||
                weapon->which_kind == DART)) {
        damage = (short)((damage * 3) / 2);
        hit_chance += hit_chance / 3;
    }
    if (!rand_percent(hit_chance)) {
        message_id_mz(213, 0);
        return 0;
    }
    message_id_mz(214, 0);
    (void)mon_damage(monster, damage);
    return 1;
}

object *
get_thrown_at_monster(object *obj, short dir, short *row, short *col)
{
    short old_row = *row;
    short old_col = *col;
    short i;
    u8 tile;

    for (i = 0; i < 24; ++i) {
        get_dir_rc(dir, row, col, 0);
        if ((*row == old_row && *col == old_col) ||
            !is_passable(*row, *col)) {
            *row = old_row;
            *col = old_col;
            return 0;
        }
        if (monster_at(*row, *col)) return monster_at(*row, *col);
        if (DUNGEON_ATTR(*row, *col) == ATTR_VISIBLE) {
            tile = DUNGEON(*row, *col);
            mvaddch((u8)*row, (u8)*col, DC_R_BLACKET);
            move((u8)rogue.row, (u8)rogue.col);
            refresh();
            DUNGEON(*row, *col) = tile;
        }
        old_row = *row;
        old_col = *col;
        if (DUNGEON(*row, *col) == TILE_TUNNEL) i += 2;
    }
    (void)obj;
    return 0;
}

void
flop_weapon(object *weapon, short row, short col)
{
    object *new_weapon;
    short i;

    for (i = 0; i < 9; ++i) {
        short r = row;
        short c = col;

        rand_around(i, &r, &c);
        if (!is_passable(r, c) || object_at(&level_objects, r, c) ||
            monster_at(r, c) || (r == rogue.row && c == rogue.col) ||
            (r == stairs_row && c == stairs_col) ||
            trap_at(r, c) != NO_TRAP) continue;
        new_weapon = alloc_object();
        if (!new_weapon) break;
        *new_weapon = *weapon;
        new_weapon->quantity = 1;
        new_weapon->ichar = 0;
        new_weapon->in_use_flags = 0;
        new_weapon->next_object = 0;
        place_at(new_weapon, r, c);
        return;
    }
    message_id_mz(215, 0);
}

void
rand_around(short i, short *r, short *c)
{
    static const char ra[9] = { 0, 1, 1, -1, -1, 0, 1, 0, -1 };
    static const char ca[9] = { 0, 1, -1, 1, -1, 1, 0, -1, 0 };
    static short row, col;
    short n;

    if (i == 0) {
        row = *r;
        col = *c;
    }
    n = (short)((i + get_rand(0, 8)) % 9);
    *r = row + ra[n];
    *c = col + ca[n];
}

static void
consume_thrown_weapon(object *weapon)
{
    if (weapon->quantity > 1) {
        --weapon->quantity;
        return;
    }
    if (weapon == rogue.weapon) unwield(weapon);
    take_from_pack(weapon, &rogue.pack);
    free_object(weapon);
}
