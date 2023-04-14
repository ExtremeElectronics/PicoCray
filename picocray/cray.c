#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "common.h"
#include "touch.c"
#include "controller.c"
#include "proc.c"
#include "controller_test.c"

int main() {
    // overclock a bit,... of course :) 

    set_sys_clock_khz(260000, true);
    
    stdio_init_all();

    setupRandom();    
    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN , GPIO_OUT);
    gpio_put(LED_PIN,1);
    gpio_put(LED_PIN,0);

    puts("\nStarting\n");

    printf("Lump %i Questions %02X[%02X] - Answers %02X[%02X]\n",LUMPSIZE,quest,QUESTIONSIZE,ans,ANSWERSIZE);
    
    if (MSselect()){
        // we are a processor
        do_proc();
    }else{
       //we are a controller
        int c='z'; //start with a run
        init_buttons();
        init_leds();
        debug=1;
        init_disp();
        setup_controller();
        puts("\n\nController\n (t)Statuses, (s)Scan, (p)dump Addr 0x20, (b)dump addr 0x17 ");
        puts(" (r)Run, (d)Debug, (u) check unallocated (c) Clear Display (x) Test Display\n");
        puts(" (y) Reset Proc Status, (@) Touch Calib/Test, (z) Run with touch and zoom\n");
        while(1){
            if (gpio_get(Start_but)==0){c='z';}

            if(c!=EOF){
                if(c=='t'){
                    debug=0; 
                    do_proc_statuses();
                }
                if(c=='s'){
                    do_I2C_scan();
                }
                if(c=='p'){
                    do_I2C_dump(0x20);
                }
                if(c=='b'){
                    do_I2C_dump(0x17);
                }
                if(c=='d'){
                    // run controller with debug
                    do_controller(2);
                }
                if(c=='r'){
                    // run controller no debug no touch
                    //inital position
                    MandXOffset=MaxMandX/1.5;
                    MandYOffset=MaxMandY/2;
                    //zoom
                    zoom=3;

                    do_controller(0);
                }
                if(c=='z'){
                    // run controller with touch
                    //position
                    MandXOffset=MaxMandX/1.5;
                    MandYOffset=MaxMandY/2;
                    //zoom
                    zoom=3;
                    do_touch_loop();
                }
                if(c=='u'){
                    do_assert_test();
                    check_for_unallocated_processors();
                }
                if(c=='x'){
                    test_display();
                }
                if(c=='c'){
                    GFX_clearScreen();
                }
                if(c=='y'){
                    resetprocstatus();
                }
                
                if(c=='@'){
                   Test_XPT_2046();
                }
            
            sleep_ms(10);
            c=getchar_timeout_us(100000);
            }//c
        }
//        do_master(0);
    }
    
    puts("\nEH?\n");
}



