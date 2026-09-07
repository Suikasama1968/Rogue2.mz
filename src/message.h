#if !defined( __MESSAGE_H__ )
#define __MESSAGE_H__

#define MESSAGE_FORMAT_STRING 0xfe
#define MESSAGE_ENTRY_SIZE    5
#define MESSAGE_END_ID        0xffff

extern void message(char *msg, boolean intrpt);
extern void remessage(void);
extern void check_message(void);
extern int rgetchar(void);
extern int get_direction(void);
extern void print_stats(int stat_mask);
extern void message_id_mz(short msg_id, const u8 *text);
extern short get_message(short msg_id, u8 *buffer, short size);
extern void message_mz(const u8 *msg, boolean intrpt);

#endif /* not __MESSAGE_H__ */
