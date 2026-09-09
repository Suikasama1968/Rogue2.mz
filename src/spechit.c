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
#include "invent.h"
#include "message.h"
#include "monster.h"
#include "pack.h"
#include "random.h"
#include "score.h"
#include "spechit.h"
#include "use.h"

static void disappear(object *monster);

void
special_hit(object *monster)
{
    if ((monster->m_flags & CONFUSED) && rand_percent(66)) return;
    if (monster->m_flags & RUSTS) rust(monster);
    if (monster->m_flags & FREEZES) freeze(monster);
    if (monster->m_flags & STINGS) sting(monster);
    if (monster->m_flags & STEALS_GOLD) steal_gold(monster);
    else if (monster->m_flags & STEALS_ITEM) steal_item(monster);
}

void
rust(object *monster)
{
    if (!rogue.armor || rogue.armor_class <= 1 ||
        rogue.armor->which_kind == LEATHER) return;
    if (rogue.armor->is_protected) {
        if (!(monster->m_flags & RUST_VANISHED)) {
            message_id(201, 0);
            monster->m_flags |= RUST_VANISHED;
        }
    } else {
        --rogue.armor->d_enchant;
        --rogue.armor_class;
        message_id(202, 0);
        print_stats(STAT_ARMOR);
    }
}

void
freeze(object *monster)
{
    short freeze_percent = 99;
    short i;
    short n;

    if (rand_percent(12)) return;
    freeze_percent -= rogue.str_current + rogue.str_current / 2;
    freeze_percent -= rogue.exp * 4;
    freeze_percent -= rogue.armor_class * 5;
    freeze_percent -= rogue.hp_max / 3;
    if (freeze_percent <= 10) return;
    monster->m_flags |= FREEZING_ROGUE;
    message_id(203, 0);
    n = (short)get_rand(4, 8);
    for (i = 0; i < n && !game_over; ++i) mv_mons();
    if (!game_over && rand_percent(freeze_percent)) {
        for (i = 0; i < 50 && !game_over; ++i) mv_mons();
        if (!game_over) killed_by(0, HYPOTHERMIA);
    }
    if (!game_over) message_id(66, 0);
    monster->m_flags &= ~FREEZING_ROGUE;
}

void
sting(object *monster)
{
    short sting_chance = 35;
    u8 name[20];

    if (rogue.str_current <= 3) return;
    sting_chance += (short)(6 * (6 - rogue.armor_class));
    if (rogue.exp > 8) sting_chance -= (short)(6 * (rogue.exp - 8));
    if (rand_percent(sting_chance)) {
        get_message(monster->m_name_id, name, sizeof(name));
        message_id(207, name);
        --rogue.str_current;
        print_stats(STAT_STRENGTH);
    }
}

void
steal_gold(object *monster)
{
    long amount;

    if (rogue.gold <= 0 || rand_percent(10)) return;
    amount = get_rand(cur_level * 10, cur_level * 30);
    if (amount > rogue.gold) amount = rogue.gold;
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
    u8 desc[ROGUE_COLUMNS];

    if (rand_percent(15)) return;
    for (obj = rogue.pack.next_object; obj; obj = obj->next_object) {
        if (!(obj->in_use_flags & BEING_USED) &&
            get_rand(1, ++count) == 1) chosen = obj;
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

int
m_confuse(object *monster)
{
    u8 name[20];

    if (!rogue_can_see(monster->row, monster->col)) {
        return 0;
    }
    if (rand_percent(45)) {
        monster->m_flags &= ~CONFUSES;
        return 0;
    }
    if (rand_percent(55)) {
        monster->m_flags &= ~CONFUSES;
        get_message(monster->m_name_id, name, sizeof(name));
        message_id(209, name);
        confuse();
        return 1;
    }
    return 0;
}

static void
disappear(object *monster)
{
    remove_monster(monster);
}
