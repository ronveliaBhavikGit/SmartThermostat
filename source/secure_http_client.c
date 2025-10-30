/******************************************************************************
* File Name: secure_http_client.c
*
* Description: This file contains the necessary functions to start the HTTPS
* client and send GET, POST, and PUT request to the HTTPS client.
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

/* Header file includes */
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"

/* FreeRTOS header file */
#include <FreeRTOS.h>
#include <task.h>

/* Cypress Secure Sockets header file */
#include "cy_secure_sockets.h"
#include "cy_tls.h"

/* Wi-Fi connection manager header files */
#include "cy_wcm.h"
#include "cy_wcm_error.h"

/* Standard C header file */
#include <string.h>

/* HTTPS client task header file. */
#include "secure_http_client.h"
#include "cy_http_client_api.h"
#include "secure_keys.h"
#include "ui.h"

#include "lwip/ip_addr.h"

#include "cy_json_parser.h"
/*******************************************************************************
 * Global Variables
 ********************************************************************************/

#define LAST_INDEX                                   (1U)
#define MEMSET_VAL                                   (0U)
#define INITIAL_VALUE                                (0U)
#define HTTPS_CLIENT_TASK_DELAY_MS                  (1000U * 60U) /* 60 seconds */

static cy_http_client_method_t http_client_method;

/* Holds the IP address obtained using Wi-Fi Connection Manager (WCM). */
static cy_wcm_ip_address_t ip_addr;

/* Secure HTTP client instance. */
static cy_http_client_t https_client;

/* Holds the security configuration such as client certificate,
 * client key, and rootCA.
 */
cy_http_client_request_header_t request;
cy_http_client_header_t header;
cy_http_client_response_t response;

/* Secure HTTP server information. */
cy_awsport_server_info_t server_info;

/*Buffer to store get response*/
uint8_t http_get_buffer[HTTP_GET_BUFFER_LENGTH];

/*Holds the HTTP header fields */
cy_http_client_header_t http_header[1];

/*Holds the fields for response header and body*/
cy_http_client_response_t http_response;

char date_header[64] = {0};

struct tm current_time;  // Stores synced local time
bool time_synced = false; // Set after first sync

static char latitude[16];   // For lat extracted from "loc"
static char longitude[16];  // For lon extracted from "loc"
static char timezone[32];   // For timezone
static char city[16];       // For City
static char timedata[32];
static char temperature[16];
static char hummidity[16];
static char windspeed[16];
static char weathercode[16];

bool syncedAll = true;

static bool temperatureSynced = false;
static bool humiditySynced = false;
static bool windSynced = false;
static bool rainSynced = false;
static bool locationSynced = false;
/******************************************************************************
* Function Prototypes
*******************************************************************************/
static void http_request(void);
static void fetch_https_client_method(void);
static void disconnect_callback_handler(cy_http_client_t handle,
                                 cy_http_client_disconn_type_t type, void *args);
static cy_rslt_t send_http_request(cy_http_client_t handle,
                            cy_http_client_method_t method,const char * pPath);
static cy_rslt_t configure_https_client(const char *host_name, uint16_t port);
static cy_rslt_t wifi_connect(void);

void parse_json_payload(const char* payload);
void parse_json_weather_payload(const char* payload, uint32_t payload_len);
/********************************************************************************
 * Function Name: wifi_connect
 ********************************************************************************
 * Summary:
 *  The device associates to the Access Point with given SSID, PASSWORD, and SECURITY
 *  type. It retries for MAX_WIFI_RETRY_COUNT times if the Wi-Fi connection fails.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t: Returns CY_RSLT_SUCCESS if the Wi-Fi connection is successfully
 *  established, a WCM error code otherwise.
 *
 *******************************************************************************/
cy_rslt_t wifi_connect(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    uint32_t retry_count = 0;
    cy_wcm_connect_params_t connect_param = {0};
    cy_wcm_config_t wcm_config = {.interface = CY_WCM_INTERFACE_TYPE_STA};

    result = cy_wcm_init(&wcm_config);

    if (CY_RSLT_SUCCESS == result)
    {
        APP_INFO(("Wi-Fi initialization is successful\n"));
        memcpy(&connect_param.ap_credentials.SSID, WIFI_SSID, sizeof(WIFI_SSID));
        memcpy(&connect_param.ap_credentials.password, WIFI_PASSWORD, sizeof(WIFI_PASSWORD));
        connect_param.ap_credentials.security = WIFI_SECURITY_TYPE;
        APP_INFO(("Join to AP: %s\n", connect_param.ap_credentials.SSID));

        /*
         * Connect to Access Point. It validates the connection parameters
         * and then establishes connection to AP.
         */
        for (retry_count = 0; retry_count < MAX_WIFI_RETRY_COUNT; retry_count++)
        {
             result = cy_wcm_connect_ap(&connect_param, &ip_addr);

             if (CY_RSLT_SUCCESS == result)
             {
                 APP_INFO(("Successfully joined Wi-Fi network %s\n", connect_param.ap_credentials.SSID));

                 if (CY_WCM_IP_VER_V4 == ip_addr.version)
                 {
                     APP_INFO(("Assigned IP address: %s\n", ip4addr_ntoa((const ip4_addr_t *)&ip_addr.ip.v4)));
                 }
                 else if (CY_WCM_IP_VER_V6 == ip_addr.version)
                 {
                     APP_INFO(("Assigned IP address: %s\n", ip6addr_ntoa((const ip6_addr_t *)&ip_addr.ip.v6)));
                 }
                 lv_obj_clear_flag(ui_WiFiIcon, LV_OBJ_FLAG_HIDDEN);

                 break;
             }

            lv_obj_add_flag(ui_WiFiIcon, LV_OBJ_FLAG_HIDDEN);
            ERR_INFO(("Failed to join Wi-Fi network. Retrying...\n"));
        }
    }

    return result;
}

/*******************************************************************************
* Function Name: disconnect_callback
********************************************************************************
* Summary:
*  Callback function for http disconnect.
*******************************************************************************/
static void disconnect_callback_handler(cy_http_client_t handle,
        cy_http_client_disconn_type_t type, void *args)
{
    printf("\nApplication Disconnect callback triggered for handle = "
            "%p type=%d\n", handle, type);

    lv_obj_add_flag(ui_WiFiIcon, LV_OBJ_FLAG_HIDDEN);
}

/*******************************************************************************
 * Function Name: send_http_request
 *******************************************************************************
 * Summary:
 *  The function handles an http send operation.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t: Returns CY_RSLT_SUCCESS if the secure HTTP client is configured
 *  successfully, otherwise, it returns CY_RSLT_TYPE_ERROR.
 *
 *******************************************************************************/
static cy_rslt_t send_http_request( cy_http_client_t handle,
        cy_http_client_method_t method, const char * pPath)
{
    /* Return value of all methods from the HTTP Client library API. */
    cy_rslt_t http_status = CY_RSLT_SUCCESS;

    /* Initialize the response object. The same buffer used for storing
     * request headers is reused here. */
    request.buffer = http_get_buffer;
    request.buffer_len = HTTP_GET_BUFFER_LENGTH;

    request.headers_len = HTTP_REQUEST_HEADER_LEN;
    request.method = method;
    request.range_end = HTTP_REQUEST_RANGE_END;
    request.range_start = HTTP_REQUEST_RANGE_START;
    request.resource_path = pPath;
    header.field = "Connection";
    header.field_len = strlen("Connection");
    header.value = "keep-alive";
    header.value_len = strlen("keep-alive");

    http_status = cy_http_client_write_header(handle, &request, &header, NUM_HTTP_HEADERS);
    if( http_status != CY_RSLT_SUCCESS )
    {
        printf("\nWrite Header ----------- Fail \n");
        return http_status;
    }
    else
    {
        printf( "\n Sending Request Headers:\n%.*s\n",( int ) request.headers_len, ( char * ) request.buffer);
    }

    http_status = cy_http_client_send(handle, &request, (uint8_t *)REQUEST_BODY, REQUEST_BODY_LENGTH, &response);
    if(CY_RSLT_SUCCESS != http_status)
    {
        printf("\nFailed to send HTTP method=%d\n Error=%ld\r\n",request.method,(unsigned long)http_status);
        http_request();
    }
    else
    {
        if ( CY_HTTP_CLIENT_METHOD_HEAD != method )
        {
            TEST_INFO(( "Received HTTP response from %.*s%.*s...\n"
                   "Response Headers:\n %.*s\n"
                   "Response Status :\n %u \n"
                   "Response Body   :\n %.*s\n",
                   ( int ) sizeof(GEO_SERVER_HOST)-LAST_INDEX, GEO_SERVER_HOST,
                   ( int ) sizeof(request.resource_path) -LAST_INDEX, request.resource_path,
                   ( int ) response.headers_len, response.header,
                   response.status_code,
                   ( int ) response.body_len, response.body ) );

        }
        printf("\n buffer_len:[%d] headers_len:[%d] header_count:[%d] body_len:[%d] content_len:[%d]\n",
                 response.buffer_len, response.headers_len, response.header_count, response.body_len, response.content_len);
    }

    /* Disconnect the HTTP client from the server. */
    http_status = cy_http_client_disconnect(handle);
    (CY_RSLT_SUCCESS != http_status)?printf("Failed to diconnect handler\r\n"):
            printf("Successfully disconnected handler\r\n");
    

    /* Delete the instance of the HTTP client. */
    http_status = cy_http_client_delete(handle);
    (CY_RSLT_SUCCESS != http_status)?printf("Failed to deleted handler\r\n"):
            printf("Successfully deleted handler\r\n");

    return http_status;
}
/*******************************************************************************
 * Function Name: configure_https_client
 *******************************************************************************
 * Summary:
 *  Configures the security parameters such as client certificate, private key,
 *  and the root CA certificate to start the HTTP client in secure mode.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  cy_rslt_t: Returns CY_RSLT_SUCCESS if the secure HTTP client is configured
 *  successfully, otherwise, it returns CY_RSLT_TYPE_ERROR.
 *
 *******************************************************************************/
static cy_rslt_t configure_https_client(const char *host_name, uint16_t port)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;
    cy_http_disconnect_callback_t http_cb;

    /* Clear existing server_info structure. */
    (void)memset(&server_info, MEMSET_VAL, sizeof(server_info));

    /* Set the server hostname and port dynamically. */
    server_info.host_name = host_name;
    server_info.port = port;

    /* Initialize the HTTP Client Library. */
    result = cy_http_client_init();
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to initialize http client.\n"));
        return result;
    }

    /* Set the disconnect callback handler (if needed). */
    http_cb = disconnect_callback_handler;

    /* Create an instance of the HTTP client for the given server information. */
    result = cy_http_client_create(NULL, &server_info, http_cb, NULL, &https_client);
    if (CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to create http client.\n"));
    }

    return result;
}

/*******************************************************************************
 * Function Name: https_client_task
 *******************************************************************************
 * Summary:
 *  Starts the HTTP client in secure mode. This example application is using a
 *  self-signed certificate which means there is no third-party certificate issuing
 *  authority involved in the authentication of the client. It is the user's
 *  responsibility to supply the necessary security configurations such as client's
 *  certificate, private key of the client, and RootCA of the client to start the
 *  HTTP client in secure mode.
 *
 * Parameters:
 *  arg - Unused.
 *
 * Return:
 *  None.
 *
 *******************************************************************************/
void https_client_task(void *arg)
{
    cy_rslt_t result = CY_RSLT_TYPE_ERROR;
    CY_UNUSED_PARAMETER(arg);

    /* Connects to the Wi-Fi Access Point. */
    result = wifi_connect();
    PRINT_AND_ASSERT(result, "Wi-Fi connection failed.\n");

    while(true)
    {
        /* Fetch the HTTPS client method. */
        fetch_https_client_method();

        /* Delay before the next iteration. */
        vTaskDelay(pdMS_TO_TICKS(HTTPS_CLIENT_TASK_DELAY_MS));
    }
}

/*******************************************************************************
 * Function Name: fetch_https_client_method
 *******************************************************************************
 * Summary:
 *  The function handles an http methods.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  None.
 *
 *******************************************************************************/
static void fetch_https_client_method(void)
{
    cy_rslt_t result = CY_RSLT_TYPE_ERROR;

    /* Step 1: Fetch geolocation data */
    printf("\nConfiguring client for Geolocation API (ipinfo.io)...\n");
    result = configure_https_client(GEO_SERVER_HOST, GEO_PORT);
    if (CY_RSLT_SUCCESS != result) {
        ERR_INFO(("Failed to configure HTTP client for geolocation API.\n"));
        return;
    }

    /* Connect the HTTP client to the geolocation server */
    result = cy_http_client_connect(https_client, TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS);
    if (CY_RSLT_SUCCESS != result) {
        ERR_INFO(("Failed to connect to the geolocation server.\n"));
        return;
    }

    printf("\nFetching geolocation data from ipinfo.io...\n");
    http_client_method = CY_HTTP_CLIENT_METHOD_GET;

    result = send_http_request(https_client, http_client_method, GEO_PATH);
    if (CY_RSLT_SUCCESS == result) {
        printf("\nSuccessfully received geolocation response. Parsing JSON...\n");
        // Parse the received JSON
        parse_json_payload((const char *)response.body);
    } else {
        ERR_INFO(("Failed to fetch geolocation data.\n"));
        return;
    }

    /* Step 2: Construct the weather API path dynamically */
    char weather_path[256] = {0};
    snprintf(weather_path, sizeof(weather_path),
             "/v1/forecast?latitude=%s&longitude=%s&models=ukmo_seamless&current=temperature_2m,relative_humidity_2m,wind_speed_10m,weather_code", latitude, longitude);

    /* Step 3: Fetch weather data (remaining steps are unchanged) */
    printf("\nFetching weather data from Open-Meteo...\n");
    result = configure_https_client(WEATHER_SERVER_HOST, WEATHER_PORT);
    if (CY_RSLT_SUCCESS != result) {
        ERR_INFO(("Failed to configure HTTP client for weather API.\n"));
        return;
    }

    result = cy_http_client_connect(https_client, TRANSPORT_SEND_RECV_TIMEOUT_MS, TRANSPORT_SEND_RECV_TIMEOUT_MS);
    if (CY_RSLT_SUCCESS != result) {
        ERR_INFO(("Failed to connect to weather server.\n"));
        return;
    }

    printf("\nSending request to Weather API...\n");
    result = send_http_request(https_client, http_client_method, weather_path);
    if (CY_RSLT_SUCCESS != result) {
        ERR_INFO(("Failed to fetch weather data.\n"));
    }
    else{
        parse_json_weather_payload((const char *)response.body, response.body_len);
    }

    syncedAll = false;
}

/*******************************************************************************
 * Function Name: http_request
 *******************************************************************************
 * Summary:
 *  The function handles an http request operation.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  None.
 *
 *******************************************************************************/
static void http_request(void)
{
    cy_rslt_t result = CY_RSLT_SUCCESS;

   /* Send the HTTP request and body to the server, and receive the response
    * from it.
    */
    result = send_http_request(https_client, http_client_method, GEO_PATH);

    if(CY_RSLT_SUCCESS != result)
    {
        ERR_INFO(("Failed to send the http request.\n"));
    }
    else
    {
        printf("\r\nSuccessfully sent GET request to http server\r\n");
        printf("\r\nThe http status code is :: %d\r\n",
                 http_response.status_code);
    }
}

/*******************************************************************************
 * JSON
 ********************************************************************************/
/* Callback for JSON parsing */
cy_rslt_t json_callback(cy_JSON_object_t* json_object, void* arg) 
{
    if (json_object == NULL) {
        printf("Error: NULL JSON object in callback!\n");
        return CY_RSLT_JSON_GENERIC_ERROR;
    }

    // Debug: Log every key-value pair received
    // printf("Debug: Callback invoked. Key: %.*s, Value: %.*s, Type: %d\n",
    //        (int)json_object->object_string_length, json_object->object_string,
    //        (int)json_object->value_length, json_object->value,
    //        json_object->value_type);

    // Handle "loc" key (for latitude and longitude)
    if (strncmp(json_object->object_string, "loc", json_object->object_string_length) == 0 &&
        strlen("loc") == json_object->object_string_length) {
        // printf("Debug: Found key 'loc'. Parsing latitude and longitude...\n");

        // Parse "loc" value, e.g., "12.9719,77.5937"
        char temp[64] = {0};
        snprintf(temp, sizeof(temp), "%.*s", json_object->value_length, json_object->value);

        char* separator = strchr(temp, ',');
        if (separator != NULL) {
            *separator = '\0';  // Null-terminate latitude
            snprintf(latitude, sizeof(latitude), "%s", temp);
            snprintf(longitude, sizeof(longitude), "%s", separator + 1);  // Parse longitude
            printf("Extracted -> Latitude: %s, Longitude: %s\n", latitude, longitude);
        } else {
            printf("Error: Could not split 'loc' into latitude and longitude!\n");
        }
    }
    // Handle "timezone" key
    else if (strncmp(json_object->object_string, "timezone", json_object->object_string_length) == 0 &&
             strlen("timezone") == json_object->object_string_length) {
        // printf("Debug: Found key 'timezone'. Extracting value...\n");
        snprintf(timezone, sizeof(timezone), "%.*s",
                 (int)json_object->value_length, json_object->value);
        printf("Extracted -> Timezone: %s\n", timezone);
    }
    else if (strncmp(json_object->object_string, "city", json_object->object_string_length) == 0 &&
             strlen("city") == json_object->object_string_length) {
        // printf("Debug: Found key 'city'. Extracting value...\n");
        snprintf(city, sizeof(city), "%.*s",
                 (int)json_object->value_length, json_object->value);
        printf("Extracted -> city: %s\n", city);
    }
    else {
        // printf("Debug: Unhandled key: %.*s\n",
        //        (int)json_object->object_string_length, json_object->object_string);
    }

    return CY_RSLT_SUCCESS;
}

static cy_rslt_t json_weather_cb(cy_JSON_object_t *object, void *arg)
{
    if(object == NULL || object->object_string == NULL)
        return CY_RSLT_SUCCESS;

    if (strncmp(object->object_string, "time", object->object_string_length) == 0 &&
        object->value_type == JSON_STRING_TYPE)
    {
        snprintf(timedata, sizeof(timedata), "%.*s",
                 (int)object->value_length, object->value);
    }
    else if (strncmp(object->object_string, "temperature_2m", object->object_string_length) == 0)
    {
        snprintf(temperature, sizeof(temperature), "%.*s",
                 (int)object->value_length, object->value);
    }
    else if (strncmp(object->object_string, "relative_humidity_2m", object->object_string_length) == 0)
    {
        snprintf(hummidity, sizeof(hummidity), "%.*s",
                 (int)object->value_length, object->value);
    }
    else if (strncmp(object->object_string, "wind_speed_10m", object->object_string_length) == 0)
    {
        snprintf(windspeed, sizeof(windspeed), "%.*s",
                 (int)object->value_length, object->value);
    }
    else if (strncmp(object->object_string, "weather_code", object->object_string_length) == 0)
    {
        snprintf(weathercode, sizeof(weathercode), "%.*s",
                 (int)object->value_length, object->value);
    }

    return CY_RSLT_SUCCESS;
}

void parse_json_payload(const char* payload) {
    if (payload == NULL || strlen(payload) == 0) {
        printf("Error: Payload is empty or NULL!\n");
        return;
    }

    // printf("Debug: Raw payload: %s\n", payload);

    // Validate JSON structure
    char* json_end = strchr(payload, '}');
    if (json_end == NULL) {
        printf("Error: Payload does not contain a valid JSON object!\n");
        return;
    }

    size_t json_length = json_end - payload + 1;
    char valid_json[json_length + 1];
    strncpy(valid_json, payload, json_length);
    valid_json[json_length] = '\0';

    // printf("Debug: Valid JSON payload: %s\n", valid_json);

    // Debug: Add callback registration validation
    cy_rslt_t reg_result = cy_JSON_parser_register_callback(json_callback, NULL);
    if (reg_result != CY_RSLT_SUCCESS) {
        printf("Error: Failed to register JSON callback! Error: %ld\n", (long)reg_result);
        return;
    }

    // printf("Debug: Callback registered. Starting parsing...\n");

    // Call the parser and capture result
    cy_rslt_t result = cy_JSON_parser(valid_json, json_length);
    if (result != CY_RSLT_SUCCESS) {
        printf("Error: Failed to parse JSON payload! Error: %ld\n", (long)result);
        return;
    }

    // printf("Debug: Parsing complete. Extracted values:\n");
    printf("Latitude: %s\n", latitude);
    printf("Longitude: %s\n", longitude);
    printf("Timezone: %s\n", timezone);
}

void parse_json_weather_payload(const char* payload, uint32_t payload_len)
{
    if (payload == NULL || payload_len == 0) {
        printf("Error: Payload is NULL or empty!\n");
        return;
    }

    // Copy payload into mutable buffer
    char *json_buf = malloc(payload_len + 1);
    if (!json_buf) {
        printf("Error: malloc failed!\n");
        return;
    }
    memcpy(json_buf, payload, payload_len);
    json_buf[payload_len] = '\0'; // null terminate

    // Register callback (do once globally ideally)
    cy_rslt_t reg_result = cy_JSON_parser_register_callback(json_weather_cb, NULL);
    if (reg_result != CY_RSLT_SUCCESS) {
        printf("Error: Failed to register JSON callback! Error: %ld\n", (long)reg_result);
        free(json_buf);
        return;
    }

    // Call parser
    cy_rslt_t result = cy_JSON_parser(json_buf, payload_len);

    if (result == CY_RSLT_SUCCESS)
    {
        printf("Time(GMT): %s\n", timedata);
        printf("Temperature: %s °C\n", temperature);
        printf("Humidity: %s %%\n", hummidity);
        printf("Wind Speed: %s km/h\n", windspeed);
        printf("Weather Code: %s\n", weathercode);
    }
    else
    {
        printf("JSON parsing failed! Error: 0x%08lX\n", (long)result);
    }

    temperatureSynced = false;
    humiditySynced = false;
    windSynced = false;
    rainSynced = false;

    free(json_buf);
}


/*******************************************************************************
 * Data Sync
 ********************************************************************************/
void sync_temperature(bool flag)
{
    if(!flag)
    {
        lv_label_set_text(ui_Temperature, temperature);
        flag = true;
    }     
}

void sync_humidity(bool flag)
{
    if(!flag)
    {
        lv_label_set_text(ui_Humidity, hummidity);
        flag = true;
    }
} 

void sync_windspeed(bool flag)
{
    if(!flag)
    {
        lv_label_set_text(ui_WindSpeed, windspeed);
        flag = true;
    }
}

void sync_rain(bool flag)
{
    if(!flag)
    {
        int code = atoi(weathercode);  // weathercode extracted from JSON

        // Codes that indicate precipitation
        bool is_rain = (
                        code == 51 || code == 53 || code == 55 ||   // Drizzle
                        code == 56 || code == 57 ||                 // Freezing drizzle
                        code == 61 || code == 63 || code == 65 ||   // Rain
                        code == 66 || code == 67 ||                 // Freezing rain
                        code == 80 || code == 81 || code == 82 ||   // Rain showers
                        code == 95 || code == 96 || code == 99      // Thunderstorm / hail
                        );

        if(is_rain)
            lv_label_set_text(ui_Rain, "Y");
        else
            lv_label_set_text(ui_Rain, "N");
            
        flag = true;
    }
}

int month_str_to_index(const char* month)
{
    const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                            "Jul","Aug","Sep","Oct","Nov","Dec"};
    for(int i=0;i<12;i++)
    {
        if(strncmp(months[i], month, 3) == 0)
            return i;
    }
    return 0;
}

time_t timegm(struct tm *t)
{
    // Save current timezone
    char *tz = getenv("TZ");
    setenv("TZ", "UTC", 1);
    tzset(); // apply new TZ

    time_t result = mktime(t); // mktime interprets t as local time (now UTC)

    // Restore previous timezone
    if (tz)
        setenv("TZ", tz, 1);
    else
        unsetenv("TZ");
    tzset();

    return result;
}

void sync_time(const char* http_headers, float timezone_offset_hours)
{
    if(time_synced || http_headers == NULL)
        return;

    char exact_time[64] = {0};

    // Extract Date header
    char *date_ptr = strstr(http_headers, "Date:");
    if(date_ptr)
    {
        date_ptr += 5;
        while(*date_ptr == ' ') date_ptr++;
        char *end = strpbrk(date_ptr, "\r\n");
        if(end != NULL && (end - date_ptr) < sizeof(exact_time))
        {
            strncpy(exact_time, date_ptr, end - date_ptr);
            exact_time[end - date_ptr] = '\0';
        }
        else
        {
            strncpy(exact_time, date_ptr, sizeof(exact_time)-1);
            exact_time[sizeof(exact_time)-1] = '\0';
        }
    }

    if(strlen(exact_time) == 0)
        return;

    // Parse GMT time
    int day, month, year, hour, min, sec;
    char month_str[4];
    if(sscanf(exact_time, "%*3s, %d %3s %d %d:%d:%d",
              &day, month_str, &year, &hour, &min, &sec) == 6)
    {
        // Fill struct tm
        current_time.tm_year = year - 1900;
        current_time.tm_mon  = month_str_to_index(month_str);
        current_time.tm_mday = day;
        current_time.tm_hour = hour;
        current_time.tm_min  = min;
        current_time.tm_sec  = sec;

        // Convert GMT to local by offset
        time_t gmt_time = timegm(&current_time);
        gmt_time += (int)(timezone_offset_hours * 3600);
        gmtime_r(&gmt_time, &current_time);  // local time

        // Update LVGL labels once
        char hour_str[3], min_str[3], day_str[3];
        snprintf(hour_str, sizeof(hour_str), "%02d", current_time.tm_hour);
        snprintf(min_str, sizeof(min_str), "%02d", current_time.tm_min);
        snprintf(day_str, sizeof(day_str), "%02d", current_time.tm_mday);

        lv_label_set_text(ui_HHH, hour_str);
        lv_label_set_text(ui_MMM, min_str); 
        lv_label_set_text(ui_Date, day_str);

        const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                "Jul","Aug","Sep","Oct","Nov","Dec"};
        lv_label_set_text(ui_Month, months[current_time.tm_mon]);

        const char* weekdays[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        lv_label_set_text(ui_Vaar, weekdays[current_time.tm_wday]);

        time_synced = true;
    }
}

void sync_location(bool flag)
{
    if(!flag)
    {
        char buffer[40];
        sprintf(buffer, "%s", city);
    
        lv_label_set_text(ui_Location, buffer);

        flag = true;
    }
}

void sync_all_data(bool flag)
{
    if(!flag)
    {
        sync_temperature(temperatureSynced);
        sync_humidity(humiditySynced);
        sync_windspeed(windSynced);
        sync_rain(rainSynced);
        sync_time((const char *)response.header, IST);
        sync_location(locationSynced);

        flag = true;
    }
}