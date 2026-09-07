/*
 * machdep.c
 *
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  No portion of this notice shall be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */

#include "machdep.h"
#include "mz_common.h"
#include "mz_system.h"

/* md_gseed() (Get Seed)
 *
 * This function returns a seed for the random number generator (RNG).  This
 * seed causes the RNG to begin generating numbers at some point in it's
 * sequence.  Without a random seed, the RNG will generate the same set
 * of numbers, and every game will start out exactly the same way.  A good
 * number to use is the process id, given by getpid() on most UNIX systems.
 *
 * You need to find some single random integer, such as:
 *   process id.
 *   current time (minutes + seconds) returned from md_gct(), if implemented.
 *
 * It will not help to return "get_rand()" or "rand()" or the return value of
 * any pseudo-RNG.  If you don't have a random number, you can just return 1,
 * but this means your games will ALWAYS start the same way, and will play
 * exactly the same way given the same input.
 */

int
md_gseed(void)
{
    return (int)fast_rand8();
}

/* md_exit():
 *
 * This function causes the program to discontinue execution and exit.
 * This function must be implemented or the program will continue to
 * hang when it should quit.
 */

void
md_exit(int status)
{
    (void)status;
    BANK_ROM();
    RESET();
}
