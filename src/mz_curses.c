/*
 * mz_curses.c
 *
 * 簡易版Unix CURSES互換関数 for MZ-700/1500
 *  文字コードはMZ-700/1500のディスプレイコードを指定する
 *  エラーハンドリングは行わない
 * Copyright (c) 2026 Suikasama1968
 */

#include <string.h>

#include "rogue.h"
#include "move.h"
#include "mz_curses.h"
#include "mz_display.h"
#include "mz_system.h"

static WINDOW main_window;
static uint8_t color_pairs[16];

uint8_t *Key =(uint8_t *)KEYDATA;

/*
    ウィンドウの初期化
*/
WINDOW *initscr(void)
{
    color_pairs[0] = 0x70;
    main_window._attrs = color_pairs[0];
    clear();
    return &main_window;
}

/* 
    色のペアーの定義を変更する
*/
int init_pair(short pair, short fg, short bg)
{
    static const uint8_t mz_color[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };

    color_pairs[pair] = (uint8_t)((mz_color[fg] << 4) | mz_color[bg]);
    return 0;
}

/*
    属性を設定する    
*/
int attrset(attr_t attrs)
{
    uint8_t pair = (uint8_t)attrs;

    main_window._attrs = color_pairs[pair];
    return 0;
}

/*
    ウィンドウをクリアする
    実画面と仮想画面を初期化
*/
int clear(void)
{
    // 仮想テキスト & アトリビュート 画面初期化
    memset((void *)TEXT_V_VRAM, DC_SPC, 2048);
    memset((void *)TEXT_V_ATTR, 0x70, 2048);

    return move(0, 0);
}

/*
    現在のカーソル位置から行末までをスペースで埋める
*/
int clrtoeol(void)
{
    uint8_t *addr = main_window._cur_addr;
    uint8_t *attr = main_window._cur_attr;

    memset(addr, DC_SPC, V_COLUMN - main_window._curx);
    memset(attr, 0x70, V_COLUMN - main_window._curx);
    return 0;
}

/*
    現在のカーソル位置からすべての行をスペースで埋める
*/
#if 0 /* Rogue2では未使用 */
int clrtobot(void)
{
    uint8_t *addr = main_window._cur_addr;
    uint8_t *attr = main_window._cur_attr;

    memset(addr, DC_SPC, 0xc800 - (uint16_t)addr);
    memset(attr, 0x70, 0xd000 - (uint16_t)attr);
    return 0;
}
#endif

/*
    カーソルを移動する
*/
int move(uint8_t y, uint8_t x)
{
        main_window._cury = (uint8_t)y;
        main_window._curx = (uint8_t)x;
        main_window._cur_addr = (uint8_t *)(TEXT_V_VRAM + y * V_COLUMN + x);
        main_window._cur_attr = (uint8_t *)(TEXT_V_ATTR + y * V_COLUMN + x);
        return 0;
}
/*
    ウィンドウの現在の位置に文字を一文字書く 
    chにはMZ-700/1500のディスプレイコードを指定する
    上位8bitはアトリビュート、下位8bitは文字コード
    指定方法 
     文字だけの場合:addch(DC_A) のように指定する
     属性をつける場合:addch(DC_A | DC_FG_RED | DC_BG_BLACK)
*/
int addch(uint16_t ch)
{
    uint8_t attr;

    attr = (uint8_t)(ch >> 8);
    main_window._cur_addr[0] = (uint8_t)ch;
    if ((attr & 0x7f) == 0) {
        attr |= main_window._attrs;
    }
    main_window._cur_attr[0] = attr;
    main_window._curx++;
    main_window._cur_addr++;
    main_window._cur_attr++;
    return 0;
}
/*
    カーソル位置を指定して文字を画面に書く
    chにはMZ-700/1500のディスプレイコードを指定する
*/
int mvaddch(uint8_t y, uint8_t x, uint16_t ch)
{
    move(y, x);
    return addch(ch); 
}

/* 
    ウィンドウの現在のカーソル位置に文字列を追加 
*/
int addstr(const uint8_t *str)
{
    return addnstr(str, 255);
}

int mvaddstr(uint8_t y, uint8_t x, const uint8_t *str)
{
    move(y, x);
    return addstr(str);
}

/* 
    ウィンドウの現在のカーソル位置に文字列から最大n文字（またはバイト）を追加
*/
int addnstr(const uint8_t *str, uint8_t length)
{
    uint16_t cset = DC_CSET_1;

    while (length && *str != '\0') {
        uint16_t ch;

        if (*str == DC_NICOCHAN_0) {
            cset = DC_CSET_0;
            ++str;
            continue;
        }
        if (*str == DC_NICOCHAN_1) {
            cset = DC_CSET_1;
            ++str;
            continue;
        }
        ch = (uint16_t)*str++ | cset | ((uint16_t)main_window._attrs << 8);
        addch(ch);
        --length;
    }
    return 0;
}

int mvaddnstr(uint8_t y, uint8_t x, const uint8_t *str, uint8_t length)
{
    move(y, x);
    return addnstr(str, length);
}

/*
    画面を更新する
    仮想画面から実画面への描画を行う
     仮想画面は80x25、そのうちの40x25を実画面に転送する
     3画面切り替え
     描画範囲は_curxで決める
*/
int refresh(void)
{
    uint8_t *from_addr;

    if (main_window._curx < 30) {
        // 0~39列目までを実画面に転送表示する
        from_addr = (uint8_t *)TEXT_V_VRAM;
    } else if (main_window._curx < 50) {
        // 20~59列目までを実画面に転送表示する
        from_addr = (uint8_t *)(TEXT_V_VRAM + 20);
    } else {
        // 40~79列目までを実画面に転送表示する
        from_addr = (uint8_t *)(TEXT_V_VRAM + 40);
    }
    /* 固定行を含む25行すべてをVRAM_Display()内で転送する。 */
    VRAM_Display(from_addr);
    return 0;
}
/*
    getchのサブ関数。MZモニタのキーコードをASCIIへ変換する。
    z88dkのMZ用getk()はMZ-ASCII（ディスプレイコード）を返すため、
    Rogue本体から通常の文字コードとして扱えるように変換する。
    デフォルトは小文字、SHIFT押下時は大文字(ただしRogueで使用するキーのみ)として返す。
 */
static uint8_t keycode_to_ascii(uint8_t key, uint8_t strobe)
{
    static const uint8_t key_table[] = {
        'y', 'z', '@',  0,   0,   0,   0,   0,
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
        'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
        '1', '2', '3', '4', '5', '6', '7', '8',
        '*', '+', '-', ' ', '0', '9', ',', '.',
         0,   0,  'k', 'j', 'l', 'h', '?', '/'
    };
    /* SHIFTで意味が変わるキー。ビット配置はKEYDATAと同じ。 */
    static const uint8_t shift_mask[] = { 0, 0xd2, 0x01, 0, 0, 0x03, 0 };
    uint8_t bit = 0x80;
    uint8_t index = 0;
    uint8_t ch;

    if (strobe == 0 || strobe > 7) return 0;
    while (!(key & bit)) {
        bit >>= 1;
        if (++index == 8) return 0;
    }
    ch = key_table[((strobe - 1) << 3) + index];
    if ((Key[8] & 0x01) && (shift_mask[strobe - 1] & bit)) {
        ch += (ch >= 'a') ? ('A' - 'a') : ('<' - ',');
    }
    return ch;
}

/* 文字キーがすべて離されるまで待つ。SHIFTキー単独は対象外。 */
static void wait_key_release(void)
{
    uint8_t strobe;
    uint8_t pressed;

    do {
        KEY_Scan();
        pressed = 0;
        for (strobe = 1; strobe <= 7; strobe++) {
            if (Key[strobe] != 0) {
                pressed = 1;
                break;
            }
        }
    } while (pressed);
}

/* 
    入力バッファの破棄
    MZ-700/1500の場合入力待ちへ移る前に、現在押されている文字キーが離されるまで待つ。 
*/
int flushinp(void)
{
    wait_key_release();
    return 0;
}

/*
  キーが押されるまで待機し、通常のASCIIコードで返す。
*/
int getch(void)
{
    uint8_t strobe;
    uint8_t key;

    while(1) {
        KEY_Scan();  // キーをスキャンして KEYDATA に格納
        // 文字が押されていたかをチェックするため1～7のストローブを順にチェックする
        for (strobe = 1; strobe <= 7; strobe++) {
            if (Key[strobe] != 0) {
                key = keycode_to_ascii(Key[strobe], strobe);
                if (key != 0) goto KEY_PRESSED;
            }
        }
    }
KEY_PRESSED:
    // Rogue本体のget_direction()に同機能の関数がありそちらを利用
    // サイズ削減のためであり、本来はmz_curses.c内で閉じるべき
    if (!is_direction(key) || key == CANCEL) wait_key_release();
    return key;
}
