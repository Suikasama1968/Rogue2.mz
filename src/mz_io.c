/*
 * mz_io.c
 *
 * Quick Disk読み込み関数 for MZ-1500
 * Copyright (c) 2026 Suikasama1968
 */
#include <stdio.h>
#include <stdlib.h>

#include "mz_common.h"
#include "mz_io.h"

/*
    指定されたファイルをバッファに読み込む
    filename : 読み込むファイル名(ASCIIコード)
    buffer : 読み込み先バッファ
    max_size : バッファの最大サイズ
*/
u16 QD_File_Read(u8 *filename, u8 *buffer, u16 max_size)
{
    u8 retcode;

RETRY:
    retcode = QD_open();
    if (retcode != 0) goto RETRY;
    retcode = QD_File_Search(filename);
    if (retcode != 0) goto RETRY;
    if (*(u16 *)QD_FILE_SIZE > max_size) return ENOSPC;
    retcode = QD_read(buffer);
    if (retcode != 0) goto RETRY;
    return 0;
}
/*
    Quick Diskをオープンする
*/
u16 QD_open(void) __z88dk_fastcall __naked
{
    #asm
    xor A       // Aレジスタ0クリア
    ld (QDPB), A
    inc A
    ld (QDPA), A
    CALL QDIO
    JR C,   QDERROR // エラー

    ld a,   05H
    ld (QDPA), A
    CALL QDIO
    JR C,   QDERROR // エラー

    ld h,   00H
    ld l,   00H
    ret

QDERROR:
    ld h,   00H
    ld l,   A   // エラーコード
    ret
    #endasm
}
/*
    指定されたファイルを検索する
    filename : 検索するファイル名(ASCIIコード)
    戻り値 : 0=成功, 0以外=エラーコード
 */
u16 QD_File_Search(u8 *filename) 
{

    #asm
    ld  hl, 2    
    add hl, sp  // 引数の位置へ移動
                // 後ろから取り出す
    ld  e, (hl) // de = filename アドレス
    inc hl
    ld  d, (hl)

    // 文字列を0DH終端に変換
    ld  hl, STRING_BUFFER
COPYLOOP:
    ld  a,  (de)
    cp  01H
    jr  c,  COPYEND
    ld  (hl),   a
    inc hl
    inc de
    jr  COPYLOOP
COPYEND:
    ld  (hl),0dH

    // インフォメーションブロックリード
    ld  hl, 0003H
    ld  (QDPA), hl
    ld  hl, QD_INFO_BLOCK
    ld  (QDPC), hl
    LD  hl, 0040H   //　READサイズ
    LD  (QDPE),hl   // 読み込みサイズセット


SEARCHLOOP:
    CALL QDIO                   // QDサブルーチン呼び出し
    jr  c,  SEARCHNG            // Cフラグセットされていたらエラー終了
    ld  hl, STRING_BUFFER       // 探すファイル名
    ld  de, QD_FILE_NAME        // 読んだファイル名
    ld  b,  11H                 // 17文字

FILENAMECHECK:
    ld  a,  (de)
    cp  (hl)
    jr NZ,  SEARCHLOOP   //違っていたら　次のブロック読み込み
    cp  0dH
    jr  z,  SEARCHOK   // ファイル名終端 OK
    inc de
    inc hl
    djnz    FILENAMECHECK

SEARCHNG:
    ld h,  00H
    ld l,   A   // エラーコード
    ret

SEARCHOK:
    ld h,   0
    ld l,   0   // intの復帰値はHLレジスタ
    ret

    #endasm
}

/*
    指定されたファイルをバッファに読み込む
    address : 読み込み先バッファ
    戻り値 : 0=成功, 0以外=エラーコード
*/
u16 QD_read(u8 *address) __z88dk_fastcall __naked
{
    #asm
    ld  (QDPC), hl
    ld  hl, (QD_FILE_SIZE)
    ld  (QDPE), hl
    ld  hl, 0103H
    ld  (QDPA), hl
    CALL QDIO
    jr c,  READNG             // Cフラグセットされていたらエラー終了 

READOK:
    ld h,   00H
    ld l,   00H
    ret

READNG:
    ld h,  00H
    ld l,   A   // エラーコード
    ret

    #endasm

}
