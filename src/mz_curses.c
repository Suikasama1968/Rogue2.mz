/*
 * mz_curses.c
 *
 * 簡易版Unix CURSES互換関数 for MZ-1500
 *  文字コードはMZ-700/1500のディスプレイコードを指定する
 *  エラーハンドリングは行わない
 * Copyright (c) 2026 Suikasama1968
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "mz_curses.h"
#include "mz_display.h"
#include "mz_system.h"

static WINDOW main_window;
static u8 color_pairs[16];

u8 *Key =(u8 *)KEYDATA;

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
    static const u8 mz_color[8] = { 0, 2, 4, 6, 1, 3, 5, 7 };

    color_pairs[pair] = (u8)((mz_color[fg] << 4) | mz_color[bg]);
    return 0;
}

/*
    属性を設定する    
*/
int attrset(attr_t attrs)
{
    u8 pair = (u8)attrs;

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
    u8 *addr = main_window._cur_addr;
    u8 *attr = main_window._cur_attr;

    memset(addr, DC_SPC, V_COLUMN - main_window._curx);
    memset(attr, 0x70, V_COLUMN - main_window._curx);
    return 0;
}

/*
    現在のカーソル位置からすべての行をスペースで埋める
*/
int clrtobot(void)
{
    u8 *addr = main_window._cur_addr;
    u8 *attr = main_window._cur_attr;

    memset(addr, DC_SPC, 0xc800 - (u16)addr);
    memset(attr, 0x70, 0xd000 - (u16)attr);
    return 0;
}
/*
    カーソルを移動する
*/
int move(u8 y, u8 x)
{
        main_window._cury = (u8)y;
        main_window._curx = (u8)x;
        main_window._cur_addr = (u8 *)(TEXT_V_VRAM + y * V_COLUMN + x);
        main_window._cur_attr = (u8 *)(TEXT_V_ATTR + y * V_COLUMN + x);
        return 0;
}
/*
    ウィンドウの現在の位置に文字を一文字書く 
    chにはMZ-1500のディスプレイコードを指定する
    上位8bitはアトリビュート、下位8bitは文字コード
    指定方法 
     文字だけの場合:addch(DC_A) のように指定する
     属性をつける場合:addch(DC_A | DC_FG_RED | DC_BG_BLACK)
*/
int addch(u16 ch)
{
    u8 attr;

    attr = (u8)(ch >> 8);
    main_window._cur_addr[0] = (u8)ch;
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
    chにはMZ-1500のディスプレイコードを指定する
*/
int mvaddch(u8 y, u8 x, u16 ch)
{
    move(y, x);
    return addch(ch); 
}

/* 
    ウィンドウの現在のカーソル位置に文字列を追加 
*/
int addstr(const u8 *str)
{
    u16 cset = DC_CSET_1;

    while (*str != '\0') {
        u16 ch;

        if (*str == DC_NICOCHAN_0) { // 英大文字・カタカナモード
            cset = DC_CSET_0;
            ++str;
            continue;
        }
        if (*str == DC_NICOCHAN_1) { // 英子文字ひらがなモード
            cset = DC_CSET_1;
            ++str;
            continue;
        }
        ch = (u16)*str++ | cset | ((u16)main_window._attrs << 8);
        addch(ch);
    }
    return 0;
}

int mvaddstr(u8 y, u8 x, const u8 *str)
{
    move(y, x);
    return addstr(str);
}

/* 
    ウィンドウの現在のカーソル位置に文字列から最大n文字（またはバイト）を追加
*/
int addnstr(const u8 *str, u8 length)
{
    while (length--) {
        addch((u16)*str++);
    }
    return 0;
}

int mvaddnstr(u8 y, u8 x, const u8 *str, u8 length)
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
    u8 *from_addr;

    if (main_window._curx < 30) {
        // 0~39列目までを実画面に転送表示する
        from_addr = (u8 *)TEXT_V_VRAM;
    } else if (main_window._curx < 50) {
        // 20~59列目までを実画面に転送表示する
        from_addr = (u8 *)(TEXT_V_VRAM + 20);
    } else {
        // 40~79列目までを実画面に転送表示する
        from_addr = (u8 *)(TEXT_V_VRAM + 40);
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
static int keycode_to_ascii(u8 key, u8 strobe)
{
    static const u8 key_table[] = {
        'y', 'z', '@',  0,   0,   0,   0,   0,
        'q', 'r', 's', 't', 'u', 'v', 'w', 'x',
        'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
        'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h',
        '1', '2', '3', '4', '5', '6', '7', '8',
        '\\','^', '-', ' ', '0', '9', ',', '.',
         0,   0,  'k', 'j', 'l', 'h', '?', '/'
    };
    /* SHIFTで意味が変わるキー。ビット配置はKEYDATAと同じ。 */
    static const u8 shift_mask[] = { 0, 0xd2, 0x01, 0, 0, 0x03, 0 };
    u8 bit = 0x80;
    u8 index = 0;
    u8 ch;

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

static int is_repeat_key(u8 key)
{
    return key == 'h' || key == 'j' || key == 'k' || key == 'l' ||
           key == 'y' || key == 'u' || key == 'b' || key == 'n';
}

/* 文字キーがすべて離されるまで待つ。SHIFTキー単独は対象外。 */
static void wait_key_release(void)
{
    u8 strobe;
    u8 pressed;

    do {
        KEY_Scan();
        pressed = 0;
        for (strobe = 1; strobe <= 7; ++strobe) {
            if (Key[strobe] != 0) {
                pressed = 1;
                break;
            }
        }
    } while (pressed);
}

/*
  キーが押されるまで待機し、通常のASCIIコードで返す。
*/
int getch(void)
{
    u8 strobe;
    int key;

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
    if (!is_repeat_key(key)) wait_key_release();
    return key;
}
