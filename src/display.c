/*
 * display.c
 * MZ-1500用の表示系ラッパー関数を定義する
 */

#include "rogue.h"
#include "display.h"
#include "message.h"
#include "mz_curses.h"
#include "monster.h"
#include "object.h"

/*
 * init_color_attr
 * カラー属性配列の初期化
 */
void init_color_attr(void)
{
    init_pair(PAIR_NORMAL,  COLOR_WHITE,  COLOR_BLACK);
    init_pair(PAIR_TERRAIN, COLOR_CYAN,   COLOR_BLACK);
    init_pair(PAIR_FLOOR,   COLOR_BLUE,   COLOR_BLACK);
    init_pair(PAIR_MONSTER, COLOR_MAGENTA,COLOR_BLACK);
    init_pair(PAIR_OBJECT,  COLOR_YELLOW, COLOR_BLACK);
    init_pair(PAIR_PLAYER,  COLOR_GREEN,  COLOR_BLACK);
    attrset(COLOR_PAIR(PAIR_NORMAL));
}

/*
 * colorize_dungeon
 * ダンジョンの各マスに色をつける
 */
void colorize_dungeon(short row, short col)
{
    u8 ch = DUNGEON(row, col);
    short pair;

    if (ch == TILE_WALL_H || ch == TILE_WALL_V ||
        ch == TILE_TUNNEL || ch == TILE_DOOR) pair = PAIR_TERRAIN;
    else if (ch == TILE_FLOOR) pair = PAIR_FLOOR;
    else if (ch == TILE_STAIRS || ch == TILE_TRAP) pair = PAIR_OBJECT;
    else pair = PAIR_NORMAL;
    attrset(COLOR_PAIR(pair));
    mvaddch((u8)row, (u8)col, ch);
}


/*
 * 分散している描画を一括してダンジョンを表示する
 */
void display_dungeon(void)
{
    print_stats(STAT_ALL);
    refresh_dungeon();
}

/*
 * オブジェクト、モンスター、プレイヤーを重ねて実画面へ転送する
 */
void refresh_dungeon(void)
{
    object *obj;
    u8 tile = DUNGEON(rogue.row, rogue.col);
    u8 attr = DUNGEON_ATTR(rogue.row, rogue.col);

    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN) {
            attrset(COLOR_PAIR(PAIR_OBJECT));
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
            mvaddch((u8)obj->row, (u8)obj->col,
                    DUNGEON(obj->row, obj->col));
        }
    }
    for (obj = level_monsters.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN) {
            attrset(COLOR_PAIR(PAIR_MONSTER));
            obj->trail_char = DUNGEON(obj->row, obj->col);
            mvaddch((u8)obj->row, (u8)obj->col, obj->m_char);
        }
    }
    attrset(COLOR_PAIR(PAIR_PLAYER));
    tile = DUNGEON(rogue.row, rogue.col);
    mvaddch((u8)rogue.row, (u8)rogue.col, (u16)rogue.fchar);
    move((u8)rogue.row, (u8)rogue.col);
    refresh();
    DUNGEON(rogue.row, rogue.col) = tile;
    DUNGEON_ATTR(rogue.row, rogue.col) = attr;
    for (obj = level_monsters.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN) {
            DUNGEON(obj->row, obj->col) = obj->trail_char;
            colorize_dungeon(obj->row, obj->col);
        }
    }
    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        if (DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN) {
            DUNGEON(obj->row, obj->col) = obj->trail_char;
            colorize_dungeon(obj->row, obj->col);
        }
    }
    attrset(COLOR_PAIR(PAIR_NORMAL));
}
