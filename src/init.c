/*
 * init.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "rogue.h"
#include "init.h"
#include "main.h"
#include "machdep.h"
#include "message.h"
#include "display.h"
#include "invent.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"

#include "mz_curses.h"
#include "mz_system.h"

int
init(void)
{
    int seed;
    WINDOW *main_window;

    if (read_mesg("MESG")) {
        return 1;
    }

    /* init memory */
    BANK_DRAM_L(); 
    memset(LOW_RAM_BEGIN, 0x00, LOW_RAM_SIZE);

    memset((void *)OBJECT_POOL_ADDR, 0x00,
           OBJECT_POOL_SIZE + OBJECT_USED_SIZE);

    /* init curses */
    main_window = initscr();
/*  if (main_window == NULL) {
        return 1;
    }
*/ /* MZ版では必ず成功 */
    init_color_attr();

    seed = md_gseed();
    (void) srrandom(seed);
    get_wand_and_ring_materials();
    make_scroll_titles();
    
    level_objects.next_object = 0;
    level_monsters.next_object = 0;
    player_init();
    party_counter = (short)get_rand(1, PARTY_TIME);
    ring_stats(0);
    return 0;
}

void
player_init(void)
{
    object *obj;

    rogue.pack.next_object = 0;
    identified_potions = 0;

    obj = alloc_object();
    get_food(obj, 1);
    (void)add_to_pack(obj, &rogue.pack, 1);

    obj = alloc_object();               /* initial armor */
    obj->what_is = ARMOR;
    obj->which_kind = RINGMAIL;
    obj->is_protected = 0;
    obj->d_enchant = 1;
    (void)add_to_pack(obj, &rogue.pack, 1);
    do_wear(obj);

    obj = alloc_object();               /* initial weapons */
    obj->what_is = WEAPON;
    obj->which_kind = MACE;
    obj->hit_enchant = obj->d_enchant = 1;
    (void)add_to_pack(obj, &rogue.pack, 1);
    do_wield(obj);

    obj = alloc_object();
    obj->what_is = WEAPON;
    obj->which_kind = BOW;
    obj->hit_enchant = 1;
    obj->d_enchant = 0;
    (void)add_to_pack(obj, &rogue.pack, 1);

    obj = alloc_object();
    obj->what_is = WEAPON;
    obj->which_kind = ARROW;
    obj->quantity = (short)get_rand(25, 35);
    obj->hit_enchant = 0;
    obj->d_enchant = 0;
    (void)add_to_pack(obj, &rogue.pack, 1);
}

void
byebye(void)
{
    message_id(12, 0);
    md_exit(0);
}
