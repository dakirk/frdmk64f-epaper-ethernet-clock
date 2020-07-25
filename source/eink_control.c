/**
 * This is a library for controlling the WaveShare 2.7inch e-Paper HAT (tri-color).
 * It is based on this library: https://github.com/waveshare/e-Paper/tree/master/Arduino/epd2in7b
 */

#include "eink_control.h"

/*******************************************************************************
 * Variables
 ******************************************************************************/
uint8_t masterRxData[TRANSFER_SIZE] = {0U};
uint8_t masterTxData[TRANSFER_SIZE] = {0U};

volatile uint32_t eink_systickCounter;

//fast lookup tables from https://github.com/pskowronek/epaper-clock-and-more/blob/master/epds/epd2in7b_fast_lut.py
const unsigned char lut_vcom_dc[] =
{
	0x00    ,0x00,
	0x00    ,0x1A    ,0x1A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x0E    ,0x01    ,0x0E    ,0x01    ,0x01,
	0x00    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x04    ,0x10    ,0x00    ,0x00    ,0x01,
	0x00    ,0x03    ,0x0E    ,0x00    ,0x00    ,0x09,
	0x00    ,0x23    ,0x00    ,0x00    ,0x00    ,0x01
};

//R21H
const unsigned char lut_ww[] =
{
	0x90    ,0x1A    ,0x1A    ,0x00    ,0x00    ,0x01,
	0x40    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x84    ,0x0E    ,0x01    ,0x0E    ,0x01    ,0x01,
	0x80    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x04    ,0x10    ,0x00    ,0x00    ,0x01,
	0x00    ,0x03    ,0x0E    ,0x00    ,0x00    ,0x09,
	0x00    ,0x23    ,0x00    ,0x00    ,0x00    ,0x01
};

//R22H    r
const unsigned char lut_bw[] =
{
	0xA0    ,0x1A    ,0x1A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x84    ,0x0E    ,0x01    ,0x0E    ,0x01    ,0x01,
	0x90    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0xB0    ,0x04    ,0x10    ,0x00    ,0x00    ,0x01,
	0xB0    ,0x03    ,0x0E    ,0x00    ,0x00    ,0x09,
	0xC0    ,0x23    ,0x00    ,0x00    ,0x00    ,0x01
};

//R23H    w
const unsigned char lut_bb[] =
{
	0x90    ,0x1A    ,0x1A    ,0x00    ,0x00    ,0x01,
	0x40    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x84    ,0x0E    ,0x01    ,0x0E    ,0x01    ,0x01,
	0x80    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x04    ,0x10    ,0x00    ,0x00    ,0x01,
	0x00    ,0x03    ,0x0E    ,0x00    ,0x00    ,0x09,
	0x00    ,0x23    ,0x00    ,0x00    ,0x00    ,0x01
};

//R24H    b
const unsigned char lut_wb[] =
{
	0x90    ,0x1A    ,0x1A    ,0x00    ,0x00    ,0x01,
	0x20    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x84    ,0x0E    ,0x01    ,0x0E    ,0x01    ,0x01,
	0x10    ,0x0A    ,0x0A    ,0x00    ,0x00    ,0x01,
	0x00    ,0x04    ,0x10    ,0x00    ,0x00    ,0x01,
	0x00    ,0x03    ,0x0E    ,0x00    ,0x00    ,0x09,
	0x00    ,0x23    ,0x00    ,0x00    ,0x00    ,0x01
};

/*******************************************************************************
 * Code
 ******************************************************************************/

void SysTick_DelayTicks(uint32_t n)
{
    eink_systickCounter = n;
    while (eink_systickCounter != 0U)
    {
    	__asm("NOP");
    }
}

void spiInit() {

    PRINTF("DSPI board to board polling example.\r\n");
    PRINTF("This example use one board as master and another as slave.\r\n");
    PRINTF("Master uses polling way and slave uses interrupt way. \r\n");
    PRINTF("Please make sure you make the correct line connection. Basically, the connection is: \r\n");
    PRINTF("DSPI_master -- DSPI_slave   \r\n");
    PRINTF("   CLK      --    CLK  \r\n");
    PRINTF("   PCS      --    PCS \r\n");
    PRINTF("   SOUT     --    SIN  \r\n");
    PRINTF("   SIN      --    SOUT \r\n");
    PRINTF("   GND      --    GND \r\n");

    uint32_t srcClock_Hz;
    dspi_master_config_t masterConfig;

    /* Master config */
    masterConfig.whichCtar                                = kDSPI_Ctar0;
    masterConfig.ctarConfig.baudRate                      = TRANSFER_BAUDRATE;
    masterConfig.ctarConfig.bitsPerFrame                  = 8U;
    masterConfig.ctarConfig.cpol                          = kDSPI_ClockPolarityActiveHigh;
    masterConfig.ctarConfig.cpha                          = kDSPI_ClockPhaseFirstEdge;
    masterConfig.ctarConfig.direction                     = kDSPI_MsbFirst;
    masterConfig.ctarConfig.pcsToSckDelayInNanoSec        = 1000000000U / TRANSFER_BAUDRATE;
    masterConfig.ctarConfig.lastSckToPcsDelayInNanoSec    = 1000000000U / TRANSFER_BAUDRATE;
    masterConfig.ctarConfig.betweenTransferDelayInNanoSec = 1000000000U / TRANSFER_BAUDRATE;

    masterConfig.whichPcs           = EXAMPLE_DSPI_MASTER_PCS_FOR_INIT;
    masterConfig.pcsActiveHighOrLow = kDSPI_PcsActiveLow;

    masterConfig.enableContinuousSCK        = false;
    masterConfig.enableRxFifoOverWrite      = false;
    masterConfig.enableModifiedTimingFormat = false;
    masterConfig.samplePoint                = kDSPI_SckToSin0Clock;

    srcClock_Hz = DSPI_MASTER_CLK_FREQ;
    DSPI_MasterInit(EXAMPLE_DSPI_MASTER_BASEADDR, &masterConfig, srcClock_Hz);
}

void spiWrite8(uint8_t data) {

	dspi_transfer_t masterXfer;

    /* Start master transfer, send data to slave */
    masterXfer.txData      = &data;
    masterXfer.rxData      = NULL;
    masterXfer.dataSize    = TRANSFER_SIZE;
    masterXfer.configFlags = kDSPI_MasterCtar0 | EXAMPLE_DSPI_MASTER_PCS_FOR_TRANSFER | kDSPI_MasterPcsContinuous;
    DSPI_MasterTransferBlocking(EXAMPLE_DSPI_MASTER_BASEADDR, &masterXfer);

}

/**
 *  @brief: module reset.
 *          often used to awaken the module in deep sleep,
 *          see Epd::Sleep();
 */
void einkReset() {

	GPIO_PinWrite(BOARD_INITPINS_RST_GPIO, BOARD_INITPINS_RST_PIN, 0);
	SysTick_DelayTicks(200U);
	GPIO_PinWrite(BOARD_INITPINS_RST_GPIO, BOARD_INITPINS_RST_PIN, 1);
	SysTick_DelayTicks(200U);

}

void einkSleep() {

	einkSendCommand(DEEP_SLEEP);
	einkSendData(0xa5);

}

/**
 *  @brief: basic function for sending commands
 */
void einkSendCommand(uint8_t data) {
	GPIO_PinWrite(BOARD_INITPINS_DC_GPIO, BOARD_INITPINS_DC_PIN, 0);

	spiWrite8(data);
}

/**
 *  @brief: basic function for sending data
 */
void einkSendData(uint8_t data) {
	GPIO_PinWrite(BOARD_INITPINS_DC_GPIO, BOARD_INITPINS_DC_PIN, 1);

	spiWrite8(data);
}

/**
 *  @brief: Wait until the BOARD_INITPINS_BUSY_PIN goes HIGH
 */
void einkWaitUntilIdle() {

	int isBusy = !GPIO_PinRead(BOARD_INITPINS_BUSY_GPIO, BOARD_INITPINS_BUSY_PIN); //active low

	while(isBusy) {

		isBusy = !GPIO_PinRead(BOARD_INITPINS_BUSY_GPIO, BOARD_INITPINS_BUSY_PIN);
		//SysTick_DelayTicks(100U);

	}
}

/**
 *  @brief: set the look-up tables
 */
void einkSetLut() {
	unsigned int count;
	einkSendCommand(LUT_FOR_VCOM);                            //vcom
	for(count = 0; count < 44; count++) {
		einkSendData(lut_vcom_dc[count]);
	}

	einkSendCommand(LUT_WHITE_TO_WHITE);                      //ww --
	for(count = 0; count < 42; count++) {
		einkSendData(lut_ww[count]);
	}

	einkSendCommand(LUT_BLACK_TO_WHITE);                      //bw r
	for(count = 0; count < 42; count++) {
		einkSendData(lut_bw[count]);
	}

	einkSendCommand(LUT_WHITE_TO_BLACK);                      //wb w
	for(count = 0; count < 42; count++) {
		einkSendData(lut_bb[count]);
	}

	einkSendCommand(LUT_BLACK_TO_BLACK);                      //bb b
	for(count = 0; count < 42; count++) {
		einkSendData(lut_wb[count]);
	}
}

/**
 * @brief: clear the frame data from the SRAM, this won't refresh the display
 */
void einkClearFrame() {

	int i;

	einkSendCommand(TCON_RESOLUTION);
	einkSendData(EPD_WIDTH >> 8);
	einkSendData(EPD_WIDTH & 0xff);        //176
	einkSendData(EPD_HEIGHT >> 8);
	einkSendData(EPD_HEIGHT & 0xff);         //264

	einkSendCommand(DATA_START_TRANSMISSION_1);
	//SysTick_DelayTicks(2U);
	for(i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
		einkSendData(0x00);
	}
	//SysTick_DelayTicks(2);
	einkSendCommand(DATA_START_TRANSMISSION_2);
	//SysTick_DelayTicks(2);
	for(i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
		einkSendData(0x00);
	}
	//SysTick_DelayTicks(2);
}

/**
 * @brief: refresh and displays the frame
 * @return 0 if drawing initiated, 1 if busy
 */
int einkDisplayFrameFromBufferNonBlocking(const unsigned char* frame_buffer_black, const unsigned char* frame_buffer_red) {

	int isBusy = !GPIO_PinRead(BOARD_INITPINS_BUSY_GPIO, BOARD_INITPINS_BUSY_PIN);

	if (isBusy) {
		return 1;
	}

	int i;

	einkSendCommand(TCON_RESOLUTION);
    einkSendData(EPD_WIDTH >> 8);
    einkSendData(EPD_WIDTH & 0xff);        //176
    einkSendData(EPD_HEIGHT >> 8);
    einkSendData(EPD_HEIGHT & 0xff);         //264

    if (frame_buffer_black != NULL) {
        einkSendCommand(DATA_START_TRANSMISSION_1);
        //SysTick_DelayTicks(2U);
        for(i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
            einkSendData(pgm_read_byte(&frame_buffer_black[i]));
            //PRINTF("%x ", pgm_read_byte(&frame_buffer_black[i]));
        }
        //SysTick_DelayTicks(2U);
    }
    if (frame_buffer_red != NULL) {
        einkSendCommand(DATA_START_TRANSMISSION_2);
        //SysTick_DelayTicks(2U);
        for(i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
            einkSendData(pgm_read_byte(&frame_buffer_red[i]));
        }
        //SysTick_DelayTicks(2U);
    }
    einkSendCommand(DISPLAY_REFRESH);

    //einkWaitUntilIdle();

    return 0;
}

/**
 * @brief: refresh and displays the frame
 */
void einkDisplayFrameFromBufferBlocking(const unsigned char* frame_buffer_black, const unsigned char* frame_buffer_red) {


	int i;

	einkSendCommand(TCON_RESOLUTION);
    einkSendData(EPD_WIDTH >> 8);
    einkSendData(EPD_WIDTH & 0xff);        //176
    einkSendData(EPD_HEIGHT >> 8);
    einkSendData(EPD_HEIGHT & 0xff);         //264

    if (frame_buffer_black != NULL) {
        einkSendCommand(DATA_START_TRANSMISSION_1);
        SysTick_DelayTicks(2U);
        for(i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
            einkSendData(pgm_read_byte(&frame_buffer_black[i]));
            //PRINTF("%x ", pgm_read_byte(&frame_buffer_black[i]));
        }
        SysTick_DelayTicks(2U);
    }
    if (frame_buffer_red != NULL) {
        einkSendCommand(DATA_START_TRANSMISSION_2);
        SysTick_DelayTicks(2U);
        for(i = 0; i < EPD_WIDTH * EPD_HEIGHT / 8; i++) {
            einkSendData(pgm_read_byte(&frame_buffer_red[i]));
        }
        SysTick_DelayTicks(2U);
    }
    einkSendCommand(DISPLAY_REFRESH);

    einkWaitUntilIdle();
}

/**
 * @brief: This displays the frame data from SRAM
 */
void einkDisplayFrameFromSRAM() {

	einkSendCommand(DISPLAY_REFRESH);
	einkWaitUntilIdle();

}

/**
 * @brief: Sets a refresh mode for the display. Currently supports "full" (the built-in lookup table)
 * and "fast" (a modified lookup table for fast refreshing, found here: https://github.com/pskowronek/epaper-clock-and-more/blob/master/epds/epd2in7b_fast_lut.py
 * @param refreshMode The refresh mode to be enabled (FAST_REFRESH or FULL_REFRESH)
 */
void einkSetRefreshMode(int refreshMode) {

	if (refreshMode == FAST_REFRESH) {
		einkSendCommand(PANEL_SETTING);
		einkSendData(FAST_REFRESH_SETTING);
		einkSetLut();
	} else {
		einkSendCommand(PANEL_SETTING);
		einkSendData(FULL_REFRESH_SETTING);
	}
}

/**
 * @brief: Initializes the SPI connection with the epd2in7b display
 */
void einkInit() {

	PRINTF("Resetting\r\n");
	einkReset();

	PRINTF("Powering on\r\n");
	einkSendCommand(POWER_ON);
	einkWaitUntilIdle();

	PRINTF("Configuring panel\r\n");
	einkSendCommand(PANEL_SETTING);
	einkSendData(FULL_REFRESH_SETTING);        //KW-BF   KWR-AF    BWROTP 0f (originally 0xaf)

	PRINTF("Setting PLL control\r\n");
	einkSendCommand(PLL_CONTROL);
	einkSendData(0x3a);       //3A 100HZ   29 150Hz 39 200HZ    31 171HZ

	PRINTF("Configuring power settings\r\n");
	einkSendCommand(POWER_SETTING);
	einkSendData(0x03);                  // VDS_EN, VDG_EN
	einkSendData(0x00);                  // VCOM_HV, VGHL_LV[1], VGHL_LV[0]
	einkSendData(0x2b);                  // VDH
	einkSendData(0x2b);                  // VDL
	einkSendData(0x09);                  // VDHR

	PRINTF("Configuring booster\r\n");
	einkSendCommand(BOOSTER_SOFT_START);
	einkSendData(0x07);
	einkSendData(0x07);
	einkSendData(0x17);

	PRINTF("Setting power optimizations\r\n");
	// Power optimization
	einkSendCommand(0xF8);
	einkSendData(0x60);
	einkSendData(0xA5);

	// Power optimization
	einkSendCommand(0xF8);
	einkSendData(0x89);
	einkSendData(0xA5);

	// Power optimization
	einkSendCommand(0xF8);
	einkSendData(0x90);
	einkSendData(0x00);

	// Power optimization
	einkSendCommand(0xF8);
	einkSendData(0x93);
	einkSendData(0x2A);

	// Power optimization
	einkSendCommand(0xF8);
	einkSendData(0x73);
	einkSendData(0x41);

	PRINTF("Other settings\r\n");
	einkSendCommand(VCM_DC_SETTING_REGISTER);
	einkSendData(0x12);

	einkSendCommand(VCOM_AND_DATA_INTERVAL_SETTING);
	einkSendData(0x87);        // define by OTP

	PRINTF("Populating lookup tables\r\n");
    einkSetLut();

    PRINTF("Refreshing display\r\n");
    einkSendCommand(PARTIAL_DISPLAY_REFRESH);
    einkSendData(0x00);

	PRINTF("\r\nTURNED ON!!!\r\n");

}

/**
 *  @brief: clear the image
 */
void paintClear(unsigned char* image, int colored) {

	int x, y;

    for (x = 0; x < PAINT_WIDTH; x++) {
        for (y = 0; y < PAINT_HEIGHT; y++) {
            paintDrawAbsolutePixel(image, x, y, colored);
        }
    }
}

/**
 *  @brief: this draws a pixel by absolute coordinates.
 *          this function won't be affected by the rotate parameter.
 */
void paintDrawAbsolutePixel(unsigned char* image, int x, int y, int colored) {
    if (x < 0 || x >= PAINT_WIDTH || y < 0 || y >= PAINT_HEIGHT) {
        return;
    }
    if (IF_INVERT_COLOR) {
        if (colored) {
            image[(x + y * PAINT_WIDTH) / 8] |= 0x80 >> (x % 8);
        } else {
            image[(x + y * PAINT_WIDTH) / 8] &= ~(0x80 >> (x % 8));
        }
    } else {
        if (colored) {
            image[(x + y * PAINT_WIDTH) / 8] &= ~(0x80 >> (x % 8));
        } else {
            image[(x + y * PAINT_WIDTH) / 8] |= 0x80 >> (x % 8);
        }
    }
}

/**
 *  @brief: this draws a pixel by the coordinates
 */
void paintDrawPixel(unsigned char* image, int x, int y, int colored, int rotate) {
    int point_temp;
    if (rotate == ROTATE_0) {
        if(x < 0 || x >= PAINT_WIDTH || y < 0 || y >= PAINT_HEIGHT) {
            return;
        }
        paintDrawAbsolutePixel(image, x, y, colored);
    } else if (rotate == ROTATE_90) {
        if(x < 0 || x >= PAINT_HEIGHT || y < 0 || y >= PAINT_WIDTH) {
          return;
        }
        point_temp = x;
        x = PAINT_WIDTH - y;
        y = point_temp;
        paintDrawAbsolutePixel(image, x, y, colored);
    } else if (rotate == ROTATE_180) {
        if(x < 0 || x >= PAINT_WIDTH || y < 0 || y >= PAINT_HEIGHT) {
          return;
        }
        x = PAINT_WIDTH - x;
        y = PAINT_HEIGHT - y;
        paintDrawAbsolutePixel(image, x, y, colored);
    } else if (rotate == ROTATE_270) {
        if(x < 0 || x >= PAINT_HEIGHT || y < 0 || y >= PAINT_WIDTH) {
          return;
        }
        point_temp = x;
        x = y;
        y = PAINT_HEIGHT - point_temp;
        paintDrawAbsolutePixel(image, x, y, colored);
    }
}

/**
 *  @brief: this draws a charactor on the frame buffer but not refresh
 */
void paintDrawChar(unsigned char* image, int x, int y, char ascii_char, sFONT* font, int colored, int scale) {
    int i, j, k, l;
    unsigned int char_offset = (ascii_char - ' ') * font->Height * (font->Width / 8 + (font->Width % 8 ? 1 : 0));
    const unsigned char* ptr = &font->table[char_offset];

    for (j = 0; j < font->Height; j++) {
        for (i = 0; i < font->Width; i++) {
            if (pgm_read_byte(ptr) & (0x80 >> (i % 8))) {

            	for (k = i*scale; k < (i+1)*scale; k++) {
            		for (l = j*scale; l < (j+1)*scale; l++) {
                        paintDrawPixel(image, x + k, y + l, colored, ROTATE_270);
            		}
            	}
            }
            if (i % 8 == 7) {
                ptr++;
            }
        }
        if (font->Width % 8 != 0) {
            ptr++;
        }
    }
}

/**
*  @brief: this displays a string on the frame buffer but not refresh
*/
void paintDrawString(unsigned char* image, int x, int y, const char* text, sFONT* font, int colored, int scale) {
    const char* p_text = text;
    unsigned int counter = 0;
    int refcolumn = x;

    /* Send the string character by character on EPD */
    while (*p_text != '\0') {
        /* Display one character on EPD */
        paintDrawChar(image, refcolumn, y, *p_text, font, colored, scale);
        /* Decrement the column position by 16 */
        refcolumn += font->Width * scale;
        /* Point on the next character */
        p_text++;
        counter++;
    }
}

///**
// *  @brief: this draws a QR code using the qrcodegen library (https://www.nayuki.io/page/qr-code-generator-library#c)
// */
//void paintDrawQRCode(unsigned char* image, const char* text, int scale) {
//
//	enum qrcodegen_Ecc errCorLvl = qrcodegen_Ecc_LOW;  // Error correction level
//
//	// Make and print the QR Code symbol
//	uint8_t qrcode[qrcodegen_BUFFER_LEN_MAX];
//	uint8_t tempBuffer[qrcodegen_BUFFER_LEN_MAX];
//	bool ok = qrcodegen_encodeText(text, tempBuffer, qrcode, errCorLvl,
//		qrcodegen_VERSION_MIN, qrcodegen_VERSION_MAX, qrcodegen_Mask_AUTO, true);
//	if (ok) {
//
//		int size = qrcodegen_getSize(qrcode);
//
//		int x, y;
//
//		//draw QR code
//		for (y = 0; y < size; y++) {
//			for (x = 0; x < size; x++) {
//
//				int i, j;
//
//				//scale QR code
//				for (i = x*scale; i < (x+1)*scale; i++) {
//					for (j = y*scale; j < (y+1)*scale; j++) {
//						paintDrawAbsolutePixel(image, i, j, qrcodegen_getModule(qrcode, x, y));
//					}
//				}
//
//				//PRINTF(qrcodegen_getModule(qrcode, x, y) ? "##" : "  ");
//			}
//			//PRINTF("\r\n");
//		}
//		//PRINTF("\r\n");
//	}
//}

