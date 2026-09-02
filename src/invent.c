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
#include "invent.h"
#include "message.h"
#include "mz_curses.h"
#include "mz_system.h"
#include "object.h"
#include "pack.h"

#define INVENTORY_PAGE_ROWS 8
#define INVENTORY_SAVE_ROWS (INVENTORY_PAGE_ROWS + 1)

#define descs_text ((u8 (*)[40])DESCS_TEXT_ADDR)
#define descs_attr ((u8 (*)[40])DESCS_ATTR_ADDR)

typedef char descs_text_size_check[
    INVENTORY_SAVE_ROWS * 40 <= DESCS_TEXT_SIZE ? 1 : -1];
typedef char descs_attr_size_check[
    INVENTORY_SAVE_ROWS * 40 <= DESCS_ATTR_SIZE ? 1 : -1];

static short mz_number(u8 *buffer, unsigned short number);
static short append_message(u8 *buffer, short length, short msg_id);
static u8 inventory_col(void);
static void save_inventory_rows(u8 col, u8 rows);
static void restore_inventory_rows(u8 col, u8 rows);

void inventory(object *pack, unsigned short mask)
{
    object *obj = pack->next_object;
    u8 col;

    while (obj && !(obj->what_is & mask)) obj = obj->next_object;
    if (!obj) {
        message_id_mz(26, 0);
        return;
    }
    col = inventory_col();
    while (obj) {
        object *next = obj;
        u8 rows = 0;
        u8 line[40];
        u8 row;

        while (next && rows < INVENTORY_PAGE_ROWS) {
            if (next->what_is & mask) ++rows;
            next = next->next_object;
        }
        save_inventory_rows(col, (u8)(rows + 1));
        row = 0;
        while (obj && row < rows) {
            if (obj->what_is & mask) {
                memset(line, 0, sizeof(line));
                line[0] = (u8)(DC_A + obj->ichar - 'a');
                line[1] = DC_R_BLACKET;
                line[2] = DC_SPC;
                get_desc(obj, (char *)(line + 3), 0);
                memset(dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
                       DC_SPC, 40);
                memset(dungeon_attr +
                       (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
                       0x70, 40);
                mvaddstr_mz((u8)(row + 1), col, line);
                ++row;
            }
            obj = obj->next_object;
        }
        memset(dungeon + (unsigned int)(rows + 1) * ROGUE_COLUMNS + col,
               DC_SPC, 40);
        memset(dungeon_attr +
               (unsigned int)(rows + 1) * ROGUE_COLUMNS + col, 0x70, 40);

        mvaddstr((u8)(rows + 1), col, (const u8 *)" --Push Space--");
        move((u8)rogue.row, (u8)rogue.col);
        refresh();
        while (rgetchar() != ' ') {}
        restore_inventory_rows(col, (u8)(rows + 1));
        move((u8)rogue.row, (u8)rogue.col);
        refresh();
        while (obj && !(obj->what_is & mask)) obj = obj->next_object;
    }
}

void get_desc(object *obj, char *desc, boolean capitalized)
{
    u8 *buffer = (u8 *)desc;
    short length;

    (void)capitalized;
    if (obj->what_is == AMULET) {
        get_message(27, buffer, ROGUE_COLUMNS);
        return;
    }
    length = mz_number(buffer, (unsigned short)obj->quantity);
    
    if (obj->what_is == GOLD) {
        append_message(buffer, length, 28);
        return;
    }
    
    if (obj->what_is == FOOD) {
        if (obj->which_kind == RATION) {
            length = append_message(buffer, length, 30);
            append_message(buffer, length, 2);
        } else {
            length = append_message(buffer, length, 31);
            append_message(buffer, length, 333);
        }
        return;
    }
    if (obj->what_is == WEAPON) {
        if (obj->quantity > 1) {
            length = append_message(buffer, length, 29);
        } else {
            length = 0;
            buffer[0] = '\0';
        }
        length = append_message(buffer, length,
                                (short)(374 + obj->which_kind));
        if (obj->in_use_flags & BEING_WIELDED) {
            append_message(buffer, length, 35);
        }
        return;
    }
    if (obj->what_is == ARMOR) {
        length = get_message((short)(382 + obj->which_kind), buffer,
                             ROGUE_COLUMNS);
        if (obj->in_use_flags & BEING_WORN) {
            append_message(buffer, length, 36);
        }
        return;
    }
    if (obj->what_is == POTION) {
        if (obj->quantity > 1) {
            length = append_message(buffer, length, 32);
        } else {
            length = 0;
            buffer[0] = '\0';
        }
        length = append_message(buffer, length,
                                (short)(334 + obj->which_kind));
        append_message(buffer, length, 4);
        return;
    }
    if (obj->what_is == SCROL) {
        if (obj->quantity > 1) length = append_message(buffer, length, 32);
        else { length = 0; buffer[0] = '\0'; }
        append_message(buffer, length, 410);
        return;
    }
    if (obj->what_is == WAND) {
        get_message((short)(389 + obj->which_kind), buffer, ROGUE_COLUMNS);
        return;
    }
    if (obj->what_is == RING) {
        get_message((short)(399 + obj->which_kind), buffer, ROGUE_COLUMNS);
        return;
    }
    buffer[0] = '\0';
}

void single_inv(short ichar)
{
    object *obj;
    char desc[ROGUE_COLUMNS];

    if (!(obj = get_letter_object(ichar))) return;
    get_desc(obj, desc, 1);
    message_mz((const u8 *)desc, 0);
}

/* MZ-1500固有の表示・文字列処理。 */
static short mz_number(u8 *buffer, unsigned short number)
{
    u8 digits[6];
    short count = 0;
    short i;

    do {
        digits[count++] = (u8)(DC_0 + number % 10);
        number /= 10;
    } while (number && count < (short)sizeof(digits));
    for (i = 0; i < count; ++i) buffer[i] = digits[count - i - 1];
    buffer[count] = '\0';
    return count;
}

static short append_message(u8 *buffer, short length, short msg_id)
{
    return length + get_message(msg_id, buffer + length,
                                ROGUE_COLUMNS - length);
}

static u8 inventory_col(void)
{
    if (rogue.col < 30) return 0;
    if (rogue.col < 50) return 20;
    return 40;
}

static void save_inventory_rows(u8 col, u8 rows)
{
    u8 row;

    for (row = 0; row < rows; ++row) {
        memcpy(descs_text[row],
               dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col, 40);
        memcpy(descs_attr[row],
               dungeon_attr + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               40);
    }
}

static void restore_inventory_rows(u8 col, u8 rows)
{
    u8 row;

    for (row = 0; row < rows; ++row) {
        memcpy(dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               descs_text[row], 40);
        memcpy(dungeon_attr + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               descs_attr[row], 40);
    }
}
