/*
 * mz_system.c
 *
 * システム制御関数 for MZ-1500
 * Copyright (c) 2026 Suikasama1968
 */

#include "mz_common.h"
#include "mz_system.h"
#include "mz_display.h"

/*
    バンクをVRAM,メモリマップドI/Oに切り替える
*/
void BANK_VRAM() __naked
{
#asm
    ld      hl, BANK_MODE
    ld      (hl), 0xe3       ; BANK_MODE = 0xe3 (VRAM)
    inc     hl
    xor     a
    ld      (hl), a          ; BANK_NUM = 0

    out     (0xe3), a        ; A=0 → VRAMバンク

    ret
#endasm
}
/*
    0x0000をバンクをDRAMに切り替える
*/
void BANK_DRAM_L() __naked
{
#asm
;    ld      hl, BANK_MODE
;    ld      (hl), 0xe0       ; BANK_MODE = 0xe0 (low DRAM)
;    inc     hl
    xor     a
;    ld      (hl), a          ; BANK_NUM = 0

    out     (0xe0), a        ; 0x0000-0x0fffをDRAMへ切り替え

    ret
#endasm
}
/*
    0xd000をバンクをDRAMに切り替える
*/
void BANK_DRAM_H() __naked
{
#asm
;    ld      hl, BANK_MODE
;    ld      (hl), 0xe1       ; BANK_MODE = 0xe1 (DRAM)
;    inc     hl
    xor     a
;    ld      (hl), a          ; BANK_NUM = 0

    out     (0xe1), a        ; DRAMバンク

    ret
#endasm
}

/*
    バンクをモニタROM,VRAM,メモリマップドI/Oに切り替える
*/
void BANK_ROM() __naked
{
#asm
;    ld      hl, BANK_MODE
;    ld      (hl), 0xe4       ; BANK_MODE = 0xe4 (ROM/VRAM/I/O)
;    inc     hl
    xor     a
;    ld      (hl), a          ; BANK_NUM = 0

    out     (0xe4), a        ; ROM/VRAM/I/Oバンク
    ret
#endasm
}

/*
    リアルタイム キーボードスキャン
    処理:キーをスキャンしてKEYDATAに格納
*/
void KEY_Scan(void) __naked
{
#asm
    di

    ; 0xe000の8255を参照できるVRAM・I/O側へ切り替える
    call    _BANK_VRAM

    ld      hl, _8255_PORT_A      ; HL = 8255 ポートA
    ld      de, KEYDATA+1         ; DE = KEYDATA+1
    ld      c, KEY_STROBE_1       ; C = 現在のストローブ値
    ld      b, 9                  ; ストローブ1～9

KEY_SCAN_LOOP:
    ld      a, c
    ld      (hl), a               ; ストローブをポートAへ出力
    ld      a, (_8255_PORT_B)     ; キー状態をポートBから取得
    cpl                            ; 押下ビットを1にする
    ld      (de), a               ; KEYDATA+1～KEYDATA+9へ格納
    inc     de
    inc     c
    djnz    KEY_SCAN_LOOP

    ; メッセージを保持している0xd000以降のDRAMへ戻す
    call    _BANK_DRAM_H

    ei
    ret
#endasm
}

/*
    仮想VRAMからVRAMに全画面転送
*/
void VRAM_Display(u8 *src) __z88dk_fastcall __naked
{
#asm

; ---- VRAM・メモリマップドI/Oへ切り替え ----------------------
push hl                    ; __z88dk_fastcall のsrcを保存
call _BANK_VRAM
pop  hl

; ---- 固定メッセージ行(0行目) --------------------------------
push hl
ld   hl, TEXT_V_VRAM
ld   de, TEXT_VRAM
ld   bc, 40
ldir

ld   hl, TEXT_V_ATTR
ld   de, TEXT_ATTR
ld   bc, 40
ldir

; ---- 横スクロールするダンジョン(1～22行目) -----------------
pop  hl
push hl
ld   bc, TEXT_VRAM_OFFSET
add  hl, bc
ld   bc, 80
add  hl, bc
ld   de, TEXT_ATTR + 40
exx

pop  hl
ld   bc, 80
add  hl, bc
ld   de, TEXT_VRAM + 40
ld   a, 22

LOOP_A:
    ; ---- VRAM転送 -------------------------------------------
    ld  bc, 40               ; 1行分(40バイト)転送
    ldir
    ld  bc, 40               ; 仮想画面の残り40桁をスキップ
    add hl, bc

    exx     // 裏レジスタに切り替え

    ; ---- アトリビュート転送 ---------------------------------------
    ld  bc, 40               ; 1行分(40バイト)転送
    ldir
    ld  bc, 40               ; 仮想画面の残り40桁をスキップ
    add hl, bc               ; 仮想次の行

    exx     // 表レジスタに交換

    dec a

    jp  nz, LOOP_A

exx

; ---- 固定ステータス行(23～24行目) ---------------------------
ld   hl, TEXT_V_VRAM + 23 * 80
ld   de, TEXT_VRAM + 23 * 40
ld   a, 2

LOOP_STATUS_TEXT:
    ld   bc, 40
    ldir
    ld   bc, 40
    add  hl, bc
    dec  a
    jp   nz, LOOP_STATUS_TEXT

ld   hl, TEXT_V_ATTR + 23 * 80
ld   de, TEXT_ATTR + 23 * 40
ld   a, 2

LOOP_STATUS_ATTR:
    ld   bc, 40
    ldir
    ld   bc, 40
    add  hl, bc
    dec  a
    jp   nz, LOOP_STATUS_ATTR

; ---- 0xd000以降をDRAMへ戻す ----------------------------------
call _BANK_DRAM_H

ret
#endasm
}
