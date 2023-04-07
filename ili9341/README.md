ILI9341 driver, based on [Adafruit ILI9341 Arduino Library](https://github.com/adafruit/Adafruit_ILI9341)

## Usage
Add *ili9341* into CMakeLists.txt, in `target_link_libraries`

### Pin mapping
By default, RST is GPIO16, CS is GPIO17, DC is GPIO20, SCK is GPIO18, TX is GPIO19, using the default SPI peripheral.\
The pin mapping can be changed using `LCD_setPins`, before initializing the display. Make sure that the chosen SCK and TX pins are usable with the selected SPI peripheral. \
The usage of the reset pin isn't enabled by default. Setting it to -1 disables it. \
The SPI peripheral can be selected using `LCD_setSPIperiph`.


### Functions:

`LCD_setPins(uint16_t dc, uint16_t cs, int16_t rst, uint16_t sck, uint16_t tx);` selects the pins used by the display \
`LCD_setSPIperiph(spi_inst_t * s);` selects the SPI peripheral used by the display \
`LCD_initDisplay();` initializes the GPIO, SPI interface and display driver \
`LCD_setRotation(uint8_t m);` sets the rotation\
`LCD_WritePixel(int x, int y, uint16_t col);` writes a single pixel to the screen\
`LCD_WriteBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *bitmap);` writes a bitmap to the screen


### DMA usage
DMA  usage is disabled by default, but can be enabled by uncommenting `#define USE_DMA 1` in *ili9341.h*
