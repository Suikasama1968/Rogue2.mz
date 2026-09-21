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
#include "display.h"
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
#include "use.h"
#include "mz_curses.h"
#include "mz_system.h"

short m_moves;
static short move_left_count;
static short heal_exp = -1;
static short heal_n;
static short heal_count;
static boolean heal_alt;
/* unsigned long rogue_turns; 未使用 */
extern short bear_trap;
extern short blind;
extern short halluc;
extern short confused;
extern short haste_self;
extern short levitate;
extern boolean being_held;
extern short e_rings, regeneration, auto_search;
extern boolean r_teleport;

int
one_move_rogue(short dirch, short pickup)
{
    short row, col;
    short r, c;
    short status;
    short length;
    object *obj;
    char *desc = (char *)TEMP_BUFFER_ADDR;

    r = rogue.row;
    c = rogue.col;

    if (confused) {
        dirch = gr_dir();
    }
    get_dir_rc(dirch, &r, &c, 1);
    row = r;
    col = c;

    if (!can_move(rogue.row, rogue.col, row, col)) {
        return MOVE_FAILED;
    }
    obj = monster_at(row, col);
    if ((being_held || bear_trap) && !obj) {
        if (being_held) {
            flushinp();
            message_id(67, 0);
        } else {
            message_id(68, 0);
            reg_move();
        }
        return (MOVE_FAILED);
    }
    if (r_teleport){
        if (rand_percent(R_TELE_PERCENT)) {
            tele();
            return STOPPED_ON_SOMETHING;
        }
    }
    if (obj) {
        rogue_hit(obj, 0);
        reg_move();
        return STOPPED_ON_SOMETHING;
    }
    if (DUNGEON(row, col) == TILE_DOOR) {
        if (cur_room == PASSAGE) {
            cur_room = get_room_number(row, col);
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

    colorize_dungeon(rogue.row, rogue.col);
    rogue.row = row;
    rogue.col = col;
    if (!levitate && trap_at(row, col) != NO_TRAP) trap_player(row, col);
    if (levitate && pickup && object_at(&level_objects, row, col)) {
        reg_move();
        return STOPPED_ON_SOMETHING;
    }
    if (pickup && (obj = pick_up(row, col, &status)) != 0) {
        get_desc(obj, desc, 1);
        length = 0;
        while (desc[length] != '\0') ++length;
        length += get_message(69, (uint8_t *)desc + length,
                              ROGUE_COLUMNS - length);
        if (obj->what_is != GOLD && length < ROGUE_COLUMNS - 4) {
            desc[length++] = (char)DC_L_BLACKET;
            desc[length++] = (char)(DC_A + obj->ichar - 'a');
            desc[length++] = (char)DC_R_BLACKET;
            desc[length] = '\0';
        }
        message((char *)desc, 1);
        if (obj->what_is == GOLD) free_object(obj);
        reg_move();
        return STOPPED_ON_SOMETHING;
    }
    reg_move();
    return MOVED;
}

#if 0 /* MZ-700/1500では未対応 */
void
multiple_move_rogue(int dirch)
{
    short row, col;
    short m;
#if !defined( ORIGINAL )
    short n, i, ch = 0;		/* 未初期化変数の警告除去のため 0 で初期化 */
    char *dir;
#endif /* not ORIGINAL */

    switch (dirch) {
    case '\010':
    case '\012':
    case '\013':
    case '\014':
    case '\031':
    case '\025':
    case '\016':
    case '\002':
#if !defined( ORIGINAL )
	dirch += 96;
	do {
	retry:
	    row = rogue.row;
	    col = rogue.col;
	    m = one_move_rogue(dirch, 1);
	    if (m == STOPPED_ON_SOMETHING || interrupted) {
		break;
	    }
	    if (m != MOVE_FAILED) {
		continue;
	    }
	    if (!pass_go || !bent_passage) {
		break;
	    }
	    for (n = 0, dir = "hjkl", i = 0; i < 4; i++) {
		row = rogue.row;
		col = rogue.col;
		get_dir_rc(dir[i], &row, &col, 1);
		if (is_passable(row, col) && dirch != dir[3 - i]) {
		    n++, ch = dir[i];
		}
	    }
	    if (n == 1) {
		dirch = ch;
		goto retry;
	    }
	    break;
	} while (!next_to_something(row, col));
	break;
#else /* ORIGINAL */
	do {
	    row = rogue.row;
	    col = rogue.col;
	    if (((m = one_move_rogue((dirch + 96), 1)) == MOVE_FAILED) ||
		(m == STOPPED_ON_SOMETHING) || interrupted) {
		break;
	    }
	} while (!next_to_something(row, col));
	break;
#endif /* ORIGINAL */
    case 'H':
    case 'J':
    case 'K':
    case 'L':
    case 'B':
    case 'Y':
    case 'U':
    case 'N':
#if !defined( ORIGINAL )
	dirch += 32;
	for (;;) {
	retry2:
	    m = one_move_rogue(dirch, 1);
	    if (interrupted) {
		break;
	    }
	    if (m == MOVED) {
		continue;
	    }
	    if (m != MOVE_FAILED || !pass_go || !bent_passage) {
		break;
	    }
	    for (n = 0, dir = "hjkl", i = 0; i < 4; i++) {
		row = rogue.row;
		col = rogue.col;
		get_dir_rc(dir[i], &row, &col, 1);
		if (is_passable(row, col) && dirch != dir[3 - i]) {
		    n++, ch = dir[i];
		}
	    }
	    if (n == 1) {
		dirch = ch;
		goto retry2;
	    }
	    break;
	}
	break;
#else /* ORIGINAL */
	while ((!interrupted) && (one_move_rogue((dirch + 32), 1) == MOVED));
	break;
#endif /* ORIGINAL */
    }
}
#endif

int is_passable(int row, int col)
{
    uint8_t tile;

    if (row < MIN_ROW || row > MAX_ROW ||
        col < 0 || col >= ROGUE_COLUMNS) {
        return 0;
    }
    tile = DUNGEON(row, col);
    return tile == TILE_FLOOR || tile == TILE_TUNNEL ||
           tile == TILE_DOOR || tile == TILE_STAIRS || tile == TILE_TRAP;
}

#if 0 /* MZ-700/1500では未対応 */
int
next_to_something(int drow, int dcol)
{
    short i, j, i_end, j_end, row, col;
    short pass_count = 0;
    unsigned short s;

    if (confused) {
	return 1;
    }
    if (blind) {
	return 0;
    }
    i_end = (rogue.row < (ROGUE_LINES - 2)) ? 1 : 0;
    j_end = (rogue.col < (ROGUE_COLUMNS - 1)) ? 1 : 0;

    for (i = ((rogue.row > MIN_ROW) ? -1 : 0); i <= i_end; i++) {
	for (j = ((rogue.col > 0) ? -1 : 0); j <= j_end; j++) {
	    if ((i == 0 && j == 0) ||
		(rogue.row + i == drow && rogue.col + j == dcol)) {
		continue;
	    }
	    row = rogue.row + i;
	    col = rogue.col + j;
	    s = dungeon[row][col];
	    if (s & HIDDEN) {
		continue;
	    }
	    /* If the rogue used to be right, up, left, down,
	     * or right of row, col, and now isn't,
	     * then don't stop */
	    if (s & (MONSTER | OBJECT | STAIRS)) {
		if ((row == drow || col == dcol) &&
		    (!(row == rogue.row || col == rogue.col))) {
		    continue;
		}
		return 1;
	    }
	    if (s & TRAP) {
		if (!(s & HIDDEN)) {
		    if ((row == drow || col == dcol) &&
			(!(row == rogue.row || col == rogue.col))) {
			continue;
		    }
		    return 1;
		}
	    }
	    if (((i - j == 1) || (i - j == -1)) && (s & TUNNEL)) {
		if (++pass_count > 1) {
		    return 1;
		}
	    }
	    if ((s & DOOR) && ((i == 0) || (j == 0))) {
		return 1;
	    }
	}
    }
    return 0;
}
#endif

int
can_move(int row1, int col1, int row2, int col2)
{
    if (!is_passable(row2, col2)) {
        return 0;
    }
    if (row1 != row2 && col1 != col2) {
        if (DUNGEON(row1, col1) == TILE_DOOR ||
            DUNGEON(row2, col2) == TILE_DOOR ||
            DUNGEON(row1, col2) == TILE_ROCK ||
            DUNGEON(row2, col1) == TILE_ROCK) return 0;
    }
    return 1;
}

#if 0 /* MZ-700/1500では未対応 */
void
move_onto(void)
{
    short ch;

    ch = get_direction();
    if (ch != CANCEL) {
	(void) one_move_rogue(ch, 0);
    }
}
#endif

char is_direction(int c)
{
    return c == 'h' || c == 'j' || c == 'k' || c == 'l' ||
           c == 'b' || c == 'y' || c == 'u' || c == 'n' || c == CANCEL;
}

boolean
check_hunger(boolean messages_only)
{
#if defined(DEBUG)
    (void)messages_only;
    return 0;
#else
    short i, n;
    boolean fainted = 0;
    if (rogue.moves_left == HUNGRY) {
        get_message(71, (uint8_t *)hunger_str, sizeof(hunger_str));
        message_id(72, 0);
    }
    if (rogue.moves_left == WEAK) {
        get_message(73, (uint8_t *)hunger_str, sizeof(hunger_str));
        message_id(74, 0);
    }
    if (rogue.moves_left <= FAINT) {
        if (rogue.moves_left == FAINT) {
            get_message(75, (uint8_t *)hunger_str, sizeof(hunger_str));
            message_id(76, 0);
        }
        n = get_rand(0, (FAINT - rogue.moves_left));
        if (n > 0) {
            fainted = 1;
            if (rand_percent(40)) {
                rogue.moves_left++;
            }
            message_id(77, 0);
            for (i = 0; i < n; i++) {
                if (coin_toss()){
                     mv_mons();
                }
            }
        }
    }
    if (messages_only) {
        return fainted;
    }
    if (rogue.moves_left <= STARVE) {
        killed_by(0, STARVATION);
        return fainted;
    }
    switch (e_rings) {
    case -1:
        rogue.moves_left -= move_left_count;
        break;
    case 0:
        rogue.moves_left--;
        break;
    case 1:
        rogue.moves_left--;
        (void)check_hunger(1);
        rogue.moves_left -= move_left_count;
        break;
    case 2:
        rogue.moves_left--;
        (void)check_hunger(1);
        rogue.moves_left--;
        break;
    }
    move_left_count ^= 1;
    return fainted;
#endif
}

boolean
reg_move(void)
{
    boolean fainted = check_hunger(0);

    /* rogue_turns++; 未使用 */
    
    mv_mons();

    if (++m_moves >= 120) {
        m_moves = 0;
        wanderer();
    }
    if (blind){
        if(!(--blind)) {
            unblind();
        }
    }
    if (halluc) {
        if (!(--halluc)) {
            unhallucinate();
        }
    }
    if (confused){
        if(!(--confused)) {
             unconfuse();
        }
    }
    if (bear_trap) {
        bear_trap--;
    }
    if (levitate) {
        if (!(--levitate)) {
            message_id(78, 0);
            if (trap_at(rogue.row, rogue.col) != NO_TRAP) {
                trap_player(rogue.row, rogue.col);
            }
        }
    }
    if (haste_self) {
        if (!(--haste_self)) {
            message_id(79, 0);
        }
    }
    heal();
    if (auto_search) {
        search(auto_search, auto_search);
    }
    return fainted;
}

void
rest(int count)
{
    int i;

    for (i = 0; i < count; i++) {
        (void) reg_move();
    }
}

int
gr_dir(void)
{
    return (*("jklhyubn" + get_rand(1, 8) - 1));
}

void
heal(void)
{
    static const uint8_t turns[] = { 0, 20, 18, 17, 14, 13, 10, 9, 8, 7, 4, 3 };

    if (rogue.hp_current >= rogue.hp_max) {
        heal_count = 0;
        return;
    }
    if (rogue.exp != heal_exp) {
        heal_exp = rogue.exp;
        heal_n = (heal_exp < 1 || heal_exp > 11) ? 2 : turns[heal_exp];
    }
    if (++heal_count >= heal_n) {
        heal_count = 0;
        rogue.hp_current += regeneration + 1;
        if ((heal_alt = !heal_alt) != 0) {
            rogue.hp_current++;
        }
        if (rogue.hp_current > rogue.hp_max) {
            rogue.hp_current = rogue.hp_max;
        }
    }
}

/* MZ-700/1500固有 */
void
reset_move_state(void)
{
    m_moves = 0;
    move_left_count = 0;
    heal_exp = -1;
    heal_count = 0;
    heal_alt = 0;
}
