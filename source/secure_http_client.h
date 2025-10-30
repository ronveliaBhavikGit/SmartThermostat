/******************************************************************************
* File Name: secure_http_client.h
*
* Description: This file contains configuration parameters for the secure HTTP
* client along with the HTML page that the client will host.
*
* Related Document: See README.md
*******************************************************************************
* Copyright 2023-2025, Cypress Semiconductor Corporation (an Infineon company) or
* an affiliate of Cypress Semiconductor Corporation.  All rights reserved.
*
* This software, including source code, documentation and related
* materials ("Software") is owned by Cypress Semiconductor Corporation
* or one of its affiliates ("Cypress") and is protected by and subject to
* worldwide patent protection (United States and foreign),
* United States copyright laws and international treaty provisions.
* Therefore, you may use this Software only as provided in the license
* agreement accompanying the software package from which you
* obtained this Software ("EULA").
* If no EULA applies, Cypress hereby grants you a personal, non-exclusive,
* non-transferable license to copy, modify, and compile the Software
* source code solely for use in connection with Cypress's
* integrated circuit products.  Any reproduction, modification, translation,
* compilation, or representation of this Software except as specified
* above is prohibited without the express written permission of Cypress.
*
* Disclaimer: THIS SOFTWARE IS PROVIDED AS-IS, WITH NO WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING, BUT NOT LIMITED TO, NONINFRINGEMENT, IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE. Cypress
* reserves the right to make changes to the Software without notice. Cypress
* does not assume any liability arising out of the application or use of the
* Software or any product or circuit described in the Software. Cypress does
* not authorize its products for use in any products where a malfunction or
* failure of the Cypress product may reasonably be expected to result in
* significant property damage, injury or death ("High Risk Product"). By
* including Cypress's product in a High Risk Product, the manufacturer
* of such system or application assumes all risk of such use and in doing
* so agrees to indemnify Cypress against all liability.
*******************************************************************************/


/*******************************************************************************
* Include guard
*******************************************************************************/
#ifndef SECURE_HTTP_CLIENT_H_
#define SECURE_HTTP_CLIENT_H_

/* FreeRTOS header file */
#include "FreeRTOS.h"
#include <task.h>
#include "cy_wcm.h"
#include "cybsp.h"
#include "cy_network_mw_core.h"
#include "cyhal_gpio.h"

#define TEST_INFO( x )                        printf x

/* Wi-Fi Credentials: Modify WIFI_SSID and WIFI_PASSWORD to match your Wi-Fi
 * network Credentials.
 */
// #define WIFI_SSID                                "dungeon_master_5g"
// #define WIFI_PASSWORD                            "dungeon_master_103"
#define WIFI_SSID                                "SOL"
#define WIFI_PASSWORD                            "Solutions@123"

/* Security type of the Wi-Fi access point. See 'cy_wcm_security_t' structure
 * in "cy_wcm.h" for more details.
 */
#define WIFI_SECURITY_TYPE                       CY_WCM_SECURITY_WPA2_AES_PSK
#define MAX_WIFI_RETRY_COUNT                     (5U)
#define APP_INFO(x)                do { printf("Info: "); printf x; } while(0);
#define ERR_INFO(x)                do { printf("Error: "); printf x; } while(0);
#define PRINT_AND_ASSERT(result, msg, args...)   \
                                     do                                 \
                                     {                                  \
                                         if (CY_RSLT_SUCCESS != result) \
                                         {                              \
                                             ERR_INFO((msg, ## args));  \
                                             CY_ASSERT(0);              \
                                         }                              \
                                     } while(0);
#define WEATHER_SERVER_HOST                         "api.open-meteo.com"
#define WEATHER_PORT                                (80)
#define WEATHER_PATH                                "/v1/forecast?latitude=12.9719&longitude=77.5937&current_weather=true"
#define GEO_SERVER_HOST                             "ipinfo.io"
#define GEO_PORT                                    (80)
#define GEO_PATH                                    "/json"

#define TRANSPORT_SEND_RECV_TIMEOUT_MS           (5000)
#define HTTP_GET_BUFFER_LENGTH                   (2048 * 4)
#define REQUEST_BODY                             (NULL)
#define HTTP_GET_PATH_AFTER_PUT                  "/"
#define REQUEST_BODY_LENGTH                      ( 0 )

/* Wi-Fi re-connection time interval in milliseconds */
#define WIFI_CONN_RETRY_INTERVAL_MSEC            (1000U)

/*End Range until where the data is expected. Set Set this to -1 if requested
 * range is all bytes from the starting
 */
#define HTTP_REQUEST_RANGE_END                   (-1)

/* Start Range from where the server should return. */
#define HTTP_REQUEST_RANGE_START                 (0U)

/* Number of headers in the header list */
#define NUM_HTTP_HEADERS                         (1U)

/* Length of the request header. */
#define HTTP_REQUEST_HEADER_LEN                  (0U)

/* HTTPS Menu for options to select the method from keyboard  */
#define MENU_HTTPS_METHOD                                                           \
        "\n"                                                                        \
        "Please select the index of HTTP method to be tested from below:\n"         \
        "\n"                                                                        \
        "1. HTTPS_GET_METHOD\n"                                                     \
        "2. HTTPS_POST_METHOD\n"                                                    \
        "3. HTTPS_PUT_METHOD\n"                                                     \
        "4. HTTPS_GET_METHOD_AFTER_PUT\n"                                           \

#define IST                                           (5.5)
/******************************************************
 *                   Enumerations
 ******************************************************/
/*  HTTP Client supported methods  */
typedef enum
{
    HTTPS_GET_METHOD=1,
    HTTPS_POST_METHOD,
    HTTPS_PUT_METHOD,
    HTTPS_GET_METHOD_AFTER_PUT,
} https_menu_t;

/*******************************************************************************
* Function Prototypes
********************************************************************************/
void https_client_task(void *arg);

void sync_temperature(bool flag);
void sync_humidity(bool flag);
void sync_windspeed(bool flag);
void sync_rain(bool flag);
void sync_time(const char* http_headers, float timezone_offset_hours);
void sync_location(bool flag);

void sync_all_data(bool flag);

#endif /* SECURE_HTTP_CLIENT_H_ */


/* [] END OF FILE */