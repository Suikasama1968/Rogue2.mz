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
#include "room.h"

void
zapp(void)
{
    short wch;
    object *wand;
    short dir, row, col;
    object *monster;

    dir = (short)get_direction();
    if (dir == CANCEL) return;
    if ((wch = (short)pack_letter(0, WAND)) == CANCEL) return;
    check_message();

    if (!(wand = get_letter_object(wch))) {
        message_id_mz(279, 0);
        return;
    }
    if (wand->what_is != WAND) {
        message_id_mz(280, 0);
        return;
    }
    if (wand->hit_enchant <= 0) {
        message_id_mz(281, 0);
    } else {
        --wand->hit_enchant;
        row = rogue.row;
        col = rogue.col;
        monster = get_zapped_monster(dir, &row, &col);
        if (monster) {
            monster->m_flags &= ~ASLEEP;
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
    switch (kind) {
    case TELE_AWAY:
        tele_away(monster);
        break;
    case PUT_TO_SLEEP:
        monster->m_flags |= ASLEEP | NAPPING;
        monster->d_enchant = 4;
        break;
    case MAGIC_MISSILE:
        rogue_hit(monster, 1);
        break;
    case CANCELLATION:
        monster->m_flags &= ~(FLIES | FLITS | SPECIAL_HIT | FLAMES |
                              SEEKS_GOLD);
        break;
    case DO_NOTHING:
        message_id_mz(282, 0);
        break;
    }
}

void
tele_away(object *monster)
{
    short row, col;

    do {
        gr_row_col(&row, &col, FLOOR | TUNNEL);
    } while ((row == rogue.row && col == rogue.col) || monster_at(row, col));
    monster->row = row;
    monster->col = col;
}
