
#ifndef _EINK_CONTROL_H_
#define _EINK_CONTROL_H_

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_dspi.h"
#include "fsl_gpio.h"
#include "board.h"

#include "pin_mux.h"
#include "clock_config.h"

#include <stdbool.h>
#include <stdio.h>

#include "epdmacros.h"
#include "fonts.h"
//#include "qrcodegen.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/

#define EXAMPLE_DSPI_MASTER_BASEADDR SPI0
#define DSPI_MASTER_CLK_SRC DSPI0_CLK_SRC
#define DSPI_MASTER_CLK_FREQ CLOCK_GetFreq(DSPI0_CLK_SRC)
#define EXAMPLE_DSPI_MASTER_PCS_FOR_INIT kDSPI_Pcs0
#define EXAMPLE_DSPI_MASTER_PCS_FOR_TRANSFER kDSPI_MasterPcs0
#define EXAMPLE_DSPI_DEALY_COUNT 0xfffffU

#define TRANSFER_SIZE 1U         /*! Transfer dataSize */
#define TRANSFER_BAUDRATE 2000000U /*! Transfer baudrate - 2M */

#define pgm_read_byte(addr)   (*(const unsigned char *)(addr)) //copied from https://github.com/espressif/arduino-esp32/blob/master/cores/esp32/pgmspace.h

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

extern volatile uint32_t eink_systickCounter;

extern uint8_t masterRxData[TRANSFER_SIZE];
extern uint8_t masterTxData[TRANSFER_SIZE];

extern const unsigned char lut_vcom_dc[];
extern const unsigned char lut_ww[];
extern const unsigned char lut_bw[];
extern const unsigned char lut_bb[];
extern const unsigned char lut_wb[];


// delay functions--will remove when transferred to FreeRTOS
void SysTick_Handler(void);
void SysTick_DelayTicks(uint32_t n);

// SPI utility functions
void spiInit();
void spiWrite8(uint8_t data);

// E-ink utility functions
void einkReset();
void einkSleep();
void einkSendCommand(uint8_t data);
void einkSendData(uint8_t data);
void einkWaitUntilIdle();
void einkSetLut();

// E-ink control functions (these are the ones to use)
void einkInit();
void einkSetRefreshMode(int refreshMode);
void einkClearFrame();
void einkDisplayFrameFromBufferBlocking(const unsigned char* frame_buffer_black, const unsigned char* frame_buffer_red);
int  einkDisplayFrameFromBufferNonBlocking(const unsigned char* frame_buffer_black, const unsigned char* frame_buffer_red);

void einkDisplayFrameFromSRAMBlocking();
void einkDisplayFrameFromSRAMNonBlocking();

// Paint functions, for modifying the image buffer before passing it through SPI
void paintClear(unsigned char* image, int colored);
void paintDrawAbsolutePixel(unsigned char* image, int x, int y, int colored);
void paintDrawPixel(unsigned char* image, int x, int y, int colored, int rotate);
void paintDrawChar(unsigned char* image, int x, int y, char ascii_char, sFONT* font, int colored, int scale);
void paintDrawString(unsigned char* image, int x, int y, const char* text, sFONT* font, int colored, int scale);
//void paintDrawQRCode(unsigned char* image, const char* text, int scale);

#endif
