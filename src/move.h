#if !defined( __MOVE_H__ )
#define __MOVE_H__

extern int one_move_rogue(short dirch, short pickup);
extern int is_passable(int row, int col);
extern int can_move(int row1, int col1, int row2, int col2);
extern char is_direction(int c);
extern boolean check_hunger(boolean messages_only);
extern boolean reg_move(void);
extern void rest(int count);
extern int gr_dir(void);
extern void heal(void);
extern void reset_move_state(void);

#if 0 /* MZ-700/1500では未対応 */
extern void multiple_move_rogue(int dirch);
extern int next_to_something(int drow, int dcol);
extern void move_onto(void);
#endif

#endif /* not __MOVE_H__ */
