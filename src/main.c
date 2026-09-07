/*
 * main.c
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
#include "main.h"
#include "init.h"
#include "level.h"
#include "machdep.h"
#include "message.h"
#include "monster.h"
#include "mz_curses.h"
#include "mz_io.h"
#include "mz_system.h"
#include "object.h"
#include "play.h"
#include "trap.h"

int
main(int argc, char *argv[])
{
    boolean first = 1;

    if (init(argc, argv)) return 1;

    for (;;) {
        clear_level();
        make_level();
        put_objects();
        put_stairs();
        add_traps();
        put_mons();
        put_player(party_room);
        print_stats(STAT_ALL);
        if (first) {
            message_id(10, 0);
        }
        first = 0;
        play_level();
        clear_level_objects();  // free_stuff(&level_objects);
        clear_level_monsters(); // free_stuff(&level_monsters);
        if (game_over) {
            break;
        }
    }

    md_exit(0);
    return 0;
}

int
read_mesg(char *argv_msgfile)
{
    /* 9Z-502Mが見える状態で、仮想VRAMをバッファとしてメッセージを読み込む */
    if (QD_File_Read((u8 *)argv_msgfile, (u8 *)MESG_LOAD_ADDR,
                     MESG_LOAD_SIZE)) return 1;
                     
    /* 圧縮データをメッセージ・モンスターテーブル領域へ展開する */
    BANK_DRAM_H();
    dzx0_decompress_fastcall((void *)MESG_ADDR,
                            (const void *)MESG_LOAD_ADDR);
    return 0;
}
