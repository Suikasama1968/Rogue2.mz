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
#include "message.h"
#include "move.h"
#include "object.h"
#include "random.h"
#include "room.h"
#include "spechit.h"
#include "throw.h"
#include "mz_system.h"

#define monster_pool ((object *)MONSTER_POOL_ADDR)
#define monster_used ((u8 *)MONSTER_USED_ADDR)

typedef char monster_pool_size_check[
    sizeof(object) * MAX_MONSTERS <= MONSTER_POOL_SIZE ? 1 : -1];
typedef char monster_used_size_check[
    MAX_MONSTERS <= MONSTER_USED_SIZE ? 1 : -1];

extern boolean detect_monster;
extern short haste_self;
extern short blind;
extern short stealthy;

#define mon_tab ((const object *)MONSTER_TABLE_ADDR)
typedef char monster_object_size_check[sizeof(object) == 38 ? 1 : -1];

static int place_monster(short row, short col, boolean wandering);

void
put_mons(void)
{
    short i;
	short n;
    short row, col;

    n = get_rand(4, 6);

    clear_level_monsters();

	for (i = 0; i < n; i++) {
        do {
            gr_row_col(&row, &col, FLOOR);
        } while ((row == rogue.row && col == rogue.col) ||
                 monster_at(row, col));
        if (place_monster(row, col, 0) && coin_toss()) {
            object *monster = monster_at(row, col);
            if (monster->m_flags & WANDERS) monster->m_flags &= ~ASLEEP;
        }
    }

    if (party_room != NO_ROOM) party_monsters(party_room, n);
}

object *
gr_monster(object *monster, int mn)
{
    *monster = mon_tab[mn];
    if (monster->m_flags & IMITATES) {
        monster->disguise = gr_obj_char();
    }
    if (cur_level > (AMULET_LEVEL + 2)) {
        monster->m_flags |= HASTED;
    }
    return monster;
}

void
mv_mons(void)
{
    object *monster, *next_monster;
    boolean flew;

    if (haste_self % 2) {
        return;
    }

    monster = level_monsters.next_monster;

	while (monster) {
        next_monster = monster->next_monster;
        if (monster->m_flags & SLOWED) {
            monster->m_flags ^= ALREADY_MOVED;
            if (monster->m_flags & ALREADY_MOVED) {
                monster = next_monster;
                continue;
            }
        }
        if (monster->m_flags & HASTED) {
            mv_monster(monster, rogue.row, rogue.col);
            if (monster_at(monster->row, monster->col) != monster) {
                monster = next_monster;
                continue;
            }
        }
        flew = 0;
        if ((monster->m_flags & FLIES) &&
            !(monster->m_flags & NAPPING) &&
            !mon_can_go(monster, rogue.row, rogue.col)) {
            flew = 1;
            mv_monster(monster, rogue.row, rogue.col);
            if (monster_at(monster->row, monster->col) != monster) {
                monster = next_monster;
                continue;
            }
        }
        if (!(flew && mon_can_go(monster, rogue.row, rogue.col))) {
            mv_monster(monster, rogue.row, rogue.col);
        }
        monster = next_monster;
    }
}

void
party_monsters(int rn, int n)
{
    short row, col, tries;
    n += n;
    while (n-- > 0) {
        for (tries = 0; tries < 100; ++tries) {
            row = get_rand(rooms[rn].top_row + 1, rooms[rn].bottom_row - 1);
            col = get_rand(rooms[rn].left_col + 1, rooms[rn].right_col - 1);
            if ((DUNGEON(row,col) == TILE_FLOOR || DUNGEON(row,col) == TILE_TUNNEL) &&
                !monster_at(row,col)) break;
        }
        if (tries == 100 || !place_monster(row,col, 0)) break;
    }
}

void
mv_monster(object *monster, short row, short col)
{
    short dr;
    short dc;
    short next_row;
    short next_col;

    if (monster->m_flags & ASLEEP) {
        if (monster->m_flags & NAPPING) {
            if (monster->nap_length > 0) {
                monster->nap_length--;
            }
            if (monster->nap_length <= 0) {
                monster->m_flags &= (~(NAPPING | ASLEEP));
            }
            return;
        }
        if ((monster->m_flags & WAKENS) &&
                   monster->row - rogue.row >= -1 &&
                   monster->row - rogue.row <= 1 &&
                   monster->col - rogue.col >= -1 &&
                   monster->col - rogue.col <= 1 &&
                   rand_percent((stealthy > 0) ?
                                (WAKE_PERCENT /
                                 (STEALTH_FACTOR + stealthy)) :
                                WAKE_PERCENT)) {
            wake_up(monster);
        }
        return;
    }

    if ((monster->m_flags & FLITS) && flit(monster)) {
        return;
    }
    if ((monster->m_flags & STATIONARY) &&
    !mon_can_go(monster, rogue.row, rogue.col)) {
        return;
    }
    if (monster->m_flags & FREEZING_ROGUE) {
        return;
    }
    if ((monster->m_flags & CONFUSED) && move_confused(monster)){
        return;
    }
    if ((monster->m_flags & CONFUSES) && m_confuse(monster)) {
        return;
    }
    if (mon_can_go(monster, rogue.row, rogue.col)) {
        mon_hit(monster, 0, 0);
        return;
    }
    if ((monster->m_flags & SEEKS_GOLD) && seek_gold(monster)) {
        return;
    }
    if ((monster->m_flags & FLAMES) && flame_broil(monster)) {
        return;
    }
    dr = row - monster->row;
    dc = col - monster->col;
    next_row = monster->row + ((dr > 0) ? 1 : ((dr < 0) ? -1 : 0));
    next_col = monster->col + ((dc > 0) ? 1 : ((dc < 0) ? -1 : 0));
    if (mtry(monster, next_row, next_col)) return;
    if (mtry(monster, next_row, monster->col)) return;
    (void)mtry(monster, monster->row, next_col);
}

int
mtry(object *monster, short row, short col)
{
    if (mon_can_go(monster, row, col) &&
        (row != rogue.row || col != rogue.col)) {
        move_mon_to(monster, row, col);
        return 1;
    }
    return 0;
}

void
move_mon_to(object *monster, short row, short col)
{
    monster->row = row;
    monster->col = col;
}

int
mon_can_go(object *monster, short row, short col)
{
    object *other;
    short dr, dc;

    dr = monster->row - row;    /* check if move distance > 1 */
    dc = monster->col - col;

    if (dr >= 2 || dr <= -2 || dc >= 2 || dc <= -2) {
        return 0;
    }
    if (!can_move(monster->row, monster->col, row, col)) {
        return 0;
    }
    other = monster_at(row, col);
    return !other || other == monster;
}

void
wake_up(object *monster)
{
    if (!(monster->m_flags & NAPPING)) {
        monster->m_flags &= (~(ASLEEP | IMITATES | WAKENS));
    }
}

void
wake_room(short rn, boolean entering, short row, short col)
{
    object *monster;
    short wake_percent;

    (void)row;
    (void)col;

    if (rn < 0 || rn >= MAXROOMS) return;
    wake_percent = (rn == party_room) ? PARTY_WAKE_PERCENT : WAKE_PERCENT;
    if (stealthy > 0) {
        wake_percent /= (STEALTH_FACTOR + stealthy);
    }
    
    monster = level_monsters.next_monster;

    while (monster) {
        if ((monster->m_flags & WAKENS) &&
            get_room_number(monster->row, monster->col) == rn &&
            rand_percent(wake_percent)) {
            monster->m_flags &= ~(ASLEEP | WAKENS);
        }
        monster = monster->next_monster;
    }
    (void)entering;
}

void
wanderer(void)
{
    object *monster;
    short row, col, i;
    boolean found = 0;

    for (i = 0; ((i < 25) && (!found)); i++) {
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

void
show_monsters(void)
{
    detect_monster = 1;
}

void
create_monster(void)
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

#if 0 /* MZ-700/1500では未対応 */
void
aim_monster(object *monster)
{
    short i, rn, d, r;

    rn = get_room_number(monster->row, monster->col);
    r = get_rand(0, 12);

    for (i = 0; i < 4; i++) {
	d = (r + i) % 4;
	if (rooms[rn].doors[d].oth_room != NO_ROOM) {
	    monster->trow = rooms[rn].doors[d].door_row;
	    monster->tcol = rooms[rn].doors[d].door_col;
	    break;
	}
    }
}
#endif

int
rogue_can_see(int row, int col)
{
    short rdif = row - rogue.row;
    short cdif = col - rogue.col;

    return (!blind && ((cur_room != NO_ROOM &&
             get_room_number(row, col) == cur_room &&
             !(rooms[cur_room].is_room & R_MAZE)) ||
            (rdif >= -1 && rdif <= 1 && cdif >= -1 && cdif <= 1)));
}

int
move_confused(object *monster)
{
    short i, row, col;

    if (!(monster->m_flags & ASLEEP)) {
        if (--monster->moves_confused <= 0) {
            monster->m_flags &= (~CONFUSED);
        }
        if (monster->m_flags & STATIONARY) {
            return (coin_toss()? 1 : 0);
        } else if (rand_percent(15)) {
            return 1;
        }
        row = monster->row;
        col = monster->col;

        for (i = 0; i < 9; i++) {
            rand_around(i, &row, &col);
            if ((row == rogue.row) && (col == rogue.col)) {
                return 0;
            }
            if (mtry(monster, row, col)) {
                return 1;
            }
        }
    }
    return 0;
}

int
flit(object *monster)
{
    short i;
    short row;
    short col;

    if (!rand_percent(FLIT_PERCENT)) return 0;
    if (rand_percent(10)) return 1;
    row = monster->row;
    col = monster->col;
    for (i = 0; i < 9; i++) {
        rand_around(i, &row, &col);
        if ((row == rogue.row) && (col == rogue.col)) continue;
        if (mtry(monster, row, col)) return 1;
    }
    return 1;
}

int
gr_obj_char(void)
{
    short r;
    static const u8 rs[] = {
        DC_PERCENT, DC_EXCLAM, DC_QUESTION, DC_R_SQ_BLACKET,
        DC_EQUAL, DC_SLASH, DC_R_BLACKET, DC_COLON, DC_STAR
    };
    
    r = get_rand(0, 8);

    return rs[r];
}

boolean
mon_sees(object *monster, int row, int col)
{
    short rn;
    short rdif;
    short cdif;

    rn = get_room_number(row, col);
    if (rn != NO_ROOM &&
        rn == get_room_number(monster->row, monster->col) &&
        !(rooms[rn].is_room & R_MAZE)) {
        return 1;
    }
    rdif = row - monster->row;
    cdif = col - monster->col;
    return (boolean)(rdif >= -1 && rdif <= 1 &&
                     cdif >= -1 && cdif <= 1);
}

#if 0 /* MZ-700/1500では未対応 */
void
mv_aquatars(void)
{
    object *monster;

    monster = level_monsters.next_monster;

    while (monster) {
	if ((monster->m_char == 'A') &&
	    mon_can_go(monster, rogue.row, rogue.col)) {
	    mv_monster(monster, rogue.row, rogue.col);
	    monster->m_flags |= ALREADY_MOVED;
	}
	monster = monster->next_monster;
    }
}
#endif

/* MZ-700/1500固有 */
static int place_monster(short row, short col, boolean wandering)
{
    u8 i, mn;
    object *monster;
    const object *type;

    for (i = 0; i < MAX_MONSTERS && monster_used[i]; ++i) {}
    if (i == MAX_MONSTERS) return 0;
    do {
        mn = get_rand(0, MONSTERS - 1);
        type = &mon_tab[mn];
    } while (cur_level < type->first_level || cur_level > type->last_level ||
             (wandering && !(type->m_flags & (WAKENS | WANDERS))));
    monster = &monster_pool[i];
    monster_used[i] = 1;
    (void)gr_monster(monster, mn);
    monster->row = row;
    monster->col = col;
    monster->next_monster = level_monsters.next_monster;
    level_monsters.next_monster = monster;
    return 1;
}

void clear_level_monsters(void)
{
    u8 i;

    level_monsters.next_monster = 0;
    i = MAX_MONSTERS - 1;
    do {
        monster_used[i] = 0;
    } while (i--);
}

object *monster_at(short row, short col)
{
    return object_at(&level_monsters, row, col);
}

void remove_monster(object *monster)
{
    object *prev = &level_monsters;
    u8 i;

    while (prev->next_monster && prev->next_monster != monster) {
        prev = prev->next_monster;
    }
    if (prev->next_monster == monster) prev->next_monster = monster->next_monster;
    for (i = 0; i < MAX_MONSTERS; i++) {
        if (monster == &monster_pool[i]) {
            monster_used[i] = 0;
            break;
        }
    }
}
