#include <i2c_fifo.h>
#include <i2c_slave.h>
#include <pico/stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

//define the size of the mand
#define MaxMandX 240
#define MaxMandY 320

int MandYOffset=0;
int MandXOffset=0;

char debug=0;
int questioncnt=0;
int lastproc=0;

uint16_t answerref[MAXProc][LUMPSIZE];

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
        printf("Processor allocation failed\n");
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

void set_poll_status(uint8_t proc,uint8_t status){
    int c;
    uint8_t rbuf[3];
    rbuf[0]=poll;
    rbuf[1]=status;
    //set poll flag 
    c= i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, 2, true);
    if (c!=2){printf("Failed to send status\n");}
}

int get_next_question(uint8_t proc){
    int r=0;
    int lump;
    if (mandx+LUMPSIZE>=MaxMandX){
        mandx=0;
        if (mandy++>=MaxMandY){
            r=1;
        }
    }
    if (r==0){      
        if (debug>0)printf("MandX:%i,MandY:%i for proc: %i\n",mandx,mandy,proc);
        for (lump=0;lump<LUMPSIZE;lump++){
            mandx++;
            if(debug>3) printf("lump:%i ",lump);
            dchar.dx[lump]=(double)((double)(mandx+lump)-MandXOffset/2)/MaxMandX*3;
            dchar.dy[lump]=(double)(mandy-MandYOffset/2)/MaxMandY*3;
            if(debug>3) printf("QC %i",questioncnt);
            if (debug>3) printf("%i %i dx:%f,%f \n",lump,questioncnt,dchar.dx[lump],dchar.dy[lump]); 
            answerref[proc][lump]=questioncnt;
            if(MaxMandX * MaxMandY<questioncnt)printf("\nITs all gone horribly wrong!\n\n");
            questioncnt++;
       }
   }
   if (debug>0)printf("return from get\n");
   return r;
}

int send_questions_to_proc(uint8_t proc){
    uint8_t rbuf[QUESTIONSIZE+1];
    rbuf[0]=quest; //mem address
    int c;
    for(int a=0;a<QUESTIONSIZE;a++){
        rbuf[a+1]=dchar.arr[a];
    }
    if (debug>2) printf("Send to Proc:%i \n",proc);
    if (debug>3) {
        Show_data(0,QUESTIONSIZE,dchar.arr);
        printf("\n");
    }
    c=i2c_write_blocking(I2C_MOD, proc+I2C_PROC_LOWEST_ADDR, rbuf, QUESTIONSIZE+1, true);
    if(c!=QUESTIONSIZE+1)printf("Failed to send question to proc %i\n",proc);
    return c==QUESTIONSIZE+1;
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
         if (debug>3){
             Show_data(0,ANSWERSIZE,ichar.arr);
             printf("\n");
         }    
         for(int lump=0;lump<LUMPSIZE;lump++){
             //get pointer into results from the proc and lump via answerref
             if (debug>2) printf(" Results %i %i %i \n",proc,lump,answerref[proc][lump]);
             results[ answerref[proc][lump] ]=ichar.i[lump];
             if (debug>2) printf("%i ",ichar.i[lump]);                  
         }
         r=1;
         if (debug>1) puts("= Answer *****\n");  
     }else{
         printf("Failed to get answers from proc %i\n",proc);
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
                printf("\n############### Unallocated Proc/ADDR Change to %02X[%02X] ################\n\n",np,sa[1]);
                AllocateProc(np);
            }else{
                printf("\n############### Unallocated Proc/ADDR Change to %02X[%02X] FAILED  ################\n\n",np,sa[1]);
            }
            sleep_ms(300); //and wait for reply.
        }else{
          printf("Out of spare Procs\n");
        }
   }//if proc on addres     


}

void do_master(char dbg){
    mandx=0,mandy=0;
    MandXOffset=MaxMandX/2;
    MandYOffset=MaxMandY/2;

    questioncnt=0;
    debug=dbg;
    puts("\nI2C Controller Start\n");

//    setup_master();

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
        proc=getnextfreeproc(); //get addess of free proc
        if (proc>=0 && go==2){ 
            if (debug>2) printf("Proc %i free add process\n",proc);
            if (get_next_question(proc)==0){
                if (debug>2) printf("SendQuestion process\n");
                //send 2*lump floats to var
                send_questions_to_proc(proc);            
                set_poll_status( proc,poll_go);
            }else{
              printf("\n\nFinished");
              //set finished - wait for all done
              go=1;//no further questions wait till all answers.
            }
        }else{
          if (debug>2) puts("No free procs");
        }

        //check for procs that have done.
        if (debug>2)puts("Check procs done" );
        proc=getnextdoneproc();
        if (proc>=0){
           if (debug>2)printf("read answer from proc %i\n",proc);
           if (do_answers(proc)){             
               set_poll_status( proc,poll_ready);
           }else{
               printf("Read answer failed\n");  
           }    
        }else{
            if (debug>2)printf("No done procs\n");
            putchar('.');
            //if finished set
            if (go==1){
                go=0; //have all answers stop
                puts("\nComplete");
            }
        }  
        tight_loop_contents();
     }//while
}

