#if !defined( __MESSAGE_H__ )
#define __MESSAGE_H__

#define MESSAGE_FORMAT_STRING 0xfe
#define MESSAGE_FORMAT_DECIMAL 0xfd
#define MESSAGE_FORMAT_UNSIGNED 0xfc
#define MESSAGE_FORMAT_LONG 0xfb
#define MESSAGE_ENTRY_SIZE    5
#define MESSAGE_END_ID        0xffff

extern void message(char *msg, boolean intrpt);
extern void check_message(void);
extern int rgetchar(void);
extern int get_direction(void);
extern void sound_bell(void);
extern void print_stats(int stat_mask);
extern void message_id(short msg_id, const u8 *text);
extern const u8 *find_message(short msg_id, u8 *length);
extern short get_message(short msg_id, u8 *buffer, short size);

#endif /* not __MESSAGE_H__ */
