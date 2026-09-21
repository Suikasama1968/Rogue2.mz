/*
 * mz_io.c
 *
 * Quick Disk / Tape 読み込み関数 for MZ-700/1500
 * Copyright (c) 2026 Suikasama1968
 * 
 */

#include "mz_io.h"

/*
    MZ-700/MZ-1500判定　
*/
static uint8_t Get_System_Info(void) __naked
{
__asm
    ld  hl, 0x0002
    ld  a, (hl)     // 0x00 : MZ-700, 0xE8 : MZ-1500
    ld  h, 0x00
    ld  l, a
    ret
__endasm;
}
/*
    Quick Diskをオープンする
*/
static uint16_t QD_open(void) __naked
{
__asm
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
__endasm;
}

/*
    指定されたファイルを検索する
    filename : 検索するファイル名(ASCIIコード)
    戻り値 : 0=成功, 0以外=エラーコード
 */
static uint16_t QD_File_Search(uint8_t *filename) __naked
{
__asm
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

__endasm;
}

/*
    指定されたファイルをバッファに読み込む
    address : 読み込み先バッファ
    戻り値 : 0=成功, 0以外=エラーコード
*/
static uint16_t QD_read(uint8_t *address) __z88dk_fastcall __naked
{
__asm
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
    
__endasm;
}

/*
    指定されたファイルをバッファに読み込む
    address : 読み込み先バッファ
    戻り値 : 0=成功, 0以外=エラーコード
*/
static uint16_t Tape_read_info(void) __naked
{
__asm
    call 0x0027       // インフォメーションブロック
    ld   h, 0
    ld   l, a         // 0=成功、1=読込エラー、2=BREAK
    ret
__endasm;
}

static uint16_t Tape_read_data(void) __naked
{
__asm
    call 0x002a       // リードデータ
    ld   h, 0
    ld   l, a
    ret
__endasm;
}

/*
    指定されたファイルをバッファに読み込む
    バンクがROMに切り替わっていること
        filename : 読み込むファイル名(ASCIIコード)
        buffer : 読み込み先バッファ
        max_size : バッファの最大サイズ
*/
uint16_t File_Read(uint8_t *filename, uint8_t *buffer, uint16_t max_size)
{
    uint8_t retcode;

    if (Get_System_Info()) {
RETRY:
        retcode = QD_open();
        if (retcode != 0) goto RETRY;

        retcode = QD_File_Search(filename);
        if (retcode != 0) goto RETRY;

        if (*(uint16_t *)QD_FILE_SIZE > max_size) return ENOSPC;

        retcode = QD_read(buffer);
        if (retcode != 0) goto RETRY;

        return 0;
    }

    retcode = Tape_read_info();
    if (retcode != 0) return retcode;

    if (*(uint16_t *)TAPE_FILE_SIZE > max_size) return ENOSPC;

    /*
     * テープヘッダーのロードアドレスではなく、
     * File_Read()が指定したバッファへ読み込ませる。
     */
    *(uint16_t *)TAPE_DATA_ADDR = (uint16_t)buffer;
    return Tape_read_data();
}
