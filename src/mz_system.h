/*
 * mz_system.h
 *
 * システム制御定義 for MZ-1500
 * Copyright (c) 2026 Suikasama1968
 */

#include "mz_common.h"

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
#define KEYDATA         (SYSTEM_WORK + 0x0000)
#define KEYDATA_SIZE    0x000a

#define LOOP_COUNT      (SYSTEM_WORK + 0x000a)
#define TIME_COUNT      (SYSTEM_WORK + 0x000b)

// システム情報
#define MZ1500          (SYSTEM_WORK + 0x000c)

// 割り込み処理情報格納先
#define VECTOR  	    (SYSTEM_WORK + 0x0010)
#define TIMER   	    (SYSTEM_WORK + 0x0012)
#define COUNTER 	    (SYSTEM_WORK + 0x0014)
#define MSec    	    895

// バンク切替状態保存先
#define BANK_MODE       (SYSTEM_WORK + 0x0020)
#define BANK_NUM        (SYSTEM_WORK + 0x0021)

// デバッグ用
#define FUNC_TRACE      (SYSTEM_WORK + 0x0030)
#define ISR_COUNT       (SYSTEM_WORK + 0x0040)

// プログラム情報 格納アドレス : 0x0100-0x0xfff
#define PROGRAM_WORK        0x0100

// 0x0100-0x04cfは空き領域

// 戦闘メッセージ用バッファ
#define HIT_MESSAGE_ADDR     (PROGRAM_WORK + 0x03d0)
#define HIT_MESSAGE_SIZE     0x0050

// モンスターデータ
#define MONSTER_POOL_ADDR   (PROGRAM_WORK + 0x0420)
#define MONSTER_POOL_SIZE   0x0260
#define MONSTER_USED_ADDR   (PROGRAM_WORK + 0x0680)
#define MONSTER_USED_SIZE   0x0020

// 画面バックアップ
#define DESCS_TEXT_ADDR     (PROGRAM_WORK + 0x06a0)
#define DESCS_TEXT_SIZE     0x0168
#define DESCS_ATTR_ADDR     (PROGRAM_WORK + 0x0808)
#define DESCS_ATTR_SIZE     0x0168

// ルームデータ
#define ROOMS_ADDR          (PROGRAM_WORK + 0x0970)
#define ROOMS_SIZE          0x01b0

// メッセージ展開用バッファ
#define MESSAGE_BUFFER_ADDR (PROGRAM_WORK + 0x0b20)
#define MESSAGE_BUFFER_SIZE 0x0050

// 識別した巻き物データ
#define ID_SCROLLS_ADDR     (PROGRAM_WORK + 0x0b70)
#define ID_SCROLLS_SIZE     0x0060

// トラップデータ
#define TRAPS_ADDR          (PROGRAM_WORK + 0x0bd0)
#define TRAPS_SIZE          0x003c
#define TRAP_HIDDEN_ADDR    (PROGRAM_WORK + 0x0c0c)
#define TRAP_HIDDEN_SIZE    0x000a

// フロア内のオブジェクトリスト先頭
#define LEVEL_OBJECTS_ADDR  (PROGRAM_WORK + 0x0c20)
#define LEVEL_OBJECTS_SIZE  0x0025
#define LEVEL_MONSTERS_ADDR (PROGRAM_WORK + 0x0c46)
#define LEVEL_MONSTERS_SIZE 0x0025

// 部屋の生成・訪問状態
#define ROOM_EXISTS_ADDR    (PROGRAM_WORK + 0x0c6c)
#define ROOM_EXISTS_SIZE    0x0009
#define ROOMS_VISITED_ADDR  (PROGRAM_WORK + 0x0c75)
#define ROOMS_VISITED_SIZE  0x0009

// 一時バッファ (スタック削減。ローカル変数が使用)
#define TEMP_BUFFER_ADDR     (PROGRAM_WORK + 0x0c80)
#define TEMP_BUFFER_SIZE     0x0080

// 迷路生成境界 (再帰呼び出しのスタック削減)
#define MAZE_BOUNDS_ADDR     (PROGRAM_WORK + 0x0d00)
#define MAZE_BOUNDS_SIZE     0x0004

// 迷路生成スタック (再帰によるプログラム領域の破壊を防止)
// 1地点につき、行＋探索方向、列、方向順の3バイトを使用する
#define MAZE_STACK_ADDR      (PROGRAM_WORK + 0x0d04)
#define MAZE_STACK_ENTRIES   125
#define MAZE_STACK_SIZE      (MAZE_STACK_ENTRIES * 3)

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

// VRAMバンクのメモリマップ
#define TEXT_VRAM        0xd000
#define TEXT_ATTR        0xd800
#define TEXT_VRAM_OFFSET 0x0800

// QuickDiskから読み込む圧縮データの一時領域
#define MESG_LOAD_ADDR          0xc000
#define MESG_LOAD_SIZE          0x1000

/*
 * DRAMデータマップ (0xd000-0xffff)
 *
 * 0xd000-0xe7ff  メッセージ
 * 0xe800-0xebff  モンスターテーブル
 * 0xec00-0xed97  巻き物タイトル（実行時生成）
 * 0xeda0-0xedf3  経験値テーブル
 * 0xee00-0xee4f  杖識別テーブル
 * 0xee50-0xeea7  指輪識別テーブル
 * 0xeea8-0xeebb  杖材質名ポインタ
 * 0xeebc-0xeed1  指輪宝石名ポインタ
 * 0xeed2-0xeedb  実装済み杖種類
 * 0xeedc-0xeee6  実装済み指輪種類
 * 0xeee7-0xeeff  予約
 * 0xef00-0xf5ef  共用オブジェクトプール
 * 0xf5f0-0xf61f  オブジェクト使用状態
 * 0xf620-0xffff  空き領域
 */
#define EXTERNAL_DATA_ADDR      0xd000
#define EXTERNAL_DATA_END       0xeee7
#define EXTERNAL_DATA_SIZE      (EXTERNAL_DATA_END - EXTERNAL_DATA_ADDR)

#define MESG_ADDR               0xd000
#define MESG_SIZE               0x1800

#define MONSTER_TABLE_ADDR      0xe800

#define SCROLL_TITLES_ADDR      0xec00
#define SCROLL_TITLES_SIZE      0x0198

#define LEVEL_POINTS_ADDR       0xeda0
#define LEVEL_POINTS_SIZE       0x0054

#define ID_WANDS_ADDR           0xee00
#define ID_WANDS_SIZE           0x0050
#define ID_RINGS_ADDR           0xee50
#define ID_RINGS_SIZE           0x0058
#define WAND_MATERIALS_ADDR     0xeea8
#define WAND_MATERIALS_SIZE     0x0014
#define GEMS_ADDR               0xeebc
#define GEMS_SIZE               0x0016
#define WAND_KINDS_ADDR         0xeed2
#define WAND_KINDS_SIZE         0x000a
#define RING_KINDS_ADDR         0xeedc
#define RING_KINDS_SIZE         0x000b

// オブジェクトデータ(最大48個)
#define OBJECT_POOL_ADDR        0xef00
#define OBJECT_POOL_SIZE        0x06f0
#define OBJECT_USED_ADDR        0xf5f0
#define OBJECT_USED_SIZE        0x0030

// PCG関連
#define PCG_RAM         0xd000
#define BANK_RED        0x02
#define BANK_GREEN      0x03
#define BANK_BLUE       0x01

#define PCG_1_ADDR      0xd000
#define PCG_2_ADDR      0xd800
#define PCG_3_ADDR      0xe000
#define PCG_4_ADDR      0xe800

extern void RESET(void) __naked;
extern u8 fast_rand8(void) __naked;
extern void KEY_Scan(void) __naked;
extern void BANK_VRAM(void) __naked;
extern void BANK_DRAM_L(void) __naked;
extern void BANK_DRAM_H(void) __naked;
extern void BANK_ROM(void) __naked;

void VRAM_Display(u8 *src) __z88dk_fastcall;

// 圧縮展開
extern void dzx0_decompress_fastcall(const void *, void *) __naked;

#ifdef __cplusplus
}
#endif

#endif // MZ_SYSTEM_H_INCLUDED
