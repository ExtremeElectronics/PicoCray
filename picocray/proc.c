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
        if ( context.datachanged==true) context.data_received=true;
        break;
    default:
        break;
    }
}

static void setup_proc_io(){
   //Slave Assert
    gpio_init(I2C_Assert);
    gpio_pull_down(I2C_Assert);
    gpio_set_dir(I2C_Assert,GPIO_IN);

}

static void setup_proc() {
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

//retuns 0 is not asserted, 1 if another proc asserted, 2 if THIS slave asserted
int TestProcAssert(){
    int r=0;
    if(context.asserting==true){
         r=2;
    }else{
        if(gpio_get(I2C_Assert)){r=1;}
    }
    return r;
}

void do_proc(){
//     int dc=0;
     puts("\nI2C Slave selected\n");
     
     sleep_ms(1000+irand(5000)); //random delay starting proc.

     setup_proc_io();
     
     clear_context();     
     int sav=0;
     printf("Waiting for Assert lock \n");
     while ((sav=TestProcAssert())<2){
         //printf("Slave Assert:%i\n",sav);
         putchar('-');
         if(sav==0){
             SetSlaveAssert();
         }else{
             sleep_ms(100+irand(500)); //random delay.
         }//if else
         sleep_ms(100);
     }//while

     setup_proc();

     printf("\n Slave loop - Assert %i\n",TestProcAssert());
     printf("Listning on %02X\n",I2C_DEFAULT_ADDRESS);
     while(1){
         sleep_us(100);
//         if(dc++>10000){
//           putchar('.');
//           dc=0;
//         }
         if(context.datachanged==true && context.data_received==true){ 
             context.datachanged=false;
              context.data_received=false;
//             if(context.mem[cmd_debug]>0) printf("Poll status %i[%i] \n",context.mem[poll],context.changed[poll]);
             // address change request 
             if(context.changed[cmd_sl_addr ]){
                 context.changed[cmd_sl_addr ]=false;                
                 if(context.asserting==true){
//                     if (context.mem[poll]==0){
                         //i2c address change ONLY IF asserting proc
                         i2c_address=context.mem[cmd_sl_addr];
                         
                         puts("\nI2C SLAVE address change");
                         //restart handler on new address
                         i2c_slave_deinit(I2C_MOD);
                         sleep_ms(20);                     
                         i2c_init(I2C_MOD, I2C_BAUDRATE);
                         // configure for proc mode
                         i2c_slave_init(I2C_MOD, i2c_address, &i2c_slave_handler);

                         printf("Listning on %02X\n",i2c_address);
                         //signal ready
                         puts("Poll Ready");
                         context.mem[poll]=poll_ready;

                         ClearSlaveAssert();
                         //disconnect USB
                         //puts("Disconnecting USB\n");

                         //Show_data(0,128);
//                     }
                 }else{
                      puts("Not Changing Address, not got assert");
                 }//if else
             }//if context ... cmd_sl_addr 
             //status changed to go
             if(context.changed[poll]){
                 context.changed[poll]=false;
                 
                 if( context.mem[poll]==poll_go){
                     gpio_put(LED_PIN,1);
//                     puts("");
                     context.mem[poll]=poll_busy;
                     //should have all variables to run
                     for(int a=0;a<QUESTIONSIZE;a++){
                         dchar.arr[a]=context.mem[quest+a];
                     }
                     for(uint8_t lumps=0;lumps<LUMPSIZE;lumps++){
                         ichar.i[lumps]=mandelbrot(dchar.dx,dchar.dy);
                         dchar.dx=dchar.dx+dchar.step;
                         if(debug)printf("x%f y%f s%f %i\n",dchar.dx,dchar.dy,dchar.step,ichar.i[lumps]); 
                         tight_loop_contents();
                     }
                     for(uint8_t a=0;a<ANSWERSIZE;a++){
                         context.mem[ans+a]=ichar.arr[a];
                     }           
                     gpio_put(LED_PIN,0);
                     context.mem[poll]=poll_done;
                     context.changed[poll]=0;
                 }    //poll go
                 
                 if(context.mem[poll]==poll_wait){
                     context.mem[poll]=poll_waiting;
                     if(debug)printf("Wait\n");
                 } //poll wait

             } //if(context.changed[poll]
         }//if context.datachanged==true
         tight_loop_contents();
     }//while
}

