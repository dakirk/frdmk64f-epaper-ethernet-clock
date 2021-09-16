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
#include "lwip/dns.h"
#include "lwip/tcp.h"
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
#include "icons.h"
#include "api_key.h"

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

#define API_URL "api.openweathermap.org"


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

volatile bool g_SecsFlag        = false;

unsigned char imgBuffer[5808];
unsigned char colorBuffer[5808];

struct tcp_pcb * weather_pcb;
char icon_str[5];
char temperature_str[7];
char weather_str[45];
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
 * @brief Simple utility function to round numbers (intended for temperatures), since this implementation of math.h doesn't seem to have one
 */
int round(double x)
{
    if (x < 0.0)
        return (int)(x - 0.5);
    else
        return (int)(x + 0.5);
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

/*!
 * @brief Function to handle daylight savings time adjustment, assuming US DST settings.
 * Copied from https://stackoverflow.com/questions/5590429/calculating-daylight-saving-time-from-only-date/5590518
 * @param time_info the tm struct holding the current time
 * @return true if daylight savings is active, false otherwise
 */
bool isDst(struct tm* time_info) {

	int month = time_info->tm_mon + 1; // +1 to make 1 represent January
	int day = time_info->tm_mday;
	int dow = time_info->tm_wday; // +1 to make 1 represent Sunday

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

// TCP code adapted from here: https://stackoverflow.com/questions/26192758/how-can-i-send-a-simple-http-request-with-a-lwip-stack

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
 * @brief Prints DHCP status of the interface when it has changed from last status.
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
 * @brief Sets up a HTTP request to the weather API
 */
err_t tcp_send_packet(void)
{
    char api_string_buf[200];

    sprintf(api_string_buf, "GET /data/2.5/weather?q=Boston&units=imperial&appid=%s HTTP/1.0\r\nHost: openweathermap.org\r\n\r\n ", weather_key);
    uint32_t len = strlen(api_string_buf);

    /* push to buffer */
    err_t error = tcp_write(weather_pcb, api_string_buf, len, TCP_WRITE_FLAG_COPY);

    if (error) {
        PRINTF("ERROR: Code: %d (tcp_send_packet :: tcp_write)\r\n", error);
        return 1;
    }

    /* now send */
    error = tcp_output(weather_pcb);
    if (error) {
        PRINTF("ERROR: Code: %d (tcp_send_packet :: tcp_output)\r\n", error);
    }
    return error;
}

/*!
 * @brief Callback triggered when a TCP (in this case, HTTP) packet is sent. Currently used for debug only.
 */
err_t tcpSendCallback(void *arg, struct tcp_pcb *tpcb, u16_t len) {
    PRINTF("Packet sent!\r\n");
    return 0;
}

/*!
 * @brief Callback triggered when a TCP (in this case, HTTP) packet is received. Currently used to parse a JSON string
 * by looking for very specific substrings matching the keys of the temperature, weather description, and icon.
 * TODO: Import a full JSON parser to support more complex data (like JSON lists or nested objects) and avoid possible bugs.
 */
err_t tcpRecvCallback(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    PRINTF("Data received.\r\n");
    if (p == NULL) {
        PRINTF("The remote host closed the connection.\r\n");
        PRINTF("Now I'm closing the connection.\r\n");
        tcp_close(tpcb);
        return ERR_OK;
    } else {
        PRINTF("Number of pbufs %d\r\n", pbuf_clen(p));
        //PRINTF("Contents of pbuf %s\r\n", (char *)p->payload);

        //The following blocks of code must NOT be re-arranged, because strtok will shorten the string as it works

        char* temperature_base = (char*)p->payload;

        // get temperature (F)
        char* temperature = strstr(temperature_base, "\"temp\":");
        if (temperature != NULL) {
            temperature += sizeof(char) * strlen("\"temp\":");

            strtok(temperature,",");
            if (temperature != NULL) {
            	PRINTF("%s\r\n", temperature);
            	strncpy(temperature_str, temperature, 6); // assume temperatures are below 999.999 degrees F
            }

        }

        char* weather_icon_base = (char*)p->payload;
        char* weather_icon = strstr(weather_icon_base, "\"icon\":\"");

        // get weather description (make this into a function?)
        if (weather_icon != NULL) {
            weather_icon += sizeof(char) * strlen("\"icon\":\"");
            strtok(weather_icon,"\"");
            if (weather_icon != NULL) {
            	PRINTF("%s\r\n", weather_icon);
            	strncpy(icon_str, weather_icon, 7);
            }
        }

        char* weather_description_base = (char*)p->payload;
        char* weather_description = strstr(weather_description_base, "\"description\":\"");

        // get weather icon
        if (weather_description != NULL) {
            weather_description += sizeof(char) * strlen("\"description\":\"");
            strtok(weather_description,"\"");
            if (weather_description != NULL) {
            	PRINTF("%s\r\n", weather_description);
            	strncpy(weather_str, weather_description, 45);
            }
        }
    }

    pbuf_free(p);

    return 0;
}

/*!
 * @brief Callback triggered when the TCP stack encounters an error. Currently used for logging only.
 */
void tcpErrorHandler(void *arg, err_t err) {
	PRINTF("Error???\r\n");
}

/*!
 * @brief Callback triggered when the TCP stack establishes a connection. Currently used for logging only.
 */
err_t connectCallback(void *arg, struct tcp_pcb *tpcb, err_t err)
{
    PRINTF("Connection Established.\r\n");
    //PRINTF("Now sending a packet\r\n");
    //tcp_send_packet();
    return 0;
}

/*!
 * @brief Function to establish a TCP connection and set up the callbacks.
 */
void tcp_setup()
{
    uint32_t data = 0xdeadbeef;

    /* create the control block */
    weather_pcb = tcp_new();    //testpcb is a global struct tcp_pcb
                            // as defined by lwIP

    /* dummy data to pass to callbacks*/

    tcp_arg(weather_pcb, &data);

    /* register callbacks with the pcb */

    tcp_err(weather_pcb, tcpErrorHandler);
    tcp_recv(weather_pcb, tcpRecvCallback);
    tcp_sent(weather_pcb, tcpSendCallback);

}

/*!
 * @brief Function to set in motion a weather API request. Uses IP address returned by DNS.
 * @param ip the IP address to send the request to, obtained from DNS
 */
void tcp_get_weather_update(ip4_addr_t ip) {
	// calls tcp_new to create new pcb (VERY IMPORTANT)
	tcp_setup();

    /* create an ip */
    //struct ip4_addr ip;
    //IP4_ADDR(&ip, 192,241,245,161);    //IP of example.com

    /* now connect */
    tcp_connect(weather_pcb, &ip, 80, connectCallback);

	tcp_send_packet();
}

void dns_callback(const char *name, const ip_addr_t *ipaddr, void *callback_arg) {
	PRINTF("IPv4 Address for %s     : %u.%u.%u.%u\r\n", name, ((u8_t *)ipaddr)[0], ((u8_t *)ipaddr)[1],
	           ((u8_t *)ipaddr)[2], ((u8_t *)ipaddr)[3]);

	tcp_get_weather_update(*ipaddr);
}

/*!
 * @brief Function to set DNS request in motion. Supports both cached IP addresses and new ones.
 * @param url The URL to decode
 */
void dns_send_http_request(const char* url) {
	ip_addr_t resolved;
	err_t errMessage = dns_gethostbyname(url, &resolved, dns_callback, NULL);

    switch(errMessage) {
		case ERR_OK:
			PRINTF("DNS OK - IP cached\r\n");
			tcp_get_weather_update(resolved);
			break;
		case ERR_INPROGRESS:
			PRINTF("DNS IN PROGRESS - using callback function\r\n");
			break;
		default:
			PRINTF("DNS ERROR: %d\r\n", errMessage);
			break;
    }
}

/*!
 * @brief Draw a weather icon based on which icon string we receive
 * @param blackBuf the black bitmap
 * @param redBuf the red bitmap
 * @param x the starting x coordinate for the icon
 * @param y the starting y coordinate for the icon
 * @param iconCode the icon string to be interpreted
 */
void drawWeather(unsigned char* blackBuf, unsigned char* redBuf, int x, int y, const char* iconCode) {

	if (strcmp(iconCode, "01d") == 0) {
		paintDrawIcon(redBuf, x, y, sun, COLORED);
	}

	else if (strcmp(iconCode, "01n") == 0) {
		paintDrawIcon(blackBuf, x, y, sun, COLORED);
	}

	else if (strcmp(iconCode, "02d") == 0) {
		paintDrawIcon(blackBuf, x, y, partcloudy_cloud, COLORED);
		paintDrawIcon(redBuf, x, y, partcloudy_sun, COLORED);
	}

	else if (strcmp(iconCode, "02n") == 0) {
		paintDrawIcon(blackBuf, x, y, partcloudy_cloud, COLORED);
		paintDrawIcon(blackBuf, x, y, partcloudy_sun, COLORED);
	}

	else if (strcmp(iconCode, "03d") == 0 || strcmp(iconCode, "03n") == 0) {
		paintDrawIcon(blackBuf, x, y, partcloudy_cloud, COLORED);
	}

	else if (strcmp(iconCode, "04d") == 0 || strcmp(iconCode, "04n") == 0) {
		paintDrawIcon(blackBuf, x, y, cloudy, COLORED);
	}

	else if (strcmp(iconCode, "09d") == 0) {
		paintDrawIcon(blackBuf, x, y, sunrainy_cloud, COLORED);
		paintDrawIcon(redBuf, x, y, sunrainy_sun, COLORED);
	}

	else if (strcmp(iconCode, "09n") == 0) {
		paintDrawIcon(blackBuf, x, y, sunrainy_cloud, COLORED);
		paintDrawIcon(blackBuf, x, y, sunrainy_sun, COLORED);
	}

	else if (strcmp(iconCode, "10d") == 0 || strcmp(iconCode, "10n") == 0) {
		paintDrawIcon(blackBuf, x, y, rain, COLORED);
	}

	else if (strcmp(iconCode, "11d") == 0 || strcmp(iconCode, "11n") == 0) {
		paintDrawIcon(blackBuf, x, y, thunder_cloud, COLORED);
		paintDrawIcon(redBuf, x, y, thunder_bolt, COLORED);
	}

	else if (strcmp(iconCode, "13d") == 0 || strcmp(iconCode, "13n") == 0) {
		paintDrawIcon(blackBuf, x, y, snow, COLORED);
	}

	else if (strcmp(iconCode, "50d") == 0 || strcmp(iconCode, "50n") == 0) {
		paintDrawIcon(blackBuf, x, y, fog, COLORED);
	}

	// Using red snow as a placeholder for missing weather icon
	else {
		paintDrawIcon(redBuf, x, y, snow, COLORED);
	}

}

/*!
 * @brief Callback function to get time from the RTC and draw the display with time and date
 */
void updateData() {

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
		char temperatureBuf[10];

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

		//draw weather description
		PRINTF("%s\r\n", temperature_str);
		int temperature_num = round(atof(temperature_str));
		PRINTF("%d\r\n", temperature_num);
		sprintf(temperatureBuf, "%d F", temperature_num);
		paintDrawString(imgBuffer, 80, 93, temperatureBuf, &Font12, COLORED, 5);

		//draw weather description
		PRINTF("%s\r\n", icon_str);
		PRINTF("%s\r\n", weather_str);
		drawWeather(imgBuffer, colorBuffer, 0, 80, icon_str);

		int weather_str_scale = strlen(weather_str) <= 18 ? 2 : 1;
		paintDrawString(imgBuffer, 4, 150, weather_str, &Font12, COLORED, weather_str_scale);

		//draw time drop shadow
		//paintDrawString(colorBuffer, -1, 22, timeBuf, &Font12, COLORED, digitScale);
		//paintDrawString(colorBuffer, -3, 20, timeBuf, &Font12, UNCOLORED, digitScale);

		//push to display
		einkDisplayFrameFromBufferNonBlocking(imgBuffer, colorBuffer);
		paintClear(imgBuffer, UNCOLORED);
		paintClear(colorBuffer, UNCOLORED);
	}

	// At 30 seconds, attempt to get weather data
	if (timeinfo->tm_sec == 30) {
		dns_send_http_request("api.openweathermap.org");
	}

    //PRINTF("Daylight savings: %d\r\n", timeinfo->tm_isdst);
	PRINTF("%02d:%02d:%02d\r\n", timeinfo->tm_hour, timeinfo->tm_min, timeinfo->tm_sec);
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

/** TCP/DNS weather setup ****************************************************************************************/

    dns_send_http_request(API_URL);

/** Main loop ************************************************************************************************/

    PRINTF("Waiting for display to finish refreshing... ");
	einkWaitUntilIdle();
	einkSetRefreshMode(FAST_REFRESH);
	PRINTF("DONE\r\n");

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
        	updateData();
        }
    }
}
#endif
