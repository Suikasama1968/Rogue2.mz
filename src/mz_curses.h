/*
 * mz_curses.h
 *
 * 簡易版Unix CURSES互換関数 for MZ-1500
 * Copyright (c) 2026 Suikasama1968
 */
#ifndef _MZ_CURSES_H
#define _MZ_CURSES_H

#ifdef __cplusplus
extern "C" {
#endif

#include "mz_common.h"

#define MZ_STR_CSET_1 0xce
#define MZ_STR_CSET_0 0xcf

/* curses compatible color subset. */
#define COLOR_BLACK   0
#define COLOR_RED     1
#define COLOR_GREEN   2
#define COLOR_YELLOW  3
#define COLOR_BLUE    4
#define COLOR_MAGENTA 5
#define COLOR_CYAN    6
#define COLOR_WHITE   7

#define A_NORMAL      0x0000
#define COLOR_PAIR(n) ((attr_t)((n) & 0x00ff))

typedef u16 attr_t;

typedef struct _win_st  WINDOW;

struct _win_st
{
    u8           _cury, _curx;   // カーソル位置
    u8          *_cur_addr;      // カーソルのV_RAMアドレス
    u8          *_cur_attr;      // カーソルのV_ATTRアドレス
    u8           _attrs;         // 現在のMZカラー属性
};

extern WINDOW *initscr(void);
extern int clear(void);
extern int move(u8, u8);
extern int addch(u16);
extern int mvaddch(u8, u8, u16);
extern int addstr(const u8 *);
extern int mvaddstr(u8, u8, const u8 *);
extern int addnstr(const u8 *, u8);
extern int mvaddnstr(u8, u8, const u8 *, u8);
extern int refresh(void);
extern int getch(void);
extern int clrtoeol(void);
extern int clrtobot(void);
extern int init_pair(short, short, short);
extern int attrset(attr_t);

#ifdef __cplusplus
}
#endif

#endif /* _MZ_CURSES_H */
