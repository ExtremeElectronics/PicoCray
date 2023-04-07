#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/dma.h"

#include "ili9341.h"

uint16_t _width;  ///< Display width as modified by current rotation
uint16_t _height; ///< Display height as modified by current rotation

int16_t _xstart = 0; ///< Internal framebuffer X offset
int16_t _ystart = 0; ///< Internal framebuffer Y offset

uint8_t rotation;


spi_inst_t *ili9341_spi = spi0;
uint16_t ili9341_pinCS = 5;
uint16_t ili9341_pinDC = 7;
int16_t ili9341_pinRST = 6;

uint16_t ili9341_pinSCK = 2;
uint16_t ili9341_pinTX = 3;

/*
spi_inst_t *ili9341_spi = spi1;
uint16_t ili9341_pinCS = 13;
uint16_t ili9341_pinDC = 14;
int16_t ili9341_pinRST = 15;

uint16_t ili9341_pinSCK = 10;
uint16_t ili9341_pinTX = 11;
*/

const uint8_t initcmd[] = {
	24, //24 commands
	0xEF, 3, 0x03, 0x80, 0x02,
	0xCF, 3, 0x00, 0xC1, 0x30,
	0xED, 4, 0x64, 0x03, 0x12, 0x81,
	0xE8, 3, 0x85, 0x00, 0x78,
	0xCB, 5, 0x39, 0x2C, 0x00, 0x34, 0x02,
	0xF7, 1, 0x20,
	0xEA, 2, 0x00, 0x00,
	ILI9341_PWCTR1, 1, 0x23,	   // Power control VRH[5:0]
	ILI9341_PWCTR2, 1, 0x10,	   // Power control SAP[2:0];BT[3:0]
	ILI9341_VMCTR1, 2, 0x3e, 0x28, // VCM control
	ILI9341_VMCTR2, 1, 0x86,	   // VCM control2
	ILI9341_MADCTL, 1, 0x48,	   // Memory Access Control
	ILI9341_VSCRSADD, 1, 0x00,	   // Vertical scroll zero
	ILI9341_PIXFMT, 1, 0x55,
	ILI9341_FRMCTR1, 2, 0x00, 0x18,
	ILI9341_DFUNCTR, 3, 0x08, 0x82, 0x27,					 // Display Function Control
	0xF2, 1, 0x00,											 // 3Gamma Function Disable
	ILI9341_GAMMASET, 1, 0x01,								 // Gamma curve selected
	ILI9341_GMCTRP1, 15, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08, // Set Gamma
	0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00,
	ILI9341_GMCTRN1, 15, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07, // Set Gamma
	0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F,
	ILI9341_SLPOUT, 0x80, // Exit Sleep
	ILI9341_DISPON, 0x80, // Display on
	0x00				  // End of list
};

#ifdef USE_DMA
uint dma_tx;
dma_channel_config dma_cfg;
void waitForDMA()
{

	dma_channel_wait_for_finish_blocking(dma_tx);
}
#endif

void LCD_setPins(uint16_t dc, uint16_t cs, int16_t rst, uint16_t sck, uint16_t tx)
{
	ili9341_pinDC = dc;
	ili9341_pinCS = cs;
	ili9341_pinRST = rst;
	ili9341_pinSCK = sck;
	ili9341_pinTX = tx;
}

void LCD_setSPIperiph(spi_inst_t *s)
{
	ili9341_spi = s;
}

void initSPI()
{
	spi_init(ili9341_spi, 500 * 40000); 
//	spi_init(ili9341_spi, 1000 * 40000); 
	
	spi_set_format(ili9341_spi, 16, SPI_CPOL_1, SPI_CPOL_1, SPI_MSB_FIRST);
	gpio_set_function(ili9341_pinSCK, GPIO_FUNC_SPI);
	gpio_set_function(ili9341_pinTX, GPIO_FUNC_SPI);

	gpio_init(ili9341_pinCS);
	gpio_set_dir(ili9341_pinCS, GPIO_OUT);
	gpio_put(ili9341_pinCS, 1);

	gpio_init(ili9341_pinDC);
	gpio_set_dir(ili9341_pinDC, GPIO_OUT);
	gpio_put(ili9341_pinDC, 1);

	if (ili9341_pinRST != -1)
	{
		gpio_init(ili9341_pinRST);
		gpio_set_dir(ili9341_pinRST, GPIO_OUT);
		gpio_put(ili9341_pinRST, 1);
	}

#ifdef USE_DMA
	dma_tx = dma_claim_unused_channel(true);
	dma_cfg = dma_channel_get_default_config(dma_tx);
	channel_config_set_transfer_data_size(&dma_cfg, DMA_SIZE_16);
	channel_config_set_dreq(&dma_cfg, spi_get_dreq(ili9341_spi, true));
#endif
}

void ILI9341_Reset()
{
	if (ili9341_pinRST != -1)
	{
		gpio_put(ili9341_pinRST, 0);
		sleep_ms(5);
		gpio_put(ili9341_pinRST, 1);
		sleep_ms(150);
	}
}

void ILI9341_Select()
{
	gpio_put(ili9341_pinCS, 0);
}

void ILI9341_DeSelect()
{
	gpio_put(ili9341_pinCS, 1);
}

void ILI9341_RegCommand()
{
	gpio_put(ili9341_pinDC, 0);
}

void ILI9341_RegData()
{
	gpio_put(ili9341_pinDC, 1);
}

void ILI9341_WriteCommand(uint8_t cmd)
{
	ILI9341_RegCommand();
	spi_set_format(ili9341_spi, 8, SPI_CPOL_1, SPI_CPOL_1, SPI_MSB_FIRST);
	spi_write_blocking(ili9341_spi, &cmd, sizeof(cmd));
}

void ILI9341_WriteData(uint8_t *buff, size_t buff_size)
{
	ILI9341_RegData();
	spi_set_format(ili9341_spi, 8, SPI_CPOL_1, SPI_CPOL_1, SPI_MSB_FIRST);
	spi_write_blocking(ili9341_spi, buff, buff_size);
}

void ILI9341_SendCommand(uint8_t commandByte, uint8_t *dataBytes,
						 uint8_t numDataBytes)
{
	ILI9341_Select();

	ILI9341_WriteCommand(commandByte);
	ILI9341_WriteData(dataBytes, numDataBytes);

	ILI9341_DeSelect();
}

void LCD_initDisplay()
{
	initSPI();
	ILI9341_Select();

	if (ili9341_pinRST < 0)
	{										   // If no hardware reset pin...
		ILI9341_WriteCommand(ILI9341_SWRESET); // Engage software reset
		sleep_ms(150);
	}
	else
		ILI9341_Reset();

	uint8_t *addr = initcmd;
	uint8_t numCommands, cmd, numArgs;
	uint16_t ms;
	numCommands = *(addr++); // Number of commands to follow
	while (numCommands--)
	{					 // For each command...
		cmd = *(addr++); // Read command
		// numArgs = *(addr++);		 // Number of args to follow
		uint8_t x = *(addr++);
		numArgs = x & 0x7F; // Mask out delay bit
		ILI9341_SendCommand(cmd, addr, numArgs);
		addr += numArgs;

		if (x & 0x80)
			sleep_ms(150);
	}

	_width = ILI9341_TFTWIDTH;
	_height = ILI9341_TFTHEIGHT;
}

void LCD_setRotation(uint8_t m)
{
	rotation = m % 4; // can't be higher than 3
	switch (rotation)
	{
	case 0:
		m = (MADCTL_MX | MADCTL_BGR);
		_width = ILI9341_TFTWIDTH;
		_height = ILI9341_TFTHEIGHT;
		break;
	case 1:
		m = (MADCTL_MV | MADCTL_BGR);
		_width = ILI9341_TFTHEIGHT;
		_height = ILI9341_TFTWIDTH;
		break;
	case 2:
		m = (MADCTL_MY | MADCTL_BGR);
		_width = ILI9341_TFTWIDTH;
		_height = ILI9341_TFTHEIGHT;
		break;
	case 3:
		m = (MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR);
		_width = ILI9341_TFTHEIGHT;
		_height = ILI9341_TFTWIDTH;
		break;
	}

	ILI9341_SendCommand(ILI9341_MADCTL, &m, 1);
}

void LCD_setAddrWindow(uint16_t x, uint16_t y, uint16_t w, uint16_t h)
{
	uint32_t xa = ((uint32_t)x << 16) | (x + w - 1);
	uint32_t ya = ((uint32_t)y << 16) | (y + h - 1);

	xa = __builtin_bswap32(xa);
	ya = __builtin_bswap32(ya);

	ILI9341_WriteCommand(ILI9341_CASET);
	ILI9341_WriteData(&xa, sizeof(xa));

	// row address set
	ILI9341_WriteCommand(ILI9341_PASET);
	ILI9341_WriteData(&ya, sizeof(ya));

	// write to RAM
	ILI9341_WriteCommand(ILI9341_RAMWR);
}

void LCD_WriteBitmap(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t *bitmap)
{
	ILI9341_Select();
	LCD_setAddrWindow(x, y, w, h); // Clipped area
	ILI9341_RegData();
	spi_set_format(ili9341_spi, 16, SPI_CPOL_1, SPI_CPOL_1, SPI_MSB_FIRST);
#ifdef USE_DMA
	dma_channel_configure(dma_tx, &dma_cfg,
						  &spi_get_hw(ili9341_spi)->dr, // write address
						  bitmap,						// read address
						  w * h,						// element count (each element is of size transfer_data_size)
						  true);						// start asap
	waitForDMA();
#else

	spi_write16_blocking(ili9341_spi, bitmap, w * h);
#endif

	ILI9341_DeSelect();
}

void LCD_WritePixel(int x, int y, uint16_t col)
{
	ILI9341_Select();
	LCD_setAddrWindow(x, y, 1, 1); // Clipped area
	ILI9341_RegData();
	spi_set_format(ili9341_spi, 16, SPI_CPOL_1, SPI_CPOL_1, SPI_MSB_FIRST);
	spi_write16_blocking(ili9341_spi, &col, 1);
	ILI9341_DeSelect();
}

//my scrolly stuff

void SendVerticalScrollDefinition(uint16_t wTFA, uint16_t wBFA) {
  // Did not pass in VSA as TFA+VSA=BFA must equal 320
  
//ILI9341_SendCommand(uint8_t commandByte, uint8_t *dataBytes, uint8_t numDataBytes)
  
  uint8_t scrcmd[6];

  scrcmd[0]=wTFA;
  scrcmd[2]=(uint16_t) ( 240-(wTFA+wBFA) ); //rotated values
//  scrcmd[2]=(uint16_t) ( 320-(wTFA+wBFA) ); //nonrotated values
  scrcmd[4]=wBFA;
  ILI9341_SendCommand(ILI9341_VSCRDEF,scrcmd,6);
  //tft.writecommand(0x33); // Vertical Scroll definition.
//  WriteData16(wTFA);   // 
//  WriteData16(320-(wTFA+wBFA));
//  WriteData16(wBFA);
}

void SendVerticalScrollStartAddress(uint16_t wVSP) {
//  tft.writecommand(0x37); // Vertical Scroll definition.
//  WriteData16(wVSP);   // 
  
  ILI9341_SendCommand(ILI9341_VSCRSADD,wVSP,2);
  
}

void LCD_Scroll(uint16_t iScrollStart,uint16_t lines)
{
    iScrollStart += lines;
    if (iScrollStart == 320) iScrollStart = lines*2;
    SendVerticalScrollStartAddress(iScrollStart);
}
