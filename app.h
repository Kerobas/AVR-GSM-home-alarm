
#ifndef APP_H_
#define APP_H_

#define USER_LIST					0
#define ADMIN_LIST					1
#define DEVELOPER_LIST				2

#define MAX_COUNT_OF_ERRORS			6


char send_report_to_developer_p(__flash const char *str);
void test_sms_channel_if_needed(void);
void reset_if_needed_by_schedule(void);
void send_alarm_signal_if_needed(void);
void guard_timer(void);
void send_sms_report_if_needed(void);
void send_sms_report(char *phone);
void check_registration(void);

extern char reset_command_accepted;
extern char error_code1;
short time_of_last_sms_test_m;


#endif /* APP_H_ */

