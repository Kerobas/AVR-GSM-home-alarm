﻿
#include "main.h"



void reset_mcu(char increment)
{
#if(DEBUG==0)
	if(config.reset_count < 10000000)
		config.reset_count++;
	eeprom_save_config();
#endif
	cli(); // запрещаем прерывания и ждем перезагрузки по сторожевому таймеру
	while(1); 
}

//*******************************************************************************************************************


int main(void)
{
	char rez;
	
#if(DEBUG==0)
	_WDT_RESET(); // сброс сторожевого таймера
	WDTCR = (1<<WDE) | (1<<WDP0) | (1<<WDP1) | (1<<WDP2); // период сторожевого таймера 2.2 секунд
#endif
	port_init();
	timer0_init();
	timer1_init();
	init_uart(true);
	ADC_init();
	i2c_init();
	sei();
	
	delay_ms(100);
	
	eeprom_read_config(true);
	check_rst_source();
	
	external_communication();
	
	beep_ms(10);
	beep_ms(10);
	beep_ms(10);
	
	rez = gsm_mdm_init();
	if(rez == false)
	{
		if(config.unable_to_turn_on_modem < 80000UL)
			config.unable_to_turn_on_modem++;
		reset_mcu(true);
	}
	reset_soft_wdt();
	
	rez = mdm_wait_registration_s(600);
	if(rez == false)
		reset_mcu(true);
	registered_in_gsm_network = true;
	reset_soft_wdt();
	
	beep_ms(10);
	beep_ms(10);
	beep_ms(200);
	
	get_sms();
	delete_all_sms(); // попытка обойти глюк модема
	
	
	while(1)
    {
		check_temperature();
		if(reset_command_accepted)
			reset_mcu(true);
		reset_soft_wdt();
		external_communication();
		get_sms();
		incoming_call_processing();
		send_alarm_signal_if_needed();
		power_control();
		reset_if_needed_by_schedule();
		test_sms_channel_if_needed();
		send_sms_report_if_needed();
		guard_timer();
		check_registration();
		while(is_queue_not_empty())
			get_message_from_mdm();			
#if(DEBUG==0)
		_SLEEP();
#endif
    }
}










