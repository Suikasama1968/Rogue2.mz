#if !defined( __INVENT_H__ )
#define __INVENT_H__

extern void inventory(object *pack, unsigned short mask);
extern void get_desc(object *obj, char *desc, boolean capitalized);
extern void single_inv(short ichar);

#endif /* not __INVENT_H__ */
