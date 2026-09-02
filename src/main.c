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

#include <string.h>

#include "rogue.h"
#include "main.h"
#include "init.h"
#include "level.h"
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
            message_id_mz(10, 0);
        }
        first = 0;
        play_level();
        clear_level_objects();  // free_stuff(&level_objects);
        clear_level_monsters(); // free_stuff(&level_monsters);
        if (game_over) {
            break;
        }
    }

    BANK_ROM();
    return 0;
}

int
read_mesg(char *argv_msgfile)
{
    const u8 *message;
    const u8 *entry;
    unsigned short data_offset;
    unsigned short file_size;
    unsigned short offset;
    unsigned short payload_size;
    u8 i;

    /* 9Z-502Mが見える状態で、仮想VRAMをバッファとしてメッセージを読み込む */
    if (QD_File_Read((u8 *)argv_msgfile, (u8 *)MESG_LOAD_ADDR,
                     MESG_LOAD_SIZE)) return 1;
    file_size = *(unsigned short *)QD_FILE_SIZE;

    /* バンク切替して、メッセージを0xd000へ格納する */
    BANK_DRAM_H();
    memcpy((u8 *)MESG_ADDR, (const u8 *)MESG_LOAD_ADDR, file_size);
    message = (const u8 *)MESG_ADDR;

    if (message[0] != MESSAGE_MAGIC_0 || message[1] != MESSAGE_MAGIC_1 ||
        message[2] != MESSAGE_MAGIC_2 || message[3] != MESSAGE_MAGIC_3 ||
        message[5] != MESSAGE_VERSION) return 1;
    data_offset = (unsigned short)message[6] |
                  ((unsigned short)message[7] << 8);
    if (data_offset != MESSAGE_HEADER_SIZE +
                       (unsigned short)message[4] * MESSAGE_ENTRY_SIZE ||
        data_offset >= file_size) return 1;
    payload_size = file_size - data_offset;
    entry = message + MESSAGE_HEADER_SIZE;
    for (i = 0; i < message[4]; ++i, entry += MESSAGE_ENTRY_SIZE) {
        offset = (unsigned short)entry[2] |
                 ((unsigned short)entry[3] << 8);
        if (entry[4] >= payload_size ||
            offset > payload_size - entry[4] - 1) return 1;
    }
    return 0;
}
