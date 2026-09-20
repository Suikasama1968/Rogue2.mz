/*
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
#include "display.h"
#include "message.h"
#include "move.h"
#include "object.h"
#include "mz_curses.h"
#include "mz_display.h"
#include "mz_system.h"

#define message_buffer ((u8 *)MESSAGE_BUFFER_ADDR)
static boolean msg_cleared = 1;
static u8 msg_col;
char hunger_str[8] = "";
extern short add_strength;

void
message(char *msg, boolean intrpt)
{
    const u8 *p;
    u8 length;
    u8 more_col;
    u8 more_length;

    (void)intrpt;
    attrset(A_NORMAL);
    if (!msg_cleared) {
        p = find_message(11, &length);
        more_length = mz_display_length(p);
        more_col = msg_col;
        if (more_col + more_length > V_COLUMN / 2) {
            more_col = (u8)(V_COLUMN / 2 - more_length);
        }
        move(MESSAGE_ROW, more_col);
        addnstr(p, length);
        refresh_dungeon();
        flushinp();
        (void)rgetchar();
        flushinp();
        check_message();
    }
    move(MESSAGE_ROW, 0);
    addstr((const u8 *)msg);
    clrtoeol();
    msg_cleared = 0;
    msg_col = mz_display_length((const u8 *)msg);
    refresh_dungeon();
}

#if 0 /* MZ-700/1500では未対応 */

void
remessage(void)
{
    if (msg_line[0]) {
	message(msg_line, 0);
    }
}

#endif

void
check_message(void)
{
	if (msg_cleared) {
		return;
	}
    move(MESSAGE_ROW, 0);
    clrtoeol();
    msg_cleared = 1;
    refresh_dungeon();
}

int get_direction(void)
{
    int dir;

    message_id(55, 0);
    while (!is_direction(dir = rgetchar())) {
    /* sound_bell() */
    }
    flushinp();
    check_message();
    return dir;
}

#if 0 /* MZ-700/1500では未対応 */

int
get_input_line(char *prompt, char *insert, char *buf, char *if_cancelled,
	       boolean add_blank, boolean do_echo)
{
    int n;

    n = do_input_line(1, 0, 0, prompt, insert,
		      buf, if_cancelled, add_blank, do_echo, 0);
    return ((n < 0) ? 0 : n);
}

int
input_line(int row, int col, char *insert, char *buf, int ch)
{
    return do_input_line(0, row, col, "", insert, buf, "", 0, 1, ch);
}

int
do_input_line(boolean is_msg, int row, int col, char *prompt, char *insert,
	      char *buf, char *if_cancelled, boolean add_blank,
	      boolean do_echo, int first_ch)
{
    short ch;
    short i = 0, n = 0;
#if defined( JAPAN )
    short k;
    char kanji[MAX_TITLE_LENGTH];
#endif /* JAPAN */

    if (is_msg) {
	message(prompt, 0);
	//n = strlen(prompt) + 1;
	n = utf8strlen(prompt) + 1;
    } else {
	mvaddstr_rogue(row, col, prompt);
    }

    if (insert[0]) {
	mvaddstr_rogue(row, col + n, insert);
	(void) strcpy(buf, insert);
	//i = strlen(insert);
	i = utf8strlen(insert);
#if defined( JAPAN )
	k = 0;
	while (k < i) {
	    ch = insert[k];
#if defined( EUC )
	    if (ch & 0x80) {	/* for EUC code by Yasha */
		kanji[k] = kanji[k + 1] = 1;
		k += 2;
	    } else {
		kanji[k] = 0;
		k++;
	    }
#else /* not EUC */
	    if (ch >= 0x81 && ch <= 0x9f || ch >= 0xe0 && ch <= 0xfc) {
		kanji[k] = kanji[k + 1] = 1;
		k += 2;
	    } else {
		kanji[k] = 0;
		k++;
	    }
#endif /* not EUC */
	}
#endif /* JAPAN */
	move(row, col + n + i);
	refresh();
    }
#if defined( JAPAN )
    for (;;) {
	if (first_ch) {
	    ch = first_ch;
	    first_ch = 0;
	} else {
	    ch = rgetchar();
	}
	if (ch == '\r' || ch == '\n' || ch == CANCEL) {
	    break;
	}
	if ((ch == '\b') && (i > 0)) {
	    i -= kanji[i - 1] ? 2 : 1;
	    if (do_echo) {
		mvaddstr_rogue(row, col + n + i, "  ");
		move(row, col + n + i);
	    }
	} else if (
#if defined( EUC )
	    (ch >= ' ' && !(ch & 0x80)) && (i < MAX_TITLE_LENGTH - 2)
#else /* Shift JIS */
	    (ch >= ' ' && ch <= '~' || ch >= 0xa0 && ch <= 0xde) && (i < MAX_TITLE_LENGTH - 2)
#endif /* not EUC */
	    ) {
	    if ((ch != ' ') || (i > 0)) {
		buf[i] = ch;
		kanji[i] = 0;
		if (do_echo) {
		    addch(ch);
		}
		i++;
	    }
	} else if (
#if defined( EUC )
	    (ch & 0x80) && (i < MAX_TITLE_LENGTH - 3)
#else /* Shift JIS */
	    (ch >= 0x81 && ch <= 0x9f || ch >= 0xe0 && ch <= 0xfc) && (i < MAX_TITLE_LENGTH - 3)
#endif /* not EUC */
	    ) {
	    buf[i] = ch;
	    buf[i + 1] = rgetchar();
	    kanji[i] = kanji[i + 1] = 1;
	    if (do_echo) {
		addch_rogue(buf[i]);
		addch_rogue(buf[i + 1]);
	    }
	    i += 2;
	}
	refresh();
    }
    if (is_msg) {
	check_message();
    }
    while ((i > 0) && (buf[i - 1] == ' ') && (kanji[i - 1] == 0)) {
	i--;
    }
    if (add_blank) {
	buf[i++] = ' ';
    }
#else /* not JAPAN */
	while (((ch = rgetchar()) != '\r') && (ch != '\n') && (ch != CANCEL)) {
	if ((ch >= ' ') && (ch <= '~') && (i < MAX_TITLE_LENGTH - 2)) {
	    if ((ch != ' ') || (i > 0)) {
		buf[i++] = ch;
		if (do_echo) {
		    addch_rogue(ch);
		}
	    }
	}
	if ((ch == '\b') && (i > 0)) {
	    i--;
	    if (do_echo) {
		mvaddch_rogue(row, col + n + i, ' ');
		move(row, col + n + i);
	    }
	}
	refresh();
    }
    if (is_msg) {
	check_message();
    }
    if (add_blank) {
	buf[i++] = ' ';
    } else {
	while ((i > 0) && (buf[i - 1] == ' ')) {
	    i--;
	}
    }
#endif /* not JAPAN */
	buf[i] = 0;

    if ((ch == CANCEL) || (i == 0) || ((i == 1) && add_blank)) {
	if (is_msg && if_cancelled) {
	    message(if_cancelled, 0);
	}
	return ((ch == CANCEL) ? -1 : 0);
    }
    return i;
}

#endif

int
rgetchar(void)
{
    return getch();
}

void
print_stats(int stat_mask)
{
    u8 *line1 = (u8 *)TEMP_BUFFER_ADDR;
    u8 *line2 = line1 + 48;
    u8 *status1 = dungeon + ROGUE_COLUMNS * STATUS_ROW_1;
    u8 *status2 = dungeon + ROGUE_COLUMNS * STATUS_ROW_2;
    u8 *attr1 = dungeon_attr + ROGUE_COLUMNS * STATUS_ROW_1;
    u8 *attr2 = dungeon_attr + ROGUE_COLUMNS * STATUS_ROW_2;
    long *values = (long *)(line2 + 48);

    /* 40列版では2行を一体で整形するため、指定項目を含む全体を再描画する。 */
    (void)stat_mask;
    attrset(A_NORMAL);
    memset(status1, TILE_ROCK, ROGUE_COLUMNS);
    memset(status2, TILE_ROCK, ROGUE_COLUMNS);
    memset(attr1, ATTR_VISIBLE, ROGUE_COLUMNS);
    memset(attr2, ATTR_VISIBLE, ROGUE_COLUMNS);
    values[0] = cur_level;
    values[1] = rogue.gold;
    values[2] = rogue.hp_current;
    values[3] = rogue.hp_max;
    mz_sprintf(line1, 527, values);
    values[0] = rogue.str_current + add_strength;
    values[1] = rogue.str_max;
    values[2] = get_armor_class(rogue.armor);
    values[3] = rogue.exp;
    values[4] = rogue.exp_points;
    mz_sprintf(line2, 528, values);
    mvaddstr(STATUS_ROW_1, 0, line1);
    addstr((const u8 *)hunger_str);
    mvaddstr(STATUS_ROW_2, 0, line2);
}

#if 0 /* MZ-700/1500では未対応 */

void
pad(char *s, short n)
{
    short i;

    //for (i = strlen(s); i < n; i++) {
    for (i = utf8strlen(s); i < n; i++) {
	addch_rogue(' ');
    }
}

boolean
is_digit(short ch)
{
    return (boolean) ((ch >= '0') && (ch <= '9'));
}

int
r_index(char *str, int ch, boolean last)
{
    int i;

    if (last) {
	//for (i = strlen(str) - 1; i >= 0; i--) {
	for (i = utf8strlen(str) - 1; i >= 0; i--) {
	    if (str[i] == ch) {
		return i;
	    }
	}
    } else {
	for (i = 0; str[i]; i++) {
	    if (str[i] == ch) {
		return i;
	    }
	}
    }
    return -1;
}
#endif

/* MZ-700/1500固有 */
/* メモリの中から対象となるメッセージを取得する */
const u8 *find_message(short msg_id, u8 *length)
{
    const u8 *base = (const u8 *)MESG_ADDR;
    const u8 *entry = base;
    const u8 *src;
    unsigned short offset;
    unsigned short id;
    u8 n;

    while (1) {
        id = (unsigned short)entry[0] |
             ((unsigned short)entry[1] << 8);
        if (id == MESSAGE_END_ID) return 0;
        if (id == (unsigned short)msg_id) {
            offset = (unsigned short)entry[2] |
                     ((unsigned short)entry[3] << 8);
            src = base + offset;
            n = 0;
            while (src[n]) n++;
            *length = n;
            return src;
        }
        entry += MESSAGE_ENTRY_SIZE;
    }
}

/* 対象となるメッセージをバッファにコピーする*/
short get_message(short msg_id, u8 *buffer, short size)
{
    const u8 *src;
    u8 stored_length;
    short length;

    src = find_message(msg_id, &stored_length);
    if (!src) return 0;
    length = stored_length;
    if (length >= size) length = size - 1;
    memcpy(buffer, src, length);
    buffer[length] = '\0';
    return length;
}

void message_id(short msg_id, const u8 *text)
{
    if (format_message(msg_id, text, message_buffer,
                       MESSAGE_BUFFER_SIZE) >= 0) {
        message((char *)message_buffer, 0);
    }
}

short
format_message(short msg_id, const u8 *text, u8 *buffer, short size)
{
    const u8 *src;
    u8 src_left;
    short length = 0;

    src = find_message(msg_id, &src_left);
    if (!src || size <= 0) return -1;
    while (src_left-- && length < size - 1) {
        u8 ch = *src++;
        if (ch == MESSAGE_FORMAT_STRING) {
            while (text && *text && length < size - 1) {
                buffer[length++] = *text++;
            }
        } else {
            buffer[length++] = ch;
        }
    }
    buffer[length] = '\0';
    return length;
}
