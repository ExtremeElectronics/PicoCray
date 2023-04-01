#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "controller.c"
#include "proc.c"

int main() {
    // overclock a bit,... of course :) 
    set_sys_clock_khz(250000, true);
    stdio_init_all();
//    ClearSlaveAssert();
    //wait for USB (if present)
    setupRandom();    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN , GPIO_OUT);
    gpio_put(LED_PIN,1);
    sleep_ms(2000);
    gpio_put(LED_PIN,0);

    puts("\nStarting\n");

    printf("Lump %02X Questions %02X[%02X] - Answers %02X[%02X]\n",LUMPSIZE,quest,QUESTIONSIZE,ans,ANSWERSIZE);
    
    if (MSselect()){
        do_slave();
    }else{
        char s[3];
        debug=1;
        while(1){
            puts("\n\nMaster\n (a)Assert, (t)Statuses, (s)Scan, (p)dump Proc 20, (b)dump 17 \n (m)Master, (d)Debug, (r)Results \n");
            scanf("%1s", s);
            if(s[0]=='a'){
                do_assert_test();
            }
            if(s[0]=='t'){
                do_master_statuses();
            }
            if(s[0]=='s'){
              do_master_scan();
            }
            if(s[0]=='p'){
              do_master_dump(0x20);
            }
            if(s[0]=='b'){
              do_master_dump(0x17);
            }
            if(s[0]=='d'){
              do_master(3);
            }
            if(s[0]=='m'){
              do_master(0);
            }
            if(s[0]=='r'){
              do_results();
            }
            sleep_ms(10);
        }
//        do_master(0);
    }
    
    puts("\nEH?\n");
}



