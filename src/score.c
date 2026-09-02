/*
 * score.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include <stdio.h>

#include "rogue.h"
#include "message.h"
#include "mz_curses.h"
#include "score.h"

static void center_ascii(short row, const char *text)
{
    short length = 0;
    while (text[length]) ++length;
    mvaddstr((u8)row, (u8)((40 - length) / 2), (const u8 *)text);
}

static short mz_display_length(const u8 *text)
{
    short length = 0;

    while (*text) {
        if (*text != MZ_STR_CSET_0 && *text != MZ_STR_CSET_1) ++length;
        ++text;
    }
    return length;
}

void killed_by(object *monster, short other)
{
#if defined(DEBUG)
    (void)monster;
    (void)other;
    rogue.hp_current = 1;
    print_stats(STAT_HP);
    return;
#else
    u8 reason[40];
    short length;
    short suffix_length;
    short i;
    short id;
    u8 text[40];
    char stats[40];

    game_over = 1;
    rogue.hp_current = 0;
    rogue.gold = rogue.gold * 9L / 10L;
    clear();
    for (id = 411; id <= 424; ++id) {
        (void)get_message(id, text, sizeof(text));
        mvaddstr_mz((u8)(id - 408), 0, text);
    }
    length = 0;
    if (monster) {
        length = get_message(monster->m_name_id, reason, sizeof(reason));
        suffix_length = get_message(176, reason + length,
                                    (short)(sizeof(reason) - length));
        if (suffix_length > 0 && reason[length] == MESSAGE_FORMAT_STRING) {
            for (i = 0; i < suffix_length; ++i) {
                reason[length + i] = reason[length + i + 1];
            }
            --suffix_length;
        }
        length += suffix_length;
    } else {
        length = get_message((short)(other == STARVATION ? 180 : 170),
                             reason, sizeof(reason));
    }
    length = mz_display_length(reason);
    mvaddstr_mz(12, (u8)((40 - length) / 2), reason);
    sprintf(stats, "Level:%d Gold:%ld Exp:%ld", cur_level,
            rogue.gold, rogue.exp_points);
    center_ascii(18, stats);
    (void)get_message(428, text, sizeof(text));
    mvaddstr_mz(20, 0, text);
    rogue.row = 0;
    rogue.col = 0;
    refresh();
    while (rgetchar() != ' ') {}
#endif
}

void win(void)
{
    u8 text[40];
    short length;
    short id;

    game_over = 1;
    clear();
    center_ascii(3, "*** YOU WIN ***");
    for (id = 182; id <= 185; ++id) {
        (void)get_message(id, text, sizeof(text));
        length = mz_display_length(text);
        mvaddstr_mz((u8)(id - 176), (u8)((40 - length) / 2), text);
    }
    center_ascii(12, "Press Space");
    rogue.row = 0;
    rogue.col = 0;
    refresh();
    while (rgetchar() != ' ') {}
}
