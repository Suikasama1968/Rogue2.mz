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
#include "invent.h"
#include "message.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "mz_curses.h"
#include "mz_system.h"

#define INVENTORY_PAGE_ROWS 8
#define INVENTORY_SAVE_ROWS (INVENTORY_PAGE_ROWS + 1)

#define descs_text ((u8 (*)[40])DESCS_TEXT_ADDR)
#define descs_attr ((u8 (*)[40])DESCS_ATTR_ADDR)
#define sc_title ((u8 (*)[34])SCROLL_TITLES_ADDR)

typedef char descs_text_size_check[
    INVENTORY_SAVE_ROWS * 40 <= DESCS_TEXT_SIZE ? 1 : -1];
typedef char descs_attr_size_check[
    INVENTORY_SAVE_ROWS * 40 <= DESCS_ATTR_SIZE ? 1 : -1];
typedef char scroll_titles_size_check[
    SCROLS * 34 <= SCROLL_TITLES_SIZE ? 1 : -1];
typedef char id_scrolls_size_check[
    sizeof(struct id) * SCROLS <= ID_SCROLLS_SIZE ? 1 : -1];
typedef char id_wands_size_check[
    sizeof(struct id) * WANDS <= ID_WANDS_SIZE ? 1 : -1];
typedef char id_rings_size_check[
    sizeof(struct id) * RINGS <= ID_RINGS_SIZE ? 1 : -1];

static short append_message(u8 *buffer, short length, short msg_id);
static short append_text(u8 *buffer, short length, const char *text);
static u8 inventory_col(void);
static void save_inventory_rows(u8 col, u8 rows);
static void restore_inventory_rows(u8 col, u8 rows);

void
inventory(object *pack, unsigned short mask)
{
    object *obj = pack->next_object;
    u8 col;

    while (obj && !(obj->what_is & mask)) obj = obj->next_object;
    if (!obj) {
        message_id(26, 0);
        return;
    }
    col = inventory_col();
    while (obj) {
        object *next = obj;
        u8 rows = 0;
        u8 *desc = (u8 *)TEMP_BUFFER_ADDR;
        u8 row;

        while (next && rows < INVENTORY_PAGE_ROWS) {
            if (next->what_is & mask) ++rows;
            next = next->next_object;
        }
        save_inventory_rows(col, (u8)(rows + 1));
        row = 0;
        while (obj && row < rows) {
            if (obj->what_is & mask) {
                get_desc(obj, (char *)desc, 0);
                memset(dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
                       DC_SPC, 40);
                memset(dungeon_attr +
                       (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
                       0x70, 40);
                DUNGEON(row + 1, col) =
                    (u8)(DC_A + obj->ichar - 'a');
                DUNGEON(row + 1, col + 1) = DC_R_BLACKET;
                DUNGEON(row + 1, col + 2) = DC_SPC;
                DUNGEON_ATTR(row + 1, col) = 0xf0;
                DUNGEON_ATTR(row + 1, col + 1) = 0xf0;
                DUNGEON_ATTR(row + 1, col + 2) = 0xf0;
                mvaddnstr((u8)(row + 1), (u8)(col + 3), desc, 37);
                ++row;
            }
            obj = obj->next_object;
        }
        memset(dungeon + (unsigned int)(rows + 1) * ROGUE_COLUMNS + col,
               DC_SPC, 40);
        memset(dungeon_attr +
               (unsigned int)(rows + 1) * ROGUE_COLUMNS + col, 0x70, 40);

        (void)get_message(518, desc, TEMP_BUFFER_SIZE);
        mvaddnstr((u8)(rows + 1), col, desc, 40);
        move((u8)rogue.row, (u8)rogue.col);
        refresh();
        wait_for_ack();
        restore_inventory_rows(col, (u8)(rows + 1));
        move((u8)rogue.row, (u8)rogue.col);
        refresh_dungeon();
        while (obj && !(obj->what_is & mask)) obj = obj->next_object;
    }
}

void
make_scroll_titles(void)
{
    short i, j, len;
    short sylls, s;
    u8 n, *title;

    for (i = 0; i < SCROLS; i++) {
        sylls = get_rand(2, 5);
        title = sc_title[i];
        *title = DC_L_BRACKET;
        len = 1;
        for (j = 0; j < sylls; j++) {
            s = get_rand(1, MAXSYLLABLES - 1);
            (void)find_message((short)(454 + s), &n);
            if (len + n - 1 >= MAX_TITLE_LENGTH - 2) break;
            (void)get_message((short)(454 + s), title + len,
                              (short)(34 - len));
            len += n;
        }
        title[len - 1] = DC_R_BRACKET;
        title[len] = '\0';
        id_scrolls[i].title = (char *)title;
        id_scrolls[i].real =
            (char *)find_message((short)(362 + i), &n);
        id_scrolls[i].id_status = UNIDENTIFIED;
    }
}

void
get_desc(object *obj, char *desc, boolean capitalized)
{
    u8 *buffer = (u8 *)desc;
    struct id *id;
    short length;

    (void)capitalized;
    length = 0;
    buffer[0] = '\0';
    switch (obj->what_is) {
    case AMULET:
        get_message(27, buffer, ROGUE_COLUMNS);
        break;
    case GOLD:
        length = mz_number(buffer, (unsigned short)obj->quantity);
        append_message(buffer, length, 28);
        break;
    case FOOD:
        length = mz_number(buffer, (unsigned short)obj->quantity);
        if (obj->which_kind == RATION) {
            length = append_message(buffer, length, 30);
            append_message(buffer, length, 2);
        } else {
            length = append_message(buffer, length, 31);
            append_message(buffer, length, 333);
        }
        break;
    case WEAPON:
        if (obj->quantity > 1) {
            length = mz_number(buffer, (unsigned short)obj->quantity);
            length = append_message(buffer, length, 29);
        }
        length = append_message(buffer, length,
                                (short)(374 + obj->which_kind));
        if (obj->in_use_flags & BEING_WIELDED) {
            append_message(buffer, length, 35);
        }
        break;
    case ARMOR:
        length = get_message((short)(382 + obj->which_kind), buffer,
                             ROGUE_COLUMNS);
        if (obj->in_use_flags & BEING_WORN) {
            append_message(buffer, length, 36);
        }
        break;
    case POTION:
        if (obj->quantity > 1) {
            length = mz_number(buffer, (unsigned short)obj->quantity);
            length = append_message(buffer, length, 32);
        }
        length = append_message(buffer, length,
                    (short)(((identified_potions &
                              (unsigned short)(1U << obj->which_kind)) != 0
                             ? 348 : 334) + obj->which_kind));
        append_message(buffer, length, 4);
        break;
    case SCROL:
        if (obj->quantity > 1) {
            length = mz_number(buffer, (unsigned short)obj->quantity);
            length = append_message(buffer, length, 32);
        }
        if (id_scrolls[obj->which_kind].id_status == IDENTIFIED) {
            length = append_text(buffer, length,
                                 id_scrolls[obj->which_kind].real);
        } else {
            length = append_text(buffer, length,
                                 id_scrolls[obj->which_kind].title);
            length = append_message(buffer, length, 33);
        }
        append_message(buffer, length, 3);
        break;
    case WAND:
        id = &id_wands[obj->which_kind];
        goto ID_OBJECT;
    case RING:
        id = &id_rings[obj->which_kind];
ID_OBJECT:
        if (obj->identified || id->id_status == IDENTIFIED) {
            length = append_text(buffer, 0, id->real);
            if (obj->what_is == RING) {
                append_message(buffer, length, 8);
            }
        } else {
            length = append_text(buffer, 0, id->title);
            append_message(buffer, length,
                           (obj->what_is == WAND) ? 5 : 8);
        }
        break;
    default:
        buffer[0] = '\0';
        break;
    }
}

void
get_wand_and_ring_materials(void)
{
    short i, j;
    boolean *used = (boolean *)TEMP_BUFFER_ADDR;
    char **wand_materials = (char **)WAND_MATERIALS_ADDR;
    char **gems = (char **)GEMS_ADDR;

    memset(used, 0, WAND_MATERIALS);
    for (i = 0; i < WANDS; i++) {
        do {
            j = get_rand(0, WAND_MATERIALS - 1);
        } while (used[j]);
        used[j] = 1;
        id_wands[i].title = wand_materials[j];
        id_wands[i].id_status = UNIDENTIFIED;
    }

    memset(used, 0, GEMS);
    for (i = 0; i < RINGS; i++) {
        do {
            j = get_rand(0, GEMS - 1);
        } while (used[j]);
        used[j] = 1;
        id_rings[i].title = gems[j];
        id_rings[i].id_status = UNIDENTIFIED;
    }
}

void
single_inv(short ichar)
{
    object *obj;
    char *desc = (char *)TEMP_BUFFER_ADDR;

    if (!(obj = get_letter_object(ichar))) {
        return;
    }
    get_desc(obj, desc, 1);
    message((char *)desc, 0);
}

/* MZ-1500固有の処理 */
static short append_message(u8 *buffer, short length, short msg_id)
{
    return length + get_message(msg_id, buffer + length,
                                ROGUE_COLUMNS - length);
}

static short append_text(u8 *buffer, short length, const char *text)
{
    while (*text && length < ROGUE_COLUMNS - 1) {
        buffer[length++] = (u8)*text++;
    }
    buffer[length] = '\0';
    return length;
}

static u8 inventory_col(void)
{
    if (rogue.col < 30) return 0;
    if (rogue.col < 50) return 20;
    return 40;
}
/* 持ち物表示場所の保存 */
static void save_inventory_rows(u8 col, u8 rows)
{
    u8 row;

    for (row = 0; row < rows; row++) {
        memcpy(descs_text[row],
               dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col, 40);
        memcpy(descs_attr[row],
               dungeon_attr + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               40);
    }
}
/* 持ち物表示場所の復元 */
static void restore_inventory_rows(u8 col, u8 rows)
{
    u8 row;

    for (row = 0; row < rows; row++) {
        memcpy(dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               descs_text[row], 40);
        memcpy(dungeon_attr + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               descs_attr[row], 40);
    }
}
