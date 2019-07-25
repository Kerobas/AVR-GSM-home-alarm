
#include "main.h"

short time_of_last_sms_test_m=0;
char error_code1 = 0;

//*******************************************************************************************************************
// информирование разработчика о неполадке
char send_report_to_developer_p(__flash const char *str)
{
	if((config.developer_phone[0][0] == '+') && (config.reports_en))
	{
		send_sms_p(str, &config.developer_phone[0][0]);
		config.reports_en--; // выключаем автоинформирование разработчикаи. Если надо, разработчик включит еще раз.
		EEPROM_save_report_to_developer();
		return true;
	}
	return false;
}

//*******************************************************************************************************************

void test_sms_channel_if_needed(void)
{
	static char first = true;
	static char sent = false;
	
	if((config.interval_of_sms_test_h) && (config.my_phone[0] == '+'))
	{
		if(first)
		{
			first = false;
			time_of_last_sms_test_m = get_time_m() - config.interval_of_sms_test_h*60 + 25; // первый тест пройдет через 25 минут
		}
		
		if((get_time_m() - time_of_last_sms_test_m) > (config.interval_of_sms_test_h*60))
		{
			if(sent == false)
			{
				send_sms_p(PSTR("SMS channel test"),  &config.my_phone[0]);
				sent=true;
			}
			else if((get_time_m() - time_of_last_sms_test_m) > (config.interval_of_sms_test_h*60+15)) // даем СМС-ке 15 минут, чтобы вернуться
			{
				if(config.sms_reset_count < 60000UL) 
					config.sms_reset_count++;
				reset_mcu(true);
			}
		}
		else
			sent = false;
	}
	else
		first = true;
}

//*******************************************************************************************************************

void reset_if_needed_by_schedule(void)
{
	Ulong time;
	
	if(config.reset_period_h)
	{
		time = get_val(time_from_start_s);
		if(time > (Ulong)config.reset_period_h*3600)
			reset_mcu(false);
	}
}

//*******************************************************************************************************************

void send_alarm_signal_if_needed(void)
{
	Uchar i;
	static char first = true;
	char sms[161];
	char *ptr;
	
	if(first)
	{
		first = false;
		motion_detected = 0;
		return;
	}
	
	if(config.guard && motion_detected)
	{
		config.guard = false;
		EEPROM_save_guard();
		ptr = sms;
		ptr += sprintf_P(ptr, PSTR("Home invasion!!!"));
		if(motion_detected & FRONT_SENSOR_MASK)
			ptr += sprintf_P(ptr, PSTR(" front"));
		if(motion_detected & SIDE_SENSOR_MASK)
			ptr += sprintf_P(ptr, PSTR(" side"));
		i=0;
		while(config.user_phone[i][0]=='+' && i<TOTAL_USER_NUMBER)
		{
			if(i != 0)
				delay_ms(1000);
			send_sms(sms, &config.user_phone[i][0]);
			i++;
		}
	}
	
	if(motion_detected & FRONT_SENSOR_MASK)
		set_val(time_from_motion_front_s, 0); // обнуляем таймер с момента последнего зафиксированного движения
	if(motion_detected & SIDE_SENSOR_MASK)
		set_val(time_from_motion_side_s, 0); // обнуляем таймер с момента последнего зафиксированного движения
	motion_detected = 0;
}

//*******************************************************************************************************************

void guard_timer(void)
{
	Uchar i;
	Ushort t;
	
	if(!config.guard)
	{
		t = get_val(time_from_button_s);
		if(!config.autoguard && t!=0xFFFF)
		{
			config.autoguard = true;
			EEPROM_save_autoguard();
		}
		if((t > 60*config.interval_after_button_m) && (t != 0xFFFF))
		{
			config.guard = true;
			EEPROM_save_guard();
			set_val(time_from_button_s, 0xFFFF);
			i=0;
			while(config.user_phone[i][0]=='+' && i<TOTAL_USER_NUMBER)
			{
				if(i != 0)
					delay_ms(1000);
				send_sms_p(PSTR("Guard is on by button."), &config.user_phone[i][0]);
				i++;
			}
		}
		else if((get_val(time_from_motion_front_s) > (Ulong)3600*config.interval_betwing_alarms_h) && 
				(get_val(time_from_motion_side_s) > (Ulong)3600*config.interval_betwing_alarms_h) && config.autoguard)
		{
			config.guard = true;
			EEPROM_save_guard();
			set_val(time_from_button_s, 0xFFFF);
			i=0;
			while(config.user_phone[i][0]=='+' && i<TOTAL_USER_NUMBER)
			{
				if(i != 0)
				delay_ms(1000);
				send_sms_p(PSTR("Guard is on after motion timer."), &config.user_phone[i][0]);
				i++;
			}
		}
	}
}

//*******************************************************************************************************************

void send_sms_report_if_needed(void)
{
	Ulong t;
	
	if(config.interval_of_reports_h != 0)
	{
		t = get_val(time_from_report_s);
		if(t > (Ulong)3600*config.interval_of_reports_h)
		{
			set_val(time_from_report_s, 0);
			send_sms_report(0); // to all users
		}
	}
}

//*******************************************************************************************************************

void send_sms_report(char *phone)
{
	Uchar i;
	Ulong time;
	float voltage;
	Ushort d, h, m, s;
	char sms[162];
	char *ptr;
	char sensor;
	
	set_val(time_from_report_s, 0);
		
	time = get_val(time_from_start_s);
	d = time/(3600UL*24UL);
	time -= d*(3600UL*24UL);
	h = time/3600UL;
	time -= h*3600UL;
	m = time/60;
	s = time - m*60;
	ptr = sms;
	ptr += sprintf_P(ptr, PSTR("time=%ud%uh%um%us;"), d, h, m, s);
	
	ptr += sprintf_P(ptr, PSTR("temp=%d;"), temperature);
	
	voltage = get_pwr_voltage_f();
	if((voltage>14.5) || (voltage < 8.0))
	{
		ptr += sprintf_P(ptr, PSTR("voltage=%.1fV"), voltage);
		if(voltage > 14.5)
			ptr += sprintf_P(ptr, PSTR("(!);"));
		else
		{
			time = get_val(time_without_power_s);
			d = time/(3600UL*24UL);
			time -= d*(3600UL*24UL);
			h = time/3600UL;
			time -= h*3600UL;
			m = time/60;
			s = time - m*60;
			ptr += sprintf_P(ptr, PSTR("(low %ud%uh%um%us);"), d, h, m, s);
		}
	}
	
	ptr += sprintf_P(ptr, PSTR("guard=%d;"), config.guard);
	
	sensor = is_sensor_present();
	if(sensor==false)
	{
		time = get_val(time_without_sensor_s);
		d = time/(3600UL*24UL);
		time -= d*(3600UL*24UL);
		h = time/3600UL;
		time -= h*3600UL;
		m = time/60;
		s = time - m*60;
		ptr += sprintf_P(ptr, PSTR("sensor=0(%ud%uh%um%us);"), d, h, m, s);
	}
	
	time = get_val(time_from_motion_front_s);
	d = time/(3600UL*24UL);
	time -= d*(3600UL*24UL);
	h = time/3600UL;
	time -= h*3600UL;
	m = time/60;
	s = time - m*60;
	ptr += sprintf_P(ptr, PSTR("fmotion=%ud%uh%um%us;"), d, h, m, s);
	
	time = get_val(time_from_motion_side_s);
	d = time/(3600UL*24UL);
	time -= d*(3600UL*24UL);
	h = time/3600UL;
	time -= h*3600UL;
	m = time/60;
	s = time - m*60;
	ptr += sprintf_P(ptr, PSTR("smotion=%ud%uh%um%us;"), d, h, m, s);
	
	ptr += sprintf_P(ptr, PSTR("rstcount=%lu;"), config.reset_count);
	
	if(phone)
	{
		send_sms(sms, phone);
	}
	else
	{
		i=0;
		while(config.user_phone[i][0]=='+' && i<TOTAL_USER_NUMBER)
		{
			if(i != 0)
				delay_ms(1000);
			send_sms(sms, &config.user_phone[i][0]);
			i++;
		}
	}
}

//*******************************************************************************************************************

void check_registration(void)
{
	static short time_of_last_check = 0;
	static char count=0;
	
	if(get_time_s() - time_of_last_check > 3*60)
	{
		if(mdm_is_registered()==false)
		{
			count++;
			if(count >= 5)
				reset_mcu(true);
		}
		else
			count=0;
		time_of_last_check = get_time_s();
	}
}












