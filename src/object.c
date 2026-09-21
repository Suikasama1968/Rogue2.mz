/*
 * object.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include <string.h>

#include "rogue.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"
#include "room.h"
#include "mz_system.h"

#define object_pool ((object *)OBJECT_POOL_ADDR)
#define object_used ((uint8_t *)OBJECT_USED_ADDR)

typedef char object_pool_size_check[
    sizeof(object) * MAX_OBJECTS <= OBJECT_POOL_SIZE ? 1 : -1];
typedef char object_used_size_check[
    MAX_OBJECTS <= OBJECT_USED_SIZE ? 1 : -1];

static short foods;
short party_counter;
unsigned short identified_potions;

fighter rogue;

void
put_objects(void)
{
    short i, n; 
    object *obj;

    n = coin_toss() ? get_rand(2, 4) : get_rand(3, 5);
 
    clear_level_objects();
    if (cur_level == party_counter) {
        make_party();
        party_counter = next_party();
    }

    if (cur_level >= AMULET_LEVEL && !has_amulet()) put_amulet();

    while (n < 8 && rand_percent(33)) ++n;

    for (i = 0; i < n; i++) {
        obj = gr_object();
        if (obj) rand_place(obj);
    }
    put_gold();
}

void
put_gold(void)
{
    int i, j;
    short row, col;

    for (i = 0; i < MAXROOMS; i++) {
        if (!room_exists[i] || !rand_percent(GOLD_PERCENT)) {
            continue;
        }
        for (j = 0; j < 50; j++) {
            row = get_rand(rooms[i].top_row + 1, rooms[i].bottom_row - 1);
            col = get_rand(rooms[i].left_col + 1, rooms[i].right_col - 1);
            if (DUNGEON(row, col) == TILE_FLOOR &&
                !object_at(&level_objects, row, col)) {
                plant_gold(row, col, 0);
                break;
            }
        }
    }
}

void plant_gold(short row, short col, boolean is_maze)
{
    object *obj;
    
    obj = alloc_object();

    if (!obj) return;
    obj->what_is = GOLD;
    obj->which_kind = 0;
    obj->quantity = get_rand(2 * cur_level, 16 * cur_level);
    if (is_maze) obj->quantity += obj->quantity / 2;
    place_at(obj, row, col);
}

void
place_at(object *obj, int row, int col)
{
    obj->row = row;
    obj->col = col;
    add_to_pack(obj, &level_objects, 0);
}
object *
object_at(object *pack, short row, short col)
{
    object *obj;
    
    obj = pack->next_object;

    while (obj && ((obj->row != row) || (obj->col != col))) {
        obj = obj->next_object;
    }
    return obj;
}

object *
get_letter_object(int ch)
{
    object *obj;
    
    obj = rogue.pack.next_object;

    while (obj && obj->ichar != ch) {
        obj = obj->next_object;
    }
    return obj;
}

object *
gr_object(void)
{
    object *obj;

    obj = alloc_object();
    if (!obj) return 0;

    if (foods < cur_level / 3) {
        obj->what_is = FOOD;
        foods++;
    } else {
        obj->what_is = gr_what_is();
    }

    switch (obj->what_is) {
    case FOOD:
        get_food(obj, 0);
        break;
    case POTION:
        gr_potion(obj);
        break;
    case SCROL:
        gr_scroll(obj);
        break;
    case WAND:
        gr_wand(obj);
        break;
    case WEAPON:
        gr_weapon(obj, 1);
        break;
    case ARMOR:
        gr_armor(obj, 1);
        break;
    case RING:
        gr_ring(obj, 1);
        break;
    }
    return obj;
}

unsigned short
gr_what_is(void)
{
    short percent;
    int i;
    static short per[] = { 30, 60, 64, 74, 83, 88, 91 };
    static unsigned short ret[] = {
        SCROL, POTION, WAND, WEAPON, ARMOR, FOOD, RING
    };

    percent = get_rand(1, 91);

    for (i = 0;; i++) {
        if (percent <= per[i]) return ret[i];
    }
}

void
gr_scroll(object *obj)
{
    short percent;
    int i;
    static const uint8_t per[SCROLS] = {
        5, 11, 16, 21, 36, 44, 51, 56, 65, 74, 80, 85
    };

    percent = get_rand(0, 85);
    obj->what_is = SCROL;
    for (i = 0;; i++) {
        if (percent <= per[i]) {
            obj->which_kind = i;
            return;
        }
    }
}

void
gr_potion(object *obj)
{
    short percent;
    int i;
    static const uint8_t per[POTIONS] = {
        10, 20, 30, 40, 50, 55, 65, 75, 85, 95, 105, 110, 114, 118
    };

    percent = get_rand(1, 118);
    obj->what_is = POTION;
    for (i = 0; i < POTIONS; i++) {
        if (percent <= per[i]) {
            obj->which_kind = i;
            return;
        }
    }
}

void 
gr_weapon(object *obj, int assign_wk)
{
    short kind;
    short percent;
    short blessing;
    short increment = 1;
    short i;

    obj->what_is = WEAPON;
    if (assign_wk) {
        obj->which_kind = get_rand(0, WEAPONS - 1);
    }
    kind = obj->which_kind;
    if (kind == DART || kind == ARROW || kind == DAGGER || kind == SHURIKEN) {
        obj->quantity = get_rand(3, 15);
    } else {
        obj->quantity = 1;
    }
    obj->hit_enchant = obj->d_enchant = 0;
    obj->is_cursed = 0;

    percent = get_rand(1, 96);
    blessing = get_rand(1, 3);

    if (percent <= 16) {
        increment = 1;
    } else if (percent <= 32) {
        increment = -1;
        obj->is_cursed = 1;
    }
    if (percent <= 32) {
        for (i = 0; i < blessing; i++) {
            if (coin_toss()) {
                obj->hit_enchant += increment;
            }else{
                 obj->d_enchant += increment;
            }
        }
    }
}

void
gr_armor(object *obj, int assign_wk)
{				/* by Yasha */
    short percent;
    short blessing;

    obj->what_is = ARMOR;
    if (assign_wk) 		/* by Yasha */
        obj->which_kind = get_rand(0, (ARMORS - 1));
    obj->class = obj->which_kind + 2;
    if (obj->which_kind == PLATE || obj->which_kind == SPLINT) {
        obj->class--;
    }
    obj->quantity = 1;
    obj->is_protected = 0;
    obj->is_cursed = 0;
    obj->d_enchant = 0;

    percent = get_rand(1, 100);
    blessing = get_rand(1, 3);

    if (percent <= 16) {
        obj->is_cursed = 1;
        obj->d_enchant -= blessing;
    } else if (percent <= 33) {
        obj->d_enchant += blessing;
    }
}

void
gr_wand(object *obj)
{
    obj->what_is = WAND;
    obj->which_kind = get_rand(0, WANDS - 1);
    if (obj->which_kind == MAGIC_MISSILE) {
        obj->hit_enchant = get_rand(6, 12);
    } else {
        obj->hit_enchant = get_rand(3, 6);
    }
}

void
get_food(object *obj, boolean force_ration)
{
    obj->what_is = FOOD;

    if (force_ration || rand_percent(80)) {
	    obj->which_kind = RATION;
    } else {
	    obj->which_kind = FRUIT;
    }
}

void put_stairs(void)
{
    short row;
    short col;

    do {
        gr_row_col(&row, &col, FLOOR | TUNNEL);
    } while (object_at(&level_objects, row, col));
    stairs_row = (uint8_t)row;
    stairs_col = (uint8_t)col;
    DUNGEON(row, col) = TILE_STAIRS;
}

int
get_armor_class(object *obj)
{
    if (obj) return obj->class + obj->d_enchant;
    return 0;
}

object
*alloc_object(void)
{
    int i;

    i = MAX_OBJECTS - 1;
    do {
        if (!object_used[i]) {
            object_used[i] = 1;
            object_pool[i].quantity = 1;
            return &object_pool[i];
        }
    } while (i--);
    return 0;
}

void
free_object(object *obj)
{
    int i;

    i = MAX_OBJECTS - 1;
    do {
        if (obj == &object_pool[i]) {
            object_used[i] = 0;
            memset(obj, 0, sizeof(object));
            return;
        }
    } while (i--);
}

void make_party(void)
{
    party_room = gr_room();
    party_objects(party_room);
}

void
show_objects(void)
{
    object *obj;

    obj = level_objects.next_object;
    while (obj) {
        obj->picked_up |= OBJECT_DETECTED;
        obj = obj->next_object;
    }
}

void put_amulet(void)
{
    object *obj = alloc_object();

    if (!obj) return;
    obj->what_is = AMULET;
    obj->which_kind = 0;
    obj->quantity = 1;
    rand_place(obj);
}

void rand_place(object *obj)
{
    short row, col;

    do {
        gr_row_col(&row, &col, FLOOR | TUNNEL);
    } while (object_at(&level_objects, row, col));
    place_at(obj, row, col);
}

#if 0 /* MZ-700/1500では未対応 */

void
new_object_for_wizard(void)
{
    short ch, max = 0;		/* 未初期化変数の使用の Warning の対策で 0 を不可 */
#if defined( ORIGINAL )
    short wk;
#endif /* ORIGINAL */
    object *obj;
    char buf[80];

    if (pack_count((object *) 0) >= MAX_PACK_COUNT) {
	message(mesg[81], 0);
	return;
    }
    message(mesg[82], 0);

    while (r_index("!?:)]=/,\033", (ch = rgetchar()), 0) == -1) {
	sound_bell();
    }
    check_message();
    if (ch == '\033') {
	return;
    }

    obj = alloc_object();

    switch (ch) {
    case '!':
	obj->what_is = POTION;
	max = POTIONS - 1;
	break;
    case '?':
	obj->what_is = SCROL;
	max = SCROLS - 1;
	break;
    case ',':
	obj->what_is = AMULET;
	break;
    case ':':
	get_food(obj, 0);
	break;
    case ')':
/*		gr_weapon(obj, 0);*/
	obj->what_is = WEAPON;
	max = WEAPONS - 1;
	break;
    case ']':
/*		gr_armor(obj);*/
	obj->what_is = ARMOR;	/* by Yasha */
	max = ARMORS - 1;
	break;
    case '/':
	gr_wand(obj);
	max = WANDS - 1;
	break;
    case '=':
	max = RINGS - 1;
	obj->what_is = RING;
	break;
    }
    if ((ch != ',') && (ch != ':')) {
#if !defined( ORIGINAL )
/*		sprintf(buf, mesg[83], name_of(obj));*/
	sprintf(buf, mesg[83], (obj->what_is == WEAPON)	/* by Yasha */
		? mesg[84] : name_of(obj));	/* by Yasha */
	for (;;) {
	    message(buf, 0);
	    for (;;) {
		ch = rgetchar();
		if ((ch != LIST && ch != CANCEL && ch < 'a') || ch > 'a' + max) {
		    sound_bell();
		} else {
		    break;
		}
	    }
	    if (ch == LIST) {
		check_message();
		list_object(obj, max);
	    } else {
		break;
	    }
	}
	check_message();
	if (ch == CANCEL) {
	    free_object(obj);
	    return;
	}
	obj->which_kind = ch - 'a';
	if (obj->what_is == RING) {
	    gr_ring(obj, 0);
	}

	if (obj->what_is == ARMOR) {	/* by Yasha */
	    gr_armor(obj, 0);	/* by Yasha */
	} else if (obj->what_is == WEAPON) {	/* by Yasha */
	    gr_weapon(obj, 0);	/* by Yasha */
	}
#else /* ORIGINAL */
	    if (get_input_line("Which kind?", "", buf, "", 0, 1)) {
	    wk = get_number(buf);
	    if ((wk >= 0) && (wk <= max)) {
		obj->which_kind = (unsigned short) wk;
		if (obj->what_is == RING) {
		    gr_ring(obj, 0);
		}
	    } else {
		sound_bell();
		goto GIL;
	    }
	} else {
	    free_object(obj);
	    return;
	}
#endif /* ORIGINAL */
    }
    get_desc(obj, buf, 1);
    message(buf, 0);
    (void) add_to_pack(obj, &rogue.pack, 1);
}

void
list_object(object *obj, short max)
{
    short i, j, maxlen, n;
    char descs[ROGUE_LINES][ROGUE_COLUMNS];
    short row, col;
    struct id *id;
    int weapon_or_armor;	/* by Yasha */
#if defined( COLOR )
    char *p;
#endif /* COLOR */
#if defined( JAPAN )
    char *msg = "  ＝スペースを押してください＝";
    short len = 30;
#else /* not JAPAN */
    char *msg = " --Press space to continue--";
    short len = 28;
#endif /* not JAPAN */

    weapon_or_armor = 0;
    switch (obj->what_is) {
    case ARMOR:
	id = id_armors;
	weapon_or_armor = 1;	/* by Yasha */
	break;
    case WEAPON:
	id = id_weapons;
	weapon_or_armor = 1;	/* by Yasha */
	break;
    case SCROL:
	id = id_scrolls;
	break;
    case POTION:
	id = id_potions;
	break;
    case WAND:
	id = id_wands;
	break;
    case RING:
	id = id_rings;
	break;
    default:
	return;
    }

    maxlen = len;
    for (i = 0; i <= max; i++) {
#if 1				/* by Yasha */
#if defined( JAPAN )
	sprintf(descs[i], " %c) %s%s", i + 'a',
		weapon_or_armor ? id[i].title : id[i].real,
		weapon_or_armor ? "" : name_of(obj));
#else /* not JAPAN */
	sprintf(descs[i], " %c) %s%s", i + 'a',
		weapon_or_armor ? "" : name_of(obj),
		weapon_or_armor ? id[i].title : id[i].real);
#endif /* not JAPAN */
#else
#if defined( JAPAN )
	sprintf(descs[i], " %c) %s%s", i + 'a', id[i].real, name_of(obj));
#else /* not JAPAN */
	sprintf(descs[i], " %c) %s%s", i + 'a', name_of(obj), id[i].real);
#endif /* not JAPAN */
#endif
	//if ((n = strlen(descs[i])) > maxlen) {
	if ((n = utf8strlen(descs[i])) > maxlen) {
	    maxlen = n;
	}
    }
    (void) strcpy(descs[i++], msg);

    col = ROGUE_COLUMNS - (maxlen + 2);
    for (row = 0; row < i; row++) {
	if (row > 0) {
	    for (j = col; j < ROGUE_COLUMNS; j++) {
		descs[row - 1][j - col] = mvinch_rogue(row, j);
	    }
	    descs[row - 1][j - col] = 0;
	}
	mvaddstr_rogue(row, col, descs[row]);
	clrtoeol();
    }
    refresh();
    wait_for_ack();

    move(0, 0);
    clrtoeol();
#if defined( COLOR )
    for (j = 1; j < i; j++) {
	move(j, col);
	for (p = descs[j - 1]; *p; p++) {
	    addch_rogue(*p);
	}
    }
#else /* not COLOR */
    for (j = 1; j < i; j++) {
	mvaddstr_rogue(j, col, descs[j - 1]);
    }
#endif /* not COLOR */
}
#endif

int
next_party(void)
{
    int n;

    n = cur_level;
    while (n % PARTY_TIME) {
        n++;
    }
    return (get_rand((n + 1), (n + PARTY_TIME)));
}

/* MZ-700/1500固有 */
void
clear_level_objects(void)
{
    object *obj = level_objects.next_object;
    object *next;

    while (obj) {
        next = obj->next_object;
        free_object(obj);
        obj = next;
    }
    level_objects.next_object = 0;
}

void
reset_object_state(void)
{
    memset(&rogue, 0, sizeof(rogue));
    rogue.hp_current = INIT_HP;
    rogue.hp_max = INIT_HP;
    rogue.str_current = 16;
    rogue.str_max = 16;
    rogue.exp = 1;
    rogue.fchar = DC_AT;
    rogue.moves_left = 1250;
    foods = 0;
}
