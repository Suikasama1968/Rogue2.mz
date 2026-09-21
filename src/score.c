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
#include "main.h"
#include "machdep.h"
#include "message.h"
#include "monster.h"
#include "pack.h"
#include "score.h"
#include "mz_curses.h"

void
killed_by(object *monster, short other)
{
#if defined(DEBUG)
    (void)monster;
    (void)other;
    rogue.hp_current = 1;
    print_stats(STAT_HP);
    return;
#else
    uint8_t reason[40];
    short length;
    short suffix_length;
    short i;
    uint8_t text[40];
    uint8_t stats[48];
    long values[3];

    rogue.hp_current = 0;
    clear();

    /* MZ-700/1500固有 */
    /* メッセージをファイル化 */
    for ( i = 0; i < 14; i++) {
        (void)get_message(500 + i, text, sizeof(text));
        mvaddstr((uint8_t)(i + 3), 5, text);
    }

    length = 0;

    if (other != QUIT) {
        rogue.gold = ((rogue.gold * 9L) / 10L);
    }   

    if (other){
        length = get_message(167 + other, reason, sizeof(reason));
    }else{
        length = get_message(get_monster_name_id(monster), reason,
                             sizeof(reason));
        suffix_length = get_message(176, reason + length,
                                    sizeof(reason) - length);
        if (suffix_length > 0 && reason[length] == MESSAGE_FORMAT_STRING) {
            for (i = 0; i < suffix_length; ++i) {
                reason[length + i] = reason[length + i + 1];
            }
            suffix_length++;
        }
        length += suffix_length;
    }

    length = mz_display_length(reason);
    mvaddstr(12, (uint8_t)((40 - length) / 2), reason);

    values[0] = cur_level;
    values[1] = rogue.gold;
    values[2] = rogue.exp_points;
    mz_sprintf(stats, 526, values);
    length = mz_display_length(stats);
    mvaddstr(18, (uint8_t)((40 - length) / 2), stats);

    (void)get_message(517, text, sizeof(text));
    mvaddstr(20, 11, text);

    rogue.row = 0;
    rogue.col = 0;
    refresh();
    wait_for_ack();
    restart_rogue();
#endif
}

void
win(void)
{
    uint8_t text[41];
    short id;

    clear();

    /* MZ-700/1500固有 */
    /* メッセージをファイル化 */
    for (id = 520; id <= 524; id++) {
        (void)get_message(id, text, sizeof(text));
        mvaddstr((uint8_t)(id - 517), 0, text);
    }

    for (id = 182; id <= 185; id++) {
        (void)get_message(id, text, sizeof(text));
        mvaddstr((uint8_t)(id - 172), 3, text);
    }

    (void)get_message(517, text, sizeof(text));
    mvaddstr(20, 0, text);
    rogue.row = 0;
    rogue.col = 0;
    refresh();
    wait_for_ack();
    md_exit(0);
}

#if 0 /* MZ-700/1500では未対応 */
void
mvaddbanner(int row, int col, int *ban)
{
    int i;
#if defined( COLOR )
    int rev = ' ' | (RGREEN << 8);
#endif /* COLOR */

    move(row, col);
    for (i = 0; i < 59; i++) {
	if (ban[i >> 3] & (0x80 >> (i & 7))) {
#if defined( COLOR )
	    addch_rogue(rev);
#else /* not COLOR */
	    addch_rogue('@');
#endif /* not COLOR */
	} else {
	    addch_rogue(' ');
	}
    }
}

void
quit(boolean from_intrpt)
{
    char buf[128];
    short i, orow = 0, ocol = 0;	/* 初期化されず使われている変数のため 0 を代入 */
    boolean mc = 0;

    md_ignore_signals();

    if (from_intrpt) {

	orow = rogue.row;
	ocol = rogue.col;
	mc = msg_cleared;

	for (i = 0; i < ROGUE_COLUMNS; i++) {
	    buf[i] = mvinch_rogue(0, i);
	}
    }
    check_message();
#if defined( JAPAN )
    message("ゲームを終了してよいのですか？", 1);
#else /* not JAPAN */
    message("Really quit?", 1);
#endif /* not JAPAN */
    if (rgetchar() != 'y') {
	md_heed_signals();
	check_message();
	if (from_intrpt) {
	    for (i = 0; i < ROGUE_COLUMNS; i++) {
		mvaddch_rogue(0, i, (unsigned char) buf[i]);
	    }
	    msg_cleared = mc;
	    move(orow, ocol);
	    refresh();
	}
	return;
    }
    if (from_intrpt) {
	clean_up(mesg[12]);	/* byebye_string */
    }
    check_message();
    killed_by((object *) 0, QUIT);
}

void
put_scores(object *monster, short other)
{
    short i, n, ne, rank;
#if !defined( TOPSCO )
    short found_pos;
#endif /* not TOPSCO */
    char scores[10][82], n_names[10][30];
    char *p, buf[100], file[100];
    FILE *fp;

    fp = NULL;
    if ((!game_dir || !*game_dir) && (p = md_ghome()) != NULL) {
	p = strcpy(file, p);
	while (*p) {
	    p++;
	}
	if (p[-1] != '/') {
	    *p++ = '/';
	}
	strcpy(p, score_file);
	if ((fp = fopen(file, "rb+")) == NULL) {
	    fp = fopen(file, "wb+");
	}
    }
    if (fp == NULL) {
	strcpy(file, score_file);
	if ((fp = fopen(file, "rb+")) == NULL) {
	    fp = fopen(file, "wb+");
	}
    }
    if (fp == NULL) {
	message(mesg[186], 0);
	sf_error();
    }
    (void) xxx(1);
    for (i = 0; i < 10; i++) {
	if ((n = fread(scores[i], sizeof(char), 80, fp)) == 0) {
	    break;
	}
	if (n < 80) {
	    sf_error();
	}
	xxxx(scores[i], 80);
	if ((n = fread(n_names[i], sizeof(char), 30, fp)) < 30) {
	    sf_error();
	}
	xxxx(n_names[i], 30);
    }
    fclose(fp);
    ne = i;

#if !defined( TOPSCO )
    found_pos = -1;
    for (i = 0; i < ne && !score_only; i++) {
	if (name_cmp(scores[i] + 15, login_name)) {
	    continue;
	}
	for (p = scores[i] + 5; *p == ' '; p++)
	    continue;
	if (rogue.gold > lget_number(p)) {
	    found_pos = i;
	} else {
	    score_only = 1;
	}
    }
    if (found_pos != -1) {
	ne--;
	for (i = found_pos; i < ne; i++) {
	    (void) strcpy(scores[i], scores[i + 1]);
	    (void) strcpy(n_names[i], n_names[i + 1]);
	}
    }
#endif /* not TOPSCO */
    rank = 10;
    if (!score_only) {
	for (i = 0; i < ne; i++) {
	    for (p = scores[i] + 5; *p == ' '; p++)
		continue;
	    if (rogue.gold <= lget_number(p)) {
		continue;
	    }
	    rank = i;
	    break;
	}
	if (ne == 0) {
	    rank = 0;
	} else if (ne < 10 && rank == 10) {
	    rank = ne;
	}
	if (rank < 10) {
	    insert_score(scores, n_names, nick_name, rank, ne, monster, other);
	    if (ne < 10) {
		ne++;
	    }
	}
    }

    md_ignore_signals();
    clear();
#if defined( JAPAN )
    mvaddstr_rogue(3, 20, mesg[187]);
#else /* not JAPAN */
    mvaddstr_rogue(3, 25, mesg[187]);
#endif /* not JAPAN */
    mvaddstr_rogue(6, 0, mesg[188]);
#if defined( COLOR )
    standend();
#endif /* COLOR */
    for (i = 0; i < ne; i++) {
	scores[i][1] = (i == 9) ? '1' : ' ';
	scores[i][2] = (i == 9) ? '0' : '1' + i;
	nickize(buf, scores[i], n_names[i]);
	if (i == rank) {
	    attron(A_REVERSE);
	    mvaddstr_rogue(i + 8, 0, buf);
	    attroff(A_REVERSE);
	} else {
	    mvaddstr_rogue(i + 8, 0, buf);
	}
    }
    refresh();
    if (rank < 10) {
	if ((fp = fopen(file, "wb")) == NULL) {
	    message(mesg[186], 0);
	    sf_error();
	}
	(void) xxx(1);
	for (i = 0; i < ne; i++) {
	    xxxx(scores[i], 80);
	    fwrite(scores[i], sizeof(char), 80, fp);
	    xxxx(n_names[i], 30);
	    fwrite(n_names[i], sizeof(char), 30, fp);
	}
	fclose(fp);
    }
    message("", 0);
    clean_up("");
}

void
insert_score(char scores[][82], char n_names[][30], char *n_name, short rank,
	     short n, object *monster, int other)
{
    short i;
    char *p = NULL;		/* 初期化されず使用される自動変数を初期化します。 */
    char buf[82];

    if (n > 0) {
	for (i = n; i > rank; i--) {
	    if ((i < 10) && (i > 0)) {
		(void) strcpy(scores[i], scores[i - 1]);
		(void) strcpy(n_names[i], n_names[i - 1]);
	    }
	}
    }
    sprintf(buf, " %2d   %6ld   %s: ", rank + 1, rogue.gold, login_name);

    if (other != WIN) {
	if (has_amulet()) {
	    (void) strcat(buf, mesg[189]);
	}
	strcat(buf, mesg[190]);
#if !defined( ORIGINAL )
	znum(buf, cur_level, 0);
#else /* ORIGINAL */
	znum(buf, max_level, 0);
#endif /* ORIGINAL */
	strcat(buf, mesg[191]);
    }
    if (other) {
	switch (other) {
	case HYPOTHERMIA:
	    p = mesg[192];
	    break;
	case STARVATION:
	    p = mesg[193];
	    break;
	case POISON_DART:
	    p = mesg[194];
	    break;
	case QUIT:
	    p = mesg[195];
	    break;
	case WIN:
	    p = mesg[196];
	    break;
	}
	(void) strcat(buf, p);
    } else {
	(void) strcat(buf, m_names[monster->m_char - 'A']);
	(void) strcat(buf, mesg[197]);
    }
    strcat(buf, "。");
    //for (i = strlen(buf); i < 79; i++) {
    for (i = utf8strlen(buf); i < 79; i++) {
	buf[i] = ' ';
    }
    buf[79] = 0;
    (void) strcpy(scores[rank], buf);
    (void) strcpy(n_names[rank], n_name);
}

    int
is_vowel(short ch)
{
    return ((ch == 'a') ||
	    (ch == 'e') || (ch == 'i') || (ch == 'o') || (ch == 'u'));
}

void
sell_pack(void)
{
    object *obj;
    short row = 2, val;
    char buf[80];

    obj = rogue.pack.next_object;

    clear();
    mvaddstr_rogue(1, 0, mesg[198]);

    while (obj) {
	if (obj->what_is != FOOD) {
	    obj->identified = 1;
	    val = get_value(obj);
	    rogue.gold += val;

	    if (row < ROGUE_LINES) {
		sprintf(buf, "%5d      ", val);
		get_desc(obj, buf + 11, 1);
		mvaddstr_rogue(row++, 0, buf);
	    }
	}
	obj = obj->next_object;
    }
    refresh();
    if (rogue.gold > MAX_GOLD) {
	rogue.gold = MAX_GOLD;
    }
    message("", 0);
}

int
get_value(object *obj)
{
    short wc;
    int val;

    wc = obj->which_kind;

    switch (obj->what_is) {
    case WEAPON:
	val = id_weapons[wc].value;
	if ((wc == ARROW) || (wc == DAGGER) || (wc == SHURIKEN) ||
	    (wc == DART)) {
	    val *= obj->quantity;
	}
	val += (obj->d_enchant * 85) + (obj->hit_enchant * 85);
	break;
    case ARMOR:
	val = id_armors[wc].value + (obj->d_enchant * 75);
	if (obj->is_protected) {
	    val += 200;
	}
	break;
    case WAND:
	val = id_wands[wc].value * (obj->class + 1);
	break;
    case SCROL:
	val = id_scrolls[wc].value * obj->quantity;
	break;
    case POTION:
	val = id_potions[wc].value * obj->quantity;
	break;
    case AMULET:
	val = 5000;
	break;
    case RING:
	val = id_rings[wc].value * (obj->class + 1);
	break;
    default:
	val = 10;
    }
    if (val <= 0) {
	val = 10;
    }
    return val;
}

void
id_all(void)
{
    short i;

    for (i = 0; i < SCROLS; i++) {
	id_scrolls[i].id_status = IDENTIFIED;
    }
    for (i = 0; i < WEAPONS; i++) {
	id_weapons[i].id_status = IDENTIFIED;
    }
    for (i = 0; i < ARMORS; i++) {
	id_armors[i].id_status = IDENTIFIED;
    }
    for (i = 0; i < WANDS; i++) {
	id_wands[i].id_status = IDENTIFIED;
    }
    for (i = 0; i < POTIONS; i++) {
	id_potions[i].id_status = IDENTIFIED;
    }
}

    void
xxxx(char *buf, short n)
{
    short i;
#if !defined( ORIGINAL )
    char c;			/* char is already defined to be unsigned */
#else /* ORIGINAL */
    unsigned char c;
#endif /* ORIGINAL */

    for (i = 0; i < n; i++) {

	/* It does not matter if accuracy is lost during this assignment */
#if !defined( ORIGINAL )
	c = (char) xxx(0);
#else /* ORIGINAL */
	c = (unsigned char) xxx(0);
#endif /* ORIGINAL */

	buf[i] ^= c;
    }
}

long
xxx(boolean st)
{
    static long f, s;
    long r;

    if (st) {
	f = 37;
	s = 7;
	return (0L);
    }
    r = ((f * s) + 9337) % 8887;
    f = s;
    s = r;
    return r;
}

void
nickize(char *buf, char *score, char *n_name)
{
    short i = 15, j;

    if (!n_name[0]) {
	(void) strcpy(buf, score);
	return;
    }
    (void) strncpy(buf, score, 16);

    while (score[i] != ':') {
	i++;
    }

    (void) strcpy(buf + 15, n_name);
    //j = strlen(buf);
    j = utf8strlen(buf);

    while (score[i]) {
	buf[j++] = score[i++];
    }
    buf[j] = 0;
    buf[79] = 0;
}

void
center(short row, char *buf)
{
    short margin;

    //margin = ((ROGUE_COLUMNS - strlen(buf)) / 2);
    margin = ((ROGUE_COLUMNS - utf8strlen(buf)) / 2);
    mvaddstr_rogue(row, margin, buf);
}

void
sf_error(void)
{
    message("", 1);
    clean_up(mesg[199]);
}
#endif
