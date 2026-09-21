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
#include "display.h"
#include "hit.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "spechit.h"
#include "trap.h"
#include "mz_curses.h"

static void consume_thrown_weapon(object *weapon);

void
throw(void)
{
    short wch;
    object *weapon;
    short dir, row, col;
    object *monster;
    uint8_t *prompt = (uint8_t *)TEMP_BUFFER_ADDR;

    dir = get_direction();
	if (dir == CANCEL) {
		return;
	}
    get_message(210, prompt, 24);
	if ((wch = pack_letter((char *)prompt, WEAPON)) == CANCEL) {
		return;
	}
    check_message();

    if (!(weapon = get_letter_object(wch))) {
        message_id(91, 0);
        return;
    }
    if ((weapon->in_use_flags & BEING_WIELDED) && weapon->is_cursed) {
        message_id(85, 0);
        return;
    }
    row = rogue.row;
    col = rogue.col;
	
    monster = get_thrown_at_monster(weapon, dir, &row, &col);

    if (monster) {
        wake_up(monster);
        check_gold_seeker(monster);

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

    hit_chance = get_hit_chance(weapon);
    damage = get_weapon_damage(weapon);
    if (weapon->which_kind == ARROW && rogue.weapon &&
        rogue.weapon->which_kind == BOW) {
        damage += get_weapon_damage(rogue.weapon);
        damage = (damage * 2) / 3;
        hit_chance += hit_chance / 3;
    } else if ((weapon->in_use_flags & BEING_WIELDED) &&
               (weapon->which_kind == DAGGER ||
                weapon->which_kind == SHURIKEN ||
                weapon->which_kind == DART)) {
        damage = (damage * 3) / 2;
        hit_chance += (hit_chance / 3);
    }
    if (!rand_percent(hit_chance)) {
        message_id(213, 0);
        return 0;
    }
    message_id(214, 0);
    (void)mon_damage(monster, damage);
    return 1;
}

object *
get_thrown_at_monster(object *obj, short dir, short *row, short *col)
{
    short old_row = *row;
    short old_col = *col;
    uint8_t i;
    uint8_t tile;

    for (i = 0; i < 24; i++) {
        get_dir_rc(dir, row, col, 0);
        if ((*row == old_row && *col == old_col) ||
            !is_passable(*row, *col)) {
            *row = old_row;
            *col = old_col;
            return 0;
        }
        if (monster_at(*row, *col)) return monster_at(*row, *col);
        if (DUNGEON_ATTR(*row, *col) != ATTR_HIDDEN) {
            tile = DUNGEON(*row, *col);
            attrset(COLOR_PAIR(PAIR_OBJECT));
            mvaddch((uint8_t)*row, (uint8_t)*col, DC_R_BLACKET);
            refresh_dungeon();
            DUNGEON(*row, *col) = tile;
            colorize_dungeon(*row, *col);
            attrset(COLOR_PAIR(PAIR_NORMAL));
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
    uint8_t i;

    for (i = 0; i < 9; i++) {
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
    message_id(215, 0);
}

void
rand_around(short i, short *r, short *c)
{
    static char pos[9] = { 8, 7, 1, 3, 4, 5, 2, 6, 0 };
    static short row, col;
    short j;
    static const signed char ra[9] = { 1, 1, -1, -1, 0, 1, 0, -1, 0 };
    static const signed char ca[9] = { 1, -1, 1, -1, 1, 0, 0, 0, -1 };

    if (i == 0) {
        short x, y, o, t;

        row = *r;
        col = *c;

        o = get_rand(1, 8);
        
        for (j = 0; j < 5; j++) {
            x = get_rand(0, 8) % 9;
            y = (x + o) % 9;
            t = pos[x];
            pos[x] = pos[y];
            pos[y] = t;
        }
    }
    j = pos[i] % 9;
    *r = row + ra[j];
    *c = col + ca[j];
}

#if 0 /* MZ-700/1500では未対応 */
void
potion_monster(object *monster, unsigned short kind)
{
    short maxhp;

    maxhp = mon_tab[monster->m_char - 'A'].hp_to_kill;

    switch (kind) {
    case RESTORE_STRENGTH:
    case LEVITATION:
    case HALLUCINATION:
    case DETECT_MONSTER:
    case DETECT_OBJECTS:
    case SEE_INVISIBLE:
	break;
    case EXTRA_HEALING:
	monster->hp_to_kill += (maxhp - monster->hp_to_kill) * 2 / 3;
	break;
    case INCREASE_STRENGTH:
    case HEALING:
    case RAISE_LEVEL:
	monster->hp_to_kill += (maxhp - monster->hp_to_kill) / 5;
	break;
    case POISON:
	mon_damage(monster, (monster->hp_to_kill / 4 + 1));
	break;
    case BLINDNESS:
	monster->m_flags |= (ASLEEP | WAKENS);
	break;
    case CONFUSION:
	monster->m_flags |= CONFUSED;
	monster->moves_confused += get_rand(12, 22);
	break;
    case HASTE_SELF:
	if (monster->m_flags & SLOWED)
	    monster->m_flags &= (~SLOWED);
	else
	    monster->m_flags |= HASTED;
	break;
    }
}
#endif

/* MZ-700/1500固有 */
static void
consume_thrown_weapon(object *weapon)
{
    if (weapon->quantity > 1) {
        weapon->quantity--;
        return;
    }
    if (weapon == rogue.weapon) unwield(weapon);
    take_from_pack(weapon, &rogue.pack);
    free_object(weapon);
}
