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
#include "mz_system.h"

void
srrandom(int seed)
{
    *(u16 *)XSHIFT = seed ? (u16)seed : 0xace1u;
}

long
rrandom(void) __naked
{
__asm
// 16-bit xorshift Z80 pseudorandom number generator by John Metcalf

// generates 16-bit pseudorandom numbers with a period of 65535
// using the xorshift method

// XSHFT ^= XSHFT << 7
// XSHFT ^= XSHFT >> 9
// XSHFT ^= XSHFT << 8__asm
    ld  hl, (XSHIFT)

    ld  a, h
    rra
    ld  a, l
    rra
    xor h
    ld  h, a

    ld  a, l
    rra
    ld  a, h
    rra
    xor l
    ld  l, a

    xor h
    ld  h, a

    ld  (XSHIFT), hl
    ld  de, 0
    ret
__endasm;
}

int
get_rand(int low, int high)
{
    return low + (int)(rrandom() % (long)(high - low + 1));
}

int
rand_percent(int percentage)
{
    return get_rand(1, 100) <= percentage;
}

int
coin_toss(void)
{
    return (int)(rrandom() & 0x01L);
}
