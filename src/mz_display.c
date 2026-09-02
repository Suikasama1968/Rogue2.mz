/*
 * mz_display.c
 *
 * ASCII <-> MZ-700/1500表示変換関数
 * Copyright (c) 2026 Suikasama1968
 */
#include "mz_common.h"
#include "mz_display.h"

/*
    ASCII文字をMZ-1500のディスプレイコードに変換 
    上位8bitはアトリビュート、下位8bitは文字コード
    アトリビュートのデフォルトは白文字、黒背景
*/
u16 ascii_to_mz(u8 ch)
{
    static const u8 display_code[] = {
        DC_SPC, DC_EXCLAM, DC_QUOT, DC_NUMBER, DC_DOLLER, DC_PERCENT,
        DC_AMP, DC_APOS, DC_L_BLACKET, DC_R_BLACKET, DC_STAR, DC_PLUS,
        DC_COMMA, DC_MINUS, DC_PERIOD, DC_SLASH,
        DC_0, DC_1, DC_2, DC_3, DC_4, DC_5, DC_6, DC_7, DC_8, DC_9,
        DC_COLON, DC_SEMICOLON, DC_LT, DC_EQUAL, DC_GT, DC_QUESTION,
        DC_AT,
        DC_A, DC_B, DC_C, DC_D, DC_E, DC_F, DC_G, DC_H, DC_I, DC_J,
        DC_K, DC_L, DC_M, DC_N, DC_O, DC_P, DC_Q, DC_R, DC_S, DC_T,
        DC_U, DC_V, DC_W, DC_X, DC_Y, DC_Z,
        DC_L_SQ_BLACKET, DC_BACK_SLASH, DC_R_SQ_BLACKET, DC_QUESTION,
        DC_D_BAR, DC_QUESTION,
        DC_A, DC_B, DC_C, DC_D, DC_E, DC_F, DC_G, DC_H, DC_I, DC_J,
        DC_K, DC_L, DC_M, DC_N, DC_O, DC_P, DC_Q, DC_R, DC_S, DC_T,
        DC_U, DC_V, DC_W, DC_X, DC_Y, DC_Z,
        DC_QUESTION, DC_PIPE, DC_QUESTION, DC_U_BAR
    };
    u16 attr = DC_FG_WHITE | DC_BG_BLACK;

    if (ch < 0x20 || ch > 0x7e) ch = '?';
    if (ch >= 'a' && ch <= 'z') attr |= DC_CSET_1;
    return (u16)display_code[ch - 0x20] | attr;
}
