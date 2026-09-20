#if !defined( __INVENT_H__ )
#define __INVENT_H__

extern void inventory(object *pack, unsigned short mask);
extern void make_scroll_titles(void);
extern void get_wand_and_ring_materials(void);
extern void get_desc(object *obj, char *desc, boolean capitalized);
extern void single_inv(short ichar);

#if 0 /* MZ-700/1500では未対応 */
extern struct id *get_id_table(object *obj);
extern void inv_armor_weapon(boolean is_weapon);
extern void discovered(void);
#endif

#endif /* not __INVENT_H__ */
