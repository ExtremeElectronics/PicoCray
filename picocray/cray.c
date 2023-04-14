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
//        char s[3];
        int c;
        init_buttons();
        debug=1;
        init_disp();
        setup_controller();
        while(1){
//            puts("\n\nController\n (t)Statuses, (s)Scan, (p)dump Addr 0x20, (b)dump addr 0x17 ");
//            puts(" (c)Controller, (d)Debug, (r)Results (u) check unallocated (x) Test Display \n(z)res to disp\n");
            puts("\n\nController\n (t)Statuses, (s)Scan, (p)dump Addr 0x20, (b)dump addr 0x17 ");
            puts(" (r)Run, (d)Debug, (u) check unallocated (c) Clear Display (x) Test Display\n");
            puts(" (y) Reset Proc Status, (@) Touch Calib/Test, (z) Run with touch and zoom\n");
//            scanf("%1s", s);
            //c=getchar(nowait);
            c=getchar_timeout_us(100000);
            printf("b %i",gpio_get(Back_but));
            if (gpio_get(Start_but)==0){c='z';}
            if(c!=EOF){
//            s[0]=c;
            //if(c=='a'){
            //    do_assert_test();
            //}
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
                //position
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

            
            
//            if(c=='r'){
//                do_results();
//            }
            if(c=='u'){
//                setup_controller();
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
            }//c
        }
//        do_master(0);
    }
    
    puts("\nEH?\n");
}



