#ifndef SRC_OLED_H
#define SRC_OLED_H

#include <stdio.h>
#include <unistd.h>
#include <stdint.h>
#include <iostream>
#include <string>
using namespace std;



	
#define OLED_CMD  	0	//写命令
#define OLED_DATA 	1	//写数据
#define OLED_MODE 	0


//#define OLED_CS_Clr()  OLED_CS=0
//#define OLED_CS_Set()  OLED_CS=1
extern char writenum[10];
#define OLED_DC_Clr()		write(myspi_fd,&writenum[2], 1)	//DC
#define OLED_DC_Set()		write(myspi_fd,&writenum[3], 1)

#define OLED_RST_Clr()		write(myspi_fd,&writenum[4], 1)
#define OLED_RST_Set()		write(myspi_fd,&writenum[5], 1)

#define OLED_SDIN_Clr()		write(myspi_fd,&writenum[6], 1)	//SDA/D1
#define OLED_SDIN_Set()		write(myspi_fd,&writenum[7], 1)

#define OLED_SCLK_Clr()		write(myspi_fd,&writenum[8], 1)	//SCL/D0
#define OLED_SCLK_Set()		write(myspi_fd,&writenum[9], 1)




//OLED模式设置
//0:4线串行模式
//1:并行8080模式

#define SIZE 		16
#define XLevelL		0x02
#define XLevelH		0x10
#define Max_Column	128
#define Max_Row		64
#define	Brightness	0xFF 
#define X_WIDTH 	128
#define Y_WIDTH 	64	    						  
//-----------------OLED端口定义----------------  					   

void oled_set(void); 		     

//OLED控制用函数
void OLED_WR_Byte(unsigned char dat,unsigned char cmd);	    
void OLED_Display_On(void);
void OLED_Display_Off(void);	   							   		    
void OLED_Init(void);
void OLED_Clear(void);
void OLED_Fill(unsigned char x1,unsigned char y1,unsigned char x2,unsigned char y2,unsigned char dot);
void OLED_ShowChar(unsigned char x,unsigned char y,unsigned char chr);
void OLED_ShowNum(unsigned char x,unsigned char y,unsigned int show_num,unsigned char len,unsigned char size2);   //cs_add num->show_num
void OLED_ShowString(unsigned char x,unsigned char y,char *p);	 
void OLED_Set_Pos(unsigned char x, unsigned char y);
void OLED_ShowCHinese(unsigned char x,unsigned char y,unsigned char no);
void OLED_DrawBMP(unsigned char x0, unsigned char y0, unsigned char x1, unsigned char y1, unsigned char BMP[]);

#endif  

