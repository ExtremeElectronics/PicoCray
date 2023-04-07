#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <hardware/adc.h>
#include <stdlib.h>
#include <hardware/watchdog.h>

//static const uint I2C_BAUDRATE = 100000; // 100 kHz
//static const uint I2C_BAUDRATE = 400000; // 400 kHz
static const uint I2C_BAUDRATE =  2000000; // 1000 kHz

// For this example, we run both the master and slave from the same board.
// You'll need to wire pin GP4 to GP6 (SDA), and pin GP5 to GP7 (SCL).
#define I2C_SDA_PIN  10 // SDA pin
#define I2C_SCL_PIN  11 // SCL pin
#define I2C_MOD  i2c1 //i2c module 
#define I2C_Assert 13 // pico can only be promoted to a process if locally driven high, stops fighting for i2c addresses
#define I2C_MS_SELECT 22 // take pin low to become a master

#define LED_PIN 25 //Busy Led 

//start of processor addresses
#define I2C_PROC_LOWEST_ADDR  0x20 //lowest processor address
#define I2C_DEFAULT_ADDRESS  0x17 //default address when powered up, not allocated a processor

static uint i2c_address = I2C_DEFAULT_ADDRESS; //default I2C address

#define MAXProc 16//max number of processors


//lumpsize number of questions/answers sent at once
#define LUMPSIZE 24
//#define QUESTIONSIZE LUMPSIZE*2*8 //LUMPSIZE 8 byte doubles x 2 (x *Y)
#define QUESTIONSIZE (LUMPSIZE+1)*8+1 //LUMPSIZE 8 byte doubles(x) + 8 byte double (y) +code
//#define ANSWERSIZE LUMPSIZE*2 //LUMPSIZE uint16's 
#define ANSWERSIZE LUMPSIZE*1 //LUMPSIZE uint16's 

//questions location in proc I2C memory
#define quest 0x10 

//answers location
#define ans quest + QUESTIONSIZE +1 //LUMPSIZE uint16 - after poll.

#if (quest+QUESTIONSIZE>255)
  #error Variables too big for i2c ram
#endif

//register locations

//master sets slave address
#define cmd_sl_addr 0x0c //sets new processor address
#define cmd_debug 0x0d //puts sets debug in processors in debug

//processor state
 #define poll quest + QUESTIONSIZE //immediatly after questions.

//state definitions
#define poll_notset 0 //ready and free
#define poll_ready 1 //ready and free
#define poll_go 2 //start computation
#define poll_busy 3 //doing computation
#define poll_done 4 //finished computation
#define poll_wait 5 // allocated, but not yet sent questions. 
#define poll_waiting 6 // aknoledge wait
#define poll_reset 0xff

void enumerate_status(int stat){
   switch(stat) {
     case poll_notset:
       puts("Not Set");
       break;
     case poll_ready:
       puts("Ready");
       break;
     case poll_waiting:
       puts("Waiting");
       break;
     case poll_wait:
       puts("Wait");
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
       printf("Undefined %i\n",stat);     
  }
}


//shared I2Cram structure
volatile static struct
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
    uint8_t i[LUMPSIZE];
//    uint16_t i[LUMPSIZE];
     
}ichar;

//Question Structure
union {
    uint8_t arr[QUESTIONSIZE]; //20x doubles
    struct{
        double dx[LUMPSIZE];
//        double dy[LUMPSIZE];
        double dy;
        uint8_t code;
    };    
} dchar;

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
     int c=0;
     if (s==0){
         printf("\n quest:%02X[%02X], poll:%02X[1], ans:%02X[%02X] \n",quest,QUESTIONSIZE,poll,ans,ANSWERSIZE);
         printf("cmd_sl:%02X, cmd_debug:%02X \n",cmd_sl_addr ,cmd_debug);
     }
     printf("\n0000 ");
     for (int m=s; m<e; m++){
         if (s==0){
            if (m>=0 && m<quest) printf("\033[33m"); //Yellow
            if (m>=quest && m<poll) printf("\033[36m"); //blue
            if (m==poll)printf("\033[31m");//red
            if (m>=ans && m<ans+ANSWERSIZE) printf("\033[35m"); //magenta
            if (m>=ans+ANSWERSIZE) printf("\033[37m"); //white
            if (m==cmd_sl_addr)printf("\033[31m");//red
            if (m==cmd_debug)printf("\033[31m");//red
         
         }
         printf("%02X ",mem[m]) ;
         if(c==7){printf("- ");}
         if(c++>14){
             c=0;
             printf("\n\033[37m%03X0 ",m/16+1);    
         }    

     }//for
     printf("\033[37m");
}

int mandelbrot(double r,double  i) {
    int max_iterations=100;
    double ir = 0;
    double nir;
    double ii = 0;
    int steps = 0;
    while (ir*ir+ii*ii <= 4 && steps < max_iterations) {
        nir = ir*ir-ii*ii+r;
        ii = 2*ir*ii + i;
        ir = nir;
        ++steps;
    }
    return steps;
}

