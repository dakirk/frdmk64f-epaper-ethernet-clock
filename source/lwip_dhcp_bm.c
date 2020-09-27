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
#include <stdlib.h>

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
#include "fsl_rtc.h"

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

#define EST -5 //Eastern standard time is -5 GMT

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

/*******************************************************************************
 * Variables
 ******************************************************************************/

volatile uint32_t g_systickCounter;
volatile bool g_SecsFlag        = false;
unsigned char imgBuffer[5808];
unsigned char colorBuffer[5808];

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Override the RTC IRQ handler.
 */
void RTC_IRQHandler(void)
{
    if (RTC_GetStatusFlags(RTC) & kRTC_AlarmFlag)
    {
        //g_AlarmPending = 1U;

        /* Clear alarm flag */
        RTC_ClearStatusFlags(RTC, kRTC_AlarmInterruptEnable);
    }
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

/*!
 * @brief Override the RTC Second IRQ handler.
 */
void RTC_Seconds_IRQHandler(void)
{
    g_SecsFlag = true;
    /* Add for ARM errata 838869, affects Cortex-M4, Cortex-M4F Store immediate overlapping
    exception return operation might vector to incorrect interrupt */
    __DSB();
}

/*!
 * @brief Interrupt service for SysTick timer.
 */
void SysTick_Handler(void)
{
    time_isr();

    if (eink_systickCounter != 0U)
    {
        eink_systickCounter--;
    }
}

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
 * @brief Gets the month as a 3-character string, for brevity/to save screen real estate
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

// copied from https://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date/5590518
bool isDst(struct tm* time_info) {

	int month = time_info->tm_mon + 1; // +1 to make 1 represent January
	int day = time_info->tm_mday;
	int dow = time_info->tm_wday + 1; // +1 to make 1 represent Sunday

	// January, February, and December are out.
	if (month < 3 || month > 11)
		return false;

	// April to October are in
	if (month > 3 && month < 11)
		return true;

	int previousSunday = day - dow;

	// In march, we are DST if our previous Sunday was on or after the 8th.
	if (month == 3)
		return previousSunday >= 8;

	// In November we must be before the first Sunday to be DST.
	// That means the previous Sunday must be before the 1st.
	return previousSunday <= 0;
}

/*!
 * @brief Gets localized time, based on a given timezone (I'm ignoring the localtime function
 * because I've been unable to get the TZ environment variable to be read properly so far)
 * @param utc the time in UTC
 * @param timezone the timezone, as a difference from UTC (ex: EST is -5)
 * @return a tm structure with the localized time
 */
struct tm* getLocalizedTime(time_t utc, int timezone) {

	time_t local_time = utc + (3600 * timezone);
	struct tm* gmt_info = gmtime(&local_time);

	// Determine if it's daylight savings right now
	local_time = utc + (3600 * (timezone + isDst(gmt_info)));

	return gmtime(&local_time);
}

/*!
 * @brief Callback method to get time from the RTC and draw the display with time and date
 */
void updateTime() {

    rtc_datetime_t date;
    RTC_GetDatetime(RTC, &date);

	uint32_t timestamp = (time_t)RTC_ConvertDatetimeToSeconds(&date);

	struct tm* timeinfo = getLocalizedTime(timestamp, EST);

	// if it's a new minute, refresh the screen
	if (timeinfo->tm_sec == 0)
	{
		PRINTF("Updating screen\r\n");

		int digitScale = 7;

		// at midnight, do a full refresh
		if (timeinfo->tm_hour == 0 && timeinfo->tm_min == 0) {
			einkSetRefreshMode(FULL_REFRESH);
			einkClearFrame();
			einkDisplayFrameFromSRAMBlocking();
			einkSetRefreshMode(FAST_REFRESH);
		}
		// clear screen every hour
		else if (timeinfo->tm_min % 60 == 0) {
			einkClearFrame();
			einkDisplayFrameFromSRAMBlocking();
			einkDisplayFrameFromSRAMBlocking();
			einkDisplayFrameFromSRAMBlocking();
		}

		char timeBuf[6];
		char dateBuf[20];

		int twelveHourTime = getTwelveHourTime(timeinfo->tm_hour);

		sprintf(timeBuf, "%d:%02d", twelveHourTime, timeinfo->tm_min);
		sprintf(dateBuf, "%s, %s %02d", getDayOfWeek(timeinfo->tm_wday), getMonth(timeinfo->tm_mon), timeinfo->tm_mday);

		paintDrawString(imgBuffer,
						strlen(timeBuf) * 7 * digitScale - 9,
						25, (timeinfo->tm_hour > 11 ? "PM" : "AM"),
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
		einkDisplayFrameFromBufferNonBlocking(imgBuffer, NULL);
		paintClear(imgBuffer, UNCOLORED);
		paintClear(colorBuffer, UNCOLORED);

		g_systickCounter = 0;
	}

    PRINTF("Daylight savings: %d\r\n", timeinfo->tm_isdst);
	PRINTF("%02d:%02d:%02d\r\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
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

/** Board setup ************************************************************************************************/

#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
    mem_range_t non_dma_memory[] = NON_DMA_MEMORY_ARRAY;
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */

    SYSMPU_Type *base = SYSMPU;
    BOARD_InitPins();
    BOARD_BootClockRUN();
    BOARD_InitDebugConsole();
    /* Disable SYSMPU. */
    base->CESR &= ~SYSMPU_CESR_VLD_MASK;

/** Timer setup ************************************************************************************************/

    /* Set systick reload value to generate 1ms interrupt */
    if (SysTick_Config(SystemCoreClock / 1000U))
    {
        while (1)
        {
        }
    }

/** E-ink setup ************************************************************************************************/

    spiInit();
    einkInit();

    einkClearFrame();
	einkDisplayFrameFromSRAMNonBlocking();

/** Ethernet setup *********************************************************************************************/

	struct netif netif;

    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
    ethernetif_config_t enet_config = {
        .phyAddress = EXAMPLE_PHY_ADDRESS,
        .clockName  = EXAMPLE_CLOCK_NAME,
        .macAddress = configMAC_ADDR,
#if defined(FSL_FEATURE_SOC_LPC_ENET_COUNT) && (FSL_FEATURE_SOC_LPC_ENET_COUNT > 0)
        .non_dma_memory = non_dma_memory,
#endif /* FSL_FEATURE_SOC_LPC_ENET_COUNT */
    };

    time_init();

    IP4_ADDR(&netif_ipaddr, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_netmask, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_gw, 0U, 0U, 0U, 0U);

    lwip_init();

    netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN, ethernet_input);
    netif_set_default(&netif);
    netif_set_up(&netif);

/** DHCP setup *************************************************************************************************/

    dhcp_start(&netif);

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" DHCP Setup\r\n");
    PRINTF("************************************************\r\n");

    // Wait for DHCP startup
    while (print_dhcp_state(&netif)) {
        /* Poll the driver, get any outstanding frames */
        ethernetif_input(&netif);

        /* Handle all system timeouts for all core protocols */
        sys_check_timeouts();
    }

/** RTC setup ************************************************************************************************/

    // Start RTC before SNTP to ensure that SNTP can access RTC
    PRINTF("Attempting RTC init... ");
    rtc_config_t rtcConfig;

    /* Init RTC */
    RTC_GetDefaultConfig(&rtcConfig);
    RTC_Init(RTC, &rtcConfig);
#if !(defined(FSL_FEATURE_RTC_HAS_NO_CR_OSCE) && FSL_FEATURE_RTC_HAS_NO_CR_OSCE)

    /* Select RTC clock source */
    RTC_SetClockSource(RTC);
#endif /* FSL_FEATURE_RTC_HAS_NO_CR_OSCE */

    /* RTC time counter has to be stopped before setting the date & time in the TSR register */
    RTC_StopTimer(RTC);

    /* Enable at the NVIC */
    EnableIRQ(RTC_IRQn);
#ifdef RTC_SECONDS_IRQS
    EnableIRQ(RTC_Seconds_IRQn);
#endif /* RTC_SECONDS_IRQS */

    RTC_EnableInterrupts(RTC, kRTC_SecondsInterruptEnable);
    PRINTF("DONE\r\n");

/** SNTP setup ***********************************************************************************************/

    PRINTF("Attempting SNTP init... ");
    sntp_setoperatingmode(SNTP_OPMODE_POLL);

    sntp_setservername(0, "pool.ntp.org");
    sntp_setservername(1, "time.google.com");

    sntp_init();
    PRINTF("DONE\r\n");

    PRINTF("Waiting for display to finish refreshing... ");
	einkWaitUntilIdle();
	einkSetRefreshMode(FAST_REFRESH);
	PRINTF("DONE\r\n");

/** Main loop ************************************************************************************************/

    while (1)
    {
        /* Poll the driver, get any outstanding frames */
        ethernetif_input(&netif);

        /* Handle all system timeouts for all core protocols */
        sys_check_timeouts();

        /* Print DHCP progress */
        print_dhcp_state(&netif);

        if (g_SecsFlag) {

        	g_SecsFlag = false;
        	updateTime();
        }
    }
}
#endif
