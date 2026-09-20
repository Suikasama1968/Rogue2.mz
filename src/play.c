/*
 * play.c
 *
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
#include "init.h"
#include "level.h"
#include "message.h"
#include "move.h"
#include "pack.h"
#include "play.h"
#include "ring.h"
#include "trap.h"
#include "throw.h"
#include "use.h"
#include "zap.h"

extern boolean trap_door;

void
play_level(void)
{
    int key;

    for (;;) {
        show_hit_message();
        if (trap_door) {
            trap_door = 0;
            return;
        }
        display_dungeon();
        key = rgetchar();
        check_message();

        switch (key) {
        case '.':
            rest(1);
            break;
        case 's':
            search(1, 0);
            break;
        case 'i':
            inventory(&rogue.pack, ALL_OBJECTS);
            break;
#if 0 /* MZ-700/1500では未対応 */
        case 'f':
	        fight(0);
	        break;
	    case 'F':
	        fight(1);
	        break;
#endif
        case 'h':
        case 'j':
        case 'k':
        case 'l':
        case 'y':
        case 'u':
        case 'n':
        case 'b':
            one_move_rogue(key, 1);
            break;
        case 'e':
            eat();
            break;
        case 'q':
            quaff();
            break;
        case 'r':
            read_scroll();
            break;
        case 'd':
            drop();
            break;
        case 't':
            throw();
            break;
        case 'z':
            zapp();
            break;
        case 'w':
            wield();
            break;
        case 'W':
            wear();
            break;
        case 'T':
            take_off();
            break;
        case 'P':
            put_on_ring();
            break;
        case 'R':
            remove_ring();
            break;
        case '>':
            if (drop_check()) {
                return;
            }
            break;
        case '<':
            if (check_up()) {
                return;
            }
            break;
        case 'Q':
            byebye();
            break;
#if 0 /* MZ-700/1500では未対応 */
        case 'H':
        case 'J':
        case 'K':
        case 'L':
        case 'B':
        case 'Y':
        case 'U':
        case 'N':
        case CTRL('H'):
        case CTRL('J'):
        case CTRL('K'):
        case CTRL('L'):
        case CTRL('Y'):
        case CTRL('U'):
        case CTRL('N'):
        case CTRL('B'):
            multiple_move_rogue(key);
            break;
        case 'm':
            move_onto();
            break;
        case CTRL('P'):
            remessage();
            break;
        case CTRL('W'):
            wizardize();
            break;
        case ')':
        case ']':
            inv_armor_weapon(key == ')');
            break;
        case 'c':
            call_it();
            break;
        case CTRL('A'):
            show_average_hp();
            break;
        case CTRL('C'):
            if (wizard) {
                new_object_for_wizard();
            }
            break;
        case ',':
            kick_into_pack();
            break;
        case '?':
            help();
            break;
        case 'D':
            discovered();
            break;
        case '/':
            identify();
            break;
        case 'o':
            options();
            break;
        case '!':
            doshell();
            break;
#endif
        default:
            break;
        }
    }
}

#if 0 /* MZ-700/1500では未対応 */
/*
 * help
 * ヘルプメッセージを表示する
 */
void
help(void)
{
    int lines, columns;
    int n;
    char disp_message[ROGUE_COLUMNS+1];

    /* 現在の画面を保存する */
    for (lines = 0; lines < ROGUE_LINES; lines++) {
	for (columns = 0; columns < ROGUE_COLUMNS; columns++) {
	    descs[lines][columns] = mvinch_rogue(lines, columns);
	}
    }

    /* ヘルプメッセージを表示する */
    clear();
    for (n = 0; help_message[n]; n++) {
	mvaddstr_rogue(n, 0, help_message[n]);
	clrtoeol();
    }
    refresh();
    wait_for_ack();

    /* 保持した画面を復帰させる */
    for (lines = 0; lines < ROGUE_LINES; lines++) {
	move(lines, 0);
	if (lines > 0 && lines < ROGUE_LINES - 1) {
	    for (columns = 0; columns < ROGUE_COLUMNS; columns++) {
		addch_rogue(descs[lines][columns]);
	    }
	} else {
	    /* メッセージデータの最後に末端記号を付加する */
	    strncpy(disp_message, descs[lines], ROGUE_COLUMNS);
	    disp_message[ROGUE_COLUMNS] = '\0';

	    addstr_rogue(disp_message);
	}
    }
    refresh();
}

void
identify(void)
{
    short ch, n;
    char *p, buf[80];

    message(mesg[155], 0);

again:
    ch = rgetchar();
    if (ch == '\033') {
	check_message();
	return;
    } else if ((ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z')) {
	check_message();
	if (ch >= 'a' && ch <= 'z')
	    ch += 'A' - 'a';
	p = m_names[ch - 'A'];
    } else if ((n = r_index("@.|-+#%^*:])?!/=,", ch, 0)) != -1) {
	check_message();
	p = o_names[n];
    } else {
	sound_bell();
	goto again;
    }
    sprintf(buf, "'%c': %s", ch, p);
    message(buf, 0);
}

/*
 * options
 * オプション画面の表示と設定を行う
 */
void
options(void)
{
    int lines, columns;
    int n, i, j, changed;
    short ch;
    short pos[ROGUE_LINES];
    boolean bbuf[ROGUE_LINES];
    char cbuf[ROGUE_LINES][MAX_TITLE_LENGTH];
    char optbuf[BUFSIZ];

    /* 現在の画面を保存する */
    for (lines = 0; lines < ROGUE_LINES; lines++) {
	for (columns = 0; columns < ROGUE_COLUMNS; columns++) {
	    descs[lines][columns] = mvinch_rogue(lines, columns);
	}
    }

    /* オプション画面を表示する */
    clear();
    for (n = 0; envopt[n].name; n++) {
	mvaddstr_rogue(n, 2, optdesc[n]);
	addstr_rogue(" (\"");
	addstr_rogue(envopt[n].name);
	addstr_rogue("\"): ");
	if (envopt[n].bp) {
	    bbuf[n] = *(envopt[n].bp);
	    addstr_rogue(bbuf[n] ? "Yes" : "No");
	} else {
	    strcpy(cbuf[n], *(envopt[n].cp));
	    if (envopt[n].ab) {
		//i = strlen(cbuf[n]);
		i = utf8strlen(cbuf[n]);
		cbuf[n][i - 1] = 0;
	    }
	    addstr_rogue(cbuf[n]);
	}
	//pos[n] = strlen(optdesc[n]) + strlen(envopt[n].name) + 7;
	pos[n] = utf8strlen(optdesc[n]) + utf8strlen(envopt[n].name) + 7;
    }

    /* オプションの設定 */
    i = 0;
    while (i >= 0 && i < n) {
	mvaddch(i, 1, '>');
	if (envopt[i].bp) {
	    move(i, pos[i] + 2);
	} else {
	    //move(i, pos[i] + strlen(cbuf[i]) + 2);
	    move(i, pos[i] + utf8strlen(cbuf[i]) + 2);
	}
	refresh();
	ch = rgetchar();
	if (ch == CANCEL)
	    break;
	if (r_index("-\r\n", ch, 0) > -1) {
	    getyx(stdscr, lines, columns);
	    mvaddch(i, 1, ' ');
	    move(lines, columns);
	    if (envopt[i].bp) {
		addstr_rogue(bbuf[i] ? "Yes" : "No");
		clrtoeol();
	    }
	    if (ch == '-') {
		i--;
	    } else {
		i++;
	    }
	    continue;
	}
	if (envopt[i].bp) {
	    if (ch >= 'A' && ch <= 'Z') {
		ch += 'a' - 'Z';
	    }
	    if (ch != 'y' && ch != 'n') {
		mvaddstr_rogue(i, pos[i] + 2, "(Yes or No)");
		continue;
	    }
	    mvaddch(i, 1, ' ');
	    move(i, pos[i] + 2);
	    addstr_rogue((ch == 'y') ? "Yes" : "No");
	    clrtoeol();
	    bbuf[i] = (ch == 'y');
	    i++;
	} else {
	    j = input_line(i, pos[i] + 2, cbuf[i], optbuf, ch);
	    mvaddch(i, 1, ' ');
	    if (j < 0) {
		break;
	    }
	    strcpy(cbuf[i], optbuf);
	    i++;
	}
    }
    changed = (i < 0 || i >= n);
    if (changed) {
	move(n + 1, 0);
#if defined( JAPAN )
	addstr_rogue("＝スペースを押してください＝");
#else /* JAPAN */
	addstr_rogue("--Press space to continue--");
#endif /* JAPAN */
	refresh();
	wait_for_ack();
    }

    if (changed) {
	optbuf[0] = 0;
	for (i = 0; i < n; i++) {
	    strcat(optbuf, ",");
	    if (envopt[i].bp) {
		if (!bbuf[i]) {
		    strcat(optbuf, "no");
		}
		strcat(optbuf, envopt[i].name);
	    } else {
		strcat(optbuf, envopt[i].name);
		strcat(optbuf, ":");
		strcat(optbuf, cbuf[i]);
	    }
	}
	set_opts(optbuf);
    }
    print_stats(STAT_ALL);

    init_color_attr();

#if !defined( JAPAN )			/* #if.. by Yasha */
    for (lines = 0; lines < ROGUE_LINES; lines++) {
#else /* not JAPAN */
    for (lines = 0; lines < ROGUE_LINES - 1; lines++) {	/* by Yasha */
#endif /* JAPAN */
	move(lines, 0);
	for (columns = 0; columns < ROGUE_COLUMNS; columns++) {
	    if (lines == ROGUE_LINES - 1 && columns == ROGUE_COLUMNS - 1) {
		continue;
	    }
	    if (lines < MIN_ROW || lines >= ROGUE_LINES - 1) {
		addch_rogue((unsigned char) descs[lines][columns]);
	    } else {
		addch_rogue(descs[lines][columns]);
	    }
	}
    }

    refresh();
}

void
doshell(void)
{
    char *cmd;

    if ((cmd = getenv("SHELL")) == NULL) {
	cmd = "/bin/sh";
    }
    move(ROGUE_LINES - 1, 0);
    refresh();
    stop_window();
    if (*org_dir) {
	chdir(org_dir);
    }
    md_ignore_signals();
    printf(mesg[157]);
    printf("\r\n");
    system(cmd);
    md_heed_signals();
    if (game_dir && *game_dir) {
	chdir(game_dir);
    }
    start_window();
    wrefresh(curscr);
}
#endif
