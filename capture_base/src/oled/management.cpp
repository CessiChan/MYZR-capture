#include "management.h"


unsigned char num=1,bits_num=0,flag;
int myspi_fd;

void ShowManagement(unsigned int ssd_size,unsigned int now_bits)
{
	unsigned char i;
	unsigned int t,w,m;
	
	if(flag == 1)
	{	

		OLED_Set_Pos(2,1);
		for(i=0;i<120;i++)
		{
		   
		   OLED_WR_Byte(0x00,OLED_DATA);
		
		}

		flag = 0;

	}	   
	
	t = ssd_size;
	w = now_bits;
	
	if(t>120) t = 120;
	
	m = 120-t;    // ���з�����ʾ����
	
	OLED_ShowNum(65,3,t,3,16);// ��ʾ���ڽ��յ���ֵ 
	OLED_Set_Pos(2,1);	//����������
	for(i=0;i<m;i++)    // ���з�����ʾ����
	{
	   
	   OLED_WR_Byte(0xff,OLED_DATA);
	
	}
	
	OLED_ShowNum(90,6,w,2,16);	   //���²���λ��
	
}

