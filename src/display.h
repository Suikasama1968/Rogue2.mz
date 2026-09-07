#if !defined( __DISPLAY_H__ )
#define __DISPLAY_H__

#define PAIR_NORMAL  0
#define PAIR_TERRAIN 1
#define PAIR_FLOOR   2
#define PAIR_MONSTER 3
#define PAIR_OBJECT  4
#define PAIR_PLAYER  5

extern void display_dungeon(void);
extern void refresh_dungeon(void);
extern void init_color_attr(void);
extern void colorize_dungeon(short row, short col);

#endif /* not __DISPLAY_H__ */
