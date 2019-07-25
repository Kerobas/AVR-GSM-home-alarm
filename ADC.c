

#include "main.h"

#define FILTER_COEFFICIENT 4 /*не более 6, чтобы Ushort не переполнился*/


static Ushort Integral = 0;
static Ushort filtered_value;

Ulong time_without_power_s=0;

//*******************************************************************************************************************

void ADC_init(void)
{
	ADMUX = (1<<REFS0)|(1<<MUX1)|(1<<MUX0);
	ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS2)|(1<<ADSC);
}

//*******************************************************************************************************************

void ADC_result_processing(void)
{
	Ushort data;
	
	((char*)(&data))[0] = ADCL;
	((char*)(&data))[1] = ADCH & 0x3;
	ADCSRA = (1<<ADEN)|(1<<ADPS1)|(1<<ADPS2)|(1<<ADSC); // старт следующего преобразования
	
	// экспоненциальный фильтр
	filtered_value = Integral>>FILTER_COEFFICIENT;
	Integral -= filtered_value;
	Integral += data;
}

//*******************************************************************************************************************

char is_external_pwr(void)
{
	if(get_val(filtered_value) > 248) // 248*(33/1024) = 8 В (порог - 8 В)
		return true;
	else
		return false;
}

//*******************************************************************************************************************

float get_pwr_voltage_f(void)
{
	float rez;
	rez = get_val(filtered_value);
	rez = rez*(33.0/1024.0);
	return rez;
}

//*******************************************************************************************************************

void check_power(void)
{
	if(is_external_pwr())
		time_without_power_s = 0;
	else
		time_without_power_s++;
}

//*******************************************************************************************************************

void power_control(void)
{
	Ulong time;
	static char report = true; // если питания нет изначально, то не информируем
	Uchar i;
	char sms[161];
	
	if(config.interval_power_off_report_m != 0)
	{
		time = get_val(time_without_power_s);
		
		if(time < (Ulong)60*config.interval_power_off_report_m)
		{
			report = false;
		}
		else if(report==false)
		{
			Ushort d, h, m, s;
			d = time/(3600UL*24UL);
			time -= d*(3600UL*24UL);
			h = time/3600UL;
			time -= h*3600UL;
			m = time/60;
			s = time - m*60;
			sprintf_P(sms, PSTR("External power is off during %ud%uh%um%us."), d, h, m, s);
			report = true;
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
}












