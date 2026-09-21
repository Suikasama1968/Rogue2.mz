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
#include "invent.h"
#include "message.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "mz_curses.h"
#include "mz_system.h"

#define INVENTORY_PAGE_ROWS 8
#define INVENTORY_SAVE_ROWS (INVENTORY_PAGE_ROWS + 1)

#define descs_text ((uint8_t (*)[40])DESCS_TEXT_ADDR)
#define descs_attr ((uint8_t (*)[40])DESCS_ATTR_ADDR)
#define sc_title ((uint8_t (*)[34])SCROLL_TITLES_ADDR)

typedef char descs_text_size_check[
    INVENTORY_SAVE_ROWS * 40 <= DESCS_TEXT_SIZE ? 1 : -1];
typedef char descs_attr_size_check[
    INVENTORY_SAVE_ROWS * 40 <= DESCS_ATTR_SIZE ? 1 : -1];
typedef char scroll_titles_size_check[
    SCROLS * 34 <= SCROLL_TITLES_SIZE ? 1 : -1];
typedef char id_scrolls_size_check[
    sizeof(struct id) * SCROLS <= ID_SCROLLS_SIZE ? 1 : -1];
typedef char id_wands_size_check[
    sizeof(struct id) * WANDS <= ID_WANDS_SIZE ? 1 : -1];
typedef char id_rings_size_check[
    sizeof(struct id) * RINGS <= ID_RINGS_SIZE ? 1 : -1];

static uint8_t append_message(uint8_t *buffer, uint8_t length, short msg_id);
static uint8_t append_text(uint8_t *buffer, uint8_t length, const char *text);
static uint8_t append_number(uint8_t *buffer, uint8_t length, short value, boolean plus);
static uint8_t inventory_col(void);
static void save_inventory_rows(uint8_t col, uint8_t rows);
static void restore_inventory_rows(uint8_t col, uint8_t rows);

void
inventory(object *pack, unsigned short mask)
{
    object *obj = pack->next_object;
    uint8_t col;

    while (obj && !(obj->what_is & mask)) obj = obj->next_object;
    if (!obj) {
        message_id(26, 0);
        return;
    }
    col = inventory_col();
    while (obj) {
        object *next = obj;
        uint8_t rows = 0;
        uint8_t *desc = (uint8_t *)TEMP_BUFFER_ADDR;
        uint8_t row;

        while (next && rows < INVENTORY_PAGE_ROWS) {
            if (next->what_is & mask) ++rows;
            next = next->next_object;
        }
        save_inventory_rows(col, (uint8_t)(rows + 1));
        row = 0;
        while (obj && row < rows) {
            if (obj->what_is & mask) {
                get_desc(obj, (char *)desc, 0);
                memset(dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
                       DC_SPC, 40);
                memset(dungeon_attr +
                       (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
                       0x70, 40);
                DUNGEON(row + 1, col) =
                    (uint8_t)(DC_A + obj->ichar - 'a');
                DUNGEON(row + 1, col + 1) = DC_R_BLACKET;
                DUNGEON(row + 1, col + 2) = DC_SPC;
                DUNGEON_ATTR(row + 1, col) = 0xf0;
                DUNGEON_ATTR(row + 1, col + 1) = 0xf0;
                DUNGEON_ATTR(row + 1, col + 2) = 0xf0;
                mvaddnstr((uint8_t)(row + 1), (uint8_t)(col + 3), desc, 37);
                ++row;
            }
            obj = obj->next_object;
        }
        memset(dungeon + (unsigned int)(rows + 1) * ROGUE_COLUMNS + col,
               DC_SPC, 40);
        memset(dungeon_attr +
               (unsigned int)(rows + 1) * ROGUE_COLUMNS + col, 0x70, 40);

        (void)get_message(518, desc, TEMP_BUFFER_SIZE);
        mvaddnstr((uint8_t)(rows + 1), col, desc, 40);
        move((uint8_t)rogue.row, (uint8_t)rogue.col);
        refresh();
        wait_for_ack();
        restore_inventory_rows(col, (uint8_t)(rows + 1));
        move((uint8_t)rogue.row, (uint8_t)rogue.col);
        refresh_dungeon();
        while (obj && !(obj->what_is & mask)) obj = obj->next_object;
    }
}

void
make_scroll_titles(void)
{
    short i, j, len;
    short sylls, s;
    uint8_t n, *title;

    for (i = 0; i < SCROLS; i++) {
        sylls = get_rand(2, 5);
        title = sc_title[i];
        *title = DC_L_BRACKET;
        len = 1;
        for (j = 0; j < sylls; j++) {
            s = get_rand(1, MAXSYLLABLES - 1);
            (void)find_message(454 + s, &n);
            if (len + n - 1 >= MAX_TITLE_LENGTH - 2) break;
            (void)get_message(454 + s, title + len, 34 - len);
            len += n;
        }
        title[len - 1] = DC_R_BRACKET;
        title[len] = '\0';
        id_scrolls[i].title = (char *)title;
        id_scrolls[i].real =
            (char *)find_message(362 + i, &n);
        id_scrolls[i].id_status = UNIDENTIFIED;
    }
}

void
get_desc(object *obj, char *desc, boolean capitalized)
{
    uint8_t *buffer = (uint8_t *)desc;
    struct id *id;
    uint8_t length;

    (void)capitalized;
    length = 0;
    buffer[0] = '\0';
    switch (obj->what_is) {
    case AMULET:
        get_message(27, buffer, ROGUE_COLUMNS);
        break;
    case GOLD:
        length = mz_number(buffer, (unsigned short)obj->quantity);
        append_message(buffer, length, 28);
        break;
    case FOOD:
        length = mz_number(buffer, (unsigned short)obj->quantity);
        if (obj->which_kind == RATION) {
            length = append_message(buffer, length, 30);
            append_message(buffer, length, 2);
        } else {
            length = append_message(buffer, length, 31);
            append_message(buffer, length, 333);
        }
        break;
    case WEAPON:
        if (obj->quantity > 1) {
            length = mz_number(buffer, (unsigned short)obj->quantity);
            length = append_message(buffer, length, 29);
        }
        if (obj->identified) {
            buffer[length++] = DC_L_BLACKET;
            length = append_number(buffer, length, obj->hit_enchant, 1);
            buffer[length++] = DC_COMMA;
            length = append_number(buffer, length, obj->d_enchant, 1);
            buffer[length++] = DC_R_BLACKET;
            buffer[length] = '\0';
        }
        length = append_message(buffer, length,
                                374 + obj->which_kind);
        if (obj->in_use_flags & BEING_WIELDED) {
            append_message(buffer, length, 35);
        }
        break;
    case ARMOR:
        if (obj->identified) {
            buffer[length++] = DC_L_BLACKET;
            length = append_number(buffer, length, obj->d_enchant, 1);
            buffer[length++] = DC_R_BLACKET;
            buffer[length] = '\0';
        }
        length = append_message(buffer, length,
                                382 + obj->which_kind);
        if (obj->identified) {
            buffer[length++] = DC_L_SQ_BLACKET;
            length = append_number(buffer, length,
                                   get_armor_class(obj), 0);
            buffer[length++] = DC_R_SQ_BLACKET;
            buffer[length] = '\0';
        }
        if (obj->in_use_flags & BEING_WORN) {
            append_message(buffer, length, 36);
        }
        break;
    case POTION:
        if (obj->quantity > 1) {
            length = mz_number(buffer, (unsigned short)obj->quantity);
            length = append_message(buffer, length, 32);
        }
        length = append_message(buffer, length,
                    ((identified_potions &
                      (unsigned short)(1U << obj->which_kind)) != 0
                     ? 348 : 334) + obj->which_kind);
        append_message(buffer, length, 4);
        break;
    case SCROL:
        if (obj->quantity > 1) {
            length = mz_number(buffer, (unsigned short)obj->quantity);
            length = append_message(buffer, length, 32);
        }
        if (id_scrolls[obj->which_kind].id_status == IDENTIFIED) {
            length = append_text(buffer, length,
                                 id_scrolls[obj->which_kind].real);
        } else {
            length = append_text(buffer, length,
                                 id_scrolls[obj->which_kind].title);
            length = append_message(buffer, length, 33);
        }
        append_message(buffer, length, 3);
        break;
    case WAND:
        id = &id_wands[obj->which_kind];
        goto ID_OBJECT;
    case RING:
        id = &id_rings[obj->which_kind];
ID_OBJECT:
        if (obj->identified || id->id_status == IDENTIFIED) {
            length = append_text(buffer, 0, id->real);
            if (obj->what_is == RING) {
                append_message(buffer, length, 8);
            }
        } else {
            length = append_text(buffer, 0, id->title);
            append_message(buffer, length,
                           (obj->what_is == WAND) ? 5 : 8);
        }
        break;
    default:
        buffer[0] = '\0';
        break;
    }
}

void
get_wand_and_ring_materials(void)
{
    short i, j;
    boolean *used = (boolean *)TEMP_BUFFER_ADDR;
    char **wand_materials = (char **)WAND_MATERIALS_ADDR;
    char **gems = (char **)GEMS_ADDR;

    memset(used, 0, WAND_MATERIALS);
    for (i = 0; i < WANDS; i++) {
        do {
            j = get_rand(0, WAND_MATERIALS - 1);
        } while (used[j]);
        used[j] = 1;
        id_wands[i].title = wand_materials[j];
        id_wands[i].id_status = UNIDENTIFIED;
    }

    memset(used, 0, GEMS);
    for (i = 0; i < RINGS; i++) {
        do {
            j = get_rand(0, GEMS - 1);
        } while (used[j]);
        used[j] = 1;
        id_rings[i].title = gems[j];
        id_rings[i].id_status = UNIDENTIFIED;
    }
}

#if 0 /* MZ-700/1500では未サポート */
void
single_inv(short ichar)
{
    object *obj;
    char *desc = (char *)TEMP_BUFFER_ADDR;

    if (!(obj = get_letter_object(ichar))) {
        return;
    }
    get_desc(obj, desc, 1);
    message((char *)desc, 0);
}

struct id *
get_id_table(object *obj)
{
    switch (obj->what_is) {
    case SCROL:
	return id_scrolls;
    case POTION:
	return id_potions;
    case WAND:
	return id_wands;
    case RING:
	return id_rings;
    case WEAPON:
	return id_weapons;
    case ARMOR:
	return id_armors;
    }
    return ((struct id *) 0);
}

#endif

#if 0 /* MZ-700/1500では未対応 */

void
inv_armor_weapon(boolean is_weapon)
{
    if (is_weapon) {
	if (rogue.weapon) {
	    single_inv(rogue.weapon->ichar);
	} else {
	    message(mesg[43], 0);
	}
    } else {
	if (rogue.armor) {
	    single_inv(rogue.armor->ichar);
	} else {
	    message(mesg[44], 0);
	}
    }
}

void
discovered(void)
{
    short i, j, n;
    short ch, maxlen, found;
    short row, col;
    struct dlist *dp, *enddp;
    struct dobj *op;
    char *p;
#if defined( JAPAN )
    char *msg = "  ＝スペースを押してください＝";
    short len = 30;
#else /* not JAPAN */
    char *msg = " --Press space to continue--";
    short len = 28;
#endif /* not JAPAN */

    message(mesg[45], 0);
    while (r_index("?!/=*\033", (ch = rgetchar()), 0) == -1) {
	sound_bell();
    }
    check_message();
    if (ch == '\033') {
	return;
    }

    found = 0;
    dp = dlist;
    for (op = dobj; op->type; op++) {
	if (ch != op->ch && ch != '*') {
	    continue;
	}
	for (i = 0; i < op->max; i++) {
	    j = op->id[i].id_status;
	    if (j == IDENTIFIED || j == CALLED) {
		dp->type = op->type;
		dp->no = i;
		dp->name = op->name;
		if (wizard || j == IDENTIFIED) {
		    dp->real = op->id[i].real;
#if defined( JAPAN )
		    dp->sub = "";
#endif /* JAPAN */
		} else {
		    dp->real = op->id[i].title;
#if defined( JAPAN )
		    dp->sub = mesg[34];
#endif /* JAPAN */
		}
#if !defined( JAPAN )
		if (op->type == WAND && is_wood[i]) {
		    dp->name = "staff ";
		}
#endif /* not JAPAN */
		found |= op->type;
		dp++;
	    }
	}
	if ((found & op->type) == 0) {
	    dp->type = op->type;
	    dp->no = -1;
	    dp->name = op->name;
	    dp++;
	}
	dp->type = 0;
	dp++;
    }
    enddp = dp;

    if (found == 0) {
	message(mesg[46], 0);
	return;
    }

    dp = dlist;

nextpage:
    i = 0;
    maxlen = len;
    while (dp < enddp && i < ROGUE_LINES - 2) {
	p = descs[i];
	if (dp->type == 0) {
	    (void) strcpy(p, "");
	} else if (dp->no < 0) {
	    (void) sprintf(p, mesg[47], dp->name);
#if !defined( JAPAN )
	    //descs[i][strlen(p) - 1] = 's';
	    descs[i][utf8strlen(p) - 1] = 's';
#endif /* not JAPAN */
	} else {
#if defined( JAPAN )
	    (void) strcpy(p, "  ");
	    (void) strcat(p, dp->real);
	    (void) strcat(p, dp->sub);
	    (void) strcat(p, dp->name);
#else /* not JAPAN */
	    p[0] = ' ';
	    (void) strcpy(p + 1, dp->name);
	    (void) strcat(p, dp->real);
	    p[1] -= 'a' - 'A';
#endif /* not JAPAN */
	}
	//if ((n = strlen(p)) > maxlen) {
	if ((n = utf8strlen(p)) > maxlen) {
	    maxlen = n;
	}
	i++;
	dp++;
    }

    if (i == 0 || (i == 1 && !descs[0][0])) {
	/*
	 * can be here only in 2nd pass (exactly one page)
	 */
	return;
    }

    strcpy(descs[i++], msg);
    col = ROGUE_COLUMNS - (maxlen + 2);
    for (row = 0; row < i; row++) {
	if (row > 0) {
	    for (j = col; j < ROGUE_COLUMNS; j++) {
		descs[row - 1][j - col] = mvinch_rogue(row, j);
	    }
	    descs[row - 1][j - col] = 0;
	}
	mvaddstr_rogue(row, col, descs[row]);
	clrtoeol();
    }
    refresh();
    wait_for_ack();

    move(0, 0);
    clrtoeol();
#if defined( COLOR )
    for (j = 1; j < i; j++) {
	move(j, col);
	for (p = descs[j - 1]; *p; p++) {
	    addch_rogue(*p);
	}
    }
#else /* not COLOR */
#if !defined( JAPAN )			/* if.. by Yasha */
    for (j = 1; j < i; j++) {
	mvaddstr_rogue(j, col, descs[j - 1]);
    }
#else /* JAPAN */
    for (j = 1; j < i; j++) {	/* by Yasha */
	move(j, col);		/* by Yasha */
	clrtoeol();		/* by Yasha */
	addstr_rogue(descs[j - 1]);	/* by Yasha */
    }				/* by Yasha */
    move(ROGUE_LINES - 1, 0);		/* by Yasha */
    clrtoeol();			/* by Yasha */
    print_stats(STAT_ALL);	/* by Yasha */
#endif /* JAPAN */
#endif /* not COLOR */

    if (dp < enddp) {
	goto nextpage;
    }
}
#endif

/* MZ-700/1500固有 */
static uint8_t append_number(uint8_t *buffer, uint8_t length, short value, boolean plus)
{
    if (value < 0) {
        buffer[length++] = DC_MINUS;
        value = -value;
    } else if (plus) {
        buffer[length++] = DC_PLUS;
    }
    return length + mz_number(buffer + length, (unsigned short)value);
}

static uint8_t append_message(uint8_t *buffer, uint8_t length, short msg_id)
{
    return length + get_message(msg_id, buffer + length,
                                ROGUE_COLUMNS - length);
}

static uint8_t append_text(uint8_t *buffer, uint8_t length, const char *text)
{
    while (*text && length < ROGUE_COLUMNS - 1) {
        buffer[length++] = (uint8_t)*text++;
    }
    buffer[length] = '\0';
    return length;
}

static uint8_t inventory_col(void)
{
    if (rogue.col < 30) return 0;
    if (rogue.col < 50) return 20;
    return 40;
}

/* 持ち物表示場所の保存 */
static void save_inventory_rows(uint8_t col, uint8_t rows)
{
    uint8_t row;

    for (row = 0; row < rows; row++) {
        memcpy(descs_text[row],
               dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col, 40);
        memcpy(descs_attr[row],
               dungeon_attr + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               40);
    }
}

/* 持ち物表示場所の復元 */
static void restore_inventory_rows(uint8_t col, uint8_t rows)
{
    uint8_t row;

    for (row = 0; row < rows; row++) {
        memcpy(dungeon + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               descs_text[row], 40);
        memcpy(dungeon_attr + (unsigned int)(row + 1) * ROGUE_COLUMNS + col,
               descs_attr[row], 40);
    }
}
