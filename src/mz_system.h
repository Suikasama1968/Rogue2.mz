/*
 * mz_system.h
 *
 * システム制御定義 for MZ-1500
 * Copyright (c) 2026 Suikasama1968
 */
#ifndef MZ_SYSTEM_H
#define MZ_SYSTEM_H

#ifdef __cplusplus
extern "C" {
#endif

// DRAM : 0x0000-0x0fff (default Monitor ROM area)
#define LOW_RAM_BEGIN       0x0000
#define LOW_RAM_END         0x1000
#define LOW_RAM_SIZE        (LOW_RAM_END - LOW_RAM_BEGIN)


// システム情報 格納先頭アドレス : 0x0000-0x00ff
#define SYSTEM_WORK     LOW_RAM_BEGIN

// キー入力結果格納先 (10バイト)
#define KEYDATA         SYSTEM_WORK + 0x0000

#define LOOP_COUNT      SYSTEM_WORK + 0x000a
#define TIME_COUNT      SYSTEM_WORK + 0x000b

// 割り込み処理情報格納先
#define VECTOR  	    SYSTEM_WORK + 0x0010
#define TIMER   	    SYSTEM_WORK + 0x0012
#define COUNTER 	    SYSTEM_WORK + 0x0014
#define MSec    	    895

// バンク切替状態保存先
#define BANK_MODE       SYSTEM_WORK + 0x0020
#define BANK_NUM        SYSTEM_WORK + 0x0021

// デバッグ用
#define FUNC_TRACE      SYSTEM_WORK + 0x0030
#define ISR_COUNT       SYSTEM_WORK + 0x0040

// フリーエリア : 0x0100-0x0xfff
// オブジェクト
#define OBJECT_POOL_ADDR    0x0100
#define OBJECT_POOL_SIZE    0x0400
#define OBJECT_USED_ADDR    0x0500
#define OBJECT_USED_SIZE    0x0020
// モンスター
#define MONSTER_POOL_ADDR   0x0520
#define MONSTER_POOL_SIZE   0x0260
#define MONSTER_USED_ADDR   0x0780
#define MONSTER_USED_SIZE   0x0020
// 画面バックアップ
#define DESCS_TEXT_ADDR     0x07a0
#define DESCS_TEXT_SIZE     0x0168
#define DESCS_ATTR_ADDR     0x0908
#define DESCS_ATTR_SIZE     0x0168
// ルームテーブル */
#define ROOMS_ADDR          0x0a70
#define ROOMS_SIZE          0x01b0

// 今後のゲームロジック用予約領域 : 0x0c20-0x0fff
#define GAME_WORK_ADDR      0x0c20
#define GAME_WORK_SIZE      0x03e0

// メモリマップドI/O
#define _8255_PORT_A    0xe000
#define _8255_PORT_B    0xe001
#define _8255_PORT_C    0xe002
#define _8255_CONTROL   0xe003
#define _8253_CH0       0xe004
#define _8253_CH1       0xe005
#define _8253_CH2       0xe006
#define _8253_CONTROL   0xe007
#define _8253_CH0_GATE  0xe008

// 8255ポートA キーストローブ
#define KEY_STROBE_0    0xf0
#define KEY_STROBE_1    0xf1
#define KEY_STROBE_2    0xf2
#define KEY_STROBE_3    0xf3
#define KEY_STROBE_4    0xf4
#define KEY_STROBE_5    0xf5
#define KEY_STROBE_6    0xf6
#define KEY_STROBE_7    0xf7
#define KEY_STROBE_8    0xf8
#define KEY_STROBE_9    0xf9

// メモリマップ関連
#define TEXT_VRAM        0xd000
#define TEXT_ATTR        0xd800
#define TEXT_VRAM_OFFSET 0x0800

// メッセージエリア
#define MESG_LOAD_ADDR  0xc000
#define MESG_LOAD_SIZE  0x1000
#define MESG_ADDR       0xd000
#define MESG_SIZE       0x3000

// PCG関連
#define PCG_RAM         0xd000
#define BANK_RED        0x02
#define BANK_GREEN      0x03
#define BANK_BLUE       0x01

#define PCG_1_ADDR      0xd000
#define PCG_2_ADDR      0xd800
#define PCG_3_ADDR      0xe000
#define PCG_4_ADDR      0xe800

extern void KEY_Scan(void);
extern void BANK_VRAM(void);
extern void BANK_DRAM_L(void);
extern void BANK_DRAM_H(void);
extern void BANK_ROM(void);

void VRAM_Display(u8 *src) __z88dk_fastcall;

// 圧縮展開
extern void dzx0_decompress_fastcall(void *, const void *);

#ifdef __cplusplus
}
#endif

#endif // MZ_SYSTEM_H_INCLUDED
