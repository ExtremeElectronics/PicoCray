#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//display includes
#include "ili9341.h"
#include "gfx.h"

//load pallette
//#include "pallette128.c"
//#include "pallette64.c"
//#include "pallette16b.c"
#include "pallette16.c"

//define the size of the mand
const double MaxMandX = 240;
const double MaxMandY = 320;

double MandYOffset=0;
double MandXOffset=0;
double zoom=1;

//save last view settings
double BackMandYOffset=0;
double BackMandXOffset=0;
double Backzoom=1;

char debug=0;
int questioncnt=0;
int lastproc=0;

uint16_t answerref[MAXProc][LUMPSIZE];
uint16_t coord[MAXProc][2];

bool addralloc[MAXProc];

uint8_t code=0xaa;

int mandx=0,mandy=0;

//uint16_t results[MaxMandX * MaxMandY];

//uint16_t *buff = NULL;
uint16_t buff[LUMPSIZE];

static void setup_controller(){
    //SDA
    gpio_init(I2C_SDA_PIN);
    gpio_set_function(I2C_SDA_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_PIN);
    //SCL
    gpio_init(I2C_SCL_PIN);
    gpio_set_function(I2C_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SCL_PIN);

    i2c_init(I2C_MOD, I2C_BAUDRATE);

    //Slave Assert
    gpio_init(I2C_Assert);
    gpio_pull_down(I2C_Assert);
    gpio_set_dir(I2C_Assert,GPIO_IN);
}

//Display routines
void init_disp(){
    LCD_initDisplay();
    LCD_setRotation(0);
//    GFX_clearScreen();
}


void write_lump_to_display(uint16_t x,uint16_t y,uint8_t * data ){
    uint8_t lump;
    for (lump=0;lump<LUMPSIZE;lump++){
        buff[lump]=mapping[data[lump] % MappingMax];
    }
    LCD_WriteBitmap(x, y, LUMPSIZE,1, buff);
}

// Processor routines.

int GetNextUnallocProc(){
    int a;
    for (a=0;a<MAXProc && addralloc[a]==true;a++){}
    if(a>MAXProc){a=-1;}
    if (debug>2)printf("Next Proc %02X\n",a);
    return a;
}

int AllocateProc(uint8_t a){
    int r=0;
    if(a>=0 && a<MAXProc){
        addralloc[a]=true;
        r=1;
    }else{
        printf("\n##################### Processor allocation failed\n");
    }
    return r;
}

bool IsProcOnAddress(uint8_t addr){
    uint8_t buf[10];
    buf[0]=0;
    int ret = i2c_read_blocking(I2C_MOD, addr, buf, 1, true);
    if (debug>2)printf("Is Slave on %02X = %i\n",addr,ret);  
    return ret==1;
}

static int get_proc_status(uint8_t addr){
    if (debug>2)printf("Get Proc Status from Addr %02X[%02X]\n",addr,addr-I2C_PROC_LOWEST_ADDR);
    int r=-99,count;
    uint8_t buf[3];
    buf[0]=poll;
    buf[1]=0;
    buf[2]=0;
    count = i2c_write_blocking(I2C_MOD, addr, buf, 1, false);
    if (count==1){
        count = i2c_read_blocking(I2C_MOD, addr, buf, 1, true); 
        if (count==1){
            r=buf[0];
        }else{
            r=-97;
            if (debug>2)printf(" cnt %i = %i/%i \n",count,buf[0],buf[1]);    
        }
    }else{
       r=-98;
       if (debug>4)printf("Can't read from  %02X\n",addr);
    }     
    if (debug>4)enumerate_status(r);
    return r;
}

static void check_proc_exists(){
    int a,sa;
    for (a=0;a<MAXProc;a++){
      sa=a+I2C_PROC_LOWEST_ADDR;
      if(get_proc_status(sa)>0){
         addralloc[a]=1;
         printf("Proc exists at %i\n",sa);
      } 
    }
}

int get_next_proc_with_status(uint8_t stat){
    if (debug>2)printf("Get Next status %i Proc\n",stat);
    int sa;
    int r=-1;
    int tries=0;
    while(1){
        lastproc++;
        if (lastproc>MAXProc)lastproc=0;
        if (addralloc[lastproc]==true){
            sa=lastproc+I2C_PROC_LOWEST_ADDR;
            if (get_proc_status(sa)==stat){
                r=lastproc;
                break;
            }
        }
        tries++;
        if(tries>MAXProc){break;};                
    }
    if (debug>2)printf("Proc Stat %i =%i\n",stat,r);
    return r;
}

void set_poll_status(uint8_t proc,uint8_t status){
    uint8_t rbuf[3];
    rbuf[0]=poll;
    rbuf[1]=status;
    //set poll flag 
    int c= i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, 2, true);
    if (c!=2){printf("\n########## Failed to send status\n");}
}


int next_mand(){
    int r=0;
    mandx++;
    if (mandx>=MaxMandX){
        mandx=0;
        mandy++;
        if (mandy>=MaxMandY){
            r=1;
        }
    }
    questioncnt++;
    return r;
}

int get_next_question(uint8_t proc){
    int r=0;
    int lump=0;
    //printf("\n");
    if (debug>0)printf("MandX:%i,MandY:%i Step:%f for proc: %i\n",mandx,mandy,dchar.step,proc);
    coord[proc][0]=mandx;
    coord[proc][1]=mandy;
    dchar.dy=mandy/MaxMandY*zoom+MandYOffset; // x/y start of lump y doesnt change 
    dchar.dx=mandx/MaxMandX*zoom+MandXOffset; 
    dchar.step=1/MaxMandX*zoom;  // stepsize for next step
    
    while (lump<LUMPSIZE && r==0){
    //    dchar.dx[lump]=mandx/MaxMandX*zoom+MandXOffset; 
        //answerref[proc][lump]=questioncnt;
        r=next_mand();
        lump++;
        }//while
    return r;
}
/*
int get_next_question(uint8_t proc){
    int r=0;
    int lump=0;
    //printf("\n");
    if (debug>0)printf("MandX:%i,MandY:%i for proc: %i\n",mandx,mandy,proc);
    coord[proc][0]=mandx;
    coord[proc][1]=mandy;
    dchar.dy=mandy/MaxMandY*zoom+MandYOffset; //only send y once ASSUMES Y WONT CHANGE DURING LUMP
    while (lump<LUMPSIZE && r==0){
        dchar.dx[lump]=mandx/MaxMandX*zoom+MandXOffset; 
        answerref[proc][lump]=questioncnt;
        r=next_mand();
        lump++;
        }//while
   return r;
}
*/

int send_questions_to_proc(uint8_t proc){
    uint8_t rbuf[QUESTIONSIZE+2];
    code++;
    if (code==0)code++;
    dchar.code=code;
    uint8_t ok=0;
    uint8_t addr=proc+I2C_PROC_LOWEST_ADDR;
    int tries=0;
    rbuf[0]=quest; //mem address dor data
    for(int a=0;a<QUESTIONSIZE;a++)rbuf[a+1]=dchar.arr[a];
    rbuf[QUESTIONSIZE+1]=poll_go; //set poll to go.
    if (debug>2) printf("Send to Proc:%i \n",proc);
    if (debug>5) {
        Show_data(0,QUESTIONSIZE,dchar.arr);
        printf("\n");
    }    

    while(ok==0 && tries<3){
        tries++;
        ok=1;
        if(i2c_write_blocking(I2C_MOD, addr, rbuf, QUESTIONSIZE+2, true)!=QUESTIONSIZE+2){
            printf("\n####### Failed to send question to proc %i\n",proc);
            ok=0;
        }
    }//while

    return ok;
}

int do_answers(uint8_t proc){
     uint8_t rbuf[ANSWERSIZE+1];
     rbuf[0]=ans;
     int c;
     int r=0;
     if (debug>3) printf("Getting Answers from Proc %i\n",proc);
     c=i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, 1, false);
     c= i2c_read_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, ANSWERSIZE, true);
     if (c==ANSWERSIZE){
         for(int a=0;a<ANSWERSIZE;a++){
             ichar.arr[a]=rbuf[a];
         }
         if (debug>5){
             Show_data(0,ANSWERSIZE,ichar.arr);
             printf("\n");
         }
         write_lump_to_display(coord[proc][0],coord[proc][1],ichar.i );    
//         write_lump_to_display(coord[proc][0],coord[proc][1],ichar.arr );
         r=1;
         if (debug>1) puts("= Answer *****\n");  
     }else{
         printf("\n############### Failed to get answers from proc %i\n",proc);
     }
     return r; 
}

void check_for_unallocated_processors(){
    uint8_t  sa[3];
    if (debug>2)puts("Check New Procs");
    if(IsProcOnAddress(I2C_DEFAULT_ADDRESS)){
        puts("Unallocated proc found");
        int np=GetNextUnallocProc();
        if (np>=0){
            sa[0]=cmd_sl_addr;
            sa[1]=np+I2C_PROC_LOWEST_ADDR ;
            sa[2]=0;
            int c=i2c_write_blocking(I2C_MOD, I2C_DEFAULT_ADDRESS, sa, 2, true);
            if(c==2){
                printf("\n*************** Unallocated Proc/ADDR Change to %02X[%02X] *****************\n\n",np,sa[1]);
                AllocateProc(np);
            }else{
                printf("\n############## Unallocated Proc/ADDR Change to %02X[%02X] FAILED  ################\n\n",np,sa[1]);
            }
            sleep_ms(350); //and wait for reply.
        }else{
          printf("\n############# Out of spare Procs\n");
        }
   }//if proc on addres     
}


void do_controller(char dbg){
    mandx=0,mandy=0;

    questioncnt=0;
    debug=dbg;
    puts("\nI2C Controller Start\n");

    int proc;
    int stat;
    int tries=3;

    puts("Check existing Procs");    
    check_proc_exists();
    puts("Master Loop\n");
    
    int go=2;//has procs to send.
    
    while(go>0){
        //check for un allocated Processors        
        if (gpio_get(I2C_Assert)==1){
            gpio_put(Led1,1);            
            check_for_unallocated_processors();
        }else{
            gpio_put(Led1,0);    
        }//if assert

        //check free processors 
        if (debug>2)puts("Check Free Procs");

        proc=get_next_proc_with_status(poll_ready);
        if (proc>=0 && go==2){ 
            if (debug>2) printf("Proc %i free add process\n",proc);
            if (get_next_question(proc)==0){
                if (debug>2) printf("SendQuestion process\n");
                //send 2*lump floats to var + poll=poll_go
                if (send_questions_to_proc(proc)==0){
                    printf("\n### Send failed %i\n",proc);                 
                }            
                //after sending questions set proc to go, check they are received, by checking status
                stat=get_proc_status(proc+I2C_PROC_LOWEST_ADDR);
/*              if(stat!=poll_busy){
                    putchar('!');
                    sleep_ms(1);
                    if (debug>2) printf("\n!!!!!!!! Proc not busy %i - %i !!!!!!!!! \n",proc,stat);
                    if (send_questions_to_proc(proc)==0){
                         printf("\n### 2nd Send failed %i\n",proc);
                    } //send questions
                }//poll busy
*/              
                tries=3;  
                while(stat!=poll_busy && tries>0){
                    putchar('!');
                    if (debug>2) printf("\n!!!!!!!! Proc not busy %i - %i !!!!!!!!! \n",proc,stat);
                    if (send_questions_to_proc(proc)==0){
                         printf("\n### 2nd Send failed %i\n",proc);     
                    } //send questions
                    stat=get_proc_status(proc+I2C_PROC_LOWEST_ADDR);
                    tries--;
                }//poll busy

               

            }else{
              printf("\n\n*********** Finished ***********");
              //set finished - wait for all done
              go=1;//no further questions wait till all answers.
            }//get next question
        }else{
            putchar('~');
            if (debug>2) puts("No free procs");
        }

        //check for procs that have done.
        if (debug>2)puts("Check procs done" );
//        proc=getnextdoneproc();
        proc=get_next_proc_with_status(poll_done);
        if (proc>=0){
            // has Done procs
            if (debug>2)printf("read answer from proc %i\n",proc);
            if (do_answers(proc)){             
                //after getting answers set proc back to ready.
                set_poll_status( proc,poll_ready);
            }else{
                printf("\n############## Read answer failed\n");  
            }    
        }else{
            if (debug>2)printf("No done procs\n");
            putchar('?');
            //if finished(go=1) set go=2 for complete
            if (go==1){
                go=0; //have all answers stop
                puts("\n************ Complete **************");
            }
        }  // has done procs
     }//while
}


int wait_for_touch(){
    printf("Wait for Touch\n");

    uint16_t x=0,y=0;
    double windowwidth=0,windowheight=0;
    double zoomspeed=2;
    double zoomlast=zoom;

    int touched=0,back=0,again=0;;
    while(touched==0){
       // if touched.
       if (gpio_get(XPT_2046_IRQ)==0){
         sleep_ms(20);
         x=MaxMandX-XPT_2046_GetX();
         y=MaxMandY-XPT_2046_GetY();
         

         //set as touched if not near the edges as these are random
         if(x>10 && x<MaxMandX-10 && y>10 && y<MaxMandY-10){
             touched=1;
             printf("Touch X:%i Y:%i \n",x,y );
             GFX_fillRect(x-5,y-5,10,10,0);
         }
       }


       //check buttons
       if(gpio_get(Back_but)==0){
          touched=1;
          back=1;
       }
       if(gpio_get(Start_but)==0){
          touched=1;
          again=1;
       }

    }

    if(debug>1)printf("Before: MandXoff:%f, MandYoff:%f, Zoom: %f \n",MandXOffset,MandYOffset,zoom);

    // calculate new mandle position and zoom    
    if (back){
        MandYOffset=BackMandYOffset;
        MandXOffset=BackMandXOffset;
        zoom=Backzoom;
        printf("Back - ");
    }else{
        printf("Touch - ");
        BackMandYOffset=MandYOffset;
        BackMandXOffset=MandXOffset;
        Backzoom=zoom;

        GFX_drawRect(x-MaxMandX/2/zoomspeed,y-MaxMandY/2/zoomspeed,MaxMandX/zoomspeed,MaxMandY/zoomspeed,45);

        if(debug>1)printf("BLH %f,%f \n",x-MaxMandX/2*zoom,y-MaxMandY/2*zoom);

        double xoffset=(x-MaxMandX/2)/MaxMandX;
        double yoffset=(y-MaxMandY/2)/MaxMandY;
                
        //if again set only alter zoom
        if(again==1){
             xoffset=0;yoffset=0;
             printf("Again - ");
        }

        zoom=zoom/zoomspeed;


        if(debug>1)printf("touch Offsets %f,%f \n",xoffset,yoffset);

        MandXOffset= MandXOffset + zoomlast/2;
        MandYOffset= MandYOffset + zoomlast/2;         
        MandXOffset= MandXOffset + xoffset*zoom;
        MandYOffset= MandYOffset + yoffset*zoom;
        MandXOffset= MandXOffset - zoom/2;
        MandYOffset= MandYOffset - zoom/2;             

        //set next zoom level
    }
    if(debug>1)printf("After: MandXoff:%f, MandYoff:%f, Zoom: %f \n     Window (%f,%f)\n",MandXOffset,MandYOffset,zoom,windowwidth,windowheight);
     
    return 1;
}

void do_touch_loop(){
    //init touch controller
    XPT_2046_Init();
    int go=1;
    //starting position
    MandXOffset=-2.1;
    MandYOffset=-1.5;
    //startng zoom
    zoom=3;

    while(go==1){
        do_controller(0);
        go=wait_for_touch();        
    }
}
