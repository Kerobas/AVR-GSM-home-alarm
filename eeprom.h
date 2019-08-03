

#ifndef EEPROM_H_
#define EEPROM_H_

#define TOTAL_USER_NUMBER       5
#define TOTAL_ADMIN_NUMBER      5
#define TOTAL_DEVELOPER_NUMBER  2
#define ID_MAX_LENGTH           64
#define MAX_TIME_FROM_EVENT     (3600UL*24UL*31)

typedef struct{
	Ulong time_from_event_s;
	Ulong time_from_report_s;
	Ulong time_from_motion_front_s;
	Ulong time_from_motion_side_s;
	Ulong time_without_sensor_s;
	Ulong reset_count;
	Ulong unable_to_turn_on_modem;
	Ulong sms_reset_count;
	Ushort reset_period_h;
	Ushort interval_of_sms_test_h;
	Ushort interval_of_reports_h;
	Ushort interval_power_off_report_m;
	Ushort interval_after_button_m;
	Ushort interval_betwing_alarms_h;
	Ushort temp_interval_h;
	Uchar mcount;
	Uchar mpause_count; 
	char reports_en;
	char admin_mode;
	char admin_phone[TOTAL_ADMIN_NUMBER][13];
	char user_phone[TOTAL_USER_NUMBER][13];
	char developer_phone[TOTAL_DEVELOPER_NUMBER][13];
	char my_phone[13];
	char last_event[100];
	char id[ID_MAX_LENGTH+1];
	char debug_voice_enable;
	char relay_enable;
	char test_mode;
	char temp_control;
	char maxtemp;
	char mintemp;
	char temphyst;
	char guard;
	char autoguard;
	char zone_mask;
	char led_mode; // режим работы красного и зеленого светодиодов. 0 - светят, светят только при снятой охране, 2 - не светят
	char first_usage; // по величине этого параметра определяется чистота EEPROM
} config_t;



extern config_t config;


void EEPROM_write(Ushort address, Uchar data);
Uchar EEPROM_read(Ushort address);
void EEPROM_write_buf(char *buf, Ushort len, Ushort address);
void EEPROM_read_buf(char *buf, Ushort len, Ushort address);
void eeprom_read_config(char start);
void eeprom_save_config(void);
void EEPROM_save_reset_count(void);
void EEPROM_save_report_to_developer(void);
void EEPROM_save_guard(void);
void EEPROM_save_tempcontrol(void);
void EEPROM_save_autoguard(void);
void event_p(__flash const char *str);
void check_rst_source(void);






#endif /* EEPROM_H_ */


