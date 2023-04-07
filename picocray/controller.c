#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//display includes
#include "ili9341.h"
#include "gfx.h"

//define the size of the mand
#define MaxMandX 240
#define MaxMandY 320
//#define MaxMandX 40
//#define MaxMandY 20

int MandYOffset=0;
int MandXOffset=0;

char debug=0;
int questioncnt=0;
int lastproc=0;

uint16_t answerref[MAXProc][LUMPSIZE];
uint16_t coord[MAXProc][2];

bool addralloc[MAXProc];

uint8_t code=0xaa;

//uint16_t results[MaxMandX * MaxMandY];

//uint16_t *buff = NULL;
uint16_t buff[LUMPSIZE];

uint16_t mapping[16]={
    GFX_RGB565(0   ,0   ,0),          //black
    GFX_RGB565(0xAA,0   ,0),       //red
    GFX_RGB565(0   ,0xAA,0),       //green
    GFX_RGB565(0xAA,0x55,0),    //brown
    GFX_RGB565(0   ,0   ,0xAA),       //blue
    GFX_RGB565(0xAA,0   ,0xAA),    //Magenta
    GFX_RGB565(0   ,0xAA,0xAA),    //Cyan
    GFX_RGB565(0xAA,0xAA,0xAA), //light Grey
    GFX_RGB565(0x55,0x55,0x55), //grey
    GFX_RGB565(0xff,0x55,0x55), //bright red
    GFX_RGB565(0x55,0xff,0x55), //bright green
    GFX_RGB565(0xff,0xff,0x55), //yellow
    GFX_RGB565(0   ,0   ,0xff),       //bright blue
    GFX_RGB565(0xff,0x55,0xff), //bright magenta
    GFX_RGB565(0x55,0xff,0xff), //bright cyan
    GFX_RGB565(0xff,0xff,0xff),  //white
};




int mandx=0,mandy=0;

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
/*    mapping[0]=GFX_RGB565(66, 30, 15);
    mapping[1]=GFX_RGB565(25, 7, 26);
    mapping[2]=GFX_RGB565(9, 1, 47);
    mapping[3]=GFX_RGB565(4, 4, 73);
    mapping[4]=GFX_RGB565(0, 7, 100);
    mapping[5]=GFX_RGB565(12, 44, 138);
    mapping[6]=GFX_RGB565(24, 82, 177);
    mapping[7]=GFX_RGB565(57, 125, 209);
    mapping[8]=GFX_RGB565(134, 181, 229);
    mapping[9]=GFX_RGB565(211, 236, 248);
    mapping[10]=GFX_RGB565(241, 233, 191);
    mapping[11]=GFX_RGB565(248, 201, 95);
    mapping[12]=GFX_RGB565(255, 170, 0);
    mapping[13]=GFX_RGB565(204, 128, 0);
    mapping[14]=GFX_RGB565(153, 87, 0);
    mapping[15]=GFX_RGB565(106, 52, 3);
*/
    LCD_initDisplay();
    LCD_setRotation(0);
    GFX_clearScreen();
    GFX_setCursor(0, 0);
}

uint16_t do_map(int c){
   return mapping[c %16];
}

//void write_lump_to_display(uint16_t x,uint16_t y,uint8_t * data ){
void write_lump_to_display(uint16_t x,uint16_t y,uint8_t * data ){
    uint8_t lump;
    for (lump=0;lump<LUMPSIZE;lump++){
        buff[lump]=mapping[data[lump] %16];
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

static int GetProcStatus(uint8_t addr){
    if (debug>2)printf("Get Proc Status from Addr %02X[%02X]\n",addr,addr-I2C_PROC_LOWEST_ADDR);
    int r=-99,count;
    uint8_t buf[3];
    buf[0]=poll;
    buf[1]=0;
    buf[2]=0;
    count = i2c_write_blocking(I2C_MOD, addr, buf, 1, true);
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
      if(GetProcStatus(sa)>0){
         addralloc[a]=1;
         printf("Proc exists at %i\n",sa);
      } 
    }
}

int getnextprocwithstatus(uint8_t stat){
    if (debug>2)printf("Get Next status %i Proc\n",stat);
    int sa;
    int r=-1;
    int tries=0;
    while(1){
        lastproc++;
        if (lastproc>MAXProc)lastproc=0;
        if (addralloc[lastproc]==true){
            sa=lastproc+I2C_PROC_LOWEST_ADDR;
            if (GetProcStatus(sa)==stat){
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
    if (debug>0)printf("MandX:%i,MandY:%i for proc: %i\n",mandx,mandy,proc);
    coord[proc][0]=mandx;
    coord[proc][1]=mandy;
    dchar.dy=(double)(mandy-MandYOffset)/MaxMandY*3; //only send y once ASSUMES Y WONT CHANGE DURING LUMP
    while (lump<LUMPSIZE && r==0){
        dchar.dx[lump]=(double)(mandx-MandXOffset)/MaxMandX*3;  //was 5
        //if (debug>3) q=mandelbrot(dchar.dx[lump],dchar.dy[lump]);
        answerref[proc][lump]=questioncnt;
        r=next_mand();
        lump++;
        }//while
   return r;
}

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
     c=i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, 1, true);
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
            sleep_ms(300); //and wait for reply.
        }else{
          printf("\n############# Out of spare Procs\n");
        }
   }//if proc on addres     


}

void do_controller(char dbg){
    mandx=0,mandy=0;
    MandXOffset=MaxMandX/1.5;
    MandYOffset=MaxMandY/2;

    questioncnt=0;
    debug=dbg;
    puts("\nI2C Controller Start\n");

    int proc;

    puts("Check existing Procs");    
    check_proc_exists();
    puts("Master Loop\n");
    
    int go=2;//has procs to send.
    
    while(go>0){
        //check for un allocated Processors        
        if (gpio_get(I2C_Assert)==1){
            check_for_unallocated_processors();
        }//if assert

        //check free processors 
        if (debug>2)puts("Check Free Procs");
//        proc=getnextfreeproc(); //get addess of free proc
        proc=getnextprocwithstatus(poll_ready);
        if (proc>=0 && go==2){ 
            if (debug>2) printf("Proc %i free add process\n",proc);
            if (get_next_question(proc)==0){
//                set_poll_status(proc, poll_wait);
                if (debug>2) printf("SendQuestion process\n");
                //send 2*lump floats to var + poll=poll_go
                if (send_questions_to_proc(proc)==0){
                    printf("\n###Send failed %i\n",proc);                 
                }            
                //after sending questions set proc to go. not needed as tagged on to questions now.
//                set_poll_status( proc,poll_go);
            }else{
              printf("\n\n*********** Finished ***********");
              //set finished - wait for all done
              go=1;//no further questions wait till all answers.
            }
        }else{
            putchar('~');
            if (debug>2) puts("No free procs");
        }

        //check for procs that have done.
        if (debug>2)puts("Check procs done" );
//        proc=getnextdoneproc();
        proc=getnextprocwithstatus(poll_done);
        if (proc>=0){
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
            //if finished set
            if (go==1){
                go=0; //have all answers stop
                puts("\n************ Complete **************");
            }
        }  
        tight_loop_contents();
     }//while
}

