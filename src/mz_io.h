/*
 * mz_io.h
 *
 * Quick Disk読み込み定義 for MZ-1500
 * Copyright (c) 2026 Suikasama1968
 */

#ifndef QDIO_H_INCLUDED
#define QDIO_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#define QDPA    0x1130  //コマンド
#define QDPB    0x1131  //
#define QDPC    0x1132  //
#define QDPE    0x1134  //
#define QDPG    0x1136  //
#define QDPI    0x1138  //
#define QDIO    0xfa00  // 実行

// QD バッファー
#define QD_INFO_BLOCK   0x10f0  // インフォメーションブロック
#define QD_FILE_ATTR    0x10f0  // 属性 
#define QD_FILE_NAME    0x10f1  // ファイル名 終端 0x0d
#define QD_FILE_SIZE    0x1104  // ファイルサイズ
#define QD_DATA_ADDR    0x1106  // データアドレス
#define QD_EXEC_ADDR    0x1108  // 実行アドレス

#define STRING_BUFFER   0x11a3

// QD エラーコード
#define ENOENT  40      // そのようなファイルやディレクトリはない
#define ENODEV  41      // デバイスが存在しない
#define EEXIST  42      // ファイルが存在する
#define EWRTP   46      // ライトプロテクエラー
#define EBUSY   50      // 装置が利用できない
#define EMFILE  51      // ファイルが多すぎる
#define ENOSPC  53      // デバイスに空きがない
#define EUNFMT  54      // アンフォーマット
#define EIO     57      // I/Oエラー(ディスク異常)   

// Quick Disk インタフェース モニタ互換
extern u16 QD_open(void);
extern u16 QD_File_Search(u8 *);
extern u16 QD_read(u8 *);

// Quick Disk インタフェース
extern u16 QD_File_Read(u8 *, u8 *, u16);

#ifdef __cplusplus
}
#endif

#endif // QDIO_H_INCLUDED
