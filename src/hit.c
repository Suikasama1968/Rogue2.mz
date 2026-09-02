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
#include "random.h"
#include "score.h"
#include "spechit.h"

boolean game_over;
extern short add_strength;

static const u8 weapon_damage_n[WEAPONS] = { 1, 1, 1, 1, 1, 2, 3, 4 };
static const u8 weapon_damage_s[WEAPONS] = { 1, 1, 2, 3, 4, 3, 4, 5 };

static void get_monster_name(object *monster, u8 *name, short size)
{
    get_message(monster->m_name_id, name, size);
}

void
mon_hit(object *monster, char *other, boolean flame)
{
    int damage, hit_chance;
    int i;
    u8 name[20];

    (void)other;
    (void)flame;

    get_monster_name(monster, name, sizeof(name));
    hit_chance = monster->m_hit_chance - 2 * rogue.exp;
    if (!rand_percent(hit_chance)) {
        message_id_mz(18, name);
        return;
    }

    damage = 0;
    for (i = 0; i < monster->m_damage_n1; ++i) {
        damage += get_rand(1, monster->m_damage_s1);
    }
    for (i = 0; i < monster->m_damage_n2; ++i) {
        damage += get_rand(1, monster->m_damage_s2);
    }
    damage -= (damage * rogue.armor_class * 3) / 100;
    message_id_mz(19, name);
    if (damage > 0) rogue_damage((short)damage, monster);
    if (!game_over && (monster->m_flags & SPECIAL_HIT)) special_hit(monster);
}

void
rogue_hit(object *monster, boolean force_hit)
{
    int damage, hit_chance;

    hit_chance = force_hit ? 100 : get_hit_chance(rogue.weapon);

    if (!rand_percent(hit_chance)) {
        message_id_mz(22, 0);
        return;
    }
    damage = get_weapon_damage(rogue.weapon);

    if (mon_damage(monster, damage)) { /* still alive? */
        message_id_mz(23, 0);
    }
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

long
lget_number(char *s)
{
    long total = 0;

    while (*s >= '0' && *s <= '9') {
        total = (10 * total) + (*s++ - '0');
    }
    return total;
}

int
to_hit(object *obj)
{
    if (!obj || obj->what_is != WEAPON || obj->which_kind >= WEAPONS) return 1;
    return weapon_damage_n[obj->which_kind] + obj->hit_enchant;
}

int
damage_for_strength(void)
{
    short strength = rogue.str_current + add_strength;
    int i;
    static const short sa[] = { 14, 17, 18, 20, 21, 30, 9999 };
    static const short ra[] = { 1, 3, 4, 5, 6, 7, 8 };

    if (strength <= 6) return strength - 5;
    for (i = 0;; ++i) {
        if (strength <= sa[i]) return ra[i];
    }
}

int
mon_damage(object *monster, int damage)
{
    u8 name[20];

    monster->m_hp -= (short)damage;
    if (monster->m_hp > 0) return 1;
    get_monster_name(monster, name, sizeof(name));
    message_id_mz(24, name);
    remove_monster(monster);
    add_exp(monster->kill_exp, 1);
    return 0;
}

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
    hit_chance += (2 * rogue.exp);
    return hit_chance;
}

int
get_weapon_damage(object *weapon)
{
    short damage;

    damage = get_w_damage(weapon) + damage_for_strength();
    damage += ((rogue.exp + 1) / 2);
    return damage;
}
