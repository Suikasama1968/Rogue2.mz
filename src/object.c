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

#include "rogue.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"
#include "room.h"
#include "mz_system.h"

#define object_pool ((object *)OBJECT_POOL_ADDR)
#define object_used ((u8 *)OBJECT_USED_ADDR)

typedef char object_pool_size_check[
    sizeof(object) * MAX_OBJECTS <= OBJECT_POOL_SIZE ? 1 : -1];
typedef char object_used_size_check[
    MAX_OBJECTS <= OBJECT_USED_SIZE ? 1 : -1];

static short foods;
short party_counter;
object level_objects;
fighter rogue = {
    0,                          /* gold */
    INIT_HP,                    /* Hp current */
    INIT_HP,                    /* Hp max */
    16, 16,                     /* Str */
    0,                          /* armor class */
    1, 0,                       /* exp, exp_points */
    0, 0,                       /* row, col */
    DC_AT,                      /* char */
    1250,                       /* moves */
    0, 0,                       /* weapon, armor */
    0, 0,                       /* rings */
    {0}                         /* pack */
};

void
put_objects(void)
{
    int i;
    int n = coin_toss() ? get_rand(2, 4) : get_rand(3, 5);
    object *obj;

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
    int rn;
    int tries;
    short row;
    short col;

    for (rn = 0; rn < MAXROOMS; ++rn) {
        if (!room_exists[rn] || !rand_percent(GOLD_PERCENT)) continue;
        for (tries = 0; tries < 50; ++tries) {
            row = (short)get_rand(rooms[rn].top_row + 1, rooms[rn].bottom_row - 1);
            col = (short)get_rand(rooms[rn].left_col + 1, rooms[rn].right_col - 1);
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
    obj->quantity = (short)get_rand(2 * cur_level, 16 * cur_level);
    if (is_maze) obj->quantity += obj->quantity / 2;
    place_at(obj, row, col);
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

object *alloc_object(void)
{
    int i;

    for (i = 0; i < MAX_OBJECTS; ++i) {
        if (!object_used[i]) {
            object_used[i] = 1;
            object_pool[i].next_object = 0;
            object_pool[i].quantity = 1;
            object_pool[i].picked_up = 0;
            object_pool[i].ichar = 0;
            object_pool[i].in_use_flags = 0;
            object_pool[i].hit_enchant = 0;
            object_pool[i].d_enchant = 0;
            object_pool[i].is_cursed = 0;
            object_pool[i].is_protected = 0;
            return &object_pool[i];
        }
    }
    return 0;
}

void free_object(object *obj)
{
    int i;

    for (i = 0; i < MAX_OBJECTS; ++i) {
        if (obj == &object_pool[i]) {
            object_used[i] = 0;
            object_pool[i].next_object = 0;
            return;
        }
    }
}

void clear_level_objects(void)
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


void place_at(object *obj, int row, int col)
{
    obj->row = (short)row;
    obj->col = (short)col;
    add_to_pack(obj, &level_objects, 0);
}

object *
gr_object(void)
{
    object *obj;
    unsigned short what_is;

    if (foods < cur_level / 3) {
        what_is = FOOD;
        ++foods;
    } else {
        what_is = gr_what_is();
    }

    /* 未移植の種類は、本家の抽選比率を変えず今回は配置しない。 */
    if (what_is != FOOD && what_is != POTION && what_is != SCROL &&
        what_is != WAND &&
        what_is != WEAPON && what_is != ARMOR && what_is != RING) return 0;
    obj = alloc_object();
    if (!obj) return 0;
    if (what_is == FOOD) get_food(obj, 0);
    else if (what_is == POTION) gr_potion(obj);
    else if (what_is == SCROL) gr_scroll(obj);
    else if (what_is == WAND) gr_wand(obj);
    else if (what_is == WEAPON) gr_weapon(obj, 1);
    else if (what_is == ARMOR) gr_armor(obj, 1);
    else gr_ring(obj, 1);
    return obj;
}

void get_food(object *obj, boolean force_ration)
{
    obj->what_is = FOOD;
    obj->which_kind = (force_ration || rand_percent(80)) ? RATION : FRUIT;
}

void gr_potion(object *obj)
{
    static const u8 per[] = { 10, 20, 30, 40, 50, 55 };
    short percent = (short)get_rand(1, 55);
    short i;

    obj->what_is = POTION;
    for (i = 0; i < (short)sizeof(per); ++i) {
        if (percent <= per[i]) {
            obj->which_kind = (unsigned short)i;
            return;
        }
    }
}

void gr_scroll(object *obj)
{
    static const u8 per[SCROLS] = {
        5, 11, 16, 21, 36, 44, 51, 56, 65, 74, 80, 85
    };
    short percent = (short)get_rand(0, 85);
    short i;

    obj->what_is = SCROL;
    for (i = 0; i < SCROLS; ++i) {
        if (percent <= per[i]) {
            obj->which_kind = (unsigned short)i;
            return;
        }
    }
}

void gr_weapon(object *obj, int assign_wk)
{
    short kind;
    short percent;
    short blessing;
    short increment;
    short i;

    obj->what_is = WEAPON;
    if (assign_wk) obj->which_kind = (unsigned short)get_rand(0, WEAPONS - 1);
    kind = (short)obj->which_kind;
    if (kind == DART || kind == ARROW || kind == DAGGER || kind == SHURIKEN) {
        obj->quantity = (short)get_rand(3, 15);
    } else {
        obj->quantity = 1;
    }
    obj->hit_enchant = obj->d_enchant = 0;
    obj->is_cursed = 0;
    percent = (short)get_rand(1, 96);
    blessing = (short)get_rand(1, 3);
    if (percent <= 16) increment = 1;
    else if (percent <= 32) {
        increment = -1;
        obj->is_cursed = 1;
    }
    if (percent <= 32) {
        for (i = 0; i < blessing; ++i) {
            if (coin_toss()) obj->hit_enchant += increment;
            else obj->d_enchant += increment;
        }
    }
}

void gr_armor(object *obj, int assign_wk)
{
    short percent;
    short blessing;

    obj->what_is = ARMOR;
    if (assign_wk) obj->which_kind = (unsigned short)get_rand(0, 6);
    obj->quantity = 1;
    obj->is_protected = 0;
    obj->is_cursed = 0;
    obj->d_enchant = 0;
    percent = (short)get_rand(1, 100);
    blessing = (short)get_rand(1, 3);
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
    static const u8 kinds[] = {
        TELE_AWAY, PUT_TO_SLEEP, MAGIC_MISSILE, CANCELLATION, DO_NOTHING
    };

    obj->what_is = WAND;
    obj->which_kind = kinds[get_rand(0, sizeof(kinds) - 1)];
    if (obj->which_kind == MAGIC_MISSILE) {
        obj->hit_enchant = (char)get_rand(6, 12);
    } else {
        obj->hit_enchant = (char)get_rand(3, 6);
    }
}

unsigned short gr_what_is(void)
{
    short percent = (short)get_rand(1, 91);

    if (percent <= 30) return SCROL;
    if (percent <= 60) return POTION;
    if (percent <= 64) return WAND;
    if (percent <= 74) return WEAPON;
    if (percent <= 83) return ARMOR;
    if (percent <= 88) return FOOD;
    return RING;
}


void rand_place(object *obj)
{
    short row;
    short col;

    do {
        gr_row_col(&row, &col, FLOOR | TUNNEL);
    } while (object_at(&level_objects, row, col));
    place_at(obj, row, col);
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

void make_party(void)
{
    party_room = (short)gr_room();
    party_objects(party_room);
}

void put_stairs(void)
{
    short row;
    short col;

    do {
        gr_row_col(&row, &col, FLOOR | TUNNEL);
    } while (object_at(&level_objects, row, col));
    stairs_row = (u8)row;
    stairs_col = (u8)col;
    DUNGEON(row, col) = TILE_STAIRS;
}
