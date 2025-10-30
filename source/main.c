/******************************************************************************
* File Name: main.c
*
* Description:
*   This is the main application file which initializes the BSP and starts
*   the RTOS scheduler. It starts a task that connects to the Wi-Fi Access
*   Point, starts the mDNS (multicast DNS) and then starts the HTTP client
*   in secure mode. All the HTTP security keys are configured in the file
*   secure_http_client.h using openSSL utility. See README.md to understand
*   how the security keys are generated.
*
* Related Document: See README.md
*
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
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "cy_retarget_io.h"
#include "secure_http_client.h"
#include "tft_task.h"
#include "FreeRTOS.h"
#include "task.h"

/* Include serial flash library and QSPI memory configurations only for the
 * kits that require the Wi-Fi firmware to be loaded in external QSPI NOR flash.
 */
#if defined(CY_ENABLE_XIP_PROGRAM)
#include "cy_serial_flash_qspi.h"
#include "cycfg_qspi_memslot.h"
#endif

/******************************************************************************
* Macros
******************************************************************************/
/* RTOS related macros. */
#define HTTPS_CLIENT_TASK_STACK_SIZE        (10U * 1024U)
#define HTTPS_CLIENT_TASK_PRIORITY          (configMAX_PRIORITIES - 3)

#define TFT_TASK_STACK_SIZE        			(20U * 1024U)
#define TFT_TASK_PRIORITY          			(configMAX_PRIORITIES - 1)
/*******************************************************************************
* Global Variables
********************************************************************************/
/* HTTPS client task handle. */
TaskHandle_t https_client_task_handle;

/*******************************************************************************
 * Function Name: main
 *******************************************************************************
 * Summary:
 *  Entry function for the application.
 *  This function initializes the BSP, UART port for debugging, and starts the RTOS scheduler.
 *
 * Parameters:
 *  void
 *
 * Return:
 *  int: Should never return.
 *
 *******************************************************************************/
int main(void)
{
    cy_rslt_t result;

    /* Initialize the Board Support Package (BSP) */
    result = cybsp_init();

    /* Board init failed. Stop program execution */
    if (result != CY_RSLT_SUCCESS)
    {
        CY_ASSERT(0);
    }

    /* Enable global interrupts */
    __enable_irq();

    /* Initialize retarget-io to use the debug UART port */
    cy_retarget_io_init(CYBSP_DEBUG_UART_TX, CYBSP_DEBUG_UART_RX, CY_RETARGET_IO_BAUDRATE);

    /* Init QSPI and enable XIP to get the Wi-Fi firmware from the QSPI NOR flash */
    #if defined(CY_ENABLE_XIP_PROGRAM)
        const uint32_t bus_frequency = 50000000lu;

        cy_serial_flash_qspi_init(smifMemConfigs[0], CYBSP_QSPI_D0, CYBSP_QSPI_D1,
                                      CYBSP_QSPI_D2, CYBSP_QSPI_D3, NC, NC, NC, NC,
                                      CYBSP_QSPI_SCK, CYBSP_QSPI_SS, bus_frequency);

        cy_serial_flash_qspi_enable_xip(true);
    #endif

    /* \x1b[2J\x1b[;H - ANSI ESC sequence to clear screen */
    APP_INFO(("\x1b[2J\x1b[;H"));
    APP_INFO(("===================================\n"));
    APP_INFO(("HTTPS Client\n"));
    APP_INFO(("===================================\n\n"));

    /* Starts the HTTPS client in secure mode. */
	xTaskCreate(tft_task, "tftTask", TFT_TASK_STACK_SIZE, NULL,
                TFT_TASK_PRIORITY,  NULL);
					
    xTaskCreate(https_client_task, "HTTPS Client", HTTPS_CLIENT_TASK_STACK_SIZE, NULL,
                HTTPS_CLIENT_TASK_PRIORITY, &https_client_task_handle);

    /* Start the FreeRTOS scheduler */
    vTaskStartScheduler();

    /* Should never get here */
    CY_ASSERT(0);
}
/* [] END OF FILE */
