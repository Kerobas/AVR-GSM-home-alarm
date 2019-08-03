

#include "main.h"


#define SIZE_OF_SMS_TEXT_BUF 320
#define MAX_SMS_LENGTH       160

__flash static const char help_text[MAX_SMS_LENGTH+1] = {
"get:set:userphones;adminphones;adminmode;voltage;id;lastmotion;guard;autoguard;sensor;frontzone;sidezone;rstcount;reportsinterval;resetperiod;"};

static char* set_phones(char *phones, char *dest, char max_num);

char command_to_wake_up_server = false;
char reset_command_accepted = false;


char find_phone_in_phone_list(char *phone, char list)
{
	Uchar i;
	
	if(list==ADMIN_LIST)
	{
		for(i=0;i<TOTAL_ADMIN_NUMBER;i++)
		{
			if(config.admin_phone[i][0] == '+')
				if(memcmp(phone, &config.admin_phone[i][0], 12) == 0)
					return true; // телефон найден в списке админов
		}
	}
	
	if(list==USER_LIST)
	{
		for(i=0;i<TOTAL_USER_NUMBER;i++)
		{
			if(config.user_phone[i][0] == '+')
				if(memcmp(phone, &config.user_phone[i][0], 12) == 0)
					return true; // телефон найден в списке юзеров
		}
	}
	
	if((list==DEVELOPER_LIST)||(list==ADMIN_LIST))
	{
		for(i=0;i<TOTAL_DEVELOPER_NUMBER;i++)
		{
			if(config.developer_phone[i][0] == '+')
			if(memcmp(phone, &config.developer_phone[i][0], 12) == 0)
				return true; // телефон найден в списке разработчиков
		}
	}
	return false;
}

//*******************************************************************************************************************

void process_sms_body(char *ptr)
{
	Uchar i, error;
	
	beep_ms(10);
	
	if(memcmp_P(ptr, PSTR("set:"), 4) == 0)
	{
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		ptr += 4;
		i=0;
		error = false;
		while(ptr && *ptr)
		{
			ptr = set_param(ptr);
			if(ptr)
				i++;
			else
				error = true;
		}
		if(i && !error)
		{
			eeprom_save_config();
			send_sms_p(PSTR("ok"), sms_rec_phone_number);
		}
		else
		{
			eeprom_read_config(false); // возвращаем всё в зад
			send_sms_p(PSTR("error"), sms_rec_phone_number);
		}
	}
	

	else if(memcmp_P(ptr, PSTR("get:"), 4) == 0)
	{
		char sms_text[SIZE_OF_SMS_TEXT_BUF];
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		ptr += 4;
		sms_text[0] = 0; // терминируем строку
		i=0;
		error = false;
		while(ptr && *ptr)
		{
			ptr = get_param(ptr, &sms_text[strlen(sms_text)]);
			if(ptr)
				i++;
			else
			{
				error = 1;
				break;
			}
			if(strlen(sms_text) > MAX_SMS_LENGTH)
			{
				error = 2;
				break;
			}
		}
		if(i && !error)
			send_sms(sms_text, sms_rec_phone_number);
		else if(error==1)
			send_sms_p(PSTR("error"), sms_rec_phone_number);
		else if(error==2)
			send_sms_p(PSTR("resulting SMS text is too long"), sms_rec_phone_number);
	}
	
	else if(memcmp_P(ptr, PSTR("resetadminmode!!"), 16) == 0)
	{
		config.admin_mode = false;
		eeprom_save_config();
		send_sms_p(PSTR("ok"), sms_rec_phone_number);
	}
	
	else if(memcmp_P(ptr, PSTR("resetmcu;"), 9) == 0)
	{
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		send_sms_p(PSTR("Reset command accepted."), sms_rec_phone_number);
		reset_command_accepted = true;
	}
	
	else if(memcmp_P(ptr, PSTR("SMS channel test"), 16) == 0)
	{
		if(memcmp(sms_rec_phone_number, &config.my_phone[0], 12) == 0)
			time_of_last_sms_test_m = get_time_m();
	}
	
	else if(memcmp_P(ptr, PSTR("help;"), 5) == 0)
	{
		if((find_phone_in_phone_list(sms_rec_phone_number, ADMIN_LIST) == false) && (config.admin_mode == true))
		{
			send_sms_p(PSTR("Access denied"), sms_rec_phone_number);
			return;
		}
		send_sms_p(help_text, sms_rec_phone_number);
	}
}


//*******************************************************************************************************************

char* get_param(char *str, char *sms_text)
{
	
	if(memcmp_P(str, PSTR("time;"), 5) == 0)
	{
		Ulong time;
		Ushort d, h, m, s;
		str += 5;
		time = get_val(time_from_start_s);
		d = time/(3600UL*24UL);
		time -= d*(3600UL*24UL);
		h = time/3600UL;
		time -= h*3600UL;
		m = time/60;
		s = time - m*60;
		sprintf_P(sms_text, PSTR("time=%ud%uh%um%us;"), d, h, m, s);
		return str;
	}
	
	if(memcmp_P(str, PSTR("lastmotion;"), 11) == 0)
	{
		Ulong time;
		Ushort d, h, m, s;
		str += 11;
		time = get_val(time_from_motion_front_s);
		d = time/(3600UL*24UL);
		time -= d*(3600UL*24UL);
		h = time/3600UL;
		time -= h*3600UL;
		m = time/60;
		s = time - m*60;
		sms_text += sprintf_P(sms_text, PSTR("frontmotion=%ud%uh%um%us;"), d, h, m, s);
		time = get_val(time_from_motion_side_s);
		d = time/(3600UL*24UL);
		time -= d*(3600UL*24UL);
		h = time/3600UL;
		time -= h*3600UL;
		m = time/60;
		s = time - m*60;
		sms_text += sprintf_P(sms_text, PSTR("sidemotion=%ud%uh%um%us;"), d, h, m, s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("version;"), 8) == 0)
	{
		str += 8;
		sms_text += sprintf_P(sms_text, PSTR("version="));
		sms_text += sprintf_P(sms_text, PSTR(SOFTWARE_VERSION));
		sms_text += sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("id;"), 3) == 0)
	{
		str += 3;
		sprintf_P(sms_text, PSTR("id=%s;"), config.id);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("rstcount;"), 9) == 0)
	{
		str += 9;
		sprintf_P(sms_text, PSTR("rstcount=%lu;mdm_rstcount=%lu;sms_rstcount=%lu;"), config.reset_count, config.unable_to_turn_on_modem, config.sms_reset_count);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("adminmode;"), 10) == 0)
	{
		str+=10;
		sprintf_P(sms_text, PSTR("adminmode=%d;"), config.admin_mode);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("userphones;"), 11) == 0)
	{
		Uchar n, i;
		
		str += 11;
		sms_text += sprintf_P(sms_text, PSTR("userphones="));
		n = 0;
		for(i=0;i<TOTAL_USER_NUMBER;i++)
		{
			if(config.user_phone[i][0] != '+')
				break;
			sms_text += sprintf_P(sms_text, PSTR("%s,"), &config.user_phone[i][0]);
			n++; // количество напечатанных телефонов
		}
		if(n)
			sms_text--;
		sprintf_P(sms_text, PSTR(";"));
		return(str);
	}
	
	else if(memcmp_P(str, PSTR("adminphones;"), 12) == 0)
	{
		Uchar n, i;
		
		str += 12;
		sms_text += sprintf_P(sms_text, PSTR("adminphones="));
		n=0;
		for(i=0;i<TOTAL_ADMIN_NUMBER;i++)
		{
			if(config.admin_phone[i][0] != '+')
				break;
			sms_text += sprintf_P(sms_text, PSTR("%s,"), &config.admin_phone[i][0]);
			n++; // количество напечатанных телефонов
		}
		if(n)
			sms_text--;
		sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("developerphones;"), 16) == 0)
	{
		Uchar n, i;
		
		str += 16;
		sms_text += sprintf_P(sms_text, PSTR("developerphones="));
		n=0;
		for(i=0;i<TOTAL_DEVELOPER_NUMBER;i++)
		{
			if(config.developer_phone[i][0] != '+')
				break;
			sms_text += sprintf_P(sms_text, PSTR("%s,"), &config.developer_phone[i][0]);
			n++; // количество напечатанных телефонов
		}
		if(n)
			sms_text--;
		sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("myphone;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("myphone=%s;"), config.my_phone);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("voltage;"), 8) == 0)
	{
		float voltage;
		
		str += 8;
		voltage = get_pwr_voltage_f();
		sms_text += sprintf_P(sms_text, PSTR("voltage=%.1fV"), voltage);
		if(voltage > 14.5)
			sms_text += sprintf_P(sms_text, PSTR("(overvoltage);"));
		else if(is_external_pwr())
			sms_text += sprintf_P(sms_text, PSTR("(ok);"));
		else
		{
			Ulong time;
			Ushort d, h, m, s;
			time = get_val(time_without_power_s);
			d = time/(3600UL*24UL);
			time -= d*(3600UL*24UL);
			h = time/3600UL;
			time -= h*3600UL;
			m = time/60;
			s = time - m*60;
			sms_text += sprintf_P(sms_text, PSTR("(low during %ud%uh%um%us);"), d, h, m, s);
		}
		return str;
	}
	
	else if(memcmp_P(str, PSTR("sensor;"), 7) == 0)
	{
		char sensor;
		
		str += 7;
		sensor = is_sensor_present();
		sms_text += sprintf_P(sms_text, PSTR("sensor=%d"), sensor);
		if(sensor==false)
		{
			Ulong time;
			Ushort d, h, m, s;
			time = get_val(time_without_sensor_s);
			d = time/(3600UL*24UL);
			time -= d*(3600UL*24UL);
			h = time/3600UL;
			time -= h*3600UL;
			m = time/60;
			s = time - m*60;
			sms_text += sprintf_P(sms_text, PSTR("(during %ud%uh%um%us;)"), d, h, m, s);
		}
		else
			sms_text += sprintf_P(sms_text, PSTR(";"));
		return str;
	}
	
	else if(memcmp_P(str, PSTR("resetperiod;"), 12) == 0)
	{
		str += 12;
		sprintf_P(sms_text, PSTR("resetperiod=%uh;"), config.reset_period_h);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("smsinterval;"), 12) == 0)
	{
		str += 12;
		sprintf_P(sms_text, PSTR("smsinterval=%uh;"), config.interval_of_sms_test_h);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("reportsinterval;"), 16) == 0)
	{
		str += 16;
		sprintf_P(sms_text, PSTR("reportsinterval=%uh;"), config.interval_of_reports_h);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("powerinterval;"), 14) == 0)
	{
		str += 14;
		sprintf_P(sms_text, PSTR("powerinterval=%um;"), config.interval_power_off_report_m);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("buttoninterval;"), 15) == 0)
	{
		str += 15;
		sprintf_P(sms_text, PSTR("startinterval=%um;"), config.interval_after_button_m);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("alarminterval;"), 14) == 0)
	{
		str += 14;
		sprintf_P(sms_text, PSTR("alarminterval=%uh;"), config.interval_betwing_alarms_h);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("smsresetcount;"), 14) == 0)
	{
		str += 14;
		sprintf_P(sms_text, PSTR("smsresetcount=%u;"), config.sms_reset_count);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("testmode;"), 9) == 0)
	{
		str += 9;
		sprintf_P(sms_text, PSTR("testmode=%u;"), config.test_mode);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("voice;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("voice=%u;"), config.debug_voice_enable);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("report;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("report=%u;"), config.reports_en);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("relay;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("relay=%u;"), config.relay_enable);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("lastevent;"), 10) == 0)
	{
		char d,h,m,s;
		Ulong t = get_val(time_from_event_s);
		str += 10;
		d = t/(3600UL*24UL);
		t -= (Ulong)d*(3600UL*24UL);
		h = t/(3600UL);
		t -= (Ulong)h*3600UL;
		m = t/60;
		s = t - m*60;
		sprintf_P(sms_text, PSTR("lastevent=%s %dd%dh%dm%ds ago;"), config.last_event, d, h, m, s);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("signal;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("signal=%ddBm;"), signal_strength);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("tempcontrol;"), 12) == 0)
	{
		str += 12;
		sprintf_P(sms_text, PSTR("tempcontrol=%d;"), config.temp_control);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("temp;"), 5) == 0)
	{
		str += 5;
		sprintf_P(sms_text, PSTR("temp=%d;"), temperature);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("maxtemp;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("maxtemp=%d;"), config.maxtemp);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("mintemp;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("mintemp=%d;"), config.mintemp);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("temphyst;"), 9) == 0)
	{
		str += 9;
		sprintf_P(sms_text, PSTR("temphyst=%d;"), config.temphyst);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("tempinterval;"), 13) == 0)
	{
		str += 13;
		sprintf_P(sms_text, PSTR("tempinterval=%uh;"), config.temp_interval_h);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("error;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("error1=%d;"), error_code1);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("guard;"), 6) == 0)
	{
		str += 6;
		sprintf_P(sms_text, PSTR("guard=%d;"), config.guard);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("autoguard;"), 10) == 0)
	{
		str += 10;
		sprintf_P(sms_text, PSTR("autoguard=%d;"), config.autoguard);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("frontzone;"), 10) == 0)
	{
		char temp;
		str += 10;
		if(config.zone_mask & 0x2)
			temp = 1;
		else
			temp = 0;
		sprintf_P(sms_text, PSTR("frontzone=%d;"), temp);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("sidezone;"), 9) == 0)
	{
		char temp;
		str += 9;
		if(config.zone_mask & 0x1)
			temp = 1;
		else
			temp = 0;
		sprintf_P(sms_text, PSTR("sidezone=%d;"), temp);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("ledmode;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("ledmode=%d;"), config.led_mode);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("mcount;"), 7) == 0)
	{
		str += 7;
		sprintf_P(sms_text, PSTR("mcount=%u;"), (uint16_t)config.mcount*10);
		return str;
	}
	
	else if(memcmp_P(str, PSTR("mpcount;"), 8) == 0)
	{
		str += 8;
		sprintf_P(sms_text, PSTR("mpcount=%u;"), (uint16_t)config.mpause_count*10);
		return str;
	}
	
	return false;
}

//*******************************************************************************************************************

char* set_param(char *ptr)
{
	if(memcmp_P(ptr, PSTR("resetperiod="), 12) == 0)
	{
		Ulong period;
		ptr+=12;
		if(isdigit(*ptr) == false)
		return false;
		period = strtoul(ptr, &ptr, 10);
		if(period > 60000)
			return false;
		if(*ptr != ';')
			return false;
		config.reset_period_h = period;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("smsinterval="), 12) == 0)
	{
		Ulong interval;
		ptr+=12;
		if(isdigit(*ptr) == false)
			return false;
		interval = strtoul(ptr, &ptr, 10);
		if( interval>60000 )
			return false;
		if(*ptr != ';')
			return false;
		config.interval_of_sms_test_h = interval;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("reportsinterval="), 16) == 0)
	{
		Ulong interval;
		ptr+=16;
		if(isdigit(*ptr) == false)
			return false;
		interval = strtoul(ptr, &ptr, 10);
		if( interval>60000 )
			return false;
		if(*ptr != ';')
			return false;
		config.interval_of_reports_h = interval;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("powerinterval="), 14) == 0)
	{
		Ulong interval;
		ptr+=14;
		if(isdigit(*ptr) == false)
			return false;
		interval = strtoul(ptr, &ptr, 10);
		if( interval>60000 )
			return false;
		if(*ptr != ';')
			return false;
		config.interval_power_off_report_m = interval;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("buttoninterval="), 15) == 0)
	{
		Ulong interval;
		ptr+=15;
		if(isdigit(*ptr) == false)
			return false;
		interval = strtoul(ptr, &ptr, 10);
		if( interval>60000 )
			return false;
		if(*ptr != ';')
			return false;
		config.interval_after_button_m = interval;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("alarminterval="), 14) == 0)
	{
		Ulong interval;
		ptr+=14;
		if(isdigit(*ptr) == false)
			return false;
		interval = strtoul(ptr, &ptr, 10);
		if( interval>60000 )
			return false;
		if(*ptr != ';')
			return false;
		config.interval_betwing_alarms_h = interval;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("adminmode="), 10) == 0)
	{
		Ushort mode;
		ptr+=10;
		if(*ptr == '0')
			mode = 0;
		else if(*ptr == '1')
			mode = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.admin_mode = mode;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("userphones="), 11) == 0)
	{
		ptr+=11;
		ptr = set_phones(ptr, &config.user_phone[0][0], TOTAL_USER_NUMBER);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("adminphones="), 12) == 0)
	{
		ptr+=12;
		ptr = set_phones(ptr, &config.admin_phone[0][0], TOTAL_ADMIN_NUMBER);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("developerphones="), 16) == 0)
	{
		ptr+=16;
		ptr = set_phones(ptr, &config.developer_phone[0][0], TOTAL_DEVELOPER_NUMBER);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("myphone="), 8) == 0)
	{
		ptr+=8;
		ptr = set_phones(ptr, &config.my_phone[0], 1);
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("id="), 3) == 0)
	{
		char *p;
		ptr+=3;
		p = strchr(ptr, ';');
		if(p)
		{
			if((Ushort)(p-ptr) <= ID_MAX_LENGTH)
			{
				memcpy(config.id, ptr, (Uchar)(p-ptr));
				config.id[(Uchar)(p-ptr)] = 0; // терминируем строку нулём
				ptr = p + 1;
				return ptr;
			}
			else
				return false;
		}
		else
			return false;
	}
	
	if(memcmp_P(ptr, PSTR("testmode="), 9) == 0)
	{
		Ushort mode;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=9;
		if(*ptr == '0')
			mode = 0;
		else if(*ptr == '1')
			mode = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.test_mode = mode;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("voice="), 6) == 0)
	{
		Ushort mode;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=6;
		if(*ptr == '0')
			mode = 0;
		else if(*ptr == '1')
			mode = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.debug_voice_enable = mode;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("report="), 7) == 0)
	{
		Ulong val;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=7;
		if(isdigit(*ptr) == false)
			return false;
		val = strtoul(ptr, &ptr, 10);
		if(val > 0xFF)
			return false;
		if(*ptr != ';')
			return false;
		config.reports_en = val;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("relay="), 6) == 0)
	{
		char relay;
		
		if(!find_phone_in_phone_list(sms_rec_phone_number, DEVELOPER_LIST))
			return false;
		ptr+=6;
		if(*ptr == '0')
			relay = 0;
		else if(*ptr == '1')
			relay = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.relay_enable = relay;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("tempcontrol="), 12) == 0)
	{
		char temp;
		
		ptr+=12;
		if(*ptr == '0')
			temp = 0;
		else if(*ptr == '1')
			temp = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.temp_control = temp;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("maxtemp="), 8) == 0)
	{
		Ulong t;
		ptr+=8;
		if(isdigit(*ptr) == false)
			return false;
		t = strtoul(ptr, &ptr, 10);
		if(t > 128)
			return false;
		if(*ptr != ';')
			return false;
		config.maxtemp = t;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("mintemp="), 8) == 0)
	{
		Ulong t;
		ptr+=8;
		if(isdigit(*ptr) == false)
			return false;
		t = strtoul(ptr, &ptr, 10);
		if(t > 128)
			return false;
		if(*ptr != ';')
			return false;
		config.mintemp = t;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("temphyst="), 9) == 0)
	{
		Ulong t;
		ptr+=9;
		if(isdigit(*ptr) == false)
			return false;
		t = strtoul(ptr, &ptr, 10);
		if(t > 10 || t < 1)
			return false;
		if(*ptr != ';')
			return false;
		config.temphyst = t;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("tempinterval="), 13) == 0)
	{
		Ulong t;
		ptr+=13;
		if(isdigit(*ptr) == false)
			return false;
		t = strtoul(ptr, &ptr, 10);
		if(t > 300 || t < 1)
			return false;
		if(*ptr != ';')
			return false;
		config.temp_interval_h = t;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("guard="), 6) == 0)
	{
		char temp;
		
		ptr+=6;
		if(*ptr == '0')
			temp = 0;
		else if(*ptr == '1')
			temp = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.guard = temp;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("autoguard="), 10) == 0)
	{
		char temp;
		
		ptr+=10;
		if(*ptr == '0')
			temp = 0;
		else if(*ptr == '1')
			temp = 1;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.autoguard = temp;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("ledmode="), 8) == 0)
	{
		char temp;
		
		ptr+=8;
		if(*ptr == '0')
			temp = 0;
		else if(*ptr == '1')
			temp = 1;
		else if(*ptr == '2')
			temp = 2;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.led_mode = temp;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("frontzone="), 10) == 0)
	{
		char temp;
		
		ptr+=10;
		if(*ptr == '0')
			temp = 0;
		else if(*ptr == '1')
			temp = FRONT_SENSOR_MASK;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.zone_mask &= ~FRONT_SENSOR_MASK;
		config.zone_mask |= temp;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("sidezone="), 9) == 0)
	{
		char temp;
		
		ptr+=9;
		if(*ptr == '0')
			temp = 0;
		else if(*ptr == '1')
			temp = SIDE_SENSOR_MASK;
		else
			return false;
		if(*++ptr != ';')
			return false;
		config.zone_mask &= ~SIDE_SENSOR_MASK;
		config.zone_mask |= temp;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("mcount="), 7) == 0)
	{
		Ulong c;
		ptr+=7;
		if(isdigit(*ptr) == false)
			return false;
		c = strtoul(ptr, &ptr, 10);
		if(c > 2550)
			return false;
		if(*ptr != ';')
			return false;
		config.mcount = c/10;
		ptr++;
		return ptr;
	}
	
	if(memcmp_P(ptr, PSTR("mpcount="), 8) == 0)
	{
		Ulong c;
		ptr+=8;
		if(isdigit(*ptr) == false)
			return false;
		c = strtoul(ptr, &ptr, 10);
		if(c > 2550)
			return false;
		if(*ptr != ';')
			return false;
		config.mpause_count = c/10;
		ptr++;
		return ptr;
	}
	
	return false;
}

//*******************************************************************************************************************
// принимает список телефонов через запятую, адрес назначения и максимальный размер области назначения
static char* set_phones(char *phones, char *dest, char max_num)
{
	Uchar n, i;
	char *ptr = phones;
	n=0;
	while(check_phone_string(ptr))
	{
		n++;
		if(n > max_num)
			return false;
		ptr+=12;
		if(*ptr == ';')
			break;
		else if(*ptr == ',')
		{
			ptr++;
			continue;
		}
		else
			return false;
	}
	ptr = phones;
	for(i=0;i<n;i++)
	{
		memcpy(&dest[i*13], ptr, 12);
		dest[i*13 + 12] = 0;
		ptr+=13;
	}
	memset(&dest[n*13], 0, 13*(max_num-n));
	return ptr;
}
//*******************************************************************************************************************

void incoming_call_processing(void)
{
	if(incoming_call == false)
		return;
	hang_up_call();
	incoming_call = false;
	if(call_from_user)
	{
		send_sms_report(phone_of_incomong_call);
		call_from_user = false;
	}
}

//*******************************************************************************************************************



























