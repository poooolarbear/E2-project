#ifndef APP_WIFI_H
#define APP_WIFI_H

#include "hal_data.h"
#include "net_client.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

#define WIFI_SSID_MAX_LEN         (32U)
#define WIFI_PASS_MAX_LEN         (64U)
#define HTTP_RESP_MAX_LEN         NET_CLIENT_HTTP_RESPONSE_MAX_LEN
#define TRANSLATE_RESULT_MAX_LEN  NET_CLIENT_TRANSLATE_RESULT_MAX_LEN

typedef enum
{
    WIFI_STATE_IDLE = 0,
    WIFI_STATE_CONNECTING,
    WIFI_STATE_CONNECTED,
    WIFI_STATE_DISCONNECTED,
    WIFI_STATE_ERROR
} wifi_state_t;

fsp_err_t app_wifi_init(void);
fsp_err_t app_wifi_connect(const char *ssid, const char *password);
fsp_err_t app_wifi_disconnect(void);
wifi_state_t app_wifi_get_state(void);
const char * app_wifi_get_state_string(void);
fsp_err_t app_wifi_http_get(const char *host, const char *path, char *response, uint16_t resp_len);
fsp_err_t app_wifi_http_post(const char *host, const char *path, const char *data, char *response, uint16_t resp_len);
fsp_err_t app_wifi_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);
fsp_err_t app_wifi_translate_google(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);
fsp_err_t app_ai_asr_stub(uint32_t average_level, char *result, uint16_t result_len);
fsp_err_t app_ai_translate_pipeline(const char *source_text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);
void app_wifi_test(void);
void app_wifi_test_at(void);
void app_wifi_test_http(void);
void app_wifi_translate_test(void);
fsp_err_t app_wifi_connect_api(const char *ssid, const char *password);
void app_wifi_send_message(wifi_state_t state, const char *message);

#endif /* APP_WIFI_H */
