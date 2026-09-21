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

#include <setjmp.h>

#include "rogue.h"
#include "main.h"
#include "init.h"
#include "level.h"
#include "machdep.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "play.h"
#include "trap.h"

#include "mz_io.h"
#include "mz_system.h"

static jmp_buf env;

int
main(void)
{
    boolean first;

    if (init()) return 1;

    if (setjmp(env)) {
        init_game();
    }
    first = 1;

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
    }
}

int
read_mesg(char *argv_msgfile)
{
    /* ROMが有効な状態で、仮想VRAMをバッファとしてメッセージを読み込む */
    if (File_Read((uint8_t *)argv_msgfile, (uint8_t *)MESG_LOAD_ADDR,
                     MESG_LOAD_SIZE)) return 1;
                     
    /* 圧縮データをメッセージ・モンスターテーブル領域へ展開する */
    BANK_DRAM_H();
    dzx0_decompress_fastcall((const void *)MESG_LOAD_ADDR,(void *)MESG_ADDR);
    return 0;
}

#if 0 /* MZ-700/1500では未対応 */
void
usage()
{
    fprintf(stderr, "usage: %s message_file [options...] [save_file]\n", progname);
    exit(1);
}
#endif

/* MZ-700/1500固有 */
void
restart_rogue(void)
{
    longjmp(env, 1);
}
