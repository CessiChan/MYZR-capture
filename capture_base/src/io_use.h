/*
 * io_use.h
 *
 *  Created on: 2016��12��12��
 *      Author: user
 */

#ifndef IO_USE_H_
#define IO_USE_H_

#define	KEY_LED 				"/dev/myled"

int led_open();
int led_lighten(char led_no);


#endif /* IO_USE_H_ */
