/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2019 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/

#include <time.h>
#include <string.h>

#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_DHCP

#include "lwip/timeouts.h"
#include "lwip/ip_addr.h"
#include "lwip/init.h"
#include "lwip/dhcp.h"
#include "lwip/apps/sntp.h"
#include "lwip/prot/dhcp.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"

#include "board.h"

#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "fsl_dspi.h"
#include "fsl_gpio.h"

#include "pin_mux.h"
#include "clock_config.h"

#include "eink_control.h"
#include "font12.cpp"
/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* MAC address configuration. */
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x11 \
    }

/* Address of PHY interface. */
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

/* System clock name. */
#define EXAMPLE_CLOCK_NAME kCLOCK_CoreSysClk

/* GPIO pin configuration. */
#define BOARD_LED_GPIO BOARD_LED_RED_GPIO
#define BOARD_LED_GPIO_PIN BOARD_LED_RED_GPIO_PIN
#define BOARD_SW_GPIO BOARD_SW3_GPIO
#define BOARD_SW_GPIO_PIN BOARD_SW3_GPIO_PIN
#define BOARD_SW_PORT BOARD_SW3_PORT
#define BOARD_SW_IRQ BOARD_SW3_IRQ
#define BOARD_SW_IRQ_HANDLER BOARD_SW3_IRQ_HANDLER


#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

volatile uint32_t g_systickCounter;
unsigned char imgBuffer[5808];
unsigned char colorBuffer[5808];

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Function to convert international (24-hour time) to American-style 12-hour time
 * @param intlTime the 24-hour notation in hours
 * @return the 12-hour notation in hours
 */
int getTwelveHourTime(int intlTime) {

	int twelveHourTime;

	if (intlTime > 12) {
		twelveHourTime = intlTime - 12;
	} else if (intlTime == 0) {
		twelveHourTime = 12;
	} else {
		twelveHourTime = intlTime;
	}

	return twelveHourTime;
}

/*!
 * @brief Gets the day of the week as a string, given a number
 * @param day the numerical representation of the day of the week (0-6)
 * @return a string for the name of the day of the week
 */
const char* getDayOfWeek(int day) {
	switch(day) {
	case 0:
		return "Sunday";
	case 1:
		return "Monday";
	case 2:
		return "Tuesday";
	case 3:
		return "Wednesday";
	case 4:
		return "Thursday";
	case 5:
		return "Friday";
	case 6:
		return "Saturday";
	default:
		return "NULLday";
	}
}

/*!
 * Gets the month as a 3-character string, for brevity/to save screen real estate
 * @param month the numerical representation of the month (0-11)
 * @return the string version of the month
 */
const char* getMonth(int month) {
	switch(month) {
	case 0:
		return "Jan";
	case 1:
		return "Feb";
	case 2:
		return "Mar";
	case 3:
		return "Apr";
	case 4:
		return "May";
	case 5:
		return "Jun";
	case 6:
		return "Jul";
	case 7:
		return "Aug";
	case 8:
		return "Sep";
	case 9:
		return "Oct";
	case 10:
		return "Nov";
	case 11:
		return "Dec";
	default:
		return "Smr";
	}
}

/*!
 * @brief Interrupt service for SysTick timer.
 */
void SysTick_Handler(void)
{
    time_isr();

    //delay stuff
    if (eink_systickCounter != 0U)
    {
        eink_systickCounter--;
    }

    //time stuff
    struct tm* timeinfo = gmtime(&global_time);

    // make sure system time has been initialized
    if (global_time != 0) {

    	// if a second has passed
    	if (g_systickCounter > 0 && g_systickCounter % 1000 == 0) {

    		// if it's a new minute, refresh the screen
    		if (timeinfo->tm_sec == 0)
    		{
        		PRINTF("Updating screen\r\n");

        		int digitScale = 7;

        		//fully clear screen every 10 minutes
        		if (timeinfo->tm_min % 30 == 0) {
        			einkClearFrame();
        			einkDisplayFrameFromSRAM();
        			einkDisplayFrameFromSRAM();
        			einkDisplayFrameFromSRAM();
        			global_time += 10; // refresh takes about 3 seconds, so we compensate
        		}

        		char timeBuf[6];
        		char dateBuf[20];

        		int twelveHourTime = getTwelveHourTime(timeinfo->tm_hour);

        		sprintf(timeBuf, "%d:%02d", twelveHourTime, timeinfo->tm_min);
        		sprintf(dateBuf, "%s, %s %02d", getDayOfWeek(timeinfo->tm_wday), getMonth(timeinfo->tm_mon), timeinfo->tm_mday);

        		paintDrawString(imgBuffer,
        						strlen(timeBuf) * 7 * digitScale - 9,
								25, (timeinfo->tm_hour > 12 ? "PM" : "AM"),
								&Font12,
								COLORED,
								2
				);

        		//draw date info
        		paintDrawString(imgBuffer, 4, 0, dateBuf, &Font12, COLORED, 2);

        		//draw time
        		paintDrawString(imgBuffer, -3, 20, timeBuf, &Font12, COLORED, digitScale);

        		//draw time drop shadow
        		paintDrawString(colorBuffer, -1, 22, timeBuf, &Font12, COLORED, digitScale);
        		paintDrawString(colorBuffer, -3, 20, timeBuf, &Font12, UNCOLORED, digitScale);

        		//push to display
        		einkDisplayFrameFromBufferNonBlocking(imgBuffer, colorBuffer);
        		paintClear(imgBuffer, UNCOLORED);
        		paintClear(colorBuffer, UNCOLORED);

        		g_systickCounter = 0;
        	}

			PRINTF("%02d:%02d:%02d\r\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
			global_time++;
		}

    	g_systickCounter++;
    }

}

/*!
 * @brief Prints DHCP status of the interface when it has changed from last status.
 *
 * @param netif network interface structure
 */
static int print_dhcp_state(struct netif *netif)
{
    static u8_t dhcp_last_state = DHCP_STATE_OFF;
    struct dhcp *dhcp           = netif_dhcp_data(netif);

    if (dhcp == NULL)
    {
        dhcp_last_state = DHCP_STATE_OFF;
    }
    else if (dhcp_last_state != dhcp->state)
    {
        dhcp_last_state = dhcp->state;

        PRINTF(" DHCP state       : ");
        switch (dhcp_last_state)
        {
            case DHCP_STATE_OFF:
                PRINTF("OFF");
                break;
            case DHCP_STATE_REQUESTING:
                PRINTF("REQUESTING");
                break;
            case DHCP_STATE_INIT:
                PRINTF("INIT");
                break;
            case DHCP_STATE_REBOOTING:
                PRINTF("REBOOTING");
                break;
            case DHCP_STATE_REBINDING:
                PRINTF("REBINDING");
                break;
            case DHCP_STATE_RENEWING:
                PRINTF("RENEWING");
                break;
            case DHCP_STATE_SELECTING:
                PRINTF("SELECTING");
                break;
            case DHCP_STATE_INFORMING:
                PRINTF("INFORMING");
                break;
            case DHCP_STATE_CHECKING:
                PRINTF("CHECKING");
                break;
            case DHCP_STATE_BOUND:
                PRINTF("BOUND");
                break;
            case DHCP_STATE_BACKING_OFF:
                PRINTF("BACKING_OFF");
                break;
            default:
                PRINTF("%u", dhcp_last_state);
                assert(0);
                break;

        }
        PRINTF("\r\n");

        if (dhcp_last_state == DHCP_STATE_BOUND)
        {
            PRINTF("\r\n IPv4 Address     : %s\r\n", ipaddr_ntoa(&netif->ip_addr));
            PRINTF(" IPv4 Subnet mask : %s\r\n", ipaddr_ntoa(&netif->netmask));
            PRINTF(" IPv4 Gateway     : %s\r\n\r\n", ipaddr_ntoa(&netif->gw));
            return 0; //finished
        }
    }
    return 1; //not finished
}

/*!
 * @brief Main function.
 */
int main(void)
{
    struct netif netif;
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
    mem_range_t non_dma_memory[] = NON_DMA_MEMORY_ARRAY;
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
    ethernetif_config_t enet_config = {
        .phyAddress = EXAMPLE_PHY_ADDRESS,
        .clockName  = EXAMPLE_CLOCK_NAME,
        .macAddress = configMAC_ADDR,
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
        .non_dma_memory = non_dma_memory,
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */
    };

    SYSMPU_Type *base = SYSMPU;
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    /* Disable SYSMPU. */
    base->CESR &= ~SYSMPU_CESR_VLD_MASK;

    /* Set systick reload value to generate 1ms interrupt */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        while (1)
        {
        }
    }

    spiInit();
    einkInit();

    einkClearFrame();
	einkDisplayFrameFromSRAM();
	einkDisplayFrameFromSRAM();
	einkDisplayFrameFromSRAM();

    time_init();

    IP4_ADDR(&netif_ipaddr, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_netmask, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_gw, 0U, 0U, 0U, 0U);

    lwip_init();

    netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

    dhcp_start(&netif);

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" DHCP example\r\n");
    PRINTF("************************************************\r\n");

    while (print_dhcp_state(&netif)) {
        /* Poll the driver, get any outstanding frames */
        ethernetif_input(&netif);

        /* Handle all system timeouts for all core protocols */
        sys_check_timeouts();
    }

    PRINTF("Attempting SNTP init\r\n");

    sntp_setoperatingmode(SNTP_OPMODE_POLL);
//  #if LWIP_DHCP
//    sntp_servermode_dhcp(1); /* get SNTP server via DHCP */
//  #else /* LWIP_DHCP */
//  #if LWIP_IPV4
//    sntp_setserver(0, netif_ip_gw4(netif_default));
//  #endif /* LWIP_IPV4 */
//  #endif /* LWIP_DHCP */

    ip4_addr_t ntp_pool_ipaddr;
    ip4_addr_t ntp_google_ipaddr;

    IP4_ADDR(&ntp_pool_ipaddr, 66U, 96U, 98U, 9U);
    IP4_ADDR(&ntp_google_ipaddr, 216U, 239U, 35U, 4U);

    sntp_setserver(0, &ntp_pool_ipaddr);
    sntp_setserver(1, &ntp_google_ipaddr);

    sntp_init();

    // eink setup

    while (1)
    {
        /* Poll the driver, get any outstanding frames */
        ethernetif_input(&netif);

        /* Handle all system timeouts for all core protocols */
        sys_check_timeouts();

        /* Print DHCP progress */
        print_dhcp_state(&netif);

        //PRINTF("sec: %d; usec: %d", sec, usec);

        //PRINTF("%02d:%02d\r\n", timeinfo->tm_hour, timeinfo->tm_min);

    }
}
#endif
