#include "sys.h"
#include "delay.h"
#include "usart.h"
#include "LCD1602.h"
#include "key.h"
#include "i2c.h"
#include "SHT2X.h"
#include "bmp.h"
#include <math.h> 

#define RATIO 800		//ratio 800-1000recommended
u16 PM25_Value = 0;     		//PM = ((pmBuf[1]<<8)+pmBuf[2])/1024*8*ratio
u16 PM25_Value_max = 200;
u8 pmBuf[7];

u8 setn = 0,mode = 0;

void Get_PM(void)
{
    char i = 0;
    char j = 0;

	  RX_Cnt = 0;
    if(B_RX_OK == 1)
    {
        for(i = 0; i<8; i++)
        {
            if((RX_Buffer[i] == 0xAA)&&(RX_Buffer[i+6]==0xFF))
            {
                goto find2;
            }
        }
        goto end2;
find2:
        for(j = 0; j<7; j++)
        {
            pmBuf[j] = RX_Buffer[i+j];
        }

        PM25_Value = (unsigned int)((pmBuf[1]*256)+pmBuf[2])*5/2048.0*RATIO;
        B_RX_OK = 0;
    }
end2:
    return;
}

void keyscan()
{
	  static u8 key_val = 0;
	
		key_val = KEY_Scan(0);
		
	  if(key_val == 1)
		{ 
			 mode = !mode;
			if(mode == 0)
			{       
				USART_Cmd(USART1, DISABLE);                  		
				LCD_Write_String(0,0,"Tmp:  C  Hum:  %");
				LCD_Write_String(0,1,"P:     Kpa G:  %");
			}
			else
			{
				USART_Cmd(USART1, ENABLE); 
				LCD_Write_String(0,0,"PM2.5:    ug/m3 ");
				LCD_Write_String(0,1,"max_P:    ug/m3 ");
				
				LCD_Write_Char(7,1,PM25_Value_max/100+'0');
				LCD_Write_Char(8,1,PM25_Value_max%100/10+'0');
				LCD_Write_Char(9,1,PM25_Value_max%10+'0');
			}
		}
		if(key_val == 2)
		{ 
				if(mode == 1)
				{
						if(PM25_Value_max < 999)
						{
								PM25_Value_max += 10;
						}
						LCD_Write_Char(7,1,PM25_Value_max/100+'0');
						LCD_Write_Char(8,1,PM25_Value_max%100/10+'0');
						LCD_Write_Char(9,1,PM25_Value_max%10+'0');
				}
		}
		if(key_val == 3)
		{ 
				if(mode == 1)
				{
						if(PM25_Value_max > 1)
						{
								PM25_Value_max -= 10;
						}
						LCD_Write_Char(7,1,PM25_Value_max/100+'0');
						LCD_Write_Char(8,1,PM25_Value_max%100/10+'0');
						LCD_Write_Char(9,1,PM25_Value_max%10+'0');
				}
		}		
}


extern  float temperatureC;
extern float humidityRH;

int main(void)
{		
	  u8 count = 0,cnt = 0,flag = 0;
    u16 beep_count = 0;
	  long True_Press;
	  short  temperature;
	  short  humidity;
		short  pro = 0;
	
		delay_init();	    	 //initial of delay function	  
		NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2); //config NVIC group2:2 priority
		delay_ms(500);
	  LCD_Init();
	  KEY_Init();
	  I2C_Configuration();
    SHT2X_Init();
		delay_ms(1000);
	  IIC_Init();
	  uart_init(2400);                   //enable COM1 
	  if(mode == 0)
		{
			count = 90;
			LCD_Write_String(0,0,"Tmp:  C  Hum:  %");
			LCD_Write_String(0,1,"P:     Kpa G:  %");
		}
		else
		{
			LCD_Write_String(0,0,"PM2.5:    ug/m3 ");
			LCD_Write_String(0,1,"max_P:    ug/m3 ");
		}
		
		while(1)
		{
			  keyscan();
			  if(count++ >= 150)
				{
						count = 0;
					  if(mode == 0)
						{
							  BEEP = 0;
								if(SHT2x_Calc_T())
								{
										temperature = (short)temperatureC;
								}
								if(SHT2x_Calc_RH())
								{
										humidity = (short)humidityRH;
								}
								LCD_Write_Char(4,0,temperature/10+'0');
								LCD_Write_Char(5,0,temperature%10+'0');
								LCD_Write_Char(13,0,humidity/10+'0');
								LCD_Write_Char(14,0,humidity%10+'0');
								
								if(humidity < 60)
									pro = 10;
								else if(humidity >= 60 && humidity < 80)
									pro = 20;
								else if(humidity >= 80 && humidity < 95)
									pro = 50;
								else if(humidity >= 95)
									pro = 99;
								LCD_Write_Char(13,1,pro/10+'0');
								LCD_Write_Char(14,1,pro%10+'0');
								
								True_Press = Convert_UncompensatedToTrue((long)BMP085_Get_UT(),(long)BMP_UP_Read());

								LCD_Write_Char(2,1,True_Press/10000000+'0');
								LCD_Write_Char(3,1,True_Press%10000000/1000000+'0');
								LCD_Write_Char(4,1,True_Press%1000000/100000+'0');
								LCD_Write_Char(5,1,'.');
								LCD_Write_Char(6,1,True_Press%100000/10000+'0');
					}
					else if(mode == 1)
					{
						  if(cnt++ > 200)
							{
								  cnt = 0;
									Get_PM();
									if (PM25_Value < 75 && PM25_Value >= 50)
										PM25_Value = short(-158.2386 + 4.477158*humidity);
									else if (PM25_Value >= 75 && PM25_Value < 100)
										PM25_Value = short(-272.188 + 4.486738*humidity);
									else if (PM25_Value >= 100 && PM25_Value < 125)
										PM25_Value = short(-113.1346 + 1.782864*humidity);
									else if (PM25_Value >= 125 && PM25_Value < 150)
										PM25_Value = short(-62.09298 + 1.039333*humidity);
									else if(PM25_Value>=150&&PM25_Value<175)
										PM25_Value = short(-62.09298 + 1.039333*humidity);
									else if (PM25_Value >= 175 && PM25_Value<200)
										PM25_Value = short(-111.8768 + 1.021185*humidity);
									else if (PM25_Value >= 200 && PM25_Value<225)
										PM25_Value = short(-106.7332 + 0.8681521*humidity);
									else if (PM25_Value >= 225 && PM25_Value<250)
										PM25_Value = short(-64.42301 + 0.5860455*humidity);
									else if (PM25_Value >= 250 && PM25_Value<275)
										PM25_Value = short(-51.56085 + 0.4743491*humidity);
									else if (PM25_Value >= 275 && PM25_Value<300)
										PM25_Value = short(-40.60561 + 0.3944302*humidity);
									else if (PM25_Value >= 300)
										PM25_Value = short(-33.2296 + 0.334042*humidity);


                  if(PM25_Value > 999)
									{
										  LCD_Write_Char(7,0,'-');
											LCD_Write_Char(8,0,'-');
											LCD_Write_Char(9,0,'-');
									}
									else
									{
											LCD_Write_Char(7,0,PM25_Value%1000/100+'0');
											LCD_Write_Char(8,0,PM25_Value%100/10+'0');
											LCD_Write_Char(9,0,PM25_Value%10+'0');
									}
								
									if(PM25_Value >= PM25_Value_max && flag == 0)
									{
										  flag = 1;
										  if(beep_count++ < 2)
											{
												BEEP = 1;
											}
											else
											{
													BEEP = 0;
												  if(beep_count > 10)
													{
														beep_count = 0;
													}
											}
											flag = 0;
									}
									else
									{
											BEEP = 0;
									}
							}
					}
				}
//				delay_ms(1);
		}
}


