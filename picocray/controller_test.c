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
   for (p=0;p<MAXProc;p++){
      s=GetProcStatus(p+I2C_PROC_LOWEST_ADDR);
      if(s>0){
          printf("Proc:%i Status:\n",p);

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
