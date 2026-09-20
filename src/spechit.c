/*
 * spechit.c
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
#include "spechit.h"
#include "hit.h"
#include "invent.h"
#include "level.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"
#include "room.h"
#include "score.h"
#include "use.h"
#include "trap.h"
#include "throw.h"
#include "move.h"

static void disappear(object *monster);

short less_hp = 0;
boolean being_held = 0;

extern short blind, levitate, ring_exp;
extern boolean sustain_strength, maintain_armor;

void
special_hit(object *monster)
{
    if ((monster->m_flags & CONFUSED) && rand_percent(66)) {
        return;
    }
    if (monster->m_flags & RUSTS) {
        rust(monster);
    }
    if ((monster->m_flags & HOLDS) && !levitate) {
        being_held = 1;
    }
    if (monster->m_flags & FREEZES) {
        freeze(monster);
    }
    if (monster->m_flags & STINGS) {
        sting(monster);
    }
    if (monster->m_flags & DRAINS_LIFE) {
        drain_life();
    }
    if (monster->m_flags & DROPS_LEVEL) {
        drop_level();
    }
    if (monster->m_flags & STEALS_GOLD) {
        steal_gold(monster);
    } else if (monster->m_flags & STEALS_ITEM) {
        steal_item(monster);
    }
}

void
rust(object *monster)
{
    if ((!rogue.armor) || (get_armor_class(rogue.armor) <= 1) ||
    (rogue.armor->which_kind == LEATHER)) {
        return;
    }
    if (rogue.armor->is_protected || maintain_armor) {
        if (monster && !(monster->m_flags & RUST_VANISHED)) {
            message_id(201, 0);
            monster->m_flags |= RUST_VANISHED;
        }
    } else {
        rogue.armor->d_enchant--;
        message_id(202, 0);
        print_stats(STAT_ARMOR);
    }
}

void
freeze(object *monster)
{
    short freeze_percent = 99;
    short i, n;

    if (rand_percent(12)) {
        return;
    }
    freeze_percent -= (rogue.str_current + (rogue.str_current / 2));
    freeze_percent -= ((rogue.exp + ring_exp) * 4);
    freeze_percent -= (get_armor_class(rogue.armor) * 5);
    freeze_percent -= (rogue.hp_max / 3);
    
    if (freeze_percent <= 10) {
        return;
    }
    monster->m_flags |= FREEZING_ROGUE;
    message_id(203, 0);
    n = get_rand(4, 8);
    for (i = 0; i < n; i++) {
        mv_mons();
    }
    if (rand_percent(freeze_percent)) {
        for (i = 0; i < 50; i++) {
            mv_mons();
        }
        killed_by(0, HYPOTHERMIA);
    }
    message_id(66, 0);
    monster->m_flags &= (~FREEZING_ROGUE);
}

void
steal_gold(object *monster)
{
    long amount;

    if (rogue.gold <= 0 || rand_percent(10)) {
        return;
    }
    
    amount = get_rand((cur_level * 10), (cur_level * 30));

    if (amount > rogue.gold) {
        amount = rogue.gold;
    }
    rogue.gold -= amount;
    message_id(204, 0);
    print_stats(STAT_GOLD);
    disappear(monster);
}

void
steal_item(object *monster)
{
    object *obj;
    object *chosen = 0;
    short count = 0;
    short quantity = 0;
    short length;
    u8 *desc = (u8 *)TEMP_BUFFER_ADDR;

	if (rand_percent(15)) {
		return;
	}

    for (obj = rogue.pack.next_object; obj; obj = obj->next_object) {
        if (obj->what_is == RING && obj->which_kind == ADORNMENT &&
            (obj->in_use_flags & ON_EITHER_HAND) && !obj->is_cursed) {
            un_put_on(obj);
            chosen = obj;
            break;
        }
    }
    if (!chosen) {
        for (obj = rogue.pack.next_object; obj; obj = obj->next_object) {
            if (!(obj->in_use_flags & BEING_USED) &&
                get_rand(1, ++count) == 1) chosen = obj;
        }
    }
    if (chosen) {
        if (chosen->what_is != WEAPON) {
            quantity = chosen->quantity;
            chosen->quantity = 1;
        }
        get_desc(chosen, (char *)desc, 0);
        length = 0;
        while (desc[length]) ++length;
        get_message(205, desc + length, ROGUE_COLUMNS - length);
        message((char *)desc, 0);
        chosen->quantity = (chosen->what_is != WEAPON) ? quantity : 1;
        vanish(chosen, 0, &rogue.pack);
    }
    disappear(monster);
}

void
disappear(object *monster)
{
    remove_monster(monster);
}

void
cough_up(object *monster)
{
    object *obj;
    short row, col, i;

    if (cur_level < max_level) {
        return;
    }
    
    if (monster->m_flags & STEALS_GOLD) {
        obj = alloc_object();
        if (!obj) return;
        obj->what_is = GOLD;
        obj->quantity = get_rand((cur_level * 15), (cur_level * 30));
    } else {
        if (!rand_percent((int)monster->drop_percent)) {
            return;
        }
        obj = gr_object();
        if (!obj) return;
    }
    row = monster->row;
    col = monster->col;

    for (i = 0; i < 9; i++) {    // メモリ削減 簡易版
        rand_around(i, &row, &col);
        if (try_to_cough(row, col, obj)) return;
    }
    free_object(obj);
}

int
try_to_cough(short row, short col, object *obj)
{
    if ((row < MIN_ROW) || (row > MAX_ROW) || (col < 0)
        || (col >= ROGUE_COLUMNS)) {
        return 0;
    }
    if (!is_passable(row, col) || object_at(&level_objects, row, col) ||
        monster_at(row, col) ||
        (row == stairs_row && col == stairs_col) ||
    trap_at(row, col) != NO_TRAP) {
        return 0;
    }
    place_at(obj, row, col);
    return 1;
}

int
seek_gold(object *monster)
{
    short i, j, rn;

	if ((rn = get_room_number(monster->row, monster->col)) < 0) {
    	return 0;
	}
    for (i = rooms[rn].top_row + 1; i < rooms[rn].bottom_row; i++) {
        for (j = rooms[rn].left_col + 1; j < rooms[rn].right_col; j++) {
            if (gold_at(i, j) && !monster_at(i, j)) {
                monster->m_flags |= CAN_FLIT;
                if (mon_can_go(monster, i, j)) {
                    move_mon_to(monster, i, j);
                    monster->m_flags |= ASLEEP;
                	monster->m_flags &= (~(WAKENS | SEEKS_GOLD | CAN_FLIT));
                    return 1;
                }
            	monster->m_flags &= (~SEEKS_GOLD);
                mv_monster(monster, i, j);
            	monster->m_flags &= (~CAN_FLIT);
                monster->m_flags |= SEEKS_GOLD;
                return 1;
            }
        }
    }
    return 0;
}

int
gold_at(short row, short col)
{
    object *obj = object_at(&level_objects, row, col);

    return obj && obj->what_is == GOLD;
}

void
check_gold_seeker(object *monster)
{
	monster->m_flags &= (~SEEKS_GOLD);
}

int
check_imitator(object *monster)
{
    u8 *name = (u8 *)TEMP_BUFFER_ADDR;

    if (monster->m_flags & IMITATES) {
        wake_up(monster);
        if (!blind) {
            get_message(monster->m_name_id, name, 20);
            message_id(206, name);
        }
        return 1;
    }
    return 0;
}

void
sting(object *monster)
{
    short sting_chance = 35;
    u8 *name = (u8 *)TEMP_BUFFER_ADDR;

    if (sustain_strength || rogue.str_current <= 3) {
        return;
    }
    sting_chance += (6 * (6 - get_armor_class(rogue.armor)));
    
    if ((rogue.exp + ring_exp) > 8) {
        sting_chance -= (6 * ((rogue.exp + ring_exp) - 8));
    }
    if (rand_percent(sting_chance)) {
        get_message(monster->m_name_id, name, 20);
        message_id(207, name);
        rogue.str_current--;
        print_stats(STAT_STRENGTH);
    }
}

void
drop_level(void)
{
    short hp;

	if (rand_percent(80) || rogue.exp <= 5) {
		return;
	}
    rogue.exp_points = level_points[rogue.exp - 2] - get_rand(9, 29);
    rogue.exp -= 2;
    hp = hp_raise();
	if ((rogue.hp_current -= hp) <= 0) {
		rogue.hp_current = 1;
	}
	if ((rogue.hp_max -= hp) <= 0) {
		rogue.hp_max = 1;
	}
    add_exp(1, 0);
    print_stats(STAT_HP | STAT_EXP);
}

void
drain_life(void)
{
    short n;

    if (rand_percent(60) || (rogue.hp_max <= 30) || (rogue.hp_current < 10)) {
        return;
    }
    n = get_rand(1, 3);
    
    if (n != 2 || !sustain_strength) {
       message_id(208, 0);
    }
    if (n != 2) {
        rogue.hp_max--;
        rogue.hp_current--;
        less_hp++;
    }
    if (n != 1 && !sustain_strength && rogue.str_current > 3) {
        rogue.str_current--;
        if (coin_toss()) {
            rogue.str_max--;
        }
    }
    print_stats(STAT_STRENGTH | STAT_HP);
}

int
m_confuse(object *monster)
{
    u8 *name = (u8 *)TEMP_BUFFER_ADDR;

    if (!rogue_can_see(monster->row, monster->col)) {
        return 0;
    }
    if (rand_percent(45)) {
        monster->m_flags &= (~CONFUSES);    /* will not confuse the rogue */
        return 0;
    }
    if (rand_percent(55)) {
        monster->m_flags &= (~CONFUSES);
        get_message(monster->m_name_id, name, 20);
        message_id(209, name);
        confuse();
        return 1;
    }
    return 0;
}

int
flame_broil(object *monster)
{
    short row, col;
    u8 *name = (u8 *)TEMP_BUFFER_ADDR;

    if ((!mon_sees(monster, rogue.row, rogue.col) || coin_toss())) {
        return 0;
    }
    row = rogue.row - monster->row;
    col = rogue.col - monster->col;
    if (row < 0) {
        row = -row;
    }
    if (col < 0) {
        col = -col;
    }
    if (((row != 0) && (col != 0) && (row != col)) || ((row > 7) || (col > 7))) {
        return 0;
    }

    get_message(200, name, 14);
    mon_hit(monster, (char *)name, 1);
    return 1;
}
