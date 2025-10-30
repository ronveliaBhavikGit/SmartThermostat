/******************************************************************************
*
* File Name: tft_tasklvgl_support.c
*
* Description: This file contains functions for initialising the display driver
* and function for flushing the color buffer in LVGL.
*
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

#include "lv_conf.h"
#include "lvgl.h"

#include "mtb_st7789v.h"
#include "string.h"
#include "lvgl_support.h"

/*******************************************************************************
* Macros
*******************************************************************************/
#define DISP_HOR_RES 320
#define DISP_VER_RES 240

/*******************************************************************************
* Global Variables
*******************************************************************************/
static lv_disp_drv_t disp_drv;

static lv_color_t disp_buf1[DISP_HOR_RES * 10];
static lv_color_t disp_buf2[DISP_HOR_RES * 10];

/*******************************************************************************
* Function Prototypes
*******************************************************************************/
static void flush_cb(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p);
void lv_port_disp_init();

/*******************************************************************************
* Function Name: void lv_port_disp_init(void)
********************************************************************************
*
* Summary: This function is responsible for initializing the display driver. 
*
* Parameters:
*  None
*
* Return:
*  None
*
*
*******************************************************************************/
void lv_port_disp_init()
{

    static lv_disp_draw_buf_t buf;
    memset(disp_buf1, 0, sizeof(disp_buf1));
    memset(disp_buf2, 0, sizeof(disp_buf2));

    lv_disp_draw_buf_init(&buf, disp_buf1, disp_buf2, DISP_HOR_RES * 10);

    lv_disp_drv_init(&disp_drv);

    disp_drv.draw_buf = &buf;
    disp_drv.flush_cb = flush_cb;
    disp_drv.hor_res = DISP_HOR_RES;
    disp_drv.ver_res = DISP_VER_RES;
    lv_disp_drv_register(&disp_drv);
}



/*******************************************************************************
* Function Name: void lv_port_disp_init(void)
********************************************************************************
*
* Summary: This function is responsible for flushing the color buffer in LVGL.
*
* Parameters:
*    x1      left coordinate of the rectangle
*    x2      right coordinate of the rectangle     
*    y1      top coordinate of the rectangle
*    y2      bottom coordinate of the rectangle
*    x2      right coordinate of the rectangle     
*    color_p pointer to an array of colors
*
* Return:
*  None
*
*
*******************************************************************************/

static void flush_cb(lv_disp_drv_t * drv, const lv_area_t * area, lv_color_t * color_p)
{
    /*Return if the area is out the screen*/

    if(area->x2 < 0) return;
    if(area->y2 < 0) return;
    if(area->x1 > DISP_HOR_RES - 1) return;
    if(area->y1 > DISP_VER_RES - 1) return;

    mtb_st7789v_write_command(0x2a);
    /* Start Column offset - Lower byte. */
    mtb_st7789v_write_data(area->x1 >> 8);  
    /* Start Column offset - Higher Byte. */
    mtb_st7789v_write_data(area->x1 & 0xFF); 
    mtb_st7789v_write_data(area->x2 >> 8);
    mtb_st7789v_write_data(area->x2 & 0xFF);

    mtb_st7789v_write_command(0x2b);
    /* Start Column offset - Lower byte */
    mtb_st7789v_write_data(area->y1 >> 8);  
    /* Start Column offset - Higher Byte */
    mtb_st7789v_write_data(area->y1 & 0xFF); 
    mtb_st7789v_write_data(area->y2 >> 8);
    mtb_st7789v_write_data(area->y2 & 0xFF);

    mtb_st7789v_write_command(0x2c);

    /* setAddrWindow(area->x1, area->y1, area->x2, area->y2); */
    for(int x=area->x1;x<=area->x2;x++)
    {
        for(int y=area->y1;y<=area->y2;y++)
        {
            mtb_st7789v_write_data(color_p->full>>8);
            mtb_st7789v_write_data(color_p->full & 0xff);
            color_p++;
        }
    }

    lv_disp_flush_ready(drv);
}
