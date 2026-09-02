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
#include "invent.h"
#include "message.h"
#include "move.h"
#include "mz_curses.h"
#include "object.h"
#include "pack.h"
#include "ring.h"
#include "trap.h"

static char next_pack_letter(object *pack)
{
    char letter;
    object *obj;

    for (letter = 'a'; letter <= 'z'; ++letter) {
        for (obj = pack->next_object; obj; obj = obj->next_object) {
            if (obj->ichar == letter) break;
        }
        if (!obj) return letter;
    }
    return '?';
}

object *add_to_pack(object *obj, object *pack, int condense)
{
    object *p = pack;

    if (condense) {
        while (p->next_object) {
            p = p->next_object;
            if (p->what_is == obj->what_is && p->which_kind == obj->which_kind &&
                obj->what_is != WAND && obj->what_is != RING) {
                p->quantity += obj->quantity;
                free_object(obj);
                return p;
            }
        }
        p = pack;
    }
    while (p->next_object) p = p->next_object;
    if (pack == &rogue.pack && !obj->ichar) {
        obj->ichar = next_pack_letter(pack);
    }
    p->next_object = obj;
    obj->next_object = 0;
    return obj;
}

void
take_from_pack(object *obj, object *pack)
{
    while (pack->next_object != obj) {
        pack = pack->next_object;
    }
    pack->next_object = pack->next_object->next_object;
}

object *pick_up(int row, int col, short *status)
{
    object *obj;
    
    obj = object_at(&level_objects, (short)row, (short)col);

    *status = 0;
    if (!obj) return 0;
    take_from_pack(obj, &level_objects);
    if (obj->what_is == GOLD) {
        rogue.gold += obj->quantity;
        *status = 1;
        return obj;
    }
    obj->picked_up = 1;
    obj = add_to_pack(obj, &rogue.pack, 1);
    *status = 1;
    return obj;
}

int pack_letter(char *prompt, unsigned short mask)
{
    object *obj;
    int ch;

    for (obj = rogue.pack.next_object; obj; obj = obj->next_object) {
        if (obj->what_is & mask) break;
    }
    if (!obj) {
        message_id_mz(93, 0);
        return CANCEL;
    }
    if (prompt) message_mz((const u8 *)prompt, 0);
    else if (mask == ALL_OBJECTS) message_id_mz(90, 0);
    else if (mask == POTION) message_id_mz(231, 0);
    else if (mask == SCROL) message_id_mz(245, 0);
    else if (mask == WAND) message_id_mz(278, 0);
    else if (mask == RING) message_id_mz(161, 0);
    else if (mask == ARMOR) message_id_mz(97, 0);
    else if (mask == WEAPON) message_id_mz(101, 0);
    else message_id_mz(262, 0);
    move((u8)rogue.row, (u8)rogue.col);
    refresh();
    ch = rgetchar();
    check_message();
    return (short)ch;
}

static void object_message(object *obj, short msg_id)
{
    char desc[ROGUE_COLUMNS];
    short length;

    get_desc(obj, desc, 0);
    for (length = 0; desc[length] != '\0'; ++length) {}
    get_message(msg_id, (u8 *)desc + length, ROGUE_COLUMNS - length);
    message_mz((u8 *)desc, 0);
}

void unwear(object *obj)
{
    if (obj) obj->in_use_flags &= ~BEING_WORN;
    rogue.armor = 0;
    rogue.armor_class = 0;
}

void do_wear(object *obj)
{
    rogue.armor = obj;
    obj->in_use_flags |= BEING_WORN;
    rogue.armor_class = (short)obj->which_kind + 2;
    if (obj->which_kind == 4 || obj->which_kind == 5) --rogue.armor_class;
    rogue.armor_class += obj->d_enchant;
}

void take_off(void)
{
    object *obj = rogue.armor;

    if (!obj) {
        message_id_mz(95, 0);
        return;
    }
    if (obj->is_cursed) {
        message_id_mz(85, 0);
        return;
    }
    unwear(obj);
    object_message(obj, 94);
    reg_move();
}

void wear(void)
{
    short ch;
    object *obj;

    if (rogue.armor) {
        message_id_mz(96, 0);
        return;
    }
    ch = (short)pack_letter(0, ARMOR);
    if (ch == CANCEL) return;
    obj = get_letter_object(ch);
    if (!obj) message_id_mz(98, 0);
    else if (obj->what_is != ARMOR) message_id_mz(99, 0);
    else {
        object_message(obj, 100);
        do_wear(obj);
        reg_move();
    }
}

void unwield(object *obj)
{
    if (obj) obj->in_use_flags &= ~BEING_WIELDED;
    rogue.weapon = 0;
}

void do_wield(object *obj)
{
    rogue.weapon = obj;
    obj->in_use_flags |= BEING_WIELDED;
}

void wield(void)
{
    short ch = (short)pack_letter(0, WEAPON);
    object *obj;

    if (rogue.weapon && rogue.weapon->is_cursed) {
        message_id_mz(85, 0);
        return;
    }
    if (ch == CANCEL) return;
    obj = get_letter_object(ch);
    if (!obj) message_id_mz(102, 0);
    else if (obj->what_is != WEAPON) message_id_mz(103, 0);
    else if (obj == rogue.weapon) message_id_mz(106, 0);
    else {
        unwield(rogue.weapon);
        object_message(obj, 107);
        do_wield(obj);
        reg_move();
    }
}

void drop(void)
{
    object *obj;
    object *new_obj;
    short ch;

    if (object_at(&level_objects, rogue.row, rogue.col) ||
        (rogue.row == stairs_row && rogue.col == stairs_col) ||
        trap_at(rogue.row, rogue.col) != NO_TRAP) {
        message_id_mz(88, 0);
        return;
    }
    if (!rogue.pack.next_object) {
        message_id_mz(89, 0);
        return;
    }
    ch = (short)pack_letter(0, ALL_OBJECTS);
    if (ch == CANCEL) return;
    obj = get_letter_object(ch);
    if (!obj) {
        message_id_mz(91, 0);
        return;
    }
    if (obj->in_use_flags &
        (BEING_WIELDED | BEING_WORN | ON_LEFT_HAND | ON_RIGHT_HAND)) {
        if (obj->is_cursed) {
            message_id_mz(85, 0);
            return;
        }
        if (obj->in_use_flags & BEING_WIELDED) unwield(obj);
        if (obj->in_use_flags & BEING_WORN) unwear(obj);
        if (obj->in_use_flags & (ON_LEFT_HAND | ON_RIGHT_HAND)) un_put_on(obj);
    }
    if (obj->quantity > 1 && obj->what_is != WEAPON) {
        --obj->quantity;
        new_obj = alloc_object();
        if (!new_obj) return;
        *new_obj = *obj;
        new_obj->quantity = 1;
        new_obj->next_object = 0;
        obj = new_obj;
    } else {
        take_from_pack(obj, &rogue.pack);
    }
    obj->ichar = 0;
    place_at(obj, rogue.row, rogue.col);
    object_message(obj, 92);
    print_stats(STAT_ARMOR);
    reg_move();
}

int has_amulet(void)
{
    object *obj = rogue.pack.next_object;

    while (obj) {
        if (obj->what_is == AMULET) return 1;
        obj = obj->next_object;
    }
    return 0;
}
