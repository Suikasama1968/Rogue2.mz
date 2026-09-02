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

typedef struct _win_st  WINDOW;

struct _win_st
{
    u8           _cury, _curx;   // カーソル位置
    u8          *cur_addr;       // カーソルのV_RAMアドレス
    u8          *cur_attr;       // カーソルのV_ATTRアドレス
};

extern WINDOW *initscr(void);
extern int clear(void);
extern int move(u8, u8);
extern int addch(u16);
extern int mvaddch(u8, u8, u16);
extern int addstr(const u8 *);
extern int mvaddstr(u8, u8, const u8 *);
extern int addstr_mz(const u8 *);
extern int mvaddstr_mz(u8, u8, const u8 *);
extern int addnstr_mz(const u8 *, u8);
extern int mvaddnstr_mz(u8, u8, const u8 *, u8);
extern int refresh(void);
extern int getch(void);
extern int clrtoeol(void);
extern int clrtobot(void);

#ifdef __cplusplus
}
#endif

#endif /* _MZ_CURSES_H */
