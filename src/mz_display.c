/*
 * mz_display.c
 *
 * ASCII <-> MZ-700/1500表示変換関数
 * Copyright (c) 2026 Suikasama1968
 */
#include "mz_common.h"
#include "mz_display.h"
#include "rogue.h"
#include "message.h"

/*
    数値を文字列に変換格納する long対応
*/
static u8 *mz_put_ulong(u8 *dst, unsigned long value)
{
    u8 work[10];
    u8 length = 0;

    do {
        work[length++] = (u8)(DC_0 + value % 10);
        value /= 10;
    } while (value);
    while (length) *dst++ = work[--length];
    return dst;
}

/*
    MZディスプレイコード対応のsprintf関数
*/
int mz_sprintf(u8 *dst, short msg_id, const long *values)
{
    u8 *start = dst;
    const u8 *format;
    u8 length;

    format = find_message(msg_id, &length);
    if (!format) {
        *dst = '\0';
        return 0;
    }
    while (length--) {
        u8 code = *format++;

        switch (code) {
        case MESSAGE_FORMAT_DECIMAL:    // %d
        case MESSAGE_FORMAT_LONG: {     // %ld
            long value = *values++;
            unsigned long magnitude;

            if (value < 0) {
                *dst++ = DC_MINUS;
                magnitude = (unsigned long)(-(value + 1)) + 1;
            } else {
                magnitude = (unsigned long)value;
            }
            dst = mz_put_ulong(dst, magnitude);
            break;
        }
        case MESSAGE_FORMAT_UNSIGNED:   // %u
            dst = mz_put_ulong(dst, (unsigned long)*values++);
            break;
        default:
            *dst++ = code;
            break;
        }
    }
    *dst = '\0';
    return (int)(dst - start);
}
