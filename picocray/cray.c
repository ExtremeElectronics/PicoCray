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
        // we are a processor
        do_proc();
    }else{
       //we are a controller
        char s[3];
        debug=1;
        init_disp();
        setup_controller();
        while(1){
//            puts("\n\nController\n (t)Statuses, (s)Scan, (p)dump Addr 0x20, (b)dump addr 0x17 ");
//            puts(" (c)Controller, (d)Debug, (r)Results (u) check unallocated (x) Test Display \n(z)res to disp\n");
            puts("\n\nController\n (t)Statuses, (s)Scan, (p)dump Addr 0x20, (b)dump addr 0x17 ");
            puts(" (r)Run, (d)Debug, (u) check unallocated (c) Clear Display (x) Test Display\n");
            puts(" (y) Reset Proc Status, (@) Touch Calib/Test \n");
            scanf("%1s", s);
            //if(s[0]=='a'){
            //    do_assert_test();
            //}
            if(s[0]=='t'){
                debug=0; 
                do_proc_statuses();
            }
            if(s[0]=='s'){
                do_I2C_scan();
            }
            if(s[0]=='p'){
                do_I2C_dump(0x20);
            }
            if(s[0]=='b'){
                do_I2C_dump(0x17);
            }
            if(s[0]=='d'){
                // run controller with debug
                do_controller(2);
            }
            if(s[0]=='r'){
                // run controller no debug no touch
                //position
                MandXOffset=MaxMandX/1.5;
                MandYOffset=MaxMandY/2;
                //zoom
                zoom=3;

                do_controller(0);
            }
            if(s[0]=='z'){
                // run controller with touch
                //position
                MandXOffset=MaxMandX/1.5;
                MandYOffset=MaxMandY/2;
                //zoom
                zoom=3;
                do_touch_loop();
            }

            
            
//            if(s[0]=='r'){
//                do_results();
//            }
            if(s[0]=='u'){
//                setup_controller();
                do_assert_test();
                check_for_unallocated_processors();
            }
            if(s[0]=='x'){
                test_display();
            }
            if(s[0]=='c'){
                GFX_clearScreen();
            }
            if(s[0]=='y'){
                resetprocstatus();
            }
            
            if(s[0]=='@'){
               Test_XPT_2046();
            }
            
            sleep_ms(10);
        }
//        do_master(0);
    }
    
    puts("\nEH?\n");
}



