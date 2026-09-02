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
#include "move.h"
#include "object.h"
#include "random.h"
#include "room.h"

static u8 rooms_visited[MAXROOMS];

void light_up_room(int rn)
{
    int row;
    int col;

    if (rn < 0 || rn >= MAXROOMS || !room_exists[rn] ||
        (rooms[rn].is_room & R_MAZE)) return;
    for (row = rooms[rn].top_row; row <= rooms[rn].bottom_row; ++row) {
        for (col = rooms[rn].left_col; col <= rooms[rn].right_col; ++col) {
            DUNGEON_ATTR(row, col) = ATTR_VISIBLE;
        }
    }
}

void light_passage(int row, int col)
{
    int r;
    int c;
    int first_row = (row > MIN_ROW) ? row - 1 : row;
    int last_row = (row < MAX_ROW) ? row + 1 : row;
    int first_col = (col > 0) ? col - 1 : col;
    int last_col = (col < ROGUE_COLUMNS - 1) ? col + 1 : col;

    for (r = first_row; r <= last_row; ++r) {
        for (c = first_col; c <= last_col; ++c) {
            if (can_move(row, col, r, c)) {
                DUNGEON_ATTR(r, c) = ATTR_VISIBLE;
            }
        }
    }
}

void darken_room(short rn)
{
    int row;
    int col;

    if (rn < 0 || rn >= MAXROOMS || !room_exists[rn] ||
        (rooms[rn].is_room & R_MAZE)) return;
    for (row = rooms[rn].top_row + 1; row < rooms[rn].bottom_row; ++row) {
        for (col = rooms[rn].left_col + 1; col < rooms[rn].right_col; ++col) {
            if (DUNGEON(row, col) != TILE_STAIRS &&
                !object_at(&level_objects, (short)row, (short)col)) {
                DUNGEON_ATTR(row, col) = ATTR_HIDDEN;
            }
        }
    }
}


void
gr_row_col(short *row, short *col, unsigned short mask)
{
    u8 tile;

    do {
        *row = (short)get_rand(MIN_ROW, MAX_ROW);
        *col = (short)get_rand(0, ROGUE_COLUMNS - 1);
        tile = DUNGEON(*row, *col);
    } while (!((tile == TILE_FLOOR && (mask & FLOOR)) ||
               (tile == TILE_TUNNEL && (mask & TUNNEL)) ||
               (tile == TILE_STAIRS && (mask & STAIRS))));
}

int
gr_room(void)
{
    short i;

    do {
        i = get_rand(0, MAXROOMS - 1);
    } while (!(rooms[i].is_room & (R_ROOM | R_MAZE)));

    return i;
}

int party_objects(int rn)
{
    short i, tries, n, row, col;
    object *obj;

    n = (short)get_rand(5, 10);
    for (i = 0; i < n; ++i) {
        obj = gr_object();
        if (!obj) continue;
        for (tries = 0; tries < 100; ++tries) {
            row = (short)get_rand(rooms[rn].top_row + 1, rooms[rn].bottom_row - 1);
            col = (short)get_rand(rooms[rn].left_col + 1, rooms[rn].right_col - 1);
            if ((DUNGEON(row,col) == TILE_FLOOR || DUNGEON(row,col) == TILE_TUNNEL) &&
                !object_at(&level_objects, row, col)) {
                place_at(obj, row, col);
                break;
            }
        }
        if (tries == 100) free_object(obj);
    }
    return n;
}

int
get_room_number(int row, int col)
{
    short i;

    for (i = 0; i < MAXROOMS; i++) {
        if (room_exists[i] &&
            row >= rooms[i].top_row && row <= rooms[i].bottom_row &&
            col >= rooms[i].left_col && col <= rooms[i].right_col) {
            return i;
        }
    }
    return (NO_ROOM);
}

int
is_all_connected(void)
{
    short i, starting_room = 0;

    for (i = 0; i < MAXROOMS; i++) {
        rooms_visited[i] = 0;
        if (rooms[i].is_room & (R_ROOM | R_MAZE)) {
            starting_room = i;
        }
    }

    visit_rooms(starting_room);

    for (i = 0; i < MAXROOMS; i++) {
        if ((rooms[i].is_room & (R_ROOM | R_MAZE)) && (!rooms_visited[i])) {
            return 0;
        }
    }
    return 1;
}

void
visit_rooms(int rn)
{
    short i;
    short oth_rn;

    rooms_visited[rn] = 1;

    for (i = 0; i < 4; i++) {
        oth_rn = rooms[rn].doors[i].oth_room;
        if ((oth_rn >= 0) && (!rooms_visited[oth_rn])) {
            visit_rooms(oth_rn);
        }
    }
}
