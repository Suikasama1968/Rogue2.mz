/*
 * init.c
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
#include "init.h"
#include "main.h"
#include "machdep.h"
#include "message.h"
#include "display.h"
#include "invent.h"
#include "move.h"
#include "object.h"
#include "pack.h"
#include "random.h"
#include "ring.h"
#include "mz_curses.h"
#include "mz_system.h"

extern short halluc, blind, confused, levitate, haste_self, extra_hp;
extern short less_hp;
extern boolean being_held;
extern boolean trap_door;
extern short bear_trap;
extern short new_level_message;

static void init_game_state(int seed);

int
init(void)
{
    if (read_mesg("MESG")) {
        return 1;
    }

    init_game();
    return 0;
}

void
player_init(void)
{
    object *obj;

    identified_potions = 0;

    obj = alloc_object();
    get_food(obj, 1);
    (void)add_to_pack(obj, &rogue.pack, 1);

    obj = alloc_object();               /* initial armor */
    obj->what_is = ARMOR;
    obj->which_kind = RINGMAIL;
    obj->class = RINGMAIL + 2;
    obj->is_protected = 0;
    obj->d_enchant = 1;
    (void)add_to_pack(obj, &rogue.pack, 1);
    do_wear(obj);

    obj = alloc_object();               /* initial weapons */
    obj->what_is = WEAPON;
    obj->which_kind = MACE;
    obj->hit_enchant = obj->d_enchant = 1;
    (void)add_to_pack(obj, &rogue.pack, 1);
    do_wield(obj);

    obj = alloc_object();
    obj->what_is = WEAPON;
    obj->which_kind = BOW;
    obj->hit_enchant = 1;
    obj->d_enchant = 0;
    (void)add_to_pack(obj, &rogue.pack, 1);

    obj = alloc_object();
    obj->what_is = WEAPON;
    obj->which_kind = ARROW;
    obj->quantity = get_rand(25, 35);
    obj->hit_enchant = 0;
    obj->d_enchant = 0;
    (void)add_to_pack(obj, &rogue.pack, 1);
}

void
byebye(void)
{
    message_id(12, 0);
    md_exit(0);
}

#if 0 /* MZ-700/1500では未対応 */

void
error_save(int sig)
{
    save_is_interactive = 0;
    save_into_file(error_file);
    clean_up("");
}

void
do_args(int argc, char *argv[])
{
    int ch;
    char *option_strings;
    extern int optind;

#if !defined( ORIGINAL )
    option_strings = "sr";
#else /* Not ORIGINAL */
    option_strings = "s";
#endif /* ORIGINAL */

    while ((ch = getopt(argc, argv, option_strings)) != EOF) {
	switch (ch) {
	case 's':
	    score_only = 1;
	    break;
#if !defined( ORIGINAL )
	case 'r':
	    do_restore = 1;
	    break;
#endif /* Not ORIGINAL */
	case '?':
	default:
	    usage();
	    break;
	}
    }

    argc -= optind;
    argv += optind;
    if (argc >= 3 || argc == 0) {
	usage();
	return;
    }

    if (read_mesg(argv[0])) {
	exit(1);
    }
#if !defined( ORIGINAL )
    if (argc == 2) {
	rest_file = argv[1];
    }
#endif /* Not ORIGINAL */

}

void
do_opts(void)
{
    char *ep, *p;
    char envname[10];
    char envbuf[BUFSIZ];

    strcpy(envname, "ROGUEOPT?");
    envbuf[0] = 0;
    for (p = "S123456789"; *p; p++) {
	envname[8] = *p;
	if ((ep = getenv(envname))) {
	    strcat(envbuf, ",");
	    strcat(envbuf, ep);
	}
    }
    set_opts(envbuf);
}

void
set_opts(char *env)
{
    short not;
    char *ep, *p;
    opt *op;
    char optname[20];

    if (*env == 0) {
	return;
    }
    ep = env;
    for (;;) {
	while (*ep == ' ' || *ep == ',') {
	    ep++;
	}

	if (*ep == 0) {
	    break;
	}

	not = 0;
	if (!strncmp("no", ep, 2) || !strncmp("NO", ep, 2)) {
	    not = 1;
	    ep += 2;
	}
	p = optname;
	while (*ep && *ep != ',' && *ep != '=' && *ep != ':') {
	    *p++ = (*ep >= 'A' && *ep <= 'Z') ? (*ep++) - 'A' + 'a' : *ep++;
	}
	*p = 0;
	for (op = envopt; op->name; op++) {
	    //if (strncmp(op->name, optname, strlen(optname))) {
	    if (strncmp(op->name, optname, utf8strlen(optname))) {
		continue;
	    }
	    if (op->bp) {
		*(op->bp) = !not;
	    }
	    if (op->cp && (*ep == '=' || *ep == ':')) {
		env_get_value(op->cp, ep + 1, op->ab, op->nc);
	    }
	}

	while (*ep && *ep != ',') {
	    ep++;
	}
    }

#if !defined( ORIGINAL )
    if (game_dir && *game_dir) {
	chdir(game_dir);
    }
#endif /* Not ORIGINAL */
}

void
env_get_value(char **s, char *e, boolean add_blank, boolean no_colon)
{
    int i = 0;
    char *t;

    t = e;

    while ((*e) && (*e != ',')) {
#if defined( EUC )
	/* EUC のマルチバイト文字は読み飛ばす */
	if (*e & 0x80) {
	    if ((*e >= '\xA1' && *e <= '\xFE')
		&& (*(e + 1) >= '\xA1' && *(e + 1) <= '\xFE')) {
		/* 漢字 */
		e += 2;
		i += 2;
	    } else if ((*e == '\x8E')
		       && ((*(e + 1) >= '\xA0') && (*(e + 1) <= '\xDF'))) {
		/* 半角カナ */
		e += 2;
		i += 2;
	    } else if ((*e == '\x8F')
		       && ((*(e + 1) >= '\xA1') && (*(e + 1) <= '\xFE'))
		       && ((*(e + 2) >= '\xA1') && (*(e + 2) <= '\xFE'))) {
		/* 補助漢字 */
		e += 3;
		i += 3;
	    } else {
		/* その他の領域 */
		e += 1;
		i += 1;
	    }
	    continue;
	}
#else /* not EUC */
	/* Shift JIS のマルチバイト文字は読み飛ばす */
	//if (*e > '\200' && *e < '\240' || *e >= '\340' && *e < '\360') {
	//    e += 2;
	//    i += 2;
	//    continue;
	//}
#endif /* not EUC */
	if (*e == ':' && no_colon) {
	    *e = ';';		/* ':' reserved for score file purposes */
	}
	e++;
	if (++i >= 30) {
	    break;
	}
    }

    /* 値入力 */
    if (!(*s = md_malloc(i + (add_blank ? 2 : 1)))) {
	clean_up(mesg[17]);
    }
    (void) strncpy(*s, t, i);
    if (add_blank) {
	(*s)[i++] = ' ';
    }
    (*s)[i] = '\0';
}
#endif
/* MZ-700/1500 固有処理 */
void
init_game(void)
{
    int seed;

    BANK_DRAM_L();
    seed = md_gseed();
    memset(LOW_RAM_BEGIN, 0x00, LOW_RAM_SIZE);
    memset((void *)OBJECT_POOL_ADDR, 0x00,
           OBJECT_POOL_SIZE + OBJECT_USED_SIZE);
    (void)initscr();
    init_color_attr();
    init_game_state(seed);
}

static void
init_game_state(int seed)
{
    reset_object_state();
    reset_message_state();

    cur_level = 0;
    max_level = 1;
    new_level_message = 0;
    reset_move_state();

    halluc = 0;
    blind = 0;
    confused = 0;
    levitate = 0;
    haste_self = 0;
    extra_hp = 0;
    less_hp = 0;
    being_held = 0;
    trap_door = 0;
    bear_trap = 0;

    (void) srrandom(seed);
    get_wand_and_ring_materials();
    make_scroll_titles();

    player_init();
    party_counter = get_rand(1, PARTY_TIME);
    ring_stats(0);
}
