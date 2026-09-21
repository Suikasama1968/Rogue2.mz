#if !defined( __INIT_H__ )
#define __INIT_H__

extern int init(void);
extern void init_game(void);
extern void player_init(void);
extern void byebye(void);
extern int read_mesg(char *argv_msgfile);

#if 0 /* MZ-700/1500では未対応 */
extern void error_save(int sig);
extern void do_args(int argc, char *argv[]);
extern void do_opts(void);
extern void set_opts(char *env);
extern void env_get_value(char **s, char *e, boolean add_blank,
                          boolean no_colon);
#endif

#endif /* not __INIT_H__ */
