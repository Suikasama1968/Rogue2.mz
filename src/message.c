/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include <stdio.h>
#include <string.h>

#include "rogue.h"
#include "message.h"
#include "move.h"
#include "mz_curses.h"
#include "mz_system.h"

#define MESSAGE_BUFFER_SIZE 80

static u8 message_buffer[MESSAGE_BUFFER_SIZE];
static boolean msg_cleared = 1;
char hunger_str[8] = "";
extern short add_strength;

static const u8 *find_message(short msg_id, u8 *length)
{
    const u8 *base = (const u8 *)MESG_ADDR;
    const u8 *entry = base + MESSAGE_HEADER_SIZE;
    unsigned short data_offset;
    unsigned short offset;
    short id;
    u8 i;

    data_offset = (unsigned short)base[6] | ((unsigned short)base[7] << 8);
    for (i = 0; i < base[4]; ++i, entry += MESSAGE_ENTRY_SIZE) {
        id = (short)((unsigned short)entry[0] |
                     ((unsigned short)entry[1] << 8));
        if (id == msg_id) {
            offset = (unsigned short)entry[2] |
                     ((unsigned short)entry[3] << 8);
            *length = entry[4];
            return base + data_offset + offset;
        }
    }
    return 0;
}

void print_stats(int stat_mask)
{
    char line1[41];
    char line2[41];
    u8 *status1 = dungeon + ROGUE_COLUMNS * STATUS_ROW_1;
    u8 *status2 = dungeon + ROGUE_COLUMNS * STATUS_ROW_2;
    u8 *attr1 = dungeon_attr + ROGUE_COLUMNS * STATUS_ROW_1;
    u8 *attr2 = dungeon_attr + ROGUE_COLUMNS * STATUS_ROW_2;

    /* 40列版では2行を一体で整形するため、指定項目を含む全体を再描画する。 */
    (void)stat_mask;
    memset(status1, TILE_ROCK, ROGUE_COLUMNS);
    memset(status2, TILE_ROCK, ROGUE_COLUMNS);
    memset(attr1, ATTR_VISIBLE, ROGUE_COLUMNS);
    memset(attr2, ATTR_VISIBLE, ROGUE_COLUMNS);
    sprintf(line1, "Level:%u Gold:%ld Hp:%d(%d) ", cur_level,
            rogue.gold, rogue.hp_current, rogue.hp_max);
    sprintf(line2, "Str:%d(%d) Arm:%d Exp:%d/%ld",
            rogue.str_current + add_strength, rogue.str_max, rogue.armor_class,
            rogue.exp, rogue.exp_points);
    mvaddstr(STATUS_ROW_1, 0, (const u8 *)line1);
    addstr_mz((const u8 *)hunger_str);
    mvaddstr(STATUS_ROW_2, 0, (const u8 *)line2);
}

short get_message(short msg_id, u8 *buffer, short size)
{
    const u8 *src;
    u8 stored_length;
    short length;
    short i;

    if (size <= 0) return 0;
    src = find_message(msg_id, &stored_length);
    if (!src) return 0;
    length = stored_length;
    if (length >= size) length = size - 1;
    for (i = 0; i < length; ++i) {
        buffer[i] = src[i];
    }
    buffer[length] = '\0';
    return length;
}

void message_mz(const u8 *msg, boolean intrpt)
{
    u8 length = 0;
    const u8 *p = msg;

    (void)intrpt;
    move(MESSAGE_ROW, 0);
    addstr_mz(msg);
    while (*p != '\0') {
        if (*p != MZ_STR_CSET_0 && *p != MZ_STR_CSET_1) ++length;
        ++p;
    }
    while (length++ < ROGUE_COLUMNS) addch(DC_SPC);
    msg_cleared = 0;
}

void message(char *msg, boolean intrpt)
{
    (void)msg;
    (void)intrpt;
    /* TODO: compressed message table and message-line rendering. */
}

void message_id_mz(short msg_id, const u8 *text)
{
    const u8 *src;
    u8 src_left;
    u8 length = 0;

    src = find_message(msg_id, &src_left);
    if (!src) return;
    while (src_left-- && length < MESSAGE_BUFFER_SIZE - 1) {
        u8 ch = *src++;
        if (ch == MESSAGE_FORMAT_STRING) {
            while (text && *text && length < MESSAGE_BUFFER_SIZE - 1) {
                message_buffer[length++] = *text++;
            }
        } else {
            message_buffer[length++] = ch;
        }
    }
    message_buffer[length] = '\0';
    message_mz(message_buffer, 0);
}

void remessage(void)
{
    /* TODO: message history. */
}

void check_message(void)
{
    u8 col;

    if (msg_cleared) return;
    move(MESSAGE_ROW, 0);
    for (col = 0; col < ROGUE_COLUMNS; ++col) addch(DC_SPC);
    msg_cleared = 1;
}

int get_direction(void)
{
    int dir;

    message_id_mz(55, 0);
    do {
        dir = rgetchar();
    } while (!is_direction(dir));
    check_message();
    return dir;
}

int rgetchar(void)
{
    return getch();
}
