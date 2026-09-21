/*
 * hit.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include "main.h"
#include "rogue.h"
#include "hit.h"
#include "level.h"
#include "message.h"
#include "monster.h"
#include "object.h"
#include "random.h"
#include "score.h"
#include "spechit.h"
#include "mz_system.h"

extern short add_strength;
extern short ring_exp, r_rings;
extern boolean being_held;

#define hit_message ((uint8_t *)HIT_MESSAGE_ADDR)

static const uint8_t weapon_damage_n[WEAPONS] = { 1, 1, 1, 1, 1, 2, 3, 4 };
static const uint8_t weapon_damage_s[WEAPONS] = { 1, 1, 2, 3, 4, 3, 4, 5 };

static void get_monster_name(object *monster, uint8_t *name, short size);
static void append_hit_message(short msg_id, const uint8_t *text);

void
mon_hit(object *monster, char *other, boolean flame)
{
    int damage, hit_chance;
    int i;
    uint16_t packed_damage;
    uint8_t *name = (uint8_t *)TEMP_BUFFER_ADDR;
    const uint8_t *attacker;

    if (other) {
        attacker = (const uint8_t *)other;
    } else {
        get_monster_name(monster, name, 20);
        attacker = name;
    }
    hit_chance = monster->m_hit_chance;
    hit_chance -= (((2 * rogue.exp) + (2 * ring_exp)) - r_rings);
    if (other) {
        hit_chance -= ((rogue.exp + ring_exp) - r_rings);
    }
    if (!rand_percent(hit_chance)) {
        append_hit_message(18, attacker);
        show_hit_message();
        return;
    }

    if (monster->m_flags & STATIONARY) {
        damage = monster->stationary_damage++;
    } else {
        damage = 0;
        packed_damage = monster->m_damage;
        for (i = 0; i < (packed_damage & 0x0f); i++) {
            damage += get_rand(1, (packed_damage >> 4) & 0x0f);
        }
        for (i = 0; i < ((packed_damage >> 8) & 0x0f); i++) {
            damage += get_rand(1, (packed_damage >> 12) & 0x0f);
        }
        if (flame && (damage -= get_armor_class(rogue.armor)) < 0) {
            damage = 1;
        }
        damage -= (damage * get_armor_class(rogue.armor) * 3) / 100;
    }
    append_hit_message(19, attacker);
    show_hit_message();
    if (damage > 0) rogue_damage(damage, monster);
    if (monster->m_flags & SPECIAL_HIT) special_hit(monster);
}

void
rogue_hit(object *monster, boolean force_hit)
{
    int damage, hit_chance;

    if (check_imitator(monster)) {
        return;
    }
    hit_chance = force_hit ? 100 : get_hit_chance(rogue.weapon);
#if 0 /* MZ-700/1500では未サポート */
    if (wizard) {
	    hit_chance *= 2;
	}
#endif
    if (!rand_percent(hit_chance)) {
        (void)format_message(22, 0, hit_message, HIT_MESSAGE_SIZE);
    } else {
        damage = get_weapon_damage(rogue.weapon);
#if 0 /* MZ-700/1500では未サポート */
        if (wizard) {
	        damage *= 3;
    	}
#endif
        if (mon_damage(monster, damage)) { /* still alive? */
            (void)format_message(23, 0, hit_message, HIT_MESSAGE_SIZE);
        }
    }
    check_gold_seeker(monster);
    wake_up(monster);
}

void
rogue_damage(short d, object *monster)
{
#if defined(DEBUG)
    (void)d;
    (void)monster;
    return;
#else
    if (d >= rogue.hp_current) {
        rogue.hp_current = 0;
        print_stats(STAT_HP);
        killed_by(monster, 0);
        return;
    }
    rogue.hp_current -= d;
    print_stats(STAT_HP);
#endif
}

int
get_damage(char *ds, boolean r)
{
    int i = 0, j, n, d, total = 0;

    while (ds[i]) {
        n = get_number(ds + i);
        while (ds[i++] != 'd')
            continue;
        d = get_number(ds + i);
        while ((ds[i] != '/') && ds[i]) {
            i++;
        }

        for (j = 0; j < n; j++) {
            if (r) {
                total += get_rand(1, d);
            } else {
                total += d;
            }
        }
        if (ds[i] == '/') {
            i++;
        }
    }
    return total;
}

int
get_w_damage(object *obj)
{
    int i;
    int damage = 0;

    if (!obj || obj->what_is != WEAPON || obj->which_kind >= WEAPONS) {
        return -1;
    }
    for (i = 0; i < weapon_damage_n[obj->which_kind] + obj->hit_enchant; ++i) {
        damage += get_rand(1, weapon_damage_s[obj->which_kind] + obj->d_enchant);
    }
    return damage;
}

int
get_number(char *s)
{
    int total = 0;

    while (*s >= '0' && *s <= '9') {
        total = (10 * total) + (*s++ - '0');
    }
    return total;
}

#if 0 /* MZ-700/1500では未サポート */
long
lget_number(char *s)
{
    long total = 0;

    while (*s >= '0' && *s <= '9') {
        total = (10 * total) + (*s++ - '0');
    }
    return total;
}
#endif

int
to_hit(object *obj)
{
    if (!obj || obj->what_is != WEAPON || obj->which_kind >= WEAPONS) {
        return 1;
    }
    return weapon_damage_n[obj->which_kind] + obj->hit_enchant;
}

int
damage_for_strength(void)
{
    short strength;
    int i;
    static const short sa[] = { 14, 17, 18, 20, 21, 30, 9999 };
    static const short ra[] = { 1, 3, 4, 5, 6, 7, 8 };

    strength = rogue.str_current + add_strength;
    if (strength <= 6) {
        return (strength - 5);
    }
    i = 0;
    for (;;) {
        if (strength <= sa[i]) {
            return (int) ra[i];
        }
    i++;
    }
}

int
mon_damage(object *monster, int damage)
{
    uint8_t *name = (uint8_t *)TEMP_BUFFER_ADDR;
    short kill_exp;

    monster->hp_to_kill -= damage;

    if (monster->hp_to_kill > 0) {
        return 1;
    }
    get_monster_name(monster, name, 20);
    kill_exp = monster->kill_exp;
    if (monster->m_flags & HOLDS) {
        being_held = 0;
    }
    remove_monster(monster);
    cough_up(monster);
    append_hit_message(24, name);
    show_hit_message();
    add_exp(kill_exp, 1);
    return 0;
}

#if 0 /* MZ-700/1500では未対応 */
void
fight(boolean to_the_death)
{
    short ch, c;
    short row, col;
    short possible_damage;
    object *monster;

    ch = get_direction();
    if (ch == CANCEL) {
	return;
    }
    row = rogue.row;
    col = rogue.col;
    get_dir_rc(ch, &row, &col, 0);

    c = mvinch_rogue(row, col);
    if (((c < 'A') || (c > 'Z')) ||
	(!can_move(rogue.row, rogue.col, row, col))) {
	    message(mesg[25], 0);
	    return;
    }
    if (!(fight_monster = object_at(&level_monsters, row, col))) {
	    return;
    }
    if (!(fight_monster->m_flags & STATIONARY)) {
	    possible_damage = ((get_damage(fight_monster->m_damage, 0) * 2) / 3);
    } else {
	    possible_damage = fight_monster->stationary_damage - 1;
    }
    while (fight_monster) {
	    (void) one_move_rogue(ch, 0);
	    if (((!to_the_death) && (rogue.hp_current <= possible_damage)) ||
	        interrupted || (!(dungeon[row][col] & MONSTER))) {
	        fight_monster = 0;
	    } else {
	        monster = object_at(&level_monsters, row, col);
	        if (monster != fight_monster) {
		       fight_monster = 0;
	        }
	    }
    }
}
#endif

void
get_dir_rc(short dir, short *row, short *col, short allow_off_screen)
{
    switch (dir) {
    case 'h':
        if (allow_off_screen || (*col > 0)) {
            (*col)--;
        }
        break;
    case 'j':
        if (allow_off_screen || (*row < (ROGUE_LINES - 2))) {
            (*row)++;
        }
        break;
    case 'k':
        if (allow_off_screen || (*row > MIN_ROW)) {
            (*row)--;
        }
        break;
    case 'l':
        if (allow_off_screen || (*col < (ROGUE_COLUMNS - 1))) {
            (*col)++;
        }
        break;
    case 'y':
        if (allow_off_screen || ((*row > MIN_ROW) && (*col > 0))) {
            (*row)--; 
            (*col)--;
        }
        break;
    case 'u':
        if (allow_off_screen || ((*row > MIN_ROW) && (*col < (ROGUE_COLUMNS - 1)))) {
            (*row)--;
            (*col)++;
        }
        break;
    case 'n':
        if (allow_off_screen || ((*row < (ROGUE_LINES - 2)) && (*col < (ROGUE_COLUMNS - 1)))) {
            (*row)++;
            (*col)++;
        }
        break;
    case 'b':
        if (allow_off_screen || ((*row < (ROGUE_LINES - 2)) && (*col > 0))) {
            (*row)++;
            (*col)--;
        }
        break;
    }
}

int
get_hit_chance(object *weapon)
{
    short hit_chance;

    hit_chance = 40 + 3 * to_hit(weapon);
    hit_chance += (((2 * rogue.exp) + (2 * ring_exp)) - r_rings);
    return hit_chance;
}

int
get_weapon_damage(object *weapon)
{
    short damage;

    damage = get_w_damage(weapon) + damage_for_strength();
    damage += ((((rogue.exp + ring_exp) - r_rings) + 1) / 2);
    return damage;
}

/* MZ-700/1500固有 */
static void
get_monster_name(object *monster, uint8_t *name, short size)
{
    get_message(get_monster_name_id(monster), name, size);
}

void
show_hit_message(void)
{
    if (hit_message[0]) {
        message((char *)hit_message, 1);
        hit_message[0] = '\0';
    }
}

static void
append_hit_message(short msg_id, const uint8_t *text)
{
    short length = 0;

    while (hit_message[length]) length++;
    if (length && length < HIT_MESSAGE_SIZE - 1) {
        hit_message[length++] = DC_SPC;
        hit_message[length] = '\0';
    }
    (void)format_message(msg_id, text, hit_message + length,
                         HIT_MESSAGE_SIZE - length);
}
