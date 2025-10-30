SET(SOURCES components/ui_comp_alarm_comp.c
    components/ui_comp_clock_dot.c
    components/ui_comp_scrolldots.c
    components/ui_comp_small_label.c
    components/ui_comp.c
    screens/ui_DashBoardScreen.c
    ui.c
    components/ui_comp_hook.c
    ui_helpers.c
    images/ui_img_wifi_png.c
    images/ui_img_temperature_png.c
    images/ui_img_humidity_png.c
    images/ui_img_windspeed_png.c
    images/ui_img_rain_png.c
    images/ui_img_smarthome_png.c)

add_library(ui ${SOURCES})
