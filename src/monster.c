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

typedef struct mz_monster_type {
    unsigned long flags;
    u8 letter;
    u8 first_level;
    u8 last_level;
    u8 hp;
    u8 hit_chance;
    u8 damage_n1;
    u8 damage_s1;
    u8 damage_n2;
    u8 damage_s2;
    unsigned short kill_exp;
    short name_id;
} mz_monster_type;

/* 本家mon_tabから、現在の階層で登場する種類を抜粋したもの。 */
static const mz_monster_type mz_mon_tab[] = {
    { ASLEEP | WANDERS | FLITS, 'B', 1, 8, 10, 60, 1, 3, 0, 0, 2, 308 },
    { ASLEEP | WAKENS, 'E', 1, 7, 11, 65, 1, 3, 0, 0, 2, 311 },
    { ASLEEP | WAKENS | WANDERS, 'H', 1, 10, 15, 67, 1, 3, 1, 2, 3, 314 },
    { ASLEEP | WAKENS | WANDERS | FLIES, 'K', 1, 6, 10, 60, 1, 4, 0, 0, 2, 317 },
    { ASLEEP | WAKENS | WANDERS | SEEKS_GOLD, 'O', 4, 13, 25, 70, 1, 6, 0, 0, 5, 321 },
    { ASLEEP | WAKENS | WANDERS | STINGS, 'R', 3, 12, 19, 70, 2, 5, 0, 0, 10, 324 },
    { ASLEEP | WAKENS | WANDERS, 'S', 1, 9, 8, 50, 1, 3, 0, 0, 2, 325 },
    { ASLEEP | WAKENS | WANDERS, 'Z', 5, 14, 21, 69, 1, 7, 0, 0, 8, 332 },
    { ASLEEP | WANDERS, 'C', 7, 16, 32, 85, 3, 3, 2, 5, 15, 309 },
    { ASLEEP | WAKENS | WANDERS, 'Q', 8, 17, 30, 78, 3, 5, 0, 0, 20, 323 },
    { ASLEEP | WAKENS | WANDERS, 'T', 13, 22, 75, 75, 4, 6, 1, 4, 125, 326 },
    { ASLEEP | WAKENS | WANDERS, 'U', 17, 26, 90, 85, 4, 10, 0, 0, 200, 327 },
    { ASLEEP | WAKENS | FLAMES, 'D', 21, 126, 145, 100, 4, 6, 4, 9, 5000, 310 }
};

#define MZ_MONSTER_TYPES (sizeof(mz_mon_tab) / sizeof(mz_mon_tab[0]))

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

    monster = level_monsters.next_object;
    while (monster) {
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
    const mz_monster_type *type;

    for (i = 0; i < MAX_MONSTERS && monster_used[i]; ++i) {}
    if (i == MAX_MONSTERS) return 0;
    do {
        mn = (u8)get_rand(0, MZ_MONSTER_TYPES - 1);
        type = &mz_mon_tab[mn];
    } while (cur_level < type->first_level || cur_level > type->last_level ||
             (wandering && !(type->flags & (WAKENS | WANDERS))));
    monster = &monster_pool[i];
    monster_used[i] = 1;
    monster->row = row; monster->col = col; monster->m_hp = type->hp;
    monster->m_char = (u8)(DC_A + type->letter - 'A');
    monster->m_flags = type->flags; monster->kill_exp = type->kill_exp;
    monster->m_hit_chance = type->hit_chance;
    monster->m_damage_n1 = type->damage_n1; monster->m_damage_s1 = type->damage_s1;
    monster->m_damage_n2 = type->damage_n2; monster->m_damage_s2 = type->damage_s2;
    monster->m_name_id = type->name_id;
    monster->next_object = level_monsters.next_object;
    level_monsters.next_object = monster;
    return 1;
}

void clear_level_monsters(void)
{
    u8 i;

    level_monsters.next_object = 0;
    for (i = 0; i < MAX_MONSTERS; ++i) monster_used[i] = 0;
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
    short rdif = (short)(row - rogue.row);
    short cdif = (short)(col - rogue.col);

    return ((cur_room != NO_ROOM &&
             get_room_number(row, col) == cur_room &&
             !(rooms[cur_room].is_room & R_MAZE)) ||
            (rdif >= -1 && rdif <= 1 && cdif >= -1 && cdif <= 1));
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
