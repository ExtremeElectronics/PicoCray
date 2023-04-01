#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hardware/adc.h>
#include <stdlib.h>
#include <hardware/watchdog.h>

//static const uint I2C_BAUDRATE = 100000; // 100 kHz
static const uint I2C_BAUDRATE = 400000; // 400 kHz
//static const uint I2C_BAUDRATE = 1000000; // 1000 kHz

// For this example, we run both the master and slave from the same board.
// You'll need to wire pin GP4 to GP6 (SDA), and pin GP5 to GP7 (SCL).
#define I2C_SDA_PIN  10 // SDA pin
#define I2C_SCL_PIN  11 // SCL pin
#define I2C_MOD  i2c1 //i2c module 
#define I2C_Assert 13 // pico can only be promoted to a process if locally driven high, stops fighting for i2c addresses
#define I2C_MS_SELECT 22 // take pin low to become a master

//start of processor addresses
#define I2C_PROC_LOWEST_ADDR  0x20 //lowest processor address
#define I2C_DEFAULT_ADDRESS  0x17 //default address when owered up, not allocated a processor

static uint i2c_address = I2C_DEFAULT_ADDRESS; //default I2C address

#define MAXProc 16//max number of processors

#define LED_PIN 25 //Busy Led 

//register locations

//master sets slave address
#define cmd_sl_addr 0x0f //sets new processor address
#define cmd_debug 0x0d //puts sets debug in processors in debug

//processor state
#define poll 0x0a 
//state definitions
#define poll_ready 1
#define poll_go 2
#define poll_busy 3
#define poll_done 4
#define poll_reset 0xff

void enumerate_status(int stat){
   switch(stat) {
     case poll_ready:
       puts("Ready");
       break;
     case poll_go:
       puts("Go");
       break;
     case poll_busy:
       puts("Busy");
       break;
     case poll_done:
       puts("Done");
       break;
     case poll_reset:
       puts("Reset");
       break;    
     default:
       printf("Undefined %i",stat);     
  }
}

//lumpsize number of questions/answers sent at once
#define LUMPSIZE 10
#define QUESTIONSIZE LUMPSIZE*2*8 //LUMPSIZE 8 byte doubles x 2
#define ANSWERSIZE LUMPSIZE*2 //LUMPSIZE uint16's 

//questions location 
#define quest 0x10 

//answers location
#define ans quest + QUESTIONSIZE //LUMPSIZE uint16

#if (quest+QUESTIONSIZE>255)
  #error Variables too big for i2c ram
#endif
//shared I2Cram structure
static struct
{
    uint8_t mem[256];
    uint8_t changed[256];
    uint8_t mem_address;
    bool mem_address_written;
    bool datachanged;
    bool asserting;
} context;

//Answer Structure
union { 
    uint8_t arr[ANSWERSIZE]; //LUMPSIZE uint16's  
    uint16_t i[LUMPSIZE];
     
}ichar;

//Question Structure
union {
    uint8_t arr[QUESTIONSIZE]; //20x doubles
    struct{
        double dx[LUMPSIZE];
        double dy[LUMPSIZE];
    };    
}dchar;

int irand(int x){
    int y=rand();
    y=y & 0xffff;
    y=y*x/65536;
    return y;
}

void setupRandom(){
    adc_init();
    adc_gpio_init(26);
    adc_select_input(0);
    printf("ADC %i \n",adc_read());
    srand(adc_read());
}

bool MSselect(){
    gpio_init(I2C_MS_SELECT );
    gpio_set_dir(I2C_MS_SELECT , GPIO_IN);
    gpio_pull_up(I2C_MS_SELECT);
    sleep_ms(1);
    return gpio_get(I2C_MS_SELECT);
}


void Show_data(int s, int e,uint8_t *  mem ){
     for (int m=s; m<e; m+=16){
         puts("");
         for (int mr=0;mr<16; mr++){
             printf("%02X ",mem[m+mr]) ;
         }//for
     }//for
}

