
#include "main.h"


config_t config;

//*******************************************************************************************************************
// здесь обязательно нужен высокий уровень оптимизации, т.к. между записями EEMWE EEWE должно быть не более 4 тактов
void __attribute__((optimize("-O3"))) EEPROM_write(Ushort address, Uchar data)
{
	char tsreg;
	tsreg = SREG;
	cli();
	while(EECR & (1<<EEWE)); // Wait for completion of previous write
	EEAR = address; // Set up address and data registers 
	EEDR = data; 
	EECR = (1<<EEMWE); // Write logical one to EEMWE 
	EECR = (1<<EEWE); // Start eeprom write by setting EEWE 
	SREG = tsreg;
}

//*******************************************************************************************************************

Uchar EEPROM_read(Ushort address)
{
	while(EECR & (1<<EEWE)); // Wait for completion of previous write 
	EEAR = address; // Set up address register 
	EECR |= (1<<EERE); // Start eeprom read by writing EERE 
	return EEDR; // Return data from data register 
}

//*******************************************************************************************************************

void EEPROM_write_buf(char *buf, Ushort len, Ushort address)
{
	while(len--)
		EEPROM_write(address++, *buf++);
}

//*******************************************************************************************************************

void EEPROM_read_buf(char *buf, Ushort len, Ushort address)
{
	while(len--)
		*buf++ = EEPROM_read(address++);
}

//*******************************************************************************************************************

void eeprom_read_config(char start)
{
	EEPROM_read_buf((char*)&config, sizeof(config_t), 0);
	if(config.first_usage != 0xAB)
	{
		memset(&config, 0, sizeof(config_t));
		config.interval_of_sms_test_h = 8;
		config.interval_of_reports_h = 24;
		config.interval_power_off_report_m = 2;
		config.interval_after_button_m = 3;
		config.interval_betwing_alarms_h = 12;
		config.test_mode = true;
		config.debug_voice_enable = true;
		config.relay_enable = true;
		config.autoguard = true;
		config.zone_mask = 0x3;
		config.maxtemp = 35;
		config.mintemp = 15;
		config.temphyst = 3;
		config.temp_interval_h = 12;
		config.temp_control = true;
		config.led_mode = 1;
		config.first_usage = 0xAB;
		eeprom_save_config();
	}
	if(start)
	{
		set_val(time_from_event_s, config.time_from_event_s);
		set_val(time_from_report_s, config.time_from_report_s);
		set_val(time_from_motion_front_s, config.time_from_motion_front_s);
		set_val(time_from_motion_side_s, config.time_from_motion_side_s);
		set_val(time_without_sensor_s, config.time_without_sensor_s);
	}
}

//*******************************************************************************************************************

void eeprom_save_config(void)
{
	config.time_from_event_s = get_val(time_from_event_s);
	config.time_from_report_s = get_val(time_from_report_s);
	config.time_from_motion_front_s = get_val(time_from_motion_front_s);
	config.time_from_motion_side_s = get_val(time_from_motion_side_s);
	config.time_without_sensor_s = get_val(time_without_sensor_s);
	EEPROM_write_buf((char*)&config, sizeof(config_t), 0);
}

//*******************************************************************************************************************

void EEPROM_save_report_to_developer(void)
{
	EEPROM_write_buf((char*)&config.reports_en, sizeof(config.reports_en), 0 + (Ushort)((char*)&config.reports_en - (char*)&config));
}

//*******************************************************************************************************************

void EEPROM_save_guard(void)
{
	EEPROM_write_buf((char*)&config.guard, sizeof(config.guard), 0 + (Ushort)((char*)&config.guard - (char*)&config));
}

//*******************************************************************************************************************

void EEPROM_save_autoguard(void)
{
	EEPROM_write_buf((char*)&config.autoguard, sizeof(config.autoguard), 0 + (Ushort)((char*)&config.autoguard - (char*)&config));
}

//*******************************************************************************************************************

void EEPROM_save_tempcontrol(void)
{
	EEPROM_write_buf((char*)&config.temp_control, sizeof(config.temp_control), 0 + (Ushort)((char*)&config.temp_control - (char*)&config));
}

//*******************************************************************************************************************

void event_p(__flash const char *str)
{
	memset(config.last_event, 0, sizeof(config.last_event));
	strcpy_P(config.last_event, str);
	set_val(time_from_event_s, 0);
	EEPROM_write_buf(config.last_event, sizeof(config.last_event), 0 + (Ushort)(config.last_event - (char*)&config));
}

//*******************************************************************************************************************

void check_rst_source(void)
{
	if((MCUCSR & (1<<3)) == false) // источником перезапуска был не сторожевой таймер (скорее всего по питанию)
	{
		event_p(PSTR("Device turned on "));
	}
	MCUCSR = 0x1F; // обнуляем статусные биты
}












