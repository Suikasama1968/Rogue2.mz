#if !defined( __MONSTER_H__ )
#define __MONSTER_H__

extern void mv_mons(void);
extern void put_mons(void);
extern void clear_level_monsters(void);
extern object *monster_at(short row, short col);
extern void remove_monster(object *monster);
extern void wake_room(short rn, boolean entering, short row, short col);
extern int rogue_can_see(int row, int col);
extern void wanderer(void);
extern void party_monsters(int rn, int n);
extern void create_monster(void);

#endif /* not __MONSTER_H__ */
