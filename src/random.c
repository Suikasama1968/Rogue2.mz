/*
 * This source herein may be modified and/or distributed by anybody who
 * so desires, with the following restrictions:
 *    1.)  This notice shall not be removed.
 *    2.)  Credit shall not be taken for the creation of this source.
 *    3.)  This code is not to be traded, sold, or used for personal
 *         gain or profit.
 *
 */
#include "random.h"

static unsigned int rng_state = 0x5a17u;

void srrandom(int seed)
{
    rng_state = (unsigned int)seed;
}

long rrandom(void)
{
    rng_state = (unsigned int)(rng_state * 109u + 89u);
    return (long)rng_state;
}

int get_rand(int low, int high)
{
    return low + (int)(rrandom() % (long)(high - low + 1));
}

int rand_percent(int percentage)
{
    return get_rand(1, 100) <= percentage;
}

int coin_toss(void)
{
    return get_rand(0, 1);
}
