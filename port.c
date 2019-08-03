
#include "main.h"

void set_pwr_key_as_output(void)
{
	DDRA |= (1<<2);
}

//*******************************************************************************************************************

void set_pwr_key_as_input(void)
{
	DDRA &= ~(1<<2);
}

//*******************************************************************************************************************

void set_pwr_key(char val)
{
	if(val)
		PORTA |= (1<<2);
	else
		PORTA &= ~(1<<2);
}

//*******************************************************************************************************************

char Is_usb_connected(void)
{
	if(PIND & (1<<5))
		return 1;
	else
		return 0;
}

//*******************************************************************************************************************

void sensor_port_init(void)
{
	DDRA &= ~0x3;
	PORTA &= ~0x3;
}

//*******************************************************************************************************************

void sensor_board_detect_port_init(void)
{
	DDRB &= ~(1<<1);
	PORTB |= (1<<1); // подтяжка
}

//*******************************************************************************************************************

char is_sensor_present(void)
{
	if(PINB & (1<<1))
		return false;
	else
		return true;
}

//*******************************************************************************************************************

void button_port_init(void)
{
	DDRD |= 1<<3;
	PORTB &= ~(1<<3);
}

//*******************************************************************************************************************

char is_button_pressed(void)
{
	if(PIND & (1<<3))
		return false;
	else
		return true;
}

//*******************************************************************************************************************

void led_port_init(void)
{
	PORTB &= ~((1<<7)|(1<<6)|(1<<5));
	DDRB |= (1<<7)|(1<<6)|(1<<5);
}

//*******************************************************************************************************************
// зеленый светодиод
void green_led_on(void) 
{
	PORTB |= (1<<5);
}

//*******************************************************************************************************************
// зеленый светодиод
void green_led_off(void)
{
	PORTB &= ~(1<<5);
}

//*******************************************************************************************************************
// красный светодиод
void red_led_on(void)
{
	PORTB |= (1<<6);
}

//*******************************************************************************************************************
// красный светодиод
void red_led_off(void)
{
	PORTB &= ~(1<<6);
}

//*******************************************************************************************************************
// оранжевый светодиод
void orange_led_on(void)
{
	PORTB |= (1<<7);
}

//*******************************************************************************************************************
// оранжевый светодиод
void orange_led_off(void)
{
	PORTB &= ~(1<<7);
}

//*******************************************************************************************************************

void relay1_on(void)
{
	PORTC |= (1<<7);
}

//*******************************************************************************************************************

void relay1_off(void)
{
	PORTC &= ~(1<<7);
}

//*******************************************************************************************************************

void relay2_on(void)
{
	PORTC |= (1<<6);
}

//*******************************************************************************************************************

void relay2_off(void)
{
	PORTC &= ~(1<<6);
}

//*******************************************************************************************************************

char is_motion(void)
{
	return(PINA & config.zone_mask);
}

//*******************************************************************************************************************

char SwitchCommChanell_USB_MCU(void)
{
	static char usbConnectedPrev = -2;
	char usbConnected;

	usbConnected = Is_usb_connected() && Is_usb_connected() && Is_usb_connected();

	if(usbConnected != usbConnectedPrev) {
		if(usbConnected == 1) {
			// deinitialize the UART
			init_uart(0);

			PORTA &= 0x0F;	//RTS,CTS,DTR,DCD - inputs
			DDRA &= 0x0F;

			PORTD &= ~((1<<6)|(1<<1)|(1<<0));   //RI,RTX,DTX- inputs
			DDRD  &= ~((1<<6)|(1<<1)|(1<<0));
			
			// release FT232RL from reset
			PORTB &= ~(1<<2);
			DDRB &= ~(1<<2);
		}
		else
		{
			// hold FT232RL in reset
			PORTB &= ~(1<<2);
			DDRB |= 1<<2;

			PORTA &= ~((1<<7)|(1<<5)); //disable SLEEP mode with RTS=0; DTR =0
			DDRA |=  (1<<7)|(1<<5);

			PORTD &= ~(1<<0);
			DDRD  = (1<<0); // TXD

			init_uart(1);
		}

		usbConnectedPrev = usbConnected;

	}

	return usbConnected;
}

//*******************************************************************************************************************

void external_communication(void)
{
	while(SwitchCommChanell_USB_MCU())
		reset_soft_wdt();
}

//*******************************************************************************************************************

void port_init(void)
{
	PORTB = 0;
	DDRB = 1<<2; // hold FT232RL in reset

	PORTA = 0; //disable SLEEP mode with RTS=0; DTR =0
	DDRA =  (1<<7)|(1<<5);

	PORTD = 0;
	DDRD  = (1<<4)|(1<<1); // TXD, BUZ, LED
	
	PORTC &= ~((1<<6)|(1<<7)); // реле выключены
	DDRC |= (1<<6)|(1<<7); // порты управления реле
	
	sensor_port_init();
	sensor_board_detect_port_init();
	led_port_init();
	button_port_init();
}

//*******************************************************************************************************************
// управление светодиодами. Вызывается с периодом 10 мс
void led_management(void)
{
	static Uchar i;
	
	if((config.guard && config.led_mode==1) || (config.led_mode==2))
	{
		green_led_off(); // зеленый светодиод
		red_led_off(); // красный светодиод
		i=0;
	}
	else
	{
		if(i<50)
			i++;
		else
			i=0;
		
		if(registered_in_gsm_network)
		{
			if(i<5)
				green_led_on(); // зеленый светодиод
			else
				green_led_off(); // зеленый светодиод
		}
		else
			green_led_off(); // зеленый светодиод
		
		if(is_external_pwr())
		{
			if(is_motion())
			{
				if(i<5)
					red_led_on(); // красный светодиод
				else
					red_led_off(); // красный светодиод
			}
			else
				red_led_on(); // красный светодиод
		}
		else
			red_led_off(); // красный светодиод
	}
	
	if(time_from_button_s != 0xFFFF)
		orange_led_on(); // оранжевый светодиод
	else
		orange_led_off(); // оранжевый светодиод
}

//*******************************************************************************************************************

// функция вызывается с периодом 10 мс
void check_motion(void)
{
	static Uchar count = 0;
	static Uchar pause_count = 0;
	char motion;
	
	if(count < 255)
		count++;
	motion = is_motion() & is_motion();
	if(motion)
	{
		if(count >= config.mcount)
			motion_detected = motion;
		pause_count = 0;
	}
	else
	{
		if(pause_count < config.mpause_count)
			pause_count++;
		else
			count = false;
	}
}

//*******************************************************************************************************************

void check_button(void)
{
	static char press_button = 0;
	static char button_state = true;
	
	if(is_button_pressed())
	{
		if(press_button < 3)
			press_button++;
		else if(button_state == false)
		{
			set_val(time_from_button_s, 0);
			button_state = true;
			beep_non_block(100);
		}
	}
	else
	{
		press_button = false;
		button_state = false;
	}
}









