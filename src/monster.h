#if !defined( __MONSTER_H__ )
#define __MONSTER_H__

extern void mv_mons(void);
extern void put_mons(void);
extern object *gr_monster(object *monster, int mn);
extern void clear_level_monsters(void);
extern object *monster_at(short row, short col);
extern void remove_monster(object *monster);
extern void wake_room(short rn, boolean entering, short row, short col);
extern int rogue_can_see(int row, int col);
extern void wanderer(void);
extern void party_monsters(int rn, int n);
extern void show_monsters(void);
extern void create_monster(void);
extern int gr_obj_char(void);
extern void mv_monster(object *monster, short row, short col);
extern int mtry(object *monster, short row, short col);
extern void move_mon_to(object *monster, short row, short col);
extern int mon_can_go(object *monster, short row, short col);
extern boolean mon_sees(object *monster, int row, int col);
extern void wake_up(object *monster);
extern int move_confused(object *monster);
extern int flit(object *monster);

#if 0 /* MZ-700/1500では未対応 */
extern void aim_monster(object *monster);
#endif
extern void mv_aquatars(void);

/* MZ-700/1500固有 */
extern short get_monster_name_id(const object *monster);

#endif /* not __MONSTER_H__ */
