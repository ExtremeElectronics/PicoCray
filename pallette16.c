
#define MappingMax 16

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

