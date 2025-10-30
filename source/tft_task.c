/******************************************************************************
*
* File Name: tft_task.c
*
* Description: This file contains task and functions related to the tft-task
* that demonstrates controlling a tft display using the LVGL Graphics Library.
*
* The project displays a start up screen with text "LVGL Demo Music player" and
* LVGL logo.
*
* The project then displays the Demo music player application.
*
*******************************************************************************
* Copyright 2023, Cypress Semiconductor Corporation (an Infineon company) or
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
 * Header file includes
 ******************************************************************************/
#include "cy_pdl.h"
#include "cyhal.h"
#include "cybsp.h"
#include "mtb_st7789v.h"
#include "tft_task.h"
#include "FreeRTOS.h"
#include "task.h"
#include "lvgl.h"
#include "lvgl_support.h"
#include "demos/widgets/lv_demo_widgets.h"
#include "examples/lv_examples.h"
#include "examples/anim/lv_example_anim.h"
#include "ui.h"
#include "stdio.h"
#include "lv_timer.h"
#include "cy_http_client_api.h"
#include "secure_http_client.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define DELAY_PARAM       (5u)
#define DELAY_300_MS      (300)   /* milliseconds */
#define DELAY_10_MS       (10)    /* milliseconds */

extern cy_http_client_response_t response;

extern bool syncedAll;

extern bool time_synced;
extern struct tm current_time;

time_t sync_epoch_time; // UTC epoch at sync
/*******************************************************************************
 * Data structure and enumeration
 ******************************************************************************/
/* The pins are defined by the st7789v library. If the display is being used
 *  on different hardware the mappings will be different. */
const mtb_st7789v_pins_t tft_pins =
{
    .db08 = CYBSP_J2_2,
    .db09 = CYBSP_J2_4,
    .db10 = CYBSP_J2_6,
    .db11 = CYBSP_J2_10,
    .db12 = CYBSP_J2_12,
    .db13 = CYBSP_D7,
    .db14 = CYBSP_D8,
    .db15 = CYBSP_D9,
    .nrd  = CYBSP_D10,
    .nwr  = CYBSP_D11,
    .dc   = CYBSP_D12,
    .rst  = CYBSP_D13
};

void clock_tick_cb(lv_timer_t *timer)
{
    if (!time_synced) return;

    time_t now = time(NULL);
    int elapsed = (int)difftime(now, sync_epoch_time);

    struct tm t = current_time;
    t.tm_sec += elapsed;
    mktime(&t);  // normalize

    static int last_displayed_day = -1;

    // Detect date rollover (day change)
    if (t.tm_mday != last_displayed_day) 
    {
        last_displayed_day = t.tm_mday;
        current_time = t;
        sync_epoch_time = now;

        // Update date UI locally
        char day_str[3];
        snprintf(day_str, sizeof(day_str), "%02d", t.tm_mday);
        lv_label_set_text(ui_Date, day_str);

        const char* months[] = {"Jan","Feb","Mar","Apr","May","Jun",
                                "Jul","Aug","Sep","Oct","Nov","Dec"};
        lv_label_set_text(ui_Month, months[t.tm_mon]);

        const char* weekdays[] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
        lv_label_set_text(ui_Vaar, weekdays[t.tm_wday]);
    }

    // Update clock display
    char buf[3];
    snprintf(buf, sizeof(buf), "%02d", t.tm_hour);
    lv_label_set_text(ui_HHH, buf);

    snprintf(buf, sizeof(buf), "%02d", t.tm_min);
    lv_label_set_text(ui_MMM, buf);
}

/*******************************************************************************
* Function Name: void tft_task(void *arg)
********************************************************************************
*
* Summary: tft_task is a handler for LVGL. It is a task function that handles 
*          LVGL-related tasks and operations. The LVGL music player demo gets
*          called inside this function after all initialization are done.
*
* Parameters:
*  arg: task argument
*
* Return:
*  None
*
*******************************************************************************/
void tft_task(void *arg)
{
    cy_rslt_t result;

    /* Initialize graphics */
    result = graphics_init();
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    
    lv_timer_create(clock_tick_cb, DELAY_300_MS, NULL); // 1-second timer for clock
    
    /* Initialize LVGL demo */
    ui_init();

    /* Main loop */
    for (;;)
    {
        lv_task_handler();               // LVGL task processing
        lv_tick_inc(DELAY_PARAM);        // increment LVGL ticks

        vTaskDelay(pdMS_TO_TICKS(DELAY_PARAM)); // FreeRTOS delay
        sync_all_data(syncedAll);        // sync your HTTP/weather data
    }
}

/*******************************************************************************
* Function Name: cy_rslt_t graphics_init(void)
********************************************************************************
*
* Summary: This function performs all graphics related initializations. 
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
cy_rslt_t  graphics_init()
{
    cy_rslt_t result;

    /* Initialize the display controller */
    result = mtb_st7789v_init8(&tft_pins);
    CY_ASSERT(result == CY_RSLT_SUCCESS);
    
    /* Perform initialization specific to the ST7789V display controller. */
    st7789v_init();

    /* Initialize LVGL and set up the essential components required for LVGL. */
    lv_init();

    /*Initialize display driver. */
    lv_port_disp_init();

    return result;

}

/*******************************************************************************
* Function Name: void st7789v_init(void)
********************************************************************************
*
* Summary: This performs initialization specific to the ST7789V display controller
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void st7789v_init(void)
{
    mtb_st7789v_write_command(0x11);    /* exit SLEEP mode*/

    cyhal_system_delay_ms(DELAY_300_MS);

    mtb_st7789v_write_command(0x36);
    mtb_st7789v_write_data(0xA0);      /* MADCTL: memory data access control*/
    mtb_st7789v_write_command(0x3A);
    mtb_st7789v_write_data(0x65);      /* COLMOD: Interface Pixel format*/
    mtb_st7789v_write_command(0xB2);
    mtb_st7789v_write_data(0x0C);
    mtb_st7789v_write_data(0x0C);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x33);
    mtb_st7789v_write_data(0x33); /*PORCTRK: Porch setting*/
    mtb_st7789v_write_command(0xB7);
    mtb_st7789v_write_data(0x35); /*GCTRL: Gate Control*/
    mtb_st7789v_write_command(0xBB);
    mtb_st7789v_write_data(0x2B); /*VCOMS: VCOM setting*/
    mtb_st7789v_write_command(0xC0);
    mtb_st7789v_write_data(0x2C); /*LCMCTRL: LCM Control*/
    mtb_st7789v_write_command(0xC2);
    mtb_st7789v_write_data(0x01);
    mtb_st7789v_write_data(0xFF); /*VDVVRHEN: VDV and VRH Command Enable*/
    mtb_st7789v_write_command(0xC3);
    mtb_st7789v_write_data(0x11); /*VRHS: VRH Set*/
    mtb_st7789v_write_command(0xC4);
    mtb_st7789v_write_data(0x20); /*VDVS: VDV Set*/

    mtb_st7789v_write_command(0xC6);
    mtb_st7789v_write_data(0x0F); /*FRCTRL2: Frame Rate control in normal mode*/
    mtb_st7789v_write_command(0xD0);
    mtb_st7789v_write_data(0xA4);
    mtb_st7789v_write_data(0xA1); /*PWCTRL1: Power Control 1*/
    mtb_st7789v_write_command(0xE0);
    mtb_st7789v_write_data(0xD0);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x05);
    mtb_st7789v_write_data(0x0E);
    mtb_st7789v_write_data(0x15);
    mtb_st7789v_write_data(0x0D);
    mtb_st7789v_write_data(0x37);
    mtb_st7789v_write_data(0x43);
    mtb_st7789v_write_data(0x47);
    mtb_st7789v_write_data(0x09);
    mtb_st7789v_write_data(0x15);
    mtb_st7789v_write_data(0x12);
    mtb_st7789v_write_data(0x16);
    mtb_st7789v_write_data(0x19); /*PVGAMCTRL: Positive Voltage Gamma control*/
    mtb_st7789v_write_command(0xE1);
    mtb_st7789v_write_data(0xD0);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x05);
    mtb_st7789v_write_data(0x0D);
    mtb_st7789v_write_data(0x0C);
    mtb_st7789v_write_data(0x06);
    mtb_st7789v_write_data(0x2D);
    mtb_st7789v_write_data(0x44);
    mtb_st7789v_write_data(0x40);
    mtb_st7789v_write_data(0x0E);
    mtb_st7789v_write_data(0x1C);
    mtb_st7789v_write_data(0x18);
    mtb_st7789v_write_data(0x16);
    mtb_st7789v_write_data(0x19); /*NVGAMCTRL: Negative Voltage Gamma control*/
    mtb_st7789v_write_command(0x2A);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x01);
    mtb_st7789v_write_data(0x3F); /*X address set*/
    mtb_st7789v_write_command(0x2B);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0x00);
    mtb_st7789v_write_data(0xEF); /*Y address set*/

    cyhal_system_delay_ms(DELAY_10_MS);
    mtb_st7789v_write_command(0x29); /*Enable Display*/

}

/* END OF FILE */
