#if !defined( __SPECHIT_H__ )
#define __SPECHIT_H__

extern void special_hit(object *monster);
extern void rust(object *monster);
extern void freeze(object *monster);
extern void sting(object *monster);
extern void steal_gold(object *monster);
extern void steal_item(object *monster);
extern int m_confuse(object *monster);

#endif /* not __SPECHIT_H__ */
