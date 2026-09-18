/*
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
#include "display.h"
#include "message.h"
#include "move.h"
#include "object.h"
#include "mz_curses.h"
#include "mz_display.h"
#include "mz_system.h"

#define message_buffer ((u8 *)MESSAGE_BUFFER_ADDR)
static boolean msg_cleared = 1;
static u8 msg_col;
char hunger_str[8] = "";
extern short add_strength;

void
message(char *msg, boolean intrpt)
{
    const u8 *p;
    u8 length;
    u8 more_col;
    u8 more_length;

    (void)intrpt;
    attrset(A_NORMAL);
    if (!msg_cleared) {
        p = find_message(11, &length);
        more_length = mz_display_length(p);
        more_col = msg_col;
        if (more_col + more_length > V_COLUMN / 2) {
            more_col = (u8)(V_COLUMN / 2 - more_length);
        }
        move(MESSAGE_ROW, more_col);
        addnstr(p, length);
        refresh_dungeon();
        flushinp();
        (void)rgetchar();
        flushinp();
        check_message();
    }
    move(MESSAGE_ROW, 0);
    addstr((const u8 *)msg);
    clrtoeol();
    msg_cleared = 0;
    msg_col = mz_display_length((const u8 *)msg);
    refresh_dungeon();
}

void
check_message(void)
{
	if (msg_cleared) {
		return;
	}
    move(MESSAGE_ROW, 0);
    clrtoeol();
    msg_cleared = 1;
    refresh_dungeon();
}

int get_direction(void)
{
    int dir;

    message_id(55, 0);
    while (!is_direction(dir = rgetchar())) {
    /* sound_bell() */
    }
    flushinp();
    check_message();
    return dir;
}

int
rgetchar(void)
{
    return getch();
}

void
print_stats(int stat_mask)
{
    u8 *line1 = (u8 *)TEMP_BUFFER_ADDR;
    u8 *line2 = line1 + 48;
    u8 *status1 = dungeon + ROGUE_COLUMNS * STATUS_ROW_1;
    u8 *status2 = dungeon + ROGUE_COLUMNS * STATUS_ROW_2;
    u8 *attr1 = dungeon_attr + ROGUE_COLUMNS * STATUS_ROW_1;
    u8 *attr2 = dungeon_attr + ROGUE_COLUMNS * STATUS_ROW_2;
    long *values = (long *)(line2 + 48);

    /* 40列版では2行を一体で整形するため、指定項目を含む全体を再描画する。 */
    (void)stat_mask;
    attrset(A_NORMAL);
    memset(status1, TILE_ROCK, ROGUE_COLUMNS);
    memset(status2, TILE_ROCK, ROGUE_COLUMNS);
    memset(attr1, ATTR_VISIBLE, ROGUE_COLUMNS);
    memset(attr2, ATTR_VISIBLE, ROGUE_COLUMNS);
    values[0] = cur_level;
    values[1] = rogue.gold;
    values[2] = rogue.hp_current;
    values[3] = rogue.hp_max;
    mz_sprintf(line1, 527, values);
    values[0] = rogue.str_current + add_strength;
    values[1] = rogue.str_max;
    values[2] = get_armor_class(rogue.armor);
    values[3] = rogue.exp;
    values[4] = rogue.exp_points;
    mz_sprintf(line2, 528, values);
    mvaddstr(STATUS_ROW_1, 0, line1);
    addstr((const u8 *)hunger_str);
    mvaddstr(STATUS_ROW_2, 0, line2);
}

/* MZ-1500固有の処理 */
const u8 *find_message(short msg_id, u8 *length)
{
    const u8 *base = (const u8 *)MESG_ADDR;
    const u8 *entry = base;
    const u8 *src;
    unsigned short offset;
    unsigned short id;
    u8 n;

    while (1) {
        id = (unsigned short)entry[0] |
             ((unsigned short)entry[1] << 8);
        if (id == MESSAGE_END_ID) return 0;
        if (id == (unsigned short)msg_id) {
            offset = (unsigned short)entry[2] |
                     ((unsigned short)entry[3] << 8);
            src = base + offset;
            n = 0;
            while (src[n]) n++;
            *length = n;
            return src;
        }
        entry += MESSAGE_ENTRY_SIZE;
    }
}

short get_message(short msg_id, u8 *buffer, short size)
{
    const u8 *src;
    u8 stored_length;
    short length;

    src = find_message(msg_id, &stored_length);
    if (!src) return 0;
    length = stored_length;
    if (length >= size) length = size - 1;
    memcpy(buffer, src, length);
    buffer[length] = '\0';
    return length;
}

void message_id(short msg_id, const u8 *text)
{
    if (format_message(msg_id, text, message_buffer,
                       MESSAGE_BUFFER_SIZE) >= 0) {
        message((char *)message_buffer, 0);
    }
}

short
format_message(short msg_id, const u8 *text, u8 *buffer, short size)
{
    const u8 *src;
    u8 src_left;
    short length = 0;

    src = find_message(msg_id, &src_left);
    if (!src || size <= 0) return -1;
    while (src_left-- && length < size - 1) {
        u8 ch = *src++;
        if (ch == MESSAGE_FORMAT_STRING) {
            while (text && *text && length < size - 1) {
                buffer[length++] = *text++;
            }
        } else {
            buffer[length++] = ch;
        }
    }
    buffer[length] = '\0';
    return length;
}
