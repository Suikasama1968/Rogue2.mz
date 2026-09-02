#if !defined( __INIT_H__ )
#define __INIT_H__

extern int init(int argc, char *argv[]);
extern void player_init(void);
extern void byebye(int sig);
extern int read_mesg(char *argv_msgfile);

#endif /* not __INIT_H__ */
