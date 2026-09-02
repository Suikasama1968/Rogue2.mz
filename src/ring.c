/*
 * ring.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include "rogue.h"
#include "ring.h"
#include "message.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"

short r_rings, add_strength;

void
put_on_ring(void)
{
    short ch;
    object *ring;

    if (r_rings == 2) {
        message_id_mz(160, 0);
        return;
    }
    ch = (short)pack_letter(0, RING);
    if (ch == CANCEL) return;
    if (!(ring = get_letter_object(ch))) {
        message_id_mz(162, 0);
        return;
    }
    if (ring->what_is != RING) {
        message_id_mz(163, 0);
        return;
    }
    if (ring->in_use_flags & (ON_LEFT_HAND | ON_RIGHT_HAND)) {
        message_id_mz(164, 0);
        return;
    }
    if (r_rings == 1) {
        ch = rogue.left_ring ? 'r' : 'l';
    } else {
        message_id_mz(158, 0);
        do {
            ch = (short)rgetchar();
        } while (ch != CANCEL && ch != 'l' && ch != 'r');
        check_message();
    }
    if (ch != 'l' && ch != 'r') return;
    if ((ch == 'l' && rogue.left_ring) ||
        (ch == 'r' && rogue.right_ring)) {
        message_id_mz(165, 0);
        return;
    }
    do_put_on(ring, (boolean)(ch == 'l'));
    ring_stats(1);
    message_id_mz(403, 0);
    (void)reg_move();
}

void
do_put_on(object *ring, boolean on_left)
{
    if (on_left) {
        ring->in_use_flags |= ON_LEFT_HAND;
        rogue.left_ring = ring;
    } else {
        ring->in_use_flags |= ON_RIGHT_HAND;
        rogue.right_ring = ring;
    }
}

void
remove_ring(void)
{
    short ch;
    object *ring;

    if (!r_rings) {
        inv_rings();
        return;
    }
    if (rogue.left_ring && !rogue.right_ring) ring = rogue.left_ring;
    else if (!rogue.left_ring && rogue.right_ring) ring = rogue.right_ring;
    else {
        message_id_mz(158, 0);
        do {
            ch = (short)rgetchar();
        } while (ch != CANCEL && ch != 'l' && ch != 'r');
        check_message();
        if (ch == CANCEL) return;
        ring = (ch == 'l') ? rogue.left_ring : rogue.right_ring;
    }
    if (!ring) {
        message_id_mz(159, 0);
    } else if (ring->is_cursed) {
        message_id_mz(85, 0);
    } else {
        un_put_on(ring);
        message_id_mz(166, 0);
        (void)reg_move();
    }
}

void
un_put_on(object *ring)
{
    if (ring) {
        if (ring->in_use_flags & ON_LEFT_HAND) {
            ring->in_use_flags &= ~ON_LEFT_HAND;
            rogue.left_ring = 0;
        } else if (ring->in_use_flags & ON_RIGHT_HAND) {
            ring->in_use_flags &= ~ON_RIGHT_HAND;
            rogue.right_ring = 0;
        }
    }
    ring_stats(1);
}

void
gr_ring(object *ring, boolean assign_wk)
{
    ring->what_is = RING;
    if (assign_wk) ring->which_kind = ADD_STRENGTH;
    do {
        ring->hit_enchant = (char)get_rand(-2, 2);
    } while (!ring->hit_enchant);
    ring->is_cursed = (u8)(ring->hit_enchant < 0);
}

void
inv_rings(void)
{
    if (!r_rings) message_id_mz(167, 0);
    else message_id_mz(403, 0);
}

void
ring_stats(boolean pr)
{
    object *ring;

    r_rings = 0;
    add_strength = 0;
    ring = rogue.left_ring;
    if (ring) {
        ++r_rings;
        add_strength += ring->hit_enchant;
    }
    ring = rogue.right_ring;
    if (ring) {
        ++r_rings;
        add_strength += ring->hit_enchant;
    }
    if (pr) print_stats(STAT_STRENGTH);
}
