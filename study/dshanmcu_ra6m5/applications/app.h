#ifndef APP_TEST_H
#define APP_TEST_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "hal_data.h"
#include "drv_uart.h"
#include <stdio.h>

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/
typedef enum
{
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_ERROR
} wifi_state_t;

/***********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 **********************************************************************************************************************/

void app_uart_test(void);

/* 陶晶驰串口屏通信函数 */
fsp_err_t app_tjc_display_init(void);
fsp_err_t app_tjc_display_set_text(const char *p_control, const char *p_text);
fsp_err_t app_tjc_display_set_text_int(const char *p_control, int value);
fsp_err_t app_tjc_display_set_value(const char *p_control, int value);
fsp_err_t app_tjc_display_set_picture(const char *p_control, int pic_id);
fsp_err_t app_tjc_display_get_value(const char *p_control);
fsp_err_t app_tjc_display_clear(void);
void app_tjc_display_test(void);

/* WIFI模块函数 */
fsp_err_t app_wifi_init(void);
fsp_err_t app_wifi_connect(const char *ssid, const char *password);
fsp_err_t app_wifi_disconnect(void);
void app_wifi_test(void);
void app_wifi_test_at(void);
void app_wifi_test_http(void);
void app_wifi_translate_test(void);
fsp_err_t app_wifi_connect_api(const char *ssid, const char *password);
void app_wifi_send_message(wifi_state_t state, const char *message);
fsp_err_t app_wifi_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);
fsp_err_t app_ai_asr_stub(uint32_t average_level, char *result, uint16_t result_len);
fsp_err_t app_ai_translate_pipeline(const char *source_text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);

#endif /*APP_TEST_H*/
