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

void do_I2C_scan(){
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

void do_I2C_dump(int addr){
    printf("\nI2C Master Dump %02X selected\n",addr);
    uint8_t rbuf[257];
    if (proc_dump(addr,0,256,rbuf)>=0){
        puts("\n");
        Show_data(0,256,rbuf);
     }
     sleep_ms(500);
}

void do_proc_statuses(){
   int p;
   int s;
   for (p=0;p<MAXProc;p++){
      s=GetProcStatus(p+I2C_PROC_LOWEST_ADDR);
      if(s>=0){
          printf("Proc:%i Status: ",p);
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

/*
void do_results(){
    printf("\n");
    for(int y=0;y<MaxMandY;y++){
        for(int x=0;x<MaxMandX;x++){
            printf("%02X ",results[x+y*MaxMandX]);
        }
        printf("\n");
    }
}


void do_res_disp(){
    printf("\nRes Disp\n");

    init_disp();

    int c=0;
    for(int y=0;y<MaxMandY;y++){
        for(int x=0;x<MaxMandX;x++){
            c=results[x+y*MaxMandX] % 16;
            GFX_drawPixel(x,y,mapping[c]);
        }
        printf("\n");
    }
}
*/

void test_display(){
    uint16_t x,y,lump;
//    LCD_initDisplay();
//    LCD_setRotation(1);
//    GFX_createFramebuf();
//    while (true)
//    {
//        GFX_clearScreen();
//        GFX_setCursor(0, 0);
//        GFX_printf("Hello GFX!\n%d", c++);
//        GFX_flush();
//        sleep_ms(500);
//    }
//    buff = malloc(100 * 1 * sizeof(uint16_t));
    for (x=0;x<100;x+=10){
      for (y=0;y<100;y++){
         for (lump=0;lump<LUMPSIZE;lump++){
//         printf("%i %i\n",x,y);
        buff[lump]=mapping[7];
//        GFX_drawPixel(x+lump,y,mapping[data[lump] %16]);
        //LCD_WritePixel(x+lump,y,mapping[data[lump] %16]);
         //LCD_WritePixel(x+lump,y,mapping[7]);
         }
         
         LCD_WriteBitmap(x, y, LUMPSIZE,1, buff);
//         sleep_ms(10);
      }
    }
    sleep_ms(500);    
//    free(buff);

    
    
}

void resetprocstatus(){
   uint8_t p;
   for (p=0;p<MAXProc;p++){
          set_poll_status(p,poll_ready);
          printf("\nPoll status reset %i ",p);
   }
   printf("\n");
}

