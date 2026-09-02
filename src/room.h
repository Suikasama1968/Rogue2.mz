#if !defined( __ROOM_H__ )
#define __ROOM_H__

extern void light_up_room(int rn);
extern void light_passage(int row, int col);
extern void darken_room(short rn);
extern int get_room_number(int row, int col);
extern void gr_row_col(short *row, short *col, unsigned short mask);
extern int is_all_connected(void);
extern void visit_rooms(int rn);
extern int gr_room(void);
extern int party_objects(int rn);

#endif /* not __ROOM_H__ */
