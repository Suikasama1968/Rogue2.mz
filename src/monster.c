/*
 * monster.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include "rogue.h"
#include "monster.h"
#include "hit.h"
#include "move.h"
#include "object.h"
#include "random.h"
#include "room.h"
#include "spechit.h"
#include "mz_system.h"

#define monster_pool ((object *)MONSTER_POOL_ADDR)
#define monster_used ((u8 *)MONSTER_USED_ADDR)

typedef char monster_pool_size_check[
    sizeof(object) * MAX_MONSTERS <= MONSTER_POOL_SIZE ? 1 : -1];
typedef char monster_used_size_check[
    MAX_MONSTERS <= MONSTER_USED_SIZE ? 1 : -1];

object level_monsters;
extern boolean detect_monster;
extern short haste_self;

#define mon_tab ((const object *)MONSTER_TABLE_ADDR)
typedef char monster_object_size_check[sizeof(object) == 38 ? 1 : -1];

static int place_monster(short row, short col, boolean wandering);

void
put_mons(void)
{
    short count = (short)get_rand(2, 4);
    short row;
    short col;
    u8 i;

    clear_level_monsters();
    for (i = 0; i < (u8)count; ++i) {
        do {
            gr_row_col(&row, &col, FLOOR);
        } while ((row == rogue.row && col == rogue.col) ||
                 monster_at(row, col));
        if (place_monster(row, col, 0) && coin_toss()) {
            object *monster = monster_at(row, col);
            if (monster->m_flags & WANDERS) monster->m_flags &= ~ASLEEP;
        }
    }
    if (party_room != NO_ROOM) party_monsters(party_room, count);
}

void
mv_mons(void)
{
    object *monster;
    object *gold;
    short dr;
    short dc;
    short row;
    short col;
    short target_row;
    short target_col;
    short rn;
    short i;
    short j;
    short moves;

    if (haste_self % 2) {
        return;
    }
    monster = level_monsters.next_object;
    while (monster) {
        if (monster->m_flags & FREEZING_ROGUE) {
            monster = monster->next_object;
            continue;
        }
        if (monster->m_flags & ASLEEP) {
            if (monster->m_flags & NAPPING) {
                if (monster->d_enchant > 0) --monster->d_enchant;
                if (monster->d_enchant <= 0) {
                    monster->m_flags &= ~(ASLEEP | NAPPING);
                }
            } else if ((monster->m_flags & WAKENS) &&
                monster->row - rogue.row >= -1 &&
                monster->row - rogue.row <= 1 &&
                monster->col - rogue.col >= -1 &&
                monster->col - rogue.col <= 1 &&
                rand_percent(WAKE_PERCENT)) {
                monster->m_flags &= ~(ASLEEP | WAKENS);
            }
        } else {
            if ((monster->m_flags & CONFUSES) && m_confuse(monster)) {
                monster = monster->next_object;
                continue;
            }
            moves = (monster->m_flags & FLIES) ? 2 : 1;
            while (moves-- > 0) {
            dr = rogue.row - monster->row;
            dc = rogue.col - monster->col;
            if (dr >= -1 && dr <= 1 && dc >= -1 && dc <= 1) {
                mon_hit(monster, 0, 0);
                if (game_over) return;
                break;
            }
            if ((monster->m_flags & FLAMES) &&
                (dr == 0 || dc == 0 || dr == dc || dr == -dc) &&
                dr >= -7 && dr <= 7 && dc >= -7 && dc <= 7 &&
                !coin_toss()) {
                mon_hit(monster, 0, 1);
                if (game_over) return;
                break;
            }
            if ((monster->m_flags & FLITS) && rand_percent(FLIT_PERCENT)) {
                if (!rand_percent(10)) {
                    for (i = 0; i < 9; ++i) {
                        row = (short)(monster->row + get_rand(-1, 1));
                        col = (short)(monster->col + get_rand(-1, 1));
                        if ((row != monster->row || col != monster->col) &&
                            (row != rogue.row || col != rogue.col) &&
                            can_move(monster->row, monster->col, row, col) &&
                            !monster_at(row, col)) {
                            monster->row = row;
                            monster->col = col;
                            break;
                        }
                    }
                }
                break;
            }

            target_row = rogue.row;
            target_col = rogue.col;
            rn = (short)get_room_number(monster->row, monster->col);
            if ((monster->m_flags & SEEKS_GOLD) && rn >= 0) {
                for (i = rooms[rn].top_row + 1;
                     i < rooms[rn].bottom_row; ++i) {
                    for (j = rooms[rn].left_col + 1;
                         j < rooms[rn].right_col; ++j) {
                        gold = object_at(&level_objects, i, j);
                        if (gold && gold->what_is == GOLD &&
                            !monster_at(i, j)) {
                            target_row = i;
                            target_col = j;
                            i = rooms[rn].bottom_row;
                            break;
                        }
                    }
                }
            }
            dr = target_row - monster->row;
            dc = target_col - monster->col;
            row = monster->row + ((dr > 0) ? 1 : ((dr < 0) ? -1 : 0));
            col = monster->col + ((dc > 0) ? 1 : ((dc < 0) ? -1 : 0));

            if (!can_move(monster->row, monster->col, row, col) ||
                monster_at(row, col)) {
                row = monster->row + ((dr > 0) ? 1 : ((dr < 0) ? -1 : 0));
                col = monster->col;
            }
            if (!can_move(monster->row, monster->col, row, col) ||
                monster_at(row, col)) {
                row = monster->row;
                col = monster->col + ((dc > 0) ? 1 : ((dc < 0) ? -1 : 0));
            }
            if ((row != rogue.row || col != rogue.col) &&
                can_move(monster->row, monster->col, row, col) &&
                !monster_at(row, col)) {
                monster->row = row;
                monster->col = col;
                if ((monster->m_flags & SEEKS_GOLD) &&
                    row == target_row && col == target_col) {
                    monster->m_flags |= ASLEEP;
                    monster->m_flags &= ~(WAKENS | SEEKS_GOLD);
                    break;
                }
            } else {
                break;
            }
            }
        }
        monster = monster->next_object;
    }
}

static int place_monster(short row, short col, boolean wandering)
{
    u8 i, mn;
    object *monster;
    const object *type;

    for (i = 0; i < MAX_MONSTERS && monster_used[i]; ++i) {}
    if (i == MAX_MONSTERS) return 0;
    do {
        mn = (u8)get_rand(0, MONSTERS - 1);
        type = &mon_tab[mn];
    } while (cur_level < type->first_level || cur_level > type->last_level ||
             (wandering && !(type->m_flags & (WAKENS | WANDERS))));
    monster = &monster_pool[i];
    monster_used[i] = 1;
    *monster = *type;
    monster->row = row;
    monster->col = col;
    monster->next_object = level_monsters.next_object;
    level_monsters.next_object = monster;
    return 1;
}

void clear_level_monsters(void)
{
    u8 i;

    level_monsters.next_object = 0;
    i = MAX_MONSTERS - 1;
    do {
        monster_used[i] = 0;
    } while (i--);
}

object *monster_at(short row, short col)
{
    object *monster = level_monsters.next_object;

    while (monster && (monster->row != row || monster->col != col)) {
        monster = monster->next_object;
    }
    return monster;
}

void remove_monster(object *monster)
{
    object *prev = &level_monsters;
    u8 i;

    while (prev->next_object && prev->next_object != monster) {
        prev = prev->next_object;
    }
    if (prev->next_object == monster) prev->next_object = monster->next_object;
    for (i = 0; i < MAX_MONSTERS; ++i) {
        if (monster == &monster_pool[i]) {
            monster_used[i] = 0;
            break;
        }
    }
}

void party_monsters(int rn, int n)
{
    short row, col, tries;
    n += n;
    while (n-- > 0) {
        for (tries = 0; tries < 100; ++tries) {
            row = (short)get_rand(rooms[rn].top_row + 1, rooms[rn].bottom_row - 1);
            col = (short)get_rand(rooms[rn].left_col + 1, rooms[rn].right_col - 1);
            if ((DUNGEON(row,col) == TILE_FLOOR || DUNGEON(row,col) == TILE_TUNNEL) &&
                !monster_at(row,col)) break;
        }
        if (tries == 100 || !place_monster(row,col, 0)) break;
    }
}

void
show_monsters(void)
{
    detect_monster = 1;
}

void create_monster(void)
{
    short dr, dc, row, col;

    for (dr = -1; dr <= 1; ++dr) {
        for (dc = -1; dc <= 1; ++dc) {
            if (!dr && !dc) continue;
            row = rogue.row + dr;
            col = rogue.col + dc;
            if (can_move(rogue.row, rogue.col, row, col) &&
                !monster_at(row, col)) {
                place_monster(row, col, 0);
                return;
            }
        }
    }
}

void wake_room(short rn, boolean entering, short row, short col)
{
    short wake_percent;
    object *monster;

    (void)row;
    (void)col;

    if (rn < 0 || rn >= MAXROOMS) return;
    wake_percent = (rn == party_room) ? PARTY_WAKE_PERCENT : WAKE_PERCENT;
    monster = level_monsters.next_object;
    while (monster) {
        if ((monster->m_flags & WAKENS) &&
            get_room_number(monster->row, monster->col) == rn &&
            rand_percent(wake_percent)) {
            monster->m_flags &= ~(ASLEEP | WAKENS);
        }
        monster = monster->next_object;
    }
    (void)entering;
}

int rogue_can_see(int row, int col)
{
    extern short blind;
    short rdif = (short)(row - rogue.row);
    short cdif = (short)(col - rogue.col);

    return (!blind && ((cur_room != NO_ROOM &&
             get_room_number(row, col) == cur_room &&
             !(rooms[cur_room].is_room & R_MAZE)) ||
            (rdif >= -1 && rdif <= 1 && cdif >= -1 && cdif <= 1)));
}

void wanderer(void)
{
    object *monster;
    short row, col, i;
    boolean found = 0;

    for (i = 0; i < 25 && !found; ++i) {
        gr_row_col(&row, &col, FLOOR | TUNNEL | STAIRS);
        if (!rogue_can_see(row, col) &&
            (row != rogue.row || col != rogue.col) &&
            !monster_at(row, col) && place_monster(row, col, 1)) {
            monster = monster_at(row, col);
            monster->m_flags &= ~ASLEEP;
            found = 1;
        }
    }
}
