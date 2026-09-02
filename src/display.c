/*
 * display.c
 *
 * MZ-1500用の表示系ラッパー関数を定義する
 */

#include "rogue.h"
#include "display.h"
#include "message.h"
#include "mz_curses.h"
#include "monster.h"
#include "object.h"

/*
 * 分散している描画を一括してダンジョンを表示する
 */
void display_dungeon(void)
{
    object *obj;
    u8 tile = DUNGEON(rogue.row, rogue.col);
    u8 attr = DUNGEON_ATTR(rogue.row, rogue.col);

    print_stats(STAT_ALL);
    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) == ATTR_VISIBLE) {
            obj->trail_char = DUNGEON(obj->row, obj->col);
            if (obj->what_is == GOLD) DUNGEON(obj->row, obj->col) = DC_STAR;
            else if (obj->what_is == AMULET) DUNGEON(obj->row, obj->col) = DC_COMMA;
            else if (obj->what_is == POTION) DUNGEON(obj->row, obj->col) = DC_EXCLAM;
            else if (obj->what_is == SCROL) DUNGEON(obj->row, obj->col) = DC_QUESTION;
            else if (obj->what_is == ARMOR) DUNGEON(obj->row, obj->col) = DC_R_SQ_BLACKET;
            else if (obj->what_is == WEAPON) DUNGEON(obj->row, obj->col) = DC_R_BLACKET;
            else if (obj->what_is == WAND) DUNGEON(obj->row, obj->col) = DC_SLASH;
            else if (obj->what_is == RING) DUNGEON(obj->row, obj->col) = DC_EQUAL;
            else DUNGEON(obj->row, obj->col) = DC_COLON;
        }
    }
    for (obj = level_monsters.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) == ATTR_VISIBLE) {
            obj->trail_char = DUNGEON(obj->row, obj->col);
            mvaddch((u8)obj->row, (u8)obj->col, obj->m_char);
        }
    }
    tile = DUNGEON(rogue.row, rogue.col);
    mvaddch((u8)rogue.row, (u8)rogue.col, (u16)rogue.fchar);
    move((u8)rogue.row, (u8)rogue.col);
    refresh();
    DUNGEON(rogue.row, rogue.col) = tile;
    DUNGEON_ATTR(rogue.row, rogue.col) = attr;
    for (obj = level_monsters.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) == ATTR_VISIBLE) {
            DUNGEON(obj->row, obj->col) = obj->trail_char;
        }
    }
    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) == ATTR_VISIBLE) {
            DUNGEON(obj->row, obj->col) = obj->trail_char;
        }
    }
}
