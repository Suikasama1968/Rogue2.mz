/*
 * mz_display.c
 *
 * ASCII <-> MZ-700/1500表示変換関数
 * Copyright (c) 2026 Suikasama1968
 */
#include "rogue.h"
#include "message.h"
#include "mz_display.h"

/*
    MZディスプレイコード文字列の表示文字数を返す
*/
uint8_t mz_display_length(const uint8_t *text)
{
    uint8_t length = 0;

    while (*text) {
        if (*text != DC_NICOCHAN_0 && *text != DC_NICOCHAN_1) length++;
        text++;
    }
    return length;
}

/*
    数値を文字列に変換格納する long対応
*/
uint8_t mz_number(uint8_t *dst, uint32_t value)
{
    uint8_t work[10];
    uint8_t length = 0;
    uint8_t count;

    do {
        work[length++] = (uint8_t)(DC_0 + value % 10);
        value /= 10;
    } while (value);
    count = length;
    while (length) *dst++ = work[--length];
    *dst = '\0';
    return count;
}

/*
    MZディスプレイコード対応のsprintf関数
*/
void mz_sprintf(uint8_t *dst, short msg_id, const int32_t *values)
{
    const uint8_t *format;
    uint8_t length;

    format = find_message(msg_id, &length);
    if (!format) {
        *dst = '\0';
        return;
    }
    while (length--) {
        uint8_t code = *format++;

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
            dst += mz_number(dst, magnitude);
            break;
        }
        case MESSAGE_FORMAT_UNSIGNED:   // %u
            dst += mz_number(dst, (unsigned long)*values++);
            break;
        default:
            *dst++ = code;
            break;
        }
    }
    *dst = '\0';
}
