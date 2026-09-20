#if !defined( __THROW_H__ )
#define __THROW_H__

extern void throw(void);
extern int throw_at_monster(object *monster, object *weapon);
extern object *get_thrown_at_monster(object *obj, short dir, short *row,
                                     short *col);
extern void flop_weapon(object *weapon, short row, short col);
extern void rand_around(short i, short *r, short *c);

#if 0 /* MZ-700/1500では未対応 */
extern void potion_monster(object *monster, unsigned short kind);
#endif

#endif /* not __THROW_H__ */
