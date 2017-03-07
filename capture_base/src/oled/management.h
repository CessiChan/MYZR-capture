#ifndef SRC_MANAGEMENT_H
#define SRC_MANAGEMENT_H
#include <string.h>
#include <stdio.h>
#include <stdint.h>

#include "oled.h"



extern uint8_t num,bits_num,flag;
extern int myspi_fd;

void ShowManagement(unsigned int ssd_size,unsigned int now_bits);

#endif

