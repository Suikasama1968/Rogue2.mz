#if !defined( __RANDOM_H__ )
#define __RANDOM_H__

extern void srrandom(int seed);
extern unsigned short rrandom(void);
extern int get_rand(int low, int high);
extern int rand_percent(int percentage);
extern int coin_toss(void);

#endif /* not __RANDOM_H__ */
