/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include "rogue.h"
#include "hit.h"
#include "invent.h"
#include "message.h"
#include "monster.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "score.h"
#include "trap.h"

short m_moves;
unsigned long rogue_turns;
extern short bear_trap;

int is_passable(int row, int col)
{
    u8 tile;

    if (row < MIN_ROW || row > MAX_ROW ||
        col < 0 || col >= ROGUE_COLUMNS) return 0;
    tile = DUNGEON(row, col);
    return tile == TILE_FLOOR || tile == TILE_TUNNEL ||
           tile == TILE_DOOR || tile == TILE_STAIRS || tile == TILE_TRAP;
}

int can_move(int row1, int col1, int row2, int col2)
{
    if (!is_passable(row2, col2)) return 0;
    if (row1 != row2 && col1 != col2) {
        if (DUNGEON(row1, col1) == TILE_DOOR ||
            DUNGEON(row2, col2) == TILE_DOOR ||
            DUNGEON(row1, col2) == TILE_ROCK ||
            DUNGEON(row2, col1) == TILE_ROCK) return 0;
    }
    return 1;
}

char is_direction(int c)
{
    return c == 'h' || c == 'j' || c == 'k' || c == 'l' ||
           c == 'b' || c == 'y' || c == 'u' || c == 'n' || c == CANCEL;
}

int one_move_rogue(short dirch, short pickup)
{
    short row = rogue.row;
    short col = rogue.col;
    short status;
    short length;
    object *obj;
    char desc[ROGUE_COLUMNS];

    if (bear_trap) {
        reg_move();
        return MOVE_FAILED;
    }

    get_dir_rc(dirch, &row, &col, 1);

    if (!can_move(rogue.row, rogue.col, row, col)) return MOVE_FAILED;
    obj = monster_at(row, col);
    if (obj) {
        rogue_hit(obj, 0);
        reg_move();
        return STOPPED_ON_SOMETHING;
    }
    if (DUNGEON(row, col) == TILE_DOOR) {
        if (cur_room == PASSAGE) {
            cur_room = (short)get_room_number(row, col);
            if (cur_room >= 0 && (rooms[cur_room].is_room & R_MAZE)) {
                light_passage(row, col);
                cur_room = PASSAGE;
            } else {
                light_up_room(cur_room);
                wake_room(cur_room, 1, row, col);
            }
        } else {
            light_passage(row, col);
        }
    } else if (DUNGEON(rogue.row, rogue.col) == TILE_DOOR &&
               DUNGEON(row, col) == TILE_TUNNEL) {
        light_passage(row, col);
        wake_room(cur_room, 0, rogue.row, rogue.col);
        darken_room(cur_room);
        cur_room = PASSAGE;
    } else if (DUNGEON(row, col) == TILE_TUNNEL) {
        light_passage(row, col);
    }

    rogue.row = row;
    rogue.col = col;
    if (trap_at(row, col) != NO_TRAP) trap_player(row, col);
    if (pickup && (obj = pick_up(row, col, &status)) != 0) {
        get_desc(obj, desc, 1);
        length = 0;
        while (desc[length] != '\0') ++length;
        length += get_message(69, (u8 *)desc + length,
                              ROGUE_COLUMNS - length);
        if (obj->what_is != GOLD && length < ROGUE_COLUMNS - 4) {
            desc[length++] = (char)DC_L_BLACKET;
            desc[length++] = (char)(DC_A + obj->ichar - 'a');
            desc[length++] = (char)DC_R_BLACKET;
            desc[length] = '\0';
        }
        message_mz((u8 *)desc, 1);
        if (obj->what_is == GOLD) free_object(obj);
        reg_move();
        return STOPPED_ON_SOMETHING;
    }
    reg_move();
    return MOVED;
}

boolean check_hunger(boolean messages_only)
{
    short i;
    short n;
    boolean fainted = 0;

    if (rogue.moves_left == HUNGRY) {
        get_message(71, (u8 *)hunger_str, sizeof(hunger_str));
        message_id_mz(72, 0);
    }
    if (rogue.moves_left == WEAK) {
        get_message(73, (u8 *)hunger_str, sizeof(hunger_str));
        message_id_mz(74, 0);
    }
    if (rogue.moves_left <= FAINT) {
        if (rogue.moves_left == FAINT) {
            get_message(75, (u8 *)hunger_str, sizeof(hunger_str));
            message_id_mz(76, 0);
        }
        n = (short)get_rand(0, FAINT - rogue.moves_left);
        if (n > 0) {
            fainted = 1;
            if (rand_percent(40)) ++rogue.moves_left;
            message_id_mz(77, 0);
            for (i = 0; i < n && !game_over; ++i) {
                if (coin_toss()) mv_mons();
            }
        }
    }
    if (messages_only) return fainted;
    if (rogue.moves_left <= STARVE) {
        killed_by(0, STARVATION);
        return fainted;
    }
    --rogue.moves_left;
    return fainted;
}

boolean reg_move(void)
{
    boolean fainted = check_hunger(0);

    ++rogue_turns;
    if (bear_trap) --bear_trap;
    if (game_over) return fainted;
    mv_mons();
    if (++m_moves >= 120) {
        m_moves = 0;
        wanderer();
    }
    heal();
    return fainted;
}

void rest(int count)
{
    int i;

    for (i = 0; i < count; ++i) reg_move();
}

void heal(void)
{
    static short heal_exp = -1;
    static short n;
    static short count;
    static boolean alternate;
    static const u8 turns[] = { 0, 20, 18, 17, 14, 13, 10, 9, 8, 7, 4, 3 };

    if (rogue.hp_current >= rogue.hp_max) {
        count = 0;
        return;
    }
    if (rogue.exp != heal_exp) {
        heal_exp = rogue.exp;
        n = (heal_exp < 1 || heal_exp > 11) ? 2 : turns[heal_exp];
    }
    if (++count >= n) {
        count = 0;
        ++rogue.hp_current;
        if ((alternate = !alternate) != 0) ++rogue.hp_current;
        if (rogue.hp_current > rogue.hp_max) rogue.hp_current = rogue.hp_max;
    }
}
