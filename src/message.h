#if !defined( __MESSAGE_H__ )
#define __MESSAGE_H__

#define MESSAGE_FORMAT_STRING 0xfe
#define MESSAGE_FORMAT_DECIMAL 0xfd
#define MESSAGE_FORMAT_UNSIGNED 0xfc
#define MESSAGE_FORMAT_LONG 0xfb
#define MESSAGE_ENTRY_SIZE    4
#define MESSAGE_END_ID        0xffff

extern void message(char *msg, boolean intrpt);
extern void check_message(void);
extern int rgetchar(void);
extern int get_direction(void);
extern void print_stats(int stat_mask);
extern void message_id(short msg_id, const uint8_t *text);
extern short format_message(short msg_id, const uint8_t *text, uint8_t *buffer,
                            short size);
extern const uint8_t *find_message(short msg_id, uint8_t *length);
extern short get_message(short msg_id, uint8_t *buffer, short size);
extern void reset_message_state(void);

#if 0 /* MZ-700/1500では未対応 */
extern void remessage(void);
extern int get_input_line(char *prompt, char *insert, char *buf,
                          char *if_cancelled, boolean add_blank,
                          boolean do_echo);
extern int input_line(int row, int col, char *insert, char *buf, int ch);
extern int do_input_line(boolean is_msg, int row, int col, char *prompt,
                         char *insert, char *buf, char *if_cancelled,
                         boolean add_blank, boolean do_echo, int first_ch);
extern void pad(char *s, short n);
extern boolean is_digit(short ch);
extern int r_index(char *str, int ch, boolean last);
#endif

#endif /* not __MESSAGE_H__ */
