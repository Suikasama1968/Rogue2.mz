/*
 * mz_curses.h
 *
 * 簡易版Unix CURSES互換関数 for MZ-700/1500
 * Copyright (c) 2026 Suikasama1968
 */
#ifndef _MZ_CURSES_H
#define _MZ_CURSES_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

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

typedef uint16_t attr_t;

typedef struct _win_st  WINDOW;

struct _win_st
{
    uint8_t           _cury, _curx;   // カーソル位置
    uint8_t          *_cur_addr;      // カーソルのV_RAMアドレス
    uint8_t          *_cur_attr;      // カーソルのV_ATTRアドレス
    uint8_t           _attrs;         // 現在のMZカラー属性
};

extern WINDOW *initscr(void);
extern int clear(void);
extern int move(uint8_t, uint8_t);
extern int addch(uint16_t);
extern int mvaddch(uint8_t, uint8_t, uint16_t);
extern int addstr(const uint8_t *);
extern int mvaddstr(uint8_t, uint8_t, const uint8_t *);
extern int addnstr(const uint8_t *, uint8_t);
extern int mvaddnstr(uint8_t, uint8_t, const uint8_t *, uint8_t);
extern int refresh(void);
extern int getch(void);
extern int flushinp(void);
extern int clrtoeol(void);
#if 0 /* Rouge2では未使用 */
extern int clrtobot(void);
#endif
extern int init_pair(short, short, short);
extern int attrset(attr_t);

#ifdef __cplusplus
}
#endif

#endif /* _MZ_CURSES_H */
