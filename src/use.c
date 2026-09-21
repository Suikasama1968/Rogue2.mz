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
#include "display.h"
#include "invent.h"
#include "level.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "use.h"
#include "mz_curses.h"

short halluc = 0;
short blind = 0;
short confused = 0;
short levitate = 0;
short haste_self = 0;
boolean see_invisible = 0;
short extra_hp = 0;
boolean detect_monster = 0;

extern short bear_trap;
extern boolean being_held;
extern boolean sustain_strength;

void
quaff(void)
{
    short ch;
    object *obj;

    ch = pack_letter(0, POTION);
    if (ch == CANCEL){
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id(91, 0);  /* 232->91 メッセージ統合 */
        return;
    }
    if (obj->what_is != POTION) {
        message_id(233, 0);
        return;
    }
    switch (obj->which_kind) {
    case INCREASE_STRENGTH:
        message_id(234, 0);
        rogue.str_current++;
        if (rogue.str_current > rogue.str_max) {
            rogue.str_max = rogue.str_current;
        }
        break;
    case RESTORE_STRENGTH:
        rogue.str_current = rogue.str_max;
        message_id(235, 0);
        break;
    case HEALING:
        message_id(236, 0);
        potion_heal(0);
        break;
    case EXTRA_HEALING:
        message_id(237, 0);
        potion_heal(1);
        break;
    case POISON:
        if (!sustain_strength) {
            rogue.str_current -= get_rand(1, 3);
            if (rogue.str_current < 1) {
                rogue.str_current = 1;
            }
        }
        message_id(238, 0);
        if (halluc) {
            unhallucinate();
        }
        break;
    case RAISE_LEVEL:
        rogue.exp_points = level_points[rogue.exp - 1];
        add_exp(1, 1);
        break;
    case BLINDNESS:
        go_blind();
        break;
    case HALLUCINATION:
        message_id(239, 0);
        halluc += get_rand(500, 800);
        break;
    case DETECT_MONSTER:
        show_monsters();
        if (!(level_monsters.next_monster)) {
            message_id(230, 0);
        }
        break;
    case DETECT_OBJECTS:
        if (level_objects.next_object) {
            if (!blind) {
                show_objects();
            }
        } else {
            message_id(230, 0);
        }
        break;
    case CONFUSION:
        message_id((halluc ? 240 : 241), 0);
        confuse();
        break;
    case LEVITATION:
        message_id(242, 0);
        levitate += get_rand(15, 30);
        being_held = bear_trap = 0;
        break;
    case HASTE_SELF:
        message_id(243, 0);
        haste_self += get_rand(11, 21);
        if (!(haste_self % 2)) {
            haste_self++;
        }
        break;
    case SEE_INVISIBLE:
        message_id(244, 0);
        if (blind) {
            unblind();
        }
        see_invisible = 1;
        break;
    }
    identified_potions |= (unsigned short)(1U << obj->which_kind);
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
    object *scroll;

    ch = pack_letter(0, SCROL);

    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id(91, 0);
        return;
    }
    if (obj->what_is != SCROL) {
        message_id(247, 0);
        return;
    }
    scroll = obj;
    switch (obj->which_kind) {
    case SCARE_MONSTER:
        message_id(248, 0);
        break;
    case HOLD_MONSTER:
        for (monster = level_monsters.next_monster; monster;
             monster = monster->next_monster) monster->m_flags |= ASLEEP;
        message_id(269, 0);
        break;
    case ENCH_WEAPON:
        if (rogue.weapon) {
            if (coin_toss()) ++rogue.weapon->hit_enchant;
            else ++rogue.weapon->d_enchant;
            message_id(249, 0);
        } else message_id(250, 0);
        break;
    case ENCH_ARMOR:
        if (rogue.armor) {
            ++rogue.armor->d_enchant;
            message_id(251, 0);
        } else message_id(252, 0);
        break;
    case IDENTIFY:
        message_id(253, 0);
        wait_for_ack();
        check_message();
        scroll->identified = 1;
        id_scrolls[scroll->which_kind].id_status = IDENTIFIED;
        idntfy();
        break;
    case TELEPORT:
        tele();
        message_id(221, 0);
        break;
    case SLEEP:
        message_id(254, 0);
        take_a_nap();
        break;
    case PROTECT_ARMOR:
        if (rogue.armor) {
            rogue.armor->is_protected = 1;
            message_id(255, 0);
        } else message_id(256, 0);
        break;
    case REMOVE_CURSE:
        for (obj = rogue.pack.next_object; obj; obj = obj->next_object) {
            obj->is_cursed = 0;
        }
        message_id(257, 0);
        break;
    case CREATE_MONSTER:
        create_monster();
        break;
    case AGGRAVATE_MONSTER:
        for (monster = level_monsters.next_monster; monster;
             monster = monster->next_monster) monster->m_flags &= ~ASLEEP;
        message_id(248, 0);
        break;
    case MAGIC_MAPPING:
        for (row = MIN_ROW; row <= MAX_ROW; ++row) {
            for (col = 0; col < ROGUE_COLUMNS; ++col) {
                if (DUNGEON(row,col) != TILE_ROCK)
                    colorize_dungeon(row, col);
            }
        }
        attrset(A_NORMAL);
        message_id(259, 0);
        break;
    }
    if (id_scrolls[scroll->which_kind].id_status != CALLED) {
        id_scrolls[scroll->which_kind].id_status = IDENTIFIED;
    }
    vanish(scroll, scroll->which_kind != SLEEP, &rogue.pack);
}

void
vanish(object *obj, short rm, object *pack)
{
    if (obj->quantity > 1) {
        obj->quantity--;
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
        extra_hp += (extra ? 2 : 1);
        rogue.hp_current = rogue.hp_max;
    } else if (ratio >= 90L) {
    	rogue.hp_max += (extra ? 1 : 0);
        extra_hp += (extra ? 1 : 1);
        rogue.hp_current = rogue.hp_max;
    } else {
        if (ratio < 33L) {
                ratio = 33L;
            }
        if (extra) {
            ratio += ratio;
        }
        add = ratio * (rogue.hp_max - rogue.hp_current) / 100L;
        rogue.hp_current += add;
        if (rogue.hp_current > rogue.hp_max) {
            rogue.hp_current = rogue.hp_max;
        }
    }
    if (blind) {
        unblind();
    }
    if (confused && extra) {
        unconfuse();
    } else if (confused) {
        confused = (confused / 2) + 1;
    }
    if (halluc && extra) {
        unhallucinate();
    } else if (halluc) {
        halluc = (halluc / 2) + 1;
    }
}

void
idntfy(void)
{
    short ch;
    object *obj;
    uint8_t *desc = (uint8_t *)TEMP_BUFFER_ADDR;
    uint8_t length;
    const uint8_t *prompt = find_message(260, &length);

AGAIN:
    ch = pack_letter((char *)prompt, ALL_OBJECTS);
    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id(91, 0);
        check_message();
        goto AGAIN;
    }
    obj->identified = 1;
    switch (obj->what_is) {
    case POTION:
        identified_potions |= (unsigned short)(1U << obj->which_kind);
        break;
    case SCROL:
        id_scrolls[obj->which_kind].id_status = IDENTIFIED;
        break;
    case WAND:
        id_wands[obj->which_kind].id_status = IDENTIFIED;
        break;
    case RING:
        id_rings[obj->which_kind].id_status = IDENTIFIED;
        break;
    }
    get_desc(obj, (char *)desc, 1);
    message((char *)desc, 0);
}

void
eat(void)
{
    short ch;
    short moves;
    object *obj;

    ch = pack_letter(0, FOOD);
    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id(91, 0);
        return;
    }
    if (obj->what_is != FOOD) {
        message_id(264, 0);
        return;
    }
    if (obj->which_kind == FRUIT || rand_percent(60)) {
        moves = get_rand(900, 1100);
        message_id(obj->which_kind == RATION ? 266 : 267, 0);
    } else {
        moves = get_rand(700, 900);
        message_id(268, 0);
        add_exp(2, 1);
    }
    rogue.moves_left /= 3;
    rogue.moves_left += moves;
    hunger_str[0] = '\0';
    vanish(obj, 1, &rogue.pack);
}

void
tele(void)
{
    if (cur_room >= 0) {
        darken_room(cur_room);
    }
    put_player(get_room_number(rogue.row, rogue.col));
    being_held = 0;
    bear_trap = 0;
}

void
unhallucinate(void)
{
    halluc = 0;
    relight();
    message_id(272, 0);
}

void unblind(void)
{
    blind = 0;
    message_id(273, 0);
    relight();
}

void
relight(void)
{
    if (cur_room == PASSAGE) light_passage(rogue.row, rogue.col);
    else light_up_room(cur_room);
}

void
take_a_nap(void)
{
    short i;

    i = get_rand(2, 5);
    while (i--) {
        mv_mons();
    }
    message_id(66, 0);
}

void go_blind(void)
{
    if (!blind) message_id(274, 0);
    blind += get_rand(500, 800);
    if (cur_room >= 0) darken_room(cur_room);
}

void
confuse(void)
{
    confused += get_rand(12, 22);
}

void
unconfuse(void)
{
    confused = 0;
    message_id(277, 0);
}
