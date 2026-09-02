/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include "rogue.h"
#include "level.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "use.h"

extern long level_points[];

void
quaff(void)
{
    short ch;
    object *obj;

    ch = (short)pack_letter(0, POTION);
    if (ch == CANCEL){
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id_mz(232, 0);
        return;
    }
    if (obj->what_is != POTION) {
        message_id_mz(233, 0);
        return;
    }
    switch (obj->which_kind) {
    case INCREASE_STRENGTH:
        message_id_mz(234, 0);
        rogue.str_current++;
        if (rogue.str_current > rogue.str_max) {
            rogue.str_max = rogue.str_current;
        }
        break;
    case RESTORE_STRENGTH:
        rogue.str_current = rogue.str_max;
        message_id_mz(235, 0);
        break;
    case HEALING:
        message_id_mz(236, 0);
        potion_heal(0);
        break;
    case EXTRA_HEALING:
        message_id_mz(237, 0);
        potion_heal(1);
        break;
    case POISON:
        rogue.str_current -= (short)get_rand(1, 3);
        if (rogue.str_current < 1) {
            rogue.str_current = 1;
        }
        message_id_mz(238, 0);
        break;
    case RAISE_LEVEL:
        rogue.exp_points = level_points[rogue.exp - 1];
        add_exp(1, 1);
        break;
    }
    print_stats(STAT_STRENGTH | STAT_HP);
    vanish(obj, 1, &rogue.pack);
}

void
read_scroll(void)
{
    short ch;
    int row, col;
    object *obj;
    object *monster;

    ch = (short)pack_letter(0, SCROL);

    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id_mz(246, 0);
        return;
    }
    if (obj->what_is != SCROL) {
        message_id_mz(247, 0);
        return;
    }
    switch (obj->which_kind) {
    case SCARE_MONSTER:
        message_id_mz(248, 0);
        break;
    case HOLD_MONSTER:
        for (monster = level_monsters.next_object; monster;
             monster = monster->next_object) monster->m_flags |= ASLEEP;
        message_id_mz(269, 0);
        break;
    case ENCH_WEAPON:
        if (rogue.weapon) {
            if (coin_toss()) ++rogue.weapon->hit_enchant;
            else ++rogue.weapon->d_enchant;
            message_id_mz(249, 0);
        } else message_id_mz(250, 0);
        break;
    case ENCH_ARMOR:
        if (rogue.armor) {
            ++rogue.armor_class;
            ++rogue.armor->d_enchant;
            message_id_mz(251, 0);
        } else message_id_mz(252, 0);
        break;
    case IDENTIFY:
        message_id_mz(253, 0);
        break;
    case TELEPORT:
        put_player(cur_room);
        message_id_mz(221, 0);
        break;
    case SLEEP:
        message_id_mz(254, 0);
        rest(get_rand(3, 6));
        break;
    case PROTECT_ARMOR:
        if (rogue.armor) {
            rogue.armor->is_protected = 1;
            message_id_mz(255, 0);
        } else message_id_mz(256, 0);
        break;
    case REMOVE_CURSE:
        for (obj = rogue.pack.next_object; obj; obj = obj->next_object) {
            obj->is_cursed = 0;
        }
        message_id_mz(257, 0);
        break;
    case CREATE_MONSTER:
        create_monster();
        break;
    case AGGRAVATE_MONSTER:
        for (monster = level_monsters.next_object; monster;
             monster = monster->next_object) monster->m_flags &= ~ASLEEP;
        message_id_mz(248, 0);
        break;
    case MAGIC_MAPPING:
        for (row = MIN_ROW; row <= MAX_ROW; ++row) {
            for (col = 0; col < ROGUE_COLUMNS; ++col) {
                if (DUNGEON(row,col) != TILE_ROCK)
                    DUNGEON_ATTR(row,col) = ATTR_VISIBLE;
            }
        }
        message_id_mz(259, 0);
        break;
    }
    vanish(obj, (short)(obj->which_kind != SLEEP), &rogue.pack);
}

void
vanish(object *obj, short rm, object *pack)
{
    if (obj->quantity > 1) {
        --obj->quantity;
    } else {
        take_from_pack(obj, pack);
        free_object(obj);
    }
    if (rm) {
        reg_move();
    }
}

void
potion_heal(int extra)
{
    long ratio;
    short add;

    rogue.hp_current += rogue.exp;

    ratio = rogue.hp_current * 100L / rogue.hp_max;
    
    if (ratio >= 100L) {
        rogue.hp_max += (extra ? 2 : 1);
        rogue.hp_current = rogue.hp_max;
    } else if (ratio >= 90L) {
    	rogue.hp_max += (extra ? 1 : 0);
        rogue.hp_current = rogue.hp_max;
    } else {
        if (ratio < 33L) {
                ratio = 33L;
            }
        if (extra) {
            ratio += ratio;
        }
        add = (short)(ratio * (rogue.hp_max - rogue.hp_current) / 100L);
        rogue.hp_current += add;
        if (rogue.hp_current > rogue.hp_max) {
            rogue.hp_current = rogue.hp_max;
        }
    }
}

void eat(void)
{
    short ch;
    short moves;
    object *obj;

    ch = pack_letter(0, FOOD);
    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id_mz(263, 0);
        return;
    }
    if (obj->what_is != FOOD) {
        message_id_mz(264, 0);
        return;
    }
    if (obj->which_kind == FRUIT || rand_percent(60)) {
        moves = (short)get_rand(900, 1100);
        message_id_mz((short)(obj->which_kind == RATION ? 266 : 267), 0);
    } else {
        moves = (short)get_rand(700, 900);
        message_id_mz(268, 0);
        add_exp(2, 1);
    }
    rogue.moves_left /= 3;
    rogue.moves_left += moves;
    hunger_str[0] = '\0';
    vanish(obj, 1, &rogue.pack);
}
