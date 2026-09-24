/*
 * trap.c
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
#include "display.h"
#include "hit.h"
#include "level.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "random.h"
#include "room.h"
#include "score.h"
#include "spechit.h"
#include "trap.h"
#include "use.h"
#include "mz_curses.h"

boolean trap_door;
short bear_trap;
extern boolean sustain_strength;
extern short ring_exp;
extern short new_level_message;
extern short blind;
#define trap_hidden ((uint8_t *)TRAP_HIDDEN_ADDR)

typedef char trap_hidden_size_check[
    MAX_TRAPS <= TRAP_HIDDEN_SIZE ? 1 : -1];

static void reveal_trap(short i);

int
trap_at(int row, int col)
{
    short i;

    for (i = 0; ((i < MAX_TRAPS) && (traps[i].trap_type != NO_TRAP)); i++) {
        if (traps[i].trap_row == row && traps[i].trap_col == col) {
            return i;
        }
    }
    return (NO_TRAP);
}

void
trap_player(short row, short col)
{
    short t;
    short i;

    if ((i = trap_at(row, col)) == NO_TRAP) {
	    return;
    }

    t = traps[i].trap_type;
    reveal_trap(i);
    if (rand_percent(rogue.exp + ring_exp)) {
        message_id(228, 0);
        return;
    }

    if (t == TRAP_DOOR) {
        trap_door = 1;
        new_level_message = 217;
        return;
    }

    message_id(217 + t * 2, 0);
    switch (t) {
    case BEAR_TRAP:
        bear_trap = get_rand(4, 7);
        break;
    case TELE_TRAP:
        tele();
        break;
    case DART_TRAP:
#if !defined(DEBUG)
#if 0
        rogue.hp_current -= get_damage("1d6", 1);
#else
        rogue.hp_current -= get_rand(1,6);
#endif
        if (rogue.hp_current < 0) rogue.hp_current = 0;
#endif
        if ((!sustain_strength) && (rogue.str_current >= 3) && rand_percent(40)) {
            rogue.str_current--;
        }
        print_stats(STAT_HP | STAT_STRENGTH);
        if (rogue.hp_current == 0) {
            killed_by(0, POISON_DART);
        }
        break;
    case SLEEPING_GAS_TRAP:
        take_a_nap();
        break;
    case RUST_TRAP:
        rust(0);
        break;
    }
}

void
add_traps(void)
{
    short i, n;
    short row, col;

    for (i = 0; i < MAX_TRAPS; i++) {
        traps[i].trap_type = NO_TRAP;
        trap_hidden[i] = 0;
    }
    trap_door = 0;
    bear_trap = 0;
    if (cur_level <= 2) {
        return;
    }
    if (cur_level <= 7) {
        n = get_rand(0, 2);
    } else if (cur_level <= 11) {
        n = get_rand(1, 2);
    } else if (cur_level <= 16) {
        n = get_rand(2, 3);
    } else if (cur_level <= 21) {
        n = get_rand(2, 4);
    } else if (cur_level <= AMULET_LEVEL + 2) {
        n = get_rand(3, 5);
    } else {
        n = get_rand(5, MAX_TRAPS);
    }

    for (i = 0; i < n; i++) {
        do {
            gr_row_col(&row, &col, FLOOR);
        } while (object_at(&level_objects, row, col) ||
                 trap_at(row, col) != NO_TRAP);
        traps[i].trap_type = get_rand(0, TRAPS - 1);
        traps[i].trap_row = row;
        traps[i].trap_col = col;
        trap_hidden[i] = 1;
    }
}

#if 0 /* MZ-700/1500では未使用 */

void
id_trap(void)
{
    message_id(229, 0);
}

void
show_traps(void)
{
    short i;

    for (i = 0; i < MAX_TRAPS && traps[i].trap_type != NO_TRAP; i++) {
        reveal_trap(i);
    }
}
#endif

void search(short n, boolean is_auto)
{
    short s;
    short i;
    short dr;
    short dc;
    short row, col;
    uint8_t *tile;

    for (s = 0; s < n; ++s) {
        for (i = 0; i < MAX_TRAPS && traps[i].trap_type != NO_TRAP; ++i) {
            if (!trap_hidden[i]) continue;
            dr = traps[i].trap_row - rogue.row;
            dc = traps[i].trap_col - rogue.col;
            if (dr >= -1 && dr <= 1 && dc >= -1 && dc <= 1 &&
                rand_percent(17 + rogue.exp + ring_exp)) {
                reveal_trap(i);
                message_id(216 + traps[i].trap_type * 2, 0);
            }
        }
        for (dr = -1; dr <= 1; dr++) {
            row = rogue.row + dr;
            if (row < MIN_ROW || row > MAX_ROW) continue;
            for (dc = -1; dc <= 1; dc++) {
                col = rogue.col + dc;
                if (col < 0 || col >= ROGUE_COLUMNS) continue;
                tile = &DUNGEON(row, col);
                if (*tile >= TILE_HIDDEN_DOOR_H &&
                    rand_percent(17 + rogue.exp + ring_exp)) {
                    reveal_hidden_tile(tile);
                    if (!blind) colorize_dungeon(row, col);
                }
            }
        }
        attrset(A_NORMAL);
        if (!is_auto) reg_move();
    }
}

/* MZ-700/1500固有 */
/* 文字コードとカラー属性を同時に更新する。 */
static void reveal_trap(short i)
{
    short row = traps[i].trap_row;
    short col = traps[i].trap_col;

    trap_hidden[i] = 0;
    DUNGEON(row, col) = TILE_TRAP;
    colorize_dungeon(row, col);
    attrset(A_NORMAL);
}
