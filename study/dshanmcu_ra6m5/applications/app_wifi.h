#ifndef APP_WIFI_H
#define APP_WIFI_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "hal_data.h"
#include "drv_wifi.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define WIFI_SSID_MAX_LEN       32
#define WIFI_PASS_MAX_LEN       64
#define HTTP_RESP_MAX_LEN       1024
#define TRANSLATE_RESULT_MAX_LEN 256

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

/* WIFI应用层初始化 */
fsp_err_t app_wifi_init(void);

/* WIFI连接管理 */
fsp_err_t app_wifi_connect(const char *ssid, const char *password);
fsp_err_t app_wifi_disconnect(void);
wifi_state_t app_wifi_get_state(void);
const char* app_wifi_get_state_string(void);

/* HTTP通信 */
fsp_err_t app_wifi_http_get(const char *host, const char *path, char *response, uint16_t resp_len);
fsp_err_t app_wifi_http_post(const char *host, const char *path, const char *data, char *response, uint16_t resp_len);

/* 翻译API */
fsp_err_t app_wifi_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);
fsp_err_t app_wifi_translate_google(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);

/* 测试函数 */
void app_wifi_test(void);
void app_wifi_test_at(void);
void app_wifi_test_http(void);

#endif /* APP_WIFI_H */
