#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <hardware/adc.h>
#include <stdlib.h>

// Our handler is called from the I2C ISR, so it must complete quickly. Blocking calls /
// printing to stdio may interfere with interrupt handling.
static void i2c_slave_handler(i2c_inst_t *i2c, i2c_slave_event_t event) {
    switch (event) {
    case I2C_SLAVE_RECEIVE: // master has written some data
        if (!context.mem_address_written) {
            // writes always start with the memory address
            context.mem_address = i2c_read_byte(i2c);
            context.mem_address_written = true;
        } else {
            // save into memory
            context.mem[context.mem_address] = i2c_read_byte(i2c);
            context.changed[context.mem_address]=true;
            context.datachanged=true;
            context.mem_address++;
        }
        break;
    case I2C_SLAVE_REQUEST: // master is requesting data
        // load from memory
        i2c_write_byte(i2c, context.mem[context.mem_address]);
        context.mem_address++;
        break;
    case I2C_SLAVE_FINISH: // master has signalled Stop / Restart
        context.mem_address_written = false;
        break;
    default:
        break;
    }
}

static void setup_slave_io(){
   //Slave Assert
    gpio_init(I2C_Assert);
    gpio_pull_down(I2C_Assert);
    gpio_set_dir(I2C_Assert,GPIO_IN);

}

static void setup_slave() {
    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);

    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_init(I2C_MOD, I2C_BAUDRATE);
    // configure I2C0 for slave mode
    i2c_slave_init(I2C_MOD, I2C_DEFAULT_ADDRESS, &i2c_slave_handler);
    
}

void clear_context(){
    for (int x=0;x<256;x++){
      context.mem[x]=0;
      context.changed[x]=0;
      context.mem_address=0;
      context.mem_address_written=0;
      context.datachanged=0;
      context.asserting=0;
    }
}

void SetSlaveAssert(){
    gpio_put(LED_PIN,1);
    context.asserting=true;
    gpio_set_dir(I2C_Assert,GPIO_OUT);
    gpio_disable_pulls(I2C_Assert);
    gpio_put(I2C_Assert,1);
}

void ClearSlaveAssert(){
    gpio_set_dir(I2C_Assert,GPIO_IN);
    gpio_pull_down(I2C_Assert);
    gpio_put(I2C_Assert,0);
    context.asserting=false;
    gpio_put(LED_PIN,0);
}

//retuns 0 is not asserted, 1 if another slave asserted, 2 if THIS slave asserted
int TestSlaveAssert(){
    int r=0;
    if(context.asserting==true){
         r=2;
    }else{
        if(gpio_get(I2C_Assert)){r=1;}
    }
    return r;
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

void do_slave(){
     int dc=0;
     puts("\nI2C Slave selected\n");
     
     sleep_ms(1000+irand(5000)); //random delay starting slave.

     setup_slave_io();
     
     clear_context();     
     int sav=0;
     printf("Waiting for Assert lock \n");
     while ((sav=TestSlaveAssert())<2){
         //printf("Slave Assert:%i\n",sav);
         putchar('-');
         if(sav==0){
             SetSlaveAssert();
         }else{
             sleep_ms(100+irand(500)); //random delay.
         }//if else
         sleep_ms(100);
     }//while

     setup_slave();

     printf("/n Slave loop - Assert %i\n",TestSlaveAssert());
     printf("Listning on %02X\n",I2C_DEFAULT_ADDRESS);
     while(1){
         if(dc++>10000){
           putchar('.');
           dc=0;
         }
         //sleep_ms(1);
         if(context.datachanged==true){ 
             if(context.mem[cmd_debug]>0)puts("\nDataChanged \n");
//             Show_data(0,128,context.mem);
             if(context.mem[cmd_debug]>0)printf("Poll status %i[%i] \n",context.mem[poll],context.changed[poll]);

             if(context.changed[cmd_sl_addr ]){
                 if (TestSlaveAssert()==2){
                   if (context.mem[poll]==0){
                     //i2c address change ONLY IF asserting slave
                     i2c_address=context.mem[cmd_sl_addr];
                     puts("\nI2C SLAVE address change");
                     //restart handler on new address
                     
                     i2c_slave_deinit(I2C_MOD);
                     sleep_ms(10);                     
                     i2c_init(I2C_MOD, I2C_BAUDRATE);
                     // configure for slave mode
                     i2c_slave_init(I2C_MOD, i2c_address, &i2c_slave_handler);

                     ClearSlaveAssert();
                     printf("Listning on %02X\n",i2c_address);
                     //signal ready
                     puts("Poll Ready");
                     context.mem[poll]=poll_ready;
                     //Show_data(0,128);
                  }
                }else{
                     puts("Not Changing Address, not got assert");
                }//if else
                context.changed[cmd_sl_addr ]=false;                
             }//if context ... cmd_sl_addr 
            
             if(context.changed[poll] && context.mem[poll]==2){
                 gpio_put(LED_PIN,1);
                 puts("");
                 context.changed[poll]=poll_busy;
                 //should have all variables to run
                 for(int a=0;a<16;a++){
                     dchar.arr[a]=context.mem[quest+a];
                 }
                 for(int lumps=0;lumps<LUMPSIZE;lumps++){
                     ichar.i[lumps]=mandelbrot(dchar.dx[lumps],dchar.dy[lumps]);
                 }
                 //printf("Ichari %i\n",ichar.i);
                 for(int a=0;a<ANSWERSIZE;a++){
                     context.mem[ans+a]=ichar.arr[a];
                 }           
//                 puts("");
                 gpio_put(LED_PIN,0);
//                 Show_data(0,128,context.mem);                 
                 context.mem[poll]=poll_done;
             } //if(context.changed[poll]==poll_go
             
             context.datachanged=false;

         }//if context.datachanged==true
         tight_loop_contents();

     }//while

}

