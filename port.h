

#ifndef PORT_H_
#define PORT_H_

#define SIDE_SENSOR_MASK  (1<<0)
#define FRONT_SENSOR_MASK (1<<1)

void port_init(void);
void set_pwr_key_as_output(void);
void set_pwr_key_as_input(void);
void set_pwr_key(char val);
void relay1_on(void);
void relay1_off(void);
void relay2_on(void);
void relay2_off(void);
char get_status(void);
char Is_usb_connected(void);
char is_motion(void);
char is_sensor_present(void);
char SwitchCommChanell_USB_MCU(void);
void external_communication(void);
void led_management(void);
void check_motion(void);
void check_button(void);


#endif /* PORT_H_ */
