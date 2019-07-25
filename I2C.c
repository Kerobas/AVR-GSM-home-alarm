#include "main.h"

Int8 temperature = -127;
static char twi_state = 0;

//*******************************************************************************************************************

void i2c_init(void)
{
	TWBR = 16;
}

//*******************************************************************************************************************

void read_temperature(void)
{
	TWCR = (1<<TWINT)|(1<<TWSTA)|(1<<TWEN)|(1<<TWIE); // send start condition and enable interrupt
	twi_state = 1;
	// the rest of the job will be done in an interrupt service routine
}

//*******************************************************************************************************************

#define SLAVE_ADDRESS	0x90
#define START			0x08
#define MT_SLA_ACK		0x40
#define ST_M_ACK		0x50
#define ST_M_NACK		0x58

ISR(TWI_vect)
{
	switch(twi_state)
	{
	case 1:
		if ((TWSR & 0xF8) == START)
		{
			TWDR = SLAVE_ADDRESS | 1; // slave address and read bit
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE); // start transmission of address
			twi_state = 2;
		}
		else
		{
			TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // send stop condition
			temperature = -126;
			twi_state = 0;
		}
		break;
	case 2:
		if ((TWSR & 0xF8) == MT_SLA_ACK)
		{
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWEA) | (1<<TWIE); // get data from slave and send ACK
			twi_state = 3;
		}
		else
		{
			TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // send stop condition
			temperature = -125;
			twi_state = 0;
		}
		break;
	case 3:
		if ((TWSR & 0xF8) == ST_M_ACK)
		{
			temperature = TWDR;
			TWCR = (1<<TWINT) | (1<<TWEN) | (1<<TWIE); // get data from slave without ACK
			twi_state = 4;
		}
		else
		{
			TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // send stop condition
			temperature = -124;
			twi_state = 0;
		}
		break;
	case 4:
	default:
		TWCR = (1<<TWINT)|(1<<TWSTO)|(1<<TWEN); // send stop condition and disable TWI interrupt
		twi_state = 0;
		break;
	}
}

//*******************************************************************************************************************

void check_temperature(void)
{
	char sms[162];
	Uchar i;
	static short time_stamp_m=0;
	static short time_stamp_s=0;
	
	if(get_time_s()-time_stamp_s > 1)
	{
		if(twi_state != 0)
			temperature = -123;
		read_temperature();
		time_stamp_s = get_time_s();
	}
	
	if(config.temp_control && temperature!=-127)
	{
		if((temperature >= config.maxtemp) || (temperature <= config.mintemp))
		{
			sprintf_P(sms, PSTR("temp=%d;"), temperature);
			i=0;
			while(config.user_phone[i][0]=='+' && i<TOTAL_USER_NUMBER)
			{
				if(i != 0)
				delay_ms(1000);
				send_sms(sms, &config.user_phone[i][0]);
				i++;
			}
			config.temp_control = false;
			EEPROM_save_tempcontrol();
		}
	}
	else if(config.temp_interval_h)
	{
		if((temperature >= config.maxtemp-config.temphyst) || (temperature <= config.mintemp+config.temphyst))
			time_stamp_m = get_time_m();
		else if(get_time_m() - time_stamp_m > 60*config.temp_interval_h)
		{
			config.temp_control = true;
			EEPROM_save_tempcontrol();
		}
	}
}












