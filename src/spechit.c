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
#include "message.h"
#include "random.h"
#include "spechit.h"

void
special_hit(object *monster)
{
    if (monster->m_flags & STINGS) sting(monster);
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
        message_id_mz(207, name);
        --rogue.str_current;
        print_stats(STAT_STRENGTH);
    }
}
