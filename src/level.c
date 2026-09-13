/*
 * level.c
 * 
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
#include "level.h"
#include "message.h"
#include "pack.h"
#include "random.h"
#include "room.h"
#include "score.h"
#include "trap.h"
#include "object.h"

#define swap(x,y) {t = x; x = y; y = t;}

extern boolean detect_monster;
extern boolean see_invisible;
extern short levitate;

u8 *dungeon = (u8 *)TEXT_V_VRAM;
u8 *dungeon_attr = (u8 *)TEXT_V_ATTR;
short cur_level = 0;
short max_level = 1;
short cur_room = NO_ROOM;
short party_room = NO_ROOM;
u8 stairs_row;
u8 stairs_col;

/* 再帰呼び出しによるスタック不足対応*/
#define MAZE_TOP       (*(u8 *)(MAZE_BOUNDS_ADDR + 0))
#define MAZE_BOTTOM    (*(u8 *)(MAZE_BOUNDS_ADDR + 1))
#define MAZE_LEFT      (*(u8 *)(MAZE_BOUNDS_ADDR + 2))
#define MAZE_RIGHT     (*(u8 *)(MAZE_BOUNDS_ADDR + 3))
#define MAZE_STACK_ROWS ((u8 *)MAZE_STACK_ADDR)
#define MAZE_STACK_COLS ((u8 *)(MAZE_STACK_ADDR + MAZE_STACK_ENTRIES))
#define MAZE_STACK_DIRS ((u8 *)(MAZE_STACK_ADDR + (MAZE_STACK_ENTRIES * 2)))

void
make_level(void)
{
    int i, j;
    int t;
    int must_exist1, must_exist2, must_exist3;
    int vertical;
    u8 random_rooms[MAXROOMS];
    boolean big_room;

    if (cur_level < LAST_DUNGEON) {
        cur_level++;
    }
    if (cur_level > max_level) {
        max_level = cur_level;
    }

    party_room = NO_ROOM;

    big_room = ((cur_level == party_counter) && rand_percent(1));
    if (big_room) {
        make_room(BIG_ROOM, 0, 0, 0);
        return;
    }

    must_exist1 = get_rand(0, 2);
    vertical = coin_toss();

    if (vertical) {
        must_exist2 = must_exist1 + 3;
        must_exist3 = must_exist2 + 3;
    } else {
        must_exist1 *= 3;
        must_exist2 = must_exist1 + 1;
        must_exist3 = must_exist2 + 1;
    }

    for (i = 0; i < MAXROOMS; ++i) {
        make_room((short)i, (short)must_exist1, (short)must_exist2,
                  (short)must_exist3);
        random_rooms[i] = (u8)i;
    }

    add_mazes();

    for (i = 0; i < MAXROOMS; ++i) {
        j = get_rand(i, MAXROOMS - 1);
        t = random_rooms[i];
        random_rooms[i] = random_rooms[j];
        random_rooms[j] = (u8)t;
    }

    for (j = 0; j < MAXROOMS; ++j) {
        i = random_rooms[j];
        if (i < MAXROOMS - 1) connect_rooms((short)i, (short)(i + 1));
        if (i < MAXROOMS - 3) connect_rooms((short)i, (short)(i + 3));
        if (i < MAXROOMS - 2 && !room_exists[i + 1] &&
            (i + 1 != 4 || vertical)) {
            connect_rooms((short)i, (short)(i + 2));
        }
        if (i < MAXROOMS - 6 && !room_exists[i + 3] &&
            (i + 3 != 4 || !vertical)) {
            connect_rooms((short)i, (short)(i + 6));
        }
        if (is_all_connected()) break;
    }
}

void
make_room(short rn, short r1, short r2, short r3)
{
    room *rm;
    int left_col, right_col, top_row, bottom_row;
    int width, height;
    int i, j;

    if (rn == BIG_ROOM) {
        rn = 0;
        left_col = get_rand(0, 10);
        right_col = get_rand(ROGUE_COLUMNS - 11, ROGUE_COLUMNS - 1);
        top_row = get_rand(MIN_ROW, MIN_ROW + 5);
        bottom_row = get_rand(MAX_ROW - 5, MAX_ROW);
    } else {
        switch (rn % 3) {
        case 0:
            left_col = 0;
            right_col = COL1 - 1;
            break;
        case 1:
            left_col = COL1 + 1;
            right_col = COL2 - 1;
            break;
        default:
        case 2:
            left_col = COL2 + 1;
            right_col = ROGUE_COLUMNS - 1;
            break;
        }

        switch (rn / 3) {
        case 0:
            top_row = MIN_ROW;
            bottom_row = ROW1 - 1;
            break;
        case 1:
            top_row = ROW1 + 1;
            bottom_row = ROW2 - 1;
            break;
        default:
        case 2:
            top_row = ROW2 + 1;
            bottom_row = MAX_ROW;
            break;
        }

        if (rn != r1 && rn != r2 && rn != r3 && rand_percent(40)) {
            room_exists[rn] = 0;
            return;
        }

        height = get_rand(4, (bottom_row - top_row + 1));
        width = get_rand(7, (right_col - left_col - 2));

        top_row += get_rand(0, (bottom_row - top_row) - height + 1);
        left_col += get_rand(0, (right_col - left_col) - width + 1);

        bottom_row = top_row + height - 1;
        right_col = left_col + width - 1;
    }

    room_exists[rn] = 1;
    rm = &rooms[rn];
    rm->is_room = R_ROOM;
    rm->left_col = (u8)left_col;
    rm->top_row = (u8)top_row;
    rm->right_col = (u8)right_col;
    rm->bottom_row = (u8)bottom_row;
    rm->center_col = (u8)((rm->left_col + rm->right_col) / 2);
    rm->center_row = (u8)((rm->top_row + rm->bottom_row) / 2);

    for (i = rm->top_row; i <= rm->bottom_row; i++) {
        for (j = rm->left_col; j <= rm->right_col; j++) {
            if (i == rm->top_row || i == rm->bottom_row) {
                DUNGEON(i, j) = TILE_WALL_H;
            } else if (j == rm->left_col || j == rm->right_col) {
                DUNGEON(i, j) = TILE_WALL_V;
            } else {
                DUNGEON(i, j) = TILE_FLOOR;
            }
        }
    }
}

int
connect_rooms(short room1, short room2)
{
    short row1, col1, row2, col2, dir, rev;
    door *dp;

    if (!room_exists[room1] || !room_exists[room2]) {
        return 0;
    }
    if (same_row(room1, room2)) {
        if (rooms[room1].left_col > rooms[room2].right_col) {
            dir = LEFT;
            rev = RIGHT;
        } else {        /*if (rooms[room2].left_col > rooms[room1].right_col) */

            dir = RIGHT;
            rev = LEFT;
        }
    } else if (same_col(room1, room2)) {
        if (rooms[room1].top_row > rooms[room2].bottom_row) {
            dir = UPWARD;
            rev = DOWN;
        } else {		/*if (rooms[room2].top_row > rooms[room1].bottom_row) */
            dir = DOWN;
            rev = UPWARD;
        }
    } else {
        return 0;
    }
    put_door(&rooms[room1], dir, &row1, &col1);
    put_door(&rooms[room2], rev, &row2, &col2);

    do {
        draw_simple_passage(row1, col1, row2, col2, dir);
    } while (rand_percent(4));

    dp = &rooms[room1].doors[dir / 2];
    dp->oth_room = room2;
    dp->oth_row = row2;
    dp->oth_col = col2;

    dp = &rooms[room2].doors[((dir + 4) % DIRS) / 2];
    dp->oth_room = room1;
    dp->oth_row = row1;
    dp->oth_col = col1;
    return 1;
}

void
clear_level(void)
{
    int i, j;

    detect_monster = 0;
    see_invisible = 0;
    memset(dungeon, TILE_ROCK, ROGUE_COLUMNS * STATUS_ROW_1);
    memset(dungeon_attr, ATTR_HIDDEN, ROGUE_COLUMNS * STATUS_ROW_1);
    for (i = 0; i < MAXROOMS; ++i) {
        room_exists[i] = 0;
        rooms[i].is_room = R_NOTHING;
        for (j = 0; j < 4; j++) {
            rooms[i].doors[j].oth_room = NO_ROOM;
        }
    }
}

void
put_door(room *rm, short dir, short *row, short *col)
{
    short r, c;

    if (rm->is_room & R_MAZE) {
        /* 迷路では扉を置かず、既存通路から区画境界まで掘り延ばす。 */
        r = rm->center_row;
        c = rm->center_col;
        while (DUNGEON(r,c) != TILE_TUNNEL) {
            ++c;
            if (c > rm->right_col) {
                c = rm->left_col;
                if (++r > rm->bottom_row) {
                    r = rm->center_row; c = rm->center_col;
                    DUNGEON(r,c) = TILE_TUNNEL;
                    break;
                }
            }
        }
        switch (dir) {
        case UPWARD:
        case DOWN:
            *row = (dir == UPWARD) ? rm->top_row : rm->bottom_row;
            *col = c;
            while (r != *row) {
                DUNGEON(r,c) = TILE_TUNNEL;
                r += (r > *row) ? -1 : 1;
            }
            break;
        case RIGHT:
        case LEFT:
            *col = (dir == LEFT) ? rm->left_col : rm->right_col;
            *row = r;
            while (c != *col) {
                DUNGEON(r,c) = TILE_TUNNEL;
                c += (c > *col) ? -1 : 1;
            }
            break;
        }
        DUNGEON(*row,*col) = TILE_TUNNEL;
        rm->doors[dir / 2].door_row = *row;
        rm->doors[dir / 2].door_col = *col;
        return;
    }
    switch (dir) {
    case UPWARD:
    case DOWN:
        *row = (dir == UPWARD) ? rm->top_row : rm->bottom_row;
        *col = (short)get_rand(rm->left_col + 1, rm->right_col - 1);
        break;
    case RIGHT:
    case LEFT:
        *col = (dir == LEFT) ? rm->left_col : rm->right_col;
        *row = (short)get_rand(rm->top_row + 1, rm->bottom_row - 1);
        break;
    }
    DUNGEON(*row, *col) = TILE_DOOR;
    rm->doors[dir / 2].door_row = *row;
    rm->doors[dir / 2].door_col = *col;
}

void
draw_simple_passage(short row1, short col1, short row2, short col2, short dir)
{
    short i;
    short middle, t;

    if (dir == LEFT || dir == RIGHT) {
        if (col1 > col2) {
            swap(row1, row2);
            swap(col1, col2);
        }
        middle = (short)get_rand(col1 + 1, col2 - 1);
        for (i = col1 + 1; i != middle; i++) {
            DUNGEON(row1, i) = TILE_TUNNEL;
        }
        for (i = row1; i != row2; i += (row1 > row2) ? -1 : 1) {
            DUNGEON(i, middle) = TILE_TUNNEL;
        }
        for (i = middle; i != col2; i++) {
            DUNGEON(row2, i) = TILE_TUNNEL;
        }
    } else {
        if (row1 > row2) {
            swap(row1, row2);
            swap(col1, col2);
        }
        middle = (short)get_rand(row1 + 1, row2 - 1);
        for (i = row1 + 1; i != middle; i++) {
            DUNGEON(i, col1) = TILE_TUNNEL;
        }
        for (i = col1; i != col2; i += (col1 > col2) ? -1 : 1) {
            DUNGEON(middle, i) = TILE_TUNNEL;
        }
        for (i = middle; i != row2; i++) {
            DUNGEON(i, col2) = TILE_TUNNEL;
        }
    }
/*    if (rand_percent(HIDE_PERCENT)) {
	    hide_boxed_passage(row1, col1, row2, col2, 1);
    }*/
}

int
same_row(int room1, int room2)
{
    return ((room1 / 3) == (room2 / 3));
}

int
same_col(int room1, int room2)
{
    return ((room1 % 3) == (room2 % 3));
}

void
add_mazes(void)
{
    short i, j, start;
    short maze_percent;
    short tr, br, lc, rc;
    room *rm;

    if (cur_level <= 1) return;
    start = (short)get_rand(0, MAXROOMS - 1);
    maze_percent = (short)((cur_level * 5) / 4);
    if (cur_level > 15) maze_percent += cur_level;

    for (i = 0; i < MAXROOMS; i++) {
        j = (short)((start + i) % MAXROOMS);
        if (room_exists[j] || !rand_percent(maze_percent)) continue;

        lc = (j % 3 == 0) ? 0 : ((j % 3 == 1) ? COL1 + 1 : COL2 + 1);
        rc = (j % 3 == 0) ? COL1 - 1 : ((j % 3 == 1) ? COL2 - 1 : ROGUE_COLUMNS - 1);
        tr = (j / 3 == 0) ? MIN_ROW : ((j / 3 == 1) ? ROW1 + 1 : ROW2 + 1);
        br = (j / 3 == 0) ? ROW1 - 1 : ((j / 3 == 1) ? ROW2 - 1 : MAX_ROW);
        ++lc; --rc; ++tr; --br;
        rm = &rooms[j];
        rm->left_col = (u8)lc; rm->right_col = (u8)rc;
        rm->top_row = (u8)tr; rm->bottom_row = (u8)br;
        rm->center_row = (u8)((tr + br) / 2);
        rm->center_col = (u8)((lc + rc) / 2);
        rm->is_room = R_MAZE;
        room_exists[j] = 1;
        make_maze((short)get_rand(tr, br), (short)get_rand(lc, rc),
                  tr, br, lc, rc);
    }
}

void
make_maze(short r, short c, short tr, short br, short lc, short rc)
{
    u8 dirs[4];
    u8 depth = 0;
    u8 row = (u8)r;
    u8 col = (u8)c;
    u8 state, direction;
    u8 i, t1, t2, t;

    /* MZ版では境界と探索スタックをワークへ置き、Cスタックを使用しない */
    MAZE_TOP = (u8)tr;
    MAZE_BOTTOM = (u8)br;
    MAZE_LEFT = (u8)lc;
    MAZE_RIGHT = (u8)rc;

new_cell:
    DUNGEON(row, col) = TILE_TUNNEL;
    dirs[0] = 0;
    dirs[1] = 1;
    dirs[2] = 2;
    dirs[3] = 3;
    if (rand_percent(33)) {
        for (i = 0; i < 10; i++) {
            t1 = (u8)get_rand(0, 3); t2 = (u8)get_rand(0, 3);
            t = dirs[t1]; dirs[t1] = dirs[t2]; dirs[t2] = t;
        }
    }
    MAZE_STACK_ROWS[depth] = row;
    MAZE_STACK_COLS[depth] = col;
    MAZE_STACK_DIRS[depth] = dirs[0] | (dirs[1] << 2) |
                             (dirs[2] << 4) | (dirs[3] << 6);

next_direction:
    state = MAZE_STACK_ROWS[depth];
    if ((state >> 5) == 4) {
        if (depth == 0) return;
        depth--;
        goto next_direction;
    }

    row = state & 0x1f;
    col = MAZE_STACK_COLS[depth];
    MAZE_STACK_ROWS[depth] = state + 0x20;
    direction = MAZE_STACK_DIRS[depth] & 3;
    MAZE_STACK_DIRS[depth] >>= 2;

    switch (direction) {
        case 0:                         /* UPWARD */
            if (row - 1 >= MAZE_TOP &&
                DUNGEON(row-1,col) != TILE_TUNNEL &&
                DUNGEON(row-1,col-1) != TILE_TUNNEL &&
                DUNGEON(row-1,col+1) != TILE_TUNNEL &&
                (row-2 < MAZE_TOP || DUNGEON(row-2,col) != TILE_TUNNEL)) {
                row--;
                depth++;
                goto new_cell;
            }
            break;
        case 1:                         /* DOWN */
            if (row + 1 <= MAZE_BOTTOM &&
                DUNGEON(row+1,col) != TILE_TUNNEL &&
                DUNGEON(row+1,col-1) != TILE_TUNNEL &&
                DUNGEON(row+1,col+1) != TILE_TUNNEL &&
                (row+2 > MAZE_BOTTOM || DUNGEON(row+2,col) != TILE_TUNNEL)) {
                row++;
                depth++;
                goto new_cell;
            }
            break;
        case 2:                         /* LEFT */
            if (col - 1 >= MAZE_LEFT &&
                DUNGEON(row,col-1) != TILE_TUNNEL &&
                DUNGEON(row-1,col-1) != TILE_TUNNEL &&
                DUNGEON(row+1,col-1) != TILE_TUNNEL &&
                (col-2 < MAZE_LEFT || DUNGEON(row,col-2) != TILE_TUNNEL)) {
                col--;
                depth++;
                goto new_cell;
            }
            break;
        case 3:                         /* RIGHT */
            if (col + 1 <= MAZE_RIGHT &&
                DUNGEON(row,col+1) != TILE_TUNNEL &&
                DUNGEON(row-1,col+1) != TILE_TUNNEL &&
                DUNGEON(row+1,col+1) != TILE_TUNNEL &&
                (col+2 > MAZE_RIGHT || DUNGEON(row,col+2) != TILE_TUNNEL)) {
                col++;
                depth++;
                goto new_cell;
            }
            break;
    }
    goto next_direction;
}

void put_player(short nr)
{
    short rn = nr, misses;
    short row = MIN_ROW;
    short col = 0;

    for (misses = 0; misses < 2 && rn == nr; misses++) {
        do {
            gr_row_col(&row, &col, FLOOR | TUNNEL | STAIRS);
        } while (trap_at(row, col) != NO_TRAP);
        rn = (short)get_room_number(row, col);
    }
    rogue.row = row;
    rogue.col = col;
    
    if (DUNGEON(row, col) == TILE_TUNNEL) {
        cur_room = PASSAGE;
        light_passage(row, col);
    } else {
        cur_room = rn;
        if (rn != NO_ROOM) light_up_room(rn);
    }
}

int drop_check(void)
{
    if (rogue.row == stairs_row && rogue.col == stairs_col) {
        if (levitate) {
            message_id(48, 0);
            return 0;
        }
        return 1;
    }
    message_id(49, 0);
    return 0;
}

int check_up(void)
{
    if (rogue.row != stairs_row || rogue.col != stairs_col) {
        message_id(50, 0);
        return 0;
    }
    if (!has_amulet()) {
        message_id(51, 0);
        return 0;
    }
    if (cur_level == 1) {
        win();
    } else {
        cur_level -= 2;
        return 1;
    }
    return 0;
}

void
add_exp(int e, boolean promotion)
{
    short new_exp;
    short hp;
    u8 mz_number[8];
    long value;

    rogue.exp_points += e;

    if (rogue.exp_points > MAX_EXP) {
        rogue.exp_points = MAX_EXP + 1;
    }
    new_exp = (short)get_exp_level(rogue.exp_points);
    while (rogue.exp < new_exp) {
        ++rogue.exp;
        if (promotion) {
            hp = (short)hp_raise();
            rogue.hp_current += hp;
            rogue.hp_max += hp;
        }
        value = rogue.exp;
        mz_sprintf(mz_number, 525, &value);
        message_id(53, mz_number);
    }
}

int
get_exp_level(long e)
{
    short i;

    for (i = 0; i < (MAX_EXP_LEVEL - 1); i++) {
        if (level_points[i] > e) {
            break;
        }
    }
    return (i + 1);
}

int
hp_raise(void)
{
    int hp;

    hp = get_rand(3, 10);
    return hp;
}
