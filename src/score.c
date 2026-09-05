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
#include "rogue.h"
#include "message.h"
#include "mz_curses.h"
#include "score.h"

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
    u8 stats[48];
    long values[3];

    game_over = 1;
    rogue.hp_current = 0;
    rogue.gold = rogue.gold * 9L / 10L;
    clear();
    for (id = 500; id <= 513; ++id) {
        (void)get_message(id, text, sizeof(text));
        mvaddstr((u8)(id - 497), 0, text);
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
    mvaddstr(12, (u8)((40 - length) / 2), reason);
    values[0] = cur_level;
    values[1] = rogue.gold;
    values[2] = rogue.exp_points;
    mz_sprintf(stats, "Level:%d Gold:%ld Exp:%ld", values);
    length = mz_display_length(stats);
    mvaddstr(18, (u8)((40 - length) / 2), stats);
    (void)get_message(517, text, sizeof(text));
    mvaddstr(20, 0, text);
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
    (void)get_message(519, text, sizeof(text));
    length = mz_display_length(text);
    mvaddstr(3, (u8)((40 - length) / 2), text);
    for (id = 182; id <= 185; ++id) {
        (void)get_message(id, text, sizeof(text));
        length = mz_display_length(text);
        mvaddstr((u8)(id - 176), (u8)((40 - length) / 2), text);
    }
    (void)get_message(520, text, sizeof(text));
    length = mz_display_length(text);
    mvaddstr(12, (u8)((40 - length) / 2), text);
    rogue.row = 0;
    rogue.col = 0;
    refresh();
    while (rgetchar() != ' ') {}
}
