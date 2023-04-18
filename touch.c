
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define LCD_WIDTH 240
#define LCD_HEIGHT 320

//touch calibration
#define Xmin 3300
#define Xmax 28600
#define Ymin 4100
#define Ymax 29000

#define USE_CS 0

#define XPT_2046_spi  spi1
//#define XPT_2046_CS  5; //not used gnd
#define XPT_2046_SCK  14
#define XPT_2046_DIN  15
#define XPT_2046_DO   8

#define XPT_2046_IRQ  9

#define    TOUCH_SLEEP	0x00
#define    TOUCH_READ_Y 0x90
#define    TOUCH_READ_X 0xD0
#define    TOUCH_PRES1  0xB0
#define    TOUCH_PRES2  0xC0



uint8_t SPI_Write_Byte(uint8_t value)                                    
{   
	uint8_t rx;
	spi_write_read_blocking(XPT_2046_spi,&value,&rx,1);
        return rx;
}

uint8_t SPI_Read_Byte(uint8_t value)                                    
{
	return SPI_Write_Byte(value);
}

uint16_t Send_XPT2046_cmd (uint8_t cmd)
{
    uint8_t indata[2];

#if (USE_CS==1)
    gpio_put(XPT_2046_CS, 0);
#endif
    SPI_Write_Byte(cmd);
    sleep_us(200);
    indata[0]=SPI_Read_Byte(0);
    indata[1]=SPI_Read_Byte(0);
    return (((uint16_t)indata[0]) << 8) | ((uint16_t)indata[1]);
#if (USE_CS==1)
    gpio_put(XPT_2046_CS, 1);
#endif
}


uint16_t XPT_2046_GetRawZ(){
   unsigned long z=0;
   int a;
   //read and ignore a few values
   for(a=0;a<5;a++){
     Send_XPT2046_cmd(TOUCH_PRES1);
   }
   //average 10 positions
   for(a=0;a<8;a++){
     z=z+Send_XPT2046_cmd(TOUCH_PRES1);
   }
   return z/8;
}

uint16_t XPT_2046_GetRawY(){
   unsigned long y=0;
   int a;
   //read and ignore a few values
   for(a=0;a<5;a++){
      Send_XPT2046_cmd(TOUCH_READ_Y);
   }
   //average 10 positions
   for(a=0;a<8;a++){
     y=y+Send_XPT2046_cmd(TOUCH_READ_Y);
   }
   return y/8;
}

uint16_t XPT_2046_GetRawX(){
   unsigned long x=0;
   int a;
   //read and ignore a few values
   for(a=0;a<5;a++){
     Send_XPT2046_cmd(TOUCH_READ_X);
   }
   //average 10 positions
   for(a=0;a<8;a++){
     x=x+Send_XPT2046_cmd(TOUCH_READ_X);
   }
   return x/8;
}


void XPT_2046_Init()
{
    spi_init(XPT_2046_spi, 4000000);

    gpio_set_function(XPT_2046_DO, GPIO_FUNC_SPI);
    gpio_set_function(XPT_2046_DIN, GPIO_FUNC_SPI);
    gpio_set_function(XPT_2046_SCK, GPIO_FUNC_SPI);

#if (USE_CS==1)      
    gpio_init(XPT_2046_CS);
    gpio_set_dir(XPT_2046_CS, GPIO_OUT);
    gpio_put(XPT_2046_CS, 1);
#endif 
    gpio_init(XPT_2046_IRQ);
    gpio_set_dir(XPT_2046_IRQ, GPIO_IN);

    XPT_2046_GetRawY();

}


uint16_t XPT_2046_GetX()
{
    uint16_t x;
    x = XPT_2046_GetRawX();
//    x = LCD_WIDTH-((x-Xmin)*LCD_WIDTH)/(Xmax-Xmin);
    x = ((x-Xmin)*LCD_WIDTH)/(Xmax-Xmin);
    if(x<=0) x = 0;
    if(x>=LCD_WIDTH) x = LCD_WIDTH;
    return x;    
}

uint16_t XPT_2046_GetY()
{
    uint16_t y;
    y = XPT_2046_GetRawY();
    printf("Raw y %i\n",y);
//    y = ((y-Ymin)*LCD_HEIGHT)/(Ymax-Ymin);
    y = LCD_HEIGHT-((y-Ymin)*LCD_HEIGHT)/(Ymax-Ymin);
    if(y<=0) y = 0;
    if(y>=LCD_HEIGHT) y = LCD_HEIGHT;
    return y;
}


