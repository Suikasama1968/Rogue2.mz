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
#include "message.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "ring.h"
#include "trap.h"
#include "mz_curses.h"
#include "mz_system.h"

static void object_message(object *obj, short msg_id);
static char next_pack_letter(object *pack);

object
*add_to_pack(object *obj, object *pack, int condense)
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

object
*pick_up(int row, int col, short *status)
{
    object *obj;
    
    obj = object_at(&level_objects, row, col);
    *status = 0;

    if (!obj) {
        return 0;
    }
    if (obj->what_is == GOLD) {
        take_from_pack(obj, &level_objects);
        rogue.gold += obj->quantity;
        *status = 1;
        return obj;
    }
    if (pack_count(obj) >= MAX_PACK_COUNT) {
        message_id(87, 0);
        return 0;
    }
    take_from_pack(obj, &level_objects);
    obj->picked_up = 1;
    obj = add_to_pack(obj, &rogue.pack, 1);
    *status = 1;
    return obj;
}

void
drop(void)
{
    object *obj;
    object *new_obj;
    short ch;

    if (object_at(&level_objects, rogue.row, rogue.col) ||
        (rogue.row == stairs_row && rogue.col == stairs_col) ||
        trap_at(rogue.row, rogue.col) != NO_TRAP) {
        message_id(88, 0);
        return;
    }
    if (!rogue.pack.next_object) {
        message_id(89, 0);
        return;
    }
    ch = pack_letter(0, ALL_OBJECTS);
    if (ch == CANCEL) return;
    obj = get_letter_object(ch);
    if (!obj) {
        message_id(91, 0);
        return;
    }
    if (obj->in_use_flags &
        (BEING_WIELDED | BEING_WORN | ON_LEFT_HAND | ON_RIGHT_HAND)) {
        if (obj->is_cursed) {
            message_id(85, 0);
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

void
wait_for_ack(void)
{
    while (rgetchar() != ' ') {}
}

int 
pack_letter(char *prompt, unsigned short mask)
{
    object *obj;
    int ch;
    short msg_id = 0;

    if (!mask_pack(&rogue.pack, mask)) {
        message_id(93, 0);
        return CANCEL;
    }
    if (!prompt) {
        switch (mask) {
        case ALL_OBJECTS:
            msg_id = 90;
            break;
        case POTION:
            msg_id = 231;
            break;
        case SCROL:
            msg_id = 245;
            break;
        case WAND:
            msg_id = 278;
            break;
        case RING:
            msg_id = 161;
            break;
        case ARMOR:
            msg_id = 97;
            break;
        case WEAPON:
            msg_id = 101;
            break;
        default:
            msg_id = 262;
            break;
        }
    }
    for (;;) {
        if (prompt) message(prompt, 0);
        else message_id(msg_id, 0);
        ch = rgetchar();
        check_message();
        if (ch == LIST) {
            inventory(&rogue.pack, mask);
            continue;
        }
        if (ch == CANCEL) return CANCEL;
        obj = get_letter_object(ch);
        if (obj && (obj->what_is & mask)) return ch;
    }
}

void
take_off(void)
{
    object *obj = rogue.armor;

    if (!obj) {
        message_id(95, 0);
        return;
    }
    if (obj->is_cursed) {
        message_id(85, 0);
        return;
    }
    unwear(obj);
    object_message(obj, 94);
    reg_move();
}

void
wear(void)
{
    short ch;
    object *obj;

    if (rogue.armor) {
        message_id(96, 0);
        return;
    }
    ch = pack_letter(0, ARMOR);

    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id(91, 0);
        return;
    }
    if (obj->what_is != ARMOR) {
        message_id(99, 0);
        return;
    }
    object_message(obj, 100);
    do_wear(obj);
    reg_move();

}

void
unwear(object *obj)
{
    if (obj) {
        obj->in_use_flags &= (~BEING_WORN);
    }   
    rogue.armor = 0;
}

void
do_wear(object *obj)
{
    rogue.armor = obj;
    obj->in_use_flags |= BEING_WORN;
    obj->identified = 1;
}

void
wield(void)
{
    short ch;
    object *obj;

    if (rogue.weapon && rogue.weapon->is_cursed) {
        message_id(85, 0);
        return;
    }
    ch = pack_letter(0, WEAPON);

    if (ch == CANCEL) {
        return;
    }
    if (!(obj = get_letter_object(ch))) {
        message_id(91, 0);
        return;

    }
    if (obj->what_is & (ARMOR | RING)) {
        message_id((obj->what_is == ARMOR) ? 104 : 105, 0);
        return;
    }
    if (obj == rogue.weapon) {
        message_id(106, 0);
    } else {
        unwield(rogue.weapon);
        object_message(obj, 107);
        do_wield(obj);
        reg_move();
    }
}

void
do_wield(object *obj)
{
    rogue.weapon = obj;
    obj->in_use_flags |= BEING_WIELDED;
}

void
unwield(object *obj)
{
    if (obj) {
        obj->in_use_flags &= (~BEING_WIELDED);
    }
    rogue.weapon = (object *)0;
}

#if 0 /* MZ-700/1500では未対応 */
void
call_it(void)
{
    short ch;
    object *obj;
    struct id *id_table;
    char buf[MAX_TITLE_LENGTH + 2];

    ch = pack_letter(mesg[108], (SCROL | POTION | WAND | RING));

    if (ch == CANCEL) {
	return;
    }
    if (!(obj = get_letter_object(ch))) {
	message(mesg[109], 0);
	return;
    }
    if (!(obj->what_is & (SCROL | POTION | WAND | RING))) {
	message(mesg[110], 0);
	return;
    }
    id_table = get_id_table(obj);

#if defined( JAPAN )
    if (get_input_line(mesg[111],
		       "", buf, id_table[obj->which_kind].title, 0, 1)) {
	ch = *buf;
#if defined( EUC )
	if (ch >= ' ' && !(ch & 0x80)) {	/* by Yasha */
	    /* alphabet or kana character; append 1 blank */
	    (void) strcat(buf, " ");
	}
#else /* not EUC */
	if (ch >= ' ' && ch <= '~' || ch >= 0xa0 && ch <= 0xde) {
	    /* alphabet or kana character; append 1 blank */
	    (void) strcat(buf, " ");
	}
#endif /* not EUC */
	id_table[obj->which_kind].id_status = CALLED;
	(void) strcpy(id_table[obj->which_kind].title, buf);
    }
#else /* not JAPAN */
    if (get_input_line(mesg[111],
		       "", buf, id_table[obj->which_kind].title, 1, 1)) {
	id_table[obj->which_kind].id_status = CALLED;
	(void) strcpy(id_table[obj->which_kind].title, buf);
    }
#endif /* not JAPAN */
}
#endif

int
pack_count(object *new_obj)
{
    object *obj;
    short count = 0;

    obj = rogue.pack.next_object;

    while (obj) {
        if (obj->what_is != WEAPON) {
            count += obj->quantity;
        } else if (!new_obj || new_obj->what_is != WEAPON ||
                   obj->which_kind != new_obj->which_kind) {
            count++;
        }
        obj = obj->next_object;
    }
    return count;
}

boolean
mask_pack(object *pack, unsigned short mask)
{
    while (pack->next_object) {
        pack = pack->next_object;
        if (pack->what_is & mask) {
            return 1;
        }
    }
    return 0;
}

#if 0 /* MZ-700/1500では未対応 */
int
is_pack_letter(short *c, unsigned short *mask)
{
    switch (*c) {
    case '?':
	*mask = SCROL;
	goto found;
    case '!':
	*mask = POTION;
	goto found;
    case ':':
	*mask = FOOD;
	goto found;
    case ')':
	*mask = WEAPON;
	goto found;
    case ']':
	*mask = ARMOR;
	goto found;
    case '/':
	*mask = WAND;
	goto found;
    case '=':
	*mask = RING;
	goto found;
    case ',':
	*mask = AMULET;
	goto found;
    default:
	return ((*c >= 'a' && *c <= 'z') || *c == CANCEL || *c == LIST);
    }
found:
    *c = LIST;
    return 1;
}
#endif

int
has_amulet(void)
{
    return (mask_pack(&rogue.pack, AMULET));
}

#if 0 /* MZ-700/1500では未対応 */
void
kick_into_pack(void)
{
    object *obj;
    char *p;
    char desc[ROGUE_COLUMNS];
    short stat;
    extern short levitate;

    if (!(dungeon[rogue.row][rogue.col] & OBJECT)) {
	message(mesg[112], 0);
    } else {
#if !defined( ORIGINAL )
	if (levitate) {
	    message(mesg[113], 0);
	    return;
	}
#endif /* ORIGINAL */
	if ((obj = pick_up(rogue.row, rogue.col, &stat))) {
	    get_desc(obj, desc, 1);
#if defined( JAPAN )
	    (void) strcat(desc, mesg[114]);
#endif /* JAPAN */
	    if (obj->what_is == GOLD) {
		message(desc, 0);
		free_object(obj);
	    } else {
		//p = desc + strlen(desc);
		p = desc + utf8strlen(desc);
		*p++ = '(';
		*p++ = obj->ichar;
		*p++ = ')';
		*p = 0;
		message(desc, 0);
	    }
	}
	if (obj || (!stat)) {
	    (void) reg_move();
	}
    }
}
#endif

/* MZ-700/1500固有 */
static void object_message(object *obj, short msg_id)
{
    char *desc = (char *)TEMP_BUFFER_ADDR;
    short length;

    get_desc(obj, desc, 0);
    for (length = 0; desc[length] != '\0'; ++length) {}
    get_message(msg_id, (u8 *)desc + length, ROGUE_COLUMNS - length);
    message((char *)desc, 0);
}

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
