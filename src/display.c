/*
 * display.c
 * MZ-700/1500用の表示系ラッパー関数を定義する
 */

#include "rogue.h"
#include "display.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "random.h"
#include "mz_curses.h"

extern short blind;
extern short halluc;
extern boolean detect_monster;
extern boolean see_invisible;
extern boolean r_see_invisible;

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
    uint8_t ch = DUNGEON(row, col);
    short pair;

    switch (ch) {
    case TILE_WALL_H:
    case TILE_WALL_V:
    case TILE_HIDDEN_DOOR_H:
    case TILE_HIDDEN_DOOR_V:
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
    mvaddch((uint8_t)row, (uint8_t)col, ch);
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
    uint16_t offset;
    uint16_t player_offset = (uint16_t)rogue.row * ROGUE_COLUMNS + rogue.col;
    uint8_t object_char;
    uint8_t tile;
    uint8_t attr = dungeon_attr[player_offset];

    /* MZ-700/1500固有: 文字面と属性面で同じオフセットを再利用する。 */
    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        offset = (uint16_t)obj->row * ROGUE_COLUMNS + obj->col;
        if ((obj->picked_up & OBJECT_DETECTED) ||
            dungeon_attr[offset] != ATTR_HIDDEN) {
            attrset(COLOR_PAIR(PAIR_OBJECT));
            obj->trail_char = dungeon[offset];
            if (dungeon_attr[offset] == ATTR_HIDDEN) {
                obj->picked_up |= OBJECT_WAS_HIDDEN;
            } else {
                obj->picked_up &= ~OBJECT_WAS_HIDDEN;
            }
            if (halluc) {
                object_char = (uint8_t)gr_obj_char();
            } else switch (obj->what_is) {
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
            mvaddch((uint8_t)obj->row, (uint8_t)obj->col, object_char);
        }
    }
    for (obj = level_monsters.next_monster; obj; obj = obj->next_monster) {
        if (!blind && (detect_monster || rogue_can_see(obj->row, obj->col)) &&
            (!(obj->m_flags & INVISIBLE) || detect_monster || see_invisible ||
             r_see_invisible)) {
            attrset(COLOR_PAIR((obj->m_flags & IMITATES) ?
                               PAIR_OBJECT : PAIR_MONSTER));
            offset = (uint16_t)obj->row * ROGUE_COLUMNS + obj->col;
            obj->trail_char = dungeon[offset];
            obj->trail_attr = dungeon_attr[offset];
            mvaddch((uint8_t)obj->row, (uint8_t)obj->col,
                    halluc ? (uint8_t)(DC_A + get_rand(0, MONSTERS - 1)) :
                    ((obj->m_flags & IMITATES) ?
                     (uint8_t)obj->disguise : obj->m_char));
        }
    }
    attrset(COLOR_PAIR(PAIR_PLAYER));
    tile = dungeon[player_offset];
    mvaddch((uint8_t)rogue.row, (uint8_t)rogue.col, (uint16_t)rogue.fchar);
    move((uint8_t)rogue.row, (uint8_t)rogue.col);
    refresh();
    dungeon[player_offset] = tile;
    dungeon_attr[player_offset] = attr;
    for (obj = level_monsters.next_monster; obj; obj = obj->next_monster) {
        if (!blind && (detect_monster || rogue_can_see(obj->row, obj->col)) &&
            (!(obj->m_flags & INVISIBLE) || detect_monster || see_invisible ||
             r_see_invisible)) {
            offset = (uint16_t)obj->row * ROGUE_COLUMNS + obj->col;
            dungeon[offset] = obj->trail_char;
            dungeon_attr[offset] = obj->trail_attr;
        }
    }
    for (obj = level_objects.next_object; obj; obj = obj->next_object) {
        offset = (uint16_t)obj->row * ROGUE_COLUMNS + obj->col;
        if ((obj->picked_up & OBJECT_DETECTED) ||
            dungeon_attr[offset] != ATTR_HIDDEN) {
            dungeon[offset] = obj->trail_char;
            if (obj->picked_up & OBJECT_WAS_HIDDEN) {
                dungeon_attr[offset] = ATTR_HIDDEN;
                obj->picked_up &= ~OBJECT_WAS_HIDDEN;
            } else {
                colorize_dungeon(obj->row, obj->col);
            }
        }
    }
    attrset(COLOR_PAIR(PAIR_NORMAL));
}

/* MZ-700/1500固有: 探索と地図の巻物で隠し地形を通常の地形へ戻す。 */
void reveal_hidden_tile(uint8_t *tile)
{
    if (*tile >= TILE_HIDDEN_DOOR_H) {
        *tile = (*tile == TILE_HIDDEN_TUNNEL) ? TILE_TUNNEL : TILE_DOOR;
    }
}
