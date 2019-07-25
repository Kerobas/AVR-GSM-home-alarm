
#include "main.h"

#define NET_BUF_SIZE	   512
#define PIN_CODE           "4801"

static char mdm_data[NET_BUF_SIZE];
char sms_rec_phone_number[13];
char phone_of_incomong_call[13];
	
char registered_in_gsm_network = false;
static Uchar unread_sms = true; // 
char call_from_user = false;
char incoming_call = false;
char server_state=0;
char switch_off_from_call = false;
Schar signal_strength = 0xFF;

//*******************************************************************************************************************

char* gsm_wait_for_string(unsigned short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	
	while(get_message_from_mdm()==false)
	{
		if((get_time_ms() - time_stamp) > 0)
			return false;
	}
	return mdm_data;	
}

//*******************************************************************************************************************

void wait_the_end_of_flow_from_mdm_ms(unsigned short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	volatile char dummy;
	
	while(1)
	{
		if((get_time_ms() - time_stamp) > 0) // интервал после последнего принятого байта
			return;
		if(is_queue_not_empty())
		{
			dummy = get_byte_from_queue();
			dummy = dummy;
			time_stamp = get_time_ms() + time_to_wait_ms; 
		}
	}
}

//*******************************************************************************************************************

char* gsm_poll_for_string(void)
{
	static Uchar state=0;
	static Ushort i;
	static short time_stamp;
	Uchar ch;
	
	if((get_time_s() - time_stamp) > 10)
	{
		if(state)
		{
			error_code1 = state;
			state = 0;
		}
	}
	if(is_queue_not_empty())
	{
		time_stamp = get_time_s();
		ch = get_byte_from_queue();
		switch(state)
		{
		case 0:
			if((ch != 0) && (ch != '\r') && (ch != '\n'))
			{
				mdm_data[0] = ch;
				i = 1;
				state = 3;
				if(ch == '>')
				{
					mdm_data[1] = 0;
					state = 0;
					return mdm_data;
				}
			}
		break;
		case 3:
			mdm_data[i] = ch;
			i++;
			if(i>=NET_BUF_SIZE)
			{
				i=0;
				state = 0; // переполнение входного буфера
				break;
			}
			if(i>=2)
			{
				if((mdm_data[i-2] == '\r')&&(mdm_data[i-1] == '\n'))
				{
					mdm_data[i-2] = 0;
					state = 0;
					return mdm_data;
				}
			}
			break;
		}
	}
	return 0;
}

//*******************************************************************************************************************

void gsm_mdm_power_up_down_seq(void)
{
	set_pwr_key_as_output();
	set_pwr_key(0);
	delay_ms(1500);
	set_pwr_key_as_input();
}

//*******************************************************************************************************************

char gsm_mdm_inter_pin(void)
{
	char rez;
	char *str;
	
	delay_ms(500);
	uart_send_str_p(PSTR("AT+CPIN?\r"));
	str = gsm_wait_for_string(5000);
	if(str == false)
		return false;
	if(strstr_P(str, PSTR("ERROR"))) 
		return false;
	if(strstr_P(str, PSTR("READY"))) // пин вводить не надо
		return true;
	if(strstr_P(str, PSTR("SIM PIN"))) // предложено ввести пин
	{
		char pin[5];
		mdm_wait_ok_ms(5000);
		delay_ms(500);
		sprintf_P(pin, PSTR(PIN_CODE));
		sprintf_P(mdm_data, PSTR("AT+CPIN=\"%s\"\r"), pin);
		uart_send_str(mdm_data);
		rez = mdm_wait_ok_ms(5000);
		return rez;
	}
	return false;
}

//*******************************************************************************************************************

char __attribute__((optimize("-Os"))) gsm_mdm_init(void)
{
	Uchar i;
	char rez;
	short time_stamp = get_time_s() + 600; // на инициализацию модема отводим 600 секунд
	
	if(config.relay_enable)
	{
		// снимаем, а потом заново подаем питание на модем
		// реле срабатывают только при наличии внешнего питания
		relay2_on(); // сначала отключаем 5V
		delay_ms(50); // задержка, чтобы реле не щелкали одновременно
		relay1_on(); // потом отключаем аккумулятор
		delay_ms(4000);
		relay1_off();
		delay_ms(50);
		relay2_off();
		delay_ms(4000);
	}
	
	while(1)
	{
		if((get_time_s() - time_stamp) > 0)
			return false;
		
		gsm_mdm_power_up_down_seq();
		// ставим просто задержку, т.к. сигнал STATUS при наличии внешнего питания всегда равен 1. Особенность SIM300
		delay_s(15);
		
		uart_send_str_p(PSTR("AT\r")); // автосинхронизация бодрейта
		if(mdm_wait_ok_ms(2000) == false)
			continue;
			
		delay_ms(1000);
		uart_send_str_p(PSTR("AT&F\r")); // сбрасываем все насторойки по умолчанию
		if(mdm_wait_ok_ms(5000) == false)
			continue;
		
		delay_ms(1000);
		uart_send_str_p(PSTR("ATE0\r")); // выключем эхо
		if(mdm_wait_ok_ms(5000) == false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			rez = gsm_mdm_inter_pin();
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CMGF=1\r")); // Включить текстовый формат СМС
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CNMI=0,1,0,0,0\r")); // режим приема СМС
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		for(i=0;i<3;i++)
		{
			delay_ms(1000);
			uart_send_str_p(PSTR("AT+CLIP=1\r")); // включить асинхронное информирование о входящих звонках
			rez = mdm_wait_ok_ms(5000);
			if(rez==true)
				break;
		}
		if(rez==false)
			continue;
		
		return true;
	}
}

//*******************************************************************************************************************

char hang_up_call(void)
{
	char i, rez;
	
	for(i=0;i<3;i++)
	{
		delay_ms(100);
		uart_send_str_p(PSTR("ATH\r"));
		rez = mdm_wait_ok_ms(5000);
		if(rez)
			break;
	}
	return rez;
}

//*******************************************************************************************************************

char mdm_wait_str(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	
	while(get_message_from_mdm()==false)
	{
		if((get_time_ms() - time_stamp) > 0)
			return false;
	}
	return true;
}

//*******************************************************************************************************************

char mdm_wait_sms_header_ms(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_ms() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("+CMGL:"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}


//*******************************************************************************************************************

char mdm_wait_prompt_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		
		if(mdm_data[0] == '>')
			return true;
		
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_ok_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("OK"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_ok_ms(unsigned short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_ms() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("OK"));
		if(ptr)	
			return true;
		ptr = strstr_P(mdm_data, PSTR("ERROR"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char wait_send_ok_s(char time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_s() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("SEND OK"));
		if(ptr)
			return true;
		ptr = strstr_P(mdm_data, PSTR("SEND FAIL"));
		if(ptr)
			return false;
	}
}

//*******************************************************************************************************************

char wait_message_from_mdm_ms(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	
	while(get_message_from_mdm()==false)
	{
		if((get_time_ms() - time_stamp) > 0)
			return false;
	}
	return true;
}

//*******************************************************************************************************************

char mdm_wait_registration_status_ms(short time_to_wait_ms)
{
	short time_stamp = get_time_ms() + time_to_wait_ms;
	char *ptr;
	
	while(1)
	{
		while(get_message_from_mdm()==false)
		{
			if((get_time_ms() - time_stamp) > 0)
				return false;
		}
		ptr = strstr_P(mdm_data, PSTR("+CREG:")); // статус регистрации в сети
		if(ptr)
		{
			ptr = strchr(ptr, ',');
			if(ptr)
			{
				Ulong n = strtoul(++ptr, 0, 10);
				if((n==1)||(n==5)) // регистрация в домашней сети, или роуминге
					return true;
				else
					return false;
			}
		}
	}
}

//*******************************************************************************************************************

char mdm_is_registered(void)
{
	char rez;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CREG?\r"));
	rez = mdm_wait_registration_status_ms(5000);
	if(rez == true)
	{
		registered_in_gsm_network = true;
		return true;
	}
	else
	{
		registered_in_gsm_network = false;
		return false;
	}
}

//*******************************************************************************************************************

char mdm_wait_registration_s(short time_to_wait_s)
{
	short time_stamp = get_time_s() + time_to_wait_s;
	char rez;
	
	while(1)
	{
		delay_ms(1000);
		uart_send_str_p(PSTR("AT+CREG?\r"));
		rez = mdm_wait_registration_status_ms(5000);
		if(rez == true)
			return true;
		if((get_time_s() - time_stamp) > 0)
			return false;
	}
}

//*******************************************************************************************************************

Uchar mdm_get_battery_level(void)
{
	char *ptr;
	char rez;
	Ulong level;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CBC\r"));
	while(1)
	{
		rez = mdm_wait_str(5000);
		if(rez==false)
			return 0xFF;
		ptr = strstr_P(mdm_data, PSTR("+CBC:"));
		if(ptr==false)
			continue;
		ptr = strchr(ptr, ':');
		if(!ptr)
			return 0xFF;
		ptr = strchr(ptr, ',');
		if(!ptr)
			return 0xFF;
		ptr++;
		if(isdigit(*ptr) == false)
			return 0xFF;
		level = strtoul(ptr, 0, 10);
		if(level<=0xFF)
			return (Uchar)level;
		else
			return 0xFF;
	}
}

//*******************************************************************************************************************

Ushort get_battery_voltage(void)
{
	char rez;
	char *ptr;
	Ulong voltage;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CBC\r"));
	while(1)
	{
		rez = mdm_wait_str(5000);
		if(rez==false)
			return 0xFFFF;
		ptr = strstr_P(mdm_data, PSTR("+CBC:"));
		if(ptr==false)
			continue;
		ptr = strchr(ptr, ':');
		if(!ptr)
			return 0xFFFF;
		ptr = strchr(ptr, ',');
		if(!ptr)
			return 0xFFFF;
		ptr = strchr(ptr, ',');
		if(!ptr)
			return 0xFFFF;
		ptr++;
		if(isdigit(*ptr) == false)
			return 0xFFFF;
		voltage = strtoul(ptr, 0, 10);
		if(voltage<=0xFFFF)
			return (Ushort)voltage;
		else
			return 0xFFFF;
	}
}

//*******************************************************************************************************************

void mdm_get_signal_strength(void)
{
	char *ptr;
	char rez;
	Ulong level;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CSQ\r"));
	while(1)
	{
		rez = mdm_wait_str(3000);
		if(rez==false)
		{
			signal_strength = 0xFF;
			return;
		}
		ptr = strstr_P(mdm_data, PSTR("+CSQ:"));
		if(ptr==false)
			continue;
		ptr = strchr(ptr, ':');
		if(!ptr)
		{
			signal_strength = 0xFF;
			return;
		}
		ptr+=2;
		if(isdigit(*ptr) == false)
		{
			signal_strength = 0xFF;
			return;
		}
		level = strtoul(ptr, 0, 10);
		if(level>=32)
		{
			signal_strength = 0xFF;
			return;
		}
		else
			signal_strength = -113 + (Schar)level*2; // получаем величину в дедцебеллмиливольтах дБм
		return;
	}
}

//*******************************************************************************************************************

char send_sms(char *str, char *phone)
{
	char rez;
	
	if(mdm_is_registered() == false)
		return false;
	delay_ms(100);
	sprintf_P(mdm_data, PSTR("AT+CMGS=\"%s\"\r"), phone);
	uart_send_str(mdm_data);
	if(mdm_wait_prompt_s(20) == false)
		return false;
	uart_send_str(str);
	uart_send_byte(0x1A);
	rez = mdm_wait_ok_s(11);
	return rez;
}

//*******************************************************************************************************************

char send_sms_p(__flash const char *str, char *phone)
{
	char rez;
	
	if(mdm_is_registered() == false)
		return false;
	delay_ms(100);
	sprintf_P(mdm_data, PSTR("AT+CMGS=\"%s\"\r"), phone);
	uart_send_str(mdm_data);
	if(mdm_wait_prompt_s(20) == false)
		return false;
	uart_send_str_p(str);
	uart_send_byte(0x1A);
	rez = mdm_wait_ok_s(11);
	return rez;
}

//*******************************************************************************************************************

char get_message_from_mdm(void)
{
	char *ptr;
	
	ptr = gsm_poll_for_string();
	if(ptr)
	{
		// асинхронные сообщения обрабатываем прямо здесь
		if(strstr_P(ptr, PSTR("+CMTI:"))) // пришла асинхронная индикация о принятой СМСке
		{
			unread_sms = true;
			return true;
		}
		ptr = strstr_P(ptr, PSTR("+CLIP:"));
		if(ptr)   // пришла асинхронная индикация о звонке
		{
			ptr += 6;
			ptr = strchr(ptr, '+');
			if(find_phone_in_phone_list(ptr, USER_LIST)) // ищем телефон в списке юзеров
			{
				call_from_user = true;
				memcpy(phone_of_incomong_call, ptr, 12); // сохраняем номер телефона, с которого произошел звонок
			}
			incoming_call = true; // вызов будет сброшен в основном цикле программы
		}
		return true;
	}
	return false;
}


//*******************************************************************************************************************
// проверка строки, содержащей телефонный номер. 
char check_phone_string(char *ptr)
{
	char i;
	
	if(*ptr++ != '+')
		return false;
	for(i=0;i<11;i++)
	{
		if(!isdigit(*ptr++))
			return false;
	}
	return true;
}

//*******************************************************************************************************************

char get_sms(void)
{
	char rez, i;
	char *ptr;
	Ulong index;
	
	if(unread_sms == false)
		return false;
	
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CMGL=\"ALL\"\r")); // запрашиваем список всех СМСок
	while(1)
	{
		rez = mdm_wait_sms_header_ms(5000);
		if(rez==true)
		{
			ptr = strchr(mdm_data, ':');
			if(ptr)
			{
				for(i=0;i<5;i++)
				{
					if(isdigit(*++ptr))
						break;
				}
				if(i==5)
					continue;
				index = strtoul(ptr, 0, 10);
				if(index>0xFFFF)
					continue;
			}
			else
				continue;
			ptr = strchr(ptr, ','); // ищем первую запятую в строке
			if(ptr)
			{
				ptr = strchr(ptr, ','); // ищем вторую запятую в строке
				if(ptr)
				{
					ptr = strchr(ptr, '+');
					if(ptr)
					{
						if(check_phone_string(ptr))
						{
							memcpy(sms_rec_phone_number, ptr, 12);
							sms_rec_phone_number[12] = 0;
						}
						else
							continue;
					}
				}
				else
					continue;
			}
			else
				continue;
			rez = mdm_wait_str(5000);
			if(rez==true)
			{
				wait_the_end_of_flow_from_mdm_ms(1000); // ждем окончания потока данных от модема, иными словами, flush
				process_sms_body(mdm_data);
				for(i=0;i<3;i++)
				{
					delay_ms(100);
					sprintf_P(mdm_data, PSTR("AT+CMGD=%d\r"), (Ushort)index); // удаляем обработанную СМСку
					uart_send_str(mdm_data);
					rez = mdm_wait_ok_ms(5000);
					if(rez == true)
						break;
				}
				delay_ms(100);
				return true;
			}
			else
			{
				delete_all_sms(); // попытка обойти глюк модема
				unread_sms = false;
				return false;
			}
		}
		else
		{
			delete_all_sms(); // попытка обойти глюк модема
			unread_sms = false;
			return false;
		}
	}
}

//*******************************************************************************************************************

char delete_all_sms(void)
{
	char rez;
	delay_ms(100);
	uart_send_str_p(PSTR("AT+CMGDA=\"DEL ALL\"\r"));
	rez = mdm_wait_ok_s(10);
	return rez;
}

//*******************************************************************************************************************
















