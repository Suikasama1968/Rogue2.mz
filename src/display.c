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

extern short blind;
extern boolean detect_monster;

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

    switch (ch) {
    case TILE_WALL_H:
    case TILE_WALL_V:
    case TILE_TUNNEL:
    case TILE_DOOR:
        pair = PAIR_TERRAIN;
        break;
    case TILE_FLOOR:
        pair = PAIR_FLOOR;
        break;
    case TILE_STAIRS:
    case TILE_TRAP:
        pair = PAIR_OBJECT;
        break;
    default:
        pair = PAIR_NORMAL;
        break;
    }
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
    u8 object_char;
    u8 tile = DUNGEON(rogue.row, rogue.col);
    u8 attr = DUNGEON_ATTR(rogue.row, rogue.col);

    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        if ((obj->picked_up & OBJECT_DETECTED) ||
            DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN) {
            attrset(COLOR_PAIR(PAIR_OBJECT));
            obj->trail_char = DUNGEON(obj->row, obj->col);
            if (DUNGEON_ATTR(obj->row, obj->col) == ATTR_HIDDEN) {
                obj->picked_up |= OBJECT_WAS_HIDDEN;
            } else {
                obj->picked_up &= ~OBJECT_WAS_HIDDEN;
            }
            switch (obj->what_is) {
            case GOLD:
                object_char = DC_STAR;
                break;
            case AMULET:
                object_char = DC_COMMA;
                break;
            case POTION:
                object_char = DC_EXCLAM;
                break;
            case SCROL:
                object_char = DC_QUESTION;
                break;
            case ARMOR:
                object_char = DC_R_SQ_BLACKET;
                break;
            case WEAPON:
                object_char = DC_R_BLACKET;
                break;
            case WAND:
                object_char = DC_SLASH;
                break;
            case RING:
                object_char = DC_EQUAL;
                break;
            default:
                object_char = DC_COLON;
                break;
            }
            DUNGEON(obj->row, obj->col) = object_char;
            mvaddch((u8)obj->row, (u8)obj->col,
                    DUNGEON(obj->row, obj->col));
        }
    }
    for (obj = level_monsters.next_object; obj; obj = obj->next_object) {
        if (!blind && (detect_monster ||
                       DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN)) {
            attrset(COLOR_PAIR(PAIR_MONSTER));
            obj->trail_char = DUNGEON(obj->row, obj->col);
            obj->picked_up = DUNGEON_ATTR(obj->row, obj->col);
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
        if (!blind && (detect_monster ||
                       DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN)) {
            DUNGEON(obj->row, obj->col) = obj->trail_char;
            DUNGEON_ATTR(obj->row, obj->col) = (u8)obj->picked_up;
        }
    }
    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        if ((obj->picked_up & OBJECT_DETECTED) ||
            DUNGEON_ATTR(obj->row, obj->col) != ATTR_HIDDEN) {
            DUNGEON(obj->row, obj->col) = obj->trail_char;
            if (obj->picked_up & OBJECT_WAS_HIDDEN) {
                DUNGEON_ATTR(obj->row, obj->col) = ATTR_HIDDEN;
                obj->picked_up &= ~OBJECT_WAS_HIDDEN;
            } else {
                colorize_dungeon(obj->row, obj->col);
            }
        }
    }
    attrset(COLOR_PAIR(PAIR_NORMAL));
}
