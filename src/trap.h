#if !defined( __TRAP_H__ )
#define __TRAP_H__

extern int trap_at(int row, int col);
extern void trap_player(short row, short col);
extern void add_traps(void);
#if 0 /* MZ-700/1500では未使用 */
extern void id_trap(void);
extern void show_traps(void);
#endif
extern void search(short n, boolean is_auto);

#endif /* not __TRAP_H__ */
