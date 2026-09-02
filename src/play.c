/*
 * play.c
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
#include "display.h"
#include "invent.h"
#include "init.h"
#include "level.h"
#include "message.h"
#include "move.h"
#include "mz_curses.h"
#include "pack.h"
#include "play.h"
#include "ring.h"
#include "trap.h"
#include "throw.h"
#include "use.h"
#include "zap.h"

extern boolean trap_door;

void
play_level(void)
{
    int key;

    for (;;) {
        if (game_over) return;
        if (trap_door) {
            trap_door = 0;
            return;
        }
        display_dungeon();
        key = rgetchar();
        check_message();

        switch (key) {
        case '.':
            rest(1);
            break;
        case 's':
            search(1, 0);
            break;
        case 'i':
            inventory(&rogue.pack, ALL_OBJECTS);
            break;
        case 'd':
            drop();
            break;
        case 'h':
        case 'j':
        case 'k':
        case 'l':
        case 'y':
        case 'u':
        case 'n':
        case 'b':
            one_move_rogue((short)key, 1);
            break;
        case 'e':
            eat();
            break;
        case 'q':
            quaff();
            break;
        case 'r':
            read_scroll();
            break;
        case 't':
            throw();
            break;
        case 'z':
            zapp();
            break;
        case 'w':
            wield();
            break;
        case 'W':
            wear();
            break;
        case 'T':
            take_off();
            break;
        case 'P':
            put_on_ring();
            break;
        case 'R':
            remove_ring();
            break;
        case '>':
            if (drop_check()) {
                return;
            }
            break;
        case '<':
            if (check_up()) {
                return;
            }
            break;
        case 'Q':
            byebye(0);
            break;
        default:
            break;
        }
    }
}
