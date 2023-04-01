#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
//#include <hardware/adc.h>
#include <stdlib.h>
//#include "common.h"

#define MaxMandX 240
#define MaxMandY 320

char debug=0;
int questioncnt=0;
uint16_t answerref[MAXProc][LUMPSIZE];
int lastproc=0;

bool addralloc[MAXProc];

uint16_t results[MaxMandX * MaxMandY];

int mandx=0,mandy=0;

static void setup_master(){
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

int GetNextProc(){
    int a;
    for (a=0;a<MAXProc && addralloc[a]==true;a++){}
    if(a>MAXProc){a=-1;}
    if (debug>2)printf("Next Proc %02X\n",a);
    return a;
}

int AllocateProc(int a){
    int r=0;
    if(a>=0 && a<MAXProc){
        addralloc[a]=true;
        r=1;
    }else{
        printf("Slave allocation failed\n");
    }
    return r;
}

bool IsProcOnAddress(int addr){
    uint8_t buf[10];
    buf[0]=0;
    int ret = i2c_read_blocking(I2C_MOD, addr, buf, 1, true);
    if (debug>2)printf("Is Slave on %02X = %i\n",addr,ret);  
    return ret==1;
}

static int GetProcStatus(char addr){
    if (debug>2)printf("Get Proc Status from Addr %02X\n",addr);
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
       if (debug>1)printf("Can't read from  %02X\n",addr);
    }     
    if (debug>2)enumerate_status(r);
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

int getnextfreeproc(){
    if (debug>2)printf("Get Next Free Proc\n");
    int sa;
    int r=-1;
    int tries=0;
    while(1){
        lastproc++;
        if (lastproc>MAXProc)lastproc=0;
        if (addralloc[lastproc]==true){
            sa=lastproc+I2C_PROC_LOWEST_ADDR;
            if (GetProcStatus(sa)==poll_ready){
                r=lastproc;
                break;
            }
        }
        tries++;
        if(tries>MAXProc){break;};                
    }
    if (debug>2)printf("Procfree=%i\n",r);
    return r;
}

int getnextdoneproc(){
    int a,sa;
    int r=-1;
    for (a=0;a<MAXProc;a++){
        if (addralloc[a]==true){
            sa=a+I2C_PROC_LOWEST_ADDR;
            if (GetProcStatus(sa)==poll_done){break;}
        }
    }
    if(a<MAXProc){r=a;}
    if (debug>2) printf("Done Procs %i\n",a);
    return r;
}

void set_poll_status(char proc,char status){
    int c;
    uint8_t rbuf[3];
    rbuf[0]=poll;
    rbuf[1]=status;
    //set poll flag 
    c= i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, 2, true);
    if (c!=2){printf("Failed to send status\n");}
}

// ########## Master Test routines ###############

int proc_dump(char addr,char MemAddress,int msg_len,uint8_t * buf){
    int r=0;
    if (debug>2)printf("Dump contents of proc address %02X\n",addr);
    buf[0]=MemAddress;
    int count = i2c_write_blocking(I2C_MOD, addr, buf, 1, true);
    if(count==0){
         printf("Cant Read from proc address %02X, Mem %i\n",addr,MemAddress);
         r=-1;
    }else{
         count = i2c_read_blocking(I2C_MOD, addr, buf, msg_len , true);
         buf[count] = '\0';
         if (debug>2)printf("Read %02X  at Mem %02X for '%i'\n",addr, MemAddress, count);
         if (count!=msg_len){
             printf("Read failed from %02X \n",addr);
             r=-1;
        }
    }
    return r;
}

void do_master_scan(){
    puts("\nI2C Master Scan selected\n");

    setup_master();
    uint8_t rbuf[2];
    int ret;
    puts("");
    for(int a=0;a<MAXProc;a++){
        ret = i2c_read_blocking(I2C_MOD, a+I2C_PROC_LOWEST_ADDR, rbuf, 1, true);
        if(ret==1){
            printf("Address: %02X - Live\n",a+I2C_PROC_LOWEST_ADDR);
        }else{
            printf("Address: %02X - Free\n",a+I2C_PROC_LOWEST_ADDR);
        }
        sleep_ms(1);
    }
    ret = i2c_read_blocking(I2C_MOD, I2C_DEFAULT_ADDRESS, rbuf, 1, true);
    if(ret==1){
        printf("\nAddress: %02X - Live\n",I2C_DEFAULT_ADDRESS);
    }else{
        printf("\nAddress: %02X - Free\n",I2C_DEFAULT_ADDRESS);
    }
}

void do_master_dump(int addr){
 
    printf("\nI2C Master Dump %02X selected\n",addr);

    setup_master();
    uint8_t rbuf[257];
    if (proc_dump(addr,0,256,rbuf)>=0){
        puts("\n");
        Show_data(0,128,rbuf);
     }
     sleep_ms(500);
}

void do_master_statuses(){
   int p;
   int s;
   setup_master();
   for (p=0;p<MAXProc;p++){
      s=GetProcStatus(p+I2C_PROC_LOWEST_ADDR);
      if(s>0){
          printf("Proc:%i Status:",p);
          enumerate_status(s);
      }
   }   
}

void do_assert_test(){
   if (gpio_get(I2C_Assert)==1){
       printf("\nAssert High\n");
   }else{
       printf("\nAssert Low\n");
   }  
}

void do_results(){
    printf("\n");
    for(int y=0;y<MaxMandY;y++){
        for(int x=0;x<MaxMandX;x++){
            printf("%02X ",results[x+y*MaxMandX]);
        }
        printf("\n");
    }
}

//end of master test routines 

int get_next_question(int proc){
   int r=0;
   int lump;
   mandx+=LUMPSIZE; 
   if (mandx>MaxMandX){
       mandx=0;
       if (mandy++>MaxMandY){
           r=1;
       }
   }
   if (r==0){      
       if (debug>0)printf("MandX:%i,MandY:%i\n",mandx,mandy);
       for (lump=0;lump<LUMPSIZE;lump++){
           dchar.dx[lump]=(double)((mandx+lump)-MaxMandX/2)/MaxMandX*3;
           dchar.dy[lump]=(double)(mandy-MaxMandY/2)/MaxMandY*3;
           answerref[proc][lump]=questioncnt;
           questioncnt++;
       }
   }
   if (debug>0)printf("return from get\n");
   return r;
}

int send_questions_to_proc(char proc){
    uint8_t rbuf[QUESTIONSIZE+1];
    rbuf[0]=quest; //mem address
    int c;
    for(int a=0;a<QUESTIONSIZE;a++){
        rbuf[a+1]=dchar.arr[a];
    }
    if (debug>2) printf("Send to Proc:%i \n",proc);
    c=i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, QUESTIONSIZE+1, true);
    if(c!=QUESTIONSIZE+1)printf("Failed to send question to proc %i\n",proc);
    return c==QUESTIONSIZE+1;
}

void do_master(char dbg){
    mandx=0,mandy=0;
    questioncnt=0;
    debug=dbg;
    puts("\nI2C Master selected\n");

    int cnt=0;
    setup_master();
    uint8_t rbuf[257];
    uint8_t  sa[3];
    
    int proc;

    puts("Check existing Procs");    
    check_proc_exists();
    puts("Master Loop\n");
    
    int go=2;
    
    while(go>0){
        cnt++;
        //check for un allocated Processors        
        if (gpio_get(I2C_Assert)==1){
            if (debug>2)puts("Check New Procs");
            if(IsProcOnAddress(I2C_DEFAULT_ADDRESS)){
                puts("Unallocated proc found");
                int np=GetNextProc();
                if (np>=0){
                    sa[0]=cmd_sl_addr;
                    sa[1]=np+I2C_PROC_LOWEST_ADDR ;
                    sa[2]=0;
                    int c=i2c_write_blocking(I2C_MOD, I2C_DEFAULT_ADDRESS, sa, 2, true);
                    if(c==2){
                        printf("\n############### Unallocated Proc/ADDR Change to %02X[%02X] ################\n\n",np,sa[1]);
                        AllocateProc(np);
                    }else{
                        printf("\n############### Unallocated Proc/ADDR Change to %02X[%02X] FAILED  ################\n\n",np,sa[1]);
                    }
                    sleep_ms(50); //and wait for reply.
                }else{
                  printf("Out of spare Procs\n");
                }
           }//if proc on addres     
        }//if assert

        if (debug>2)puts("Check Free Procs");
        proc=getnextfreeproc(); //get addess of free proc
        if (proc>=0){ 
            if (debug>2) printf("Proc %i free add process\n",proc);
            //sleep_ms(100);
            if (get_next_question(proc)==0){
                //send 2*lump floats to var
                if (debug>2) printf("SendQuestion process\n");
                send_questions_to_proc(proc);            
                set_poll_status( proc,poll_go);
            }else{
              printf("\n\nFinished");
              //set finished - wait for all done
              go=1;
            }
        }else{
          if (debug>2) puts("No free procs");
        }

        if (debug>2)puts("Check waiting answers" );
        proc=getnextdoneproc();
        if (proc>=0){
           if (debug>2)printf("read answer from proc %i\n",proc);
           rbuf[0]=ans;
           int c=i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, 1, true);
           c= i2c_read_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, ANSWERSIZE, true);
//           Show_data(0,4,rbuf);
           if (c==ANSWERSIZE){
              //puts("");
              for(int a=0;a<ANSWERSIZE;a++){
                  ichar.arr[a]=rbuf[a];
              }
              for(int lump=0;lump<LUMPSIZE;lump++){
                  //get pointer into results from the proc and lump via answerref
                  results[ answerref[proc][lump] ]=ichar.i[lump];
                  if (debug>2) printf("%i ",ichar.i[lump]);                  
              }
              if (debug>1) puts("= Answer *****\n");  
              
              set_poll_status( proc,poll_ready);
           }else{
               printf("Read answer failed\n");  
           }    
        }else{
            if (debug>2)printf("No done procs\n");
            putchar('.');
            //if finished set
            if (go==1){
                go=0;
                puts("\nComplete");
            }
        }  
        tight_loop_contents();
     }//while
}

