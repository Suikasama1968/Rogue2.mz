/*
 * zap.c
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
#include "zap.h"
#include "hit.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "spechit.h"

extern boolean being_held;

void
zapp(void)
{
    short wch;
    object *wand;
    short dir, row, col;
    object *monster;

    dir = (short)get_direction();
	if (dir == CANCEL) {
		return;
	}
	if ((wch = (short)pack_letter(0, WAND)) == CANCEL) {
		return;
	}
    check_message();

    if (!(wand = get_letter_object(wch))) {
        message_id(91, 0);
        return;
    }
    if (wand->what_is != WAND) {
        message_id(280, 0);
        return;
    }
    if (wand->hit_enchant <= 0) {
        message_id(281, 0);
    } else {
        wand->hit_enchant--;
        row = rogue.row;
        col = rogue.col;
        monster = get_zapped_monster(dir, &row, &col);
        if (monster) {
            wake_up(monster);
            check_gold_seeker(monster);
            zap_monster(monster, wand->which_kind);
        }
    }
    (void)reg_move();
}

object *
get_zapped_monster(short dir, short *row, short *col)
{
    short old_row, old_col;
    object *monster;

    for (;;) {
        old_row = *row;
        old_col = *col;
        get_dir_rc(dir, row, col, 0);
        if ((*row == old_row && *col == old_col) ||
            !is_passable(*row, *col)) return 0;
        monster = monster_at(*row, *col);
        if (monster) return monster;
    }
}

object *
get_missiled_monster(short dir, short *row, short *col)
{
    return get_zapped_monster(dir, row, col);
}

void
zap_monster(object *monster, unsigned short kind)
{
    short row, col;
    object *next;
    u8 trail, attr;

    switch (kind) {
    case SLOW_MONSTER:
        if (monster->m_flags & HASTED) {
            monster->m_flags &= ~HASTED;
        } else {
            monster->m_flags &= ~ALREADY_MOVED;
            monster->m_flags |= SLOWED;
        }
        break;
    case HASTE_MONSTER:
        if (monster->m_flags & SLOWED) {
            monster->m_flags &= ~(SLOWED | ALREADY_MOVED);
        } else {
            monster->m_flags |= HASTED;
        }
        break;
    case TELE_AWAY:
        tele_away(monster);
        break;
    case INVISIBILITY:
        monster->m_flags |= INVISIBLE;
        break;
    case POLYMORPH:
        if (monster->m_flags & HOLDS) {
            being_held = 0;
        }
        row = monster->row;
        col = monster->col;
        next = monster->next_object;
        trail = monster->trail_char;
        attr = monster->picked_up;
        (void)gr_monster(monster, get_rand(0, MONSTERS - 1));
        monster->row = row;
        monster->col = col;
        monster->next_object = next;
        monster->trail_char = trail;
        monster->picked_up = attr;
        if (!(monster->m_flags & IMITATES)) {
            wake_up(monster);
        }
        break;
    case PUT_TO_SLEEP:
        monster->m_flags |= ASLEEP | NAPPING;
        monster->d_enchant = (char)get_rand(3, 6);
        break;
    case CONFUSE_MONSTER:
        monster->m_flags |= CONFUSED;
        monster->moves_confused += (char)get_rand(12, 22);
        break;
    case MAGIC_MISSILE:
        rogue_hit(monster, 1);
        break;
    case CANCELLATION:
        if (monster->m_flags & HOLDS) being_held = 0;
        if (monster->m_flags & STEALS_ITEM) monster->drop_percent = 0;
        monster->m_flags &= ~(FLIES | FLITS | SPECIAL_HIT | INVISIBLE |
                              FLAMES | IMITATES | CONFUSES | SEEKS_GOLD |
                              HOLDS);
        break;
    case DO_NOTHING:
        message_id(281, 0);
        break;
    }
}

void
tele_away(object *monster)
{
    short row, col;

    if (monster->m_flags & HOLDS) {
        being_held = 0;
    }
    do {
        gr_row_col(&row, &col, FLOOR | TUNNEL);
    } while ((row == rogue.row && col == rogue.col) || monster_at(row, col));
    monster->row = row;
    monster->col = col;
}
