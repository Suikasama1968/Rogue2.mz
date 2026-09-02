#if !defined( __MOVE_H__ )
#define __MOVE_H__

extern int one_move_rogue(short dirch, short pickup);
extern int is_passable(int row, int col);
extern int can_move(int row1, int col1, int row2, int col2);
extern char is_direction(int c);
extern boolean check_hunger(boolean messages_only);
extern boolean reg_move(void);
extern void rest(int count);
extern void heal(void);

#endif /* not __MOVE_H__ */
