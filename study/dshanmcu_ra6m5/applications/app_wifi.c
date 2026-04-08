#include "app_wifi.h"
#include "app_ai_config.h"
#include "drv_uart.h"

static wifi_state_t g_wifi_state = WIFI_STATE_IDLE;
static char g_wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
static char g_wifi_pass[WIFI_PASS_MAX_LEN] = {0};

fsp_err_t app_wifi_init(void)
{
    fsp_err_t err = net_client_init();

    if (FSP_SUCCESS != err)
    {
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }

    g_wifi_state = WIFI_STATE_IDLE;
    return FSP_SUCCESS;
}

fsp_err_t app_wifi_connect(const char *ssid, const char *password)
{
    fsp_err_t err;

    if ((NULL == ssid) || (NULL == password))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    strncpy(g_wifi_ssid, ssid, WIFI_SSID_MAX_LEN - 1U);
    strncpy(g_wifi_pass, password, WIFI_PASS_MAX_LEN - 1U);
    g_wifi_state = WIFI_STATE_CONNECTING;

    err = net_client_connect_wifi(ssid, password);
    if (FSP_SUCCESS != err)
    {
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }

    g_wifi_state = WIFI_STATE_CONNECTED;
    return FSP_SUCCESS;
}

fsp_err_t app_wifi_disconnect(void)
{
    fsp_err_t err = net_client_disconnect_wifi();

    if (FSP_SUCCESS != err)
    {
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }

    g_wifi_state = WIFI_STATE_DISCONNECTED;
    return FSP_SUCCESS;
}

wifi_state_t app_wifi_get_state(void)
{
    return g_wifi_state;
}

const char * app_wifi_get_state_string(void)
{
    switch (g_wifi_state)
    {
        case WIFI_STATE_IDLE:
            return "IDLE";
        case WIFI_STATE_CONNECTING:
            return "CONNECTING";
        case WIFI_STATE_CONNECTED:
            return "CONNECTED";
        case WIFI_STATE_DISCONNECTED:
            return "DISCONNECTED";
        case WIFI_STATE_ERROR:
            return "ERROR";
        default:
            return "UNKNOWN";
    }
}

fsp_err_t app_wifi_http_get(const char *host, const char *path, char *response, uint16_t resp_len)
{
    return net_client_http_get(host, path, response, resp_len);
}

fsp_err_t app_wifi_http_post(const char *host, const char *path, const char *data, char *response, uint16_t resp_len)
{
    return net_client_http_post(host, path, data, response, resp_len);
}

fsp_err_t app_wifi_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len)
{
    return net_client_translate_baidu(text, from_lang, to_lang, result, result_len);
}

fsp_err_t app_wifi_translate_google(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len)
{
    FSP_PARAMETER_NOT_USED(text);
    FSP_PARAMETER_NOT_USED(from_lang);
    FSP_PARAMETER_NOT_USED(to_lang);
    FSP_PARAMETER_NOT_USED(result);
    FSP_PARAMETER_NOT_USED(result_len);
    return FSP_ERR_UNSUPPORTED;
}

fsp_err_t app_ai_asr_stub(uint32_t average_level, char *result, uint16_t result_len)
{
    if ((NULL == result) || (result_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if (average_level > 1200U)
    {
        snprintf(result, result_len, "please translate this sentence");
    }
    else if (average_level > 700U)
    {
        snprintf(result, result_len, "hello this is my voice translator");
    }
    else
    {
        snprintf(result, result_len, "hello");
    }

    return FSP_SUCCESS;
}

fsp_err_t app_ai_translate_pipeline(const char *source_text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len)
{
    if ((NULL == source_text) || (NULL == from_lang) || (NULL == to_lang) || (NULL == result) || (result_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    return net_client_translate_baidu(source_text, from_lang, to_lang, result, result_len);
}

void app_wifi_test(void)
{
    char response[64];

    if (FSP_SUCCESS == app_wifi_init())
    {
        (void) drv_wifi_send_at_cmd("AT\r\n", response, sizeof(response), 2000U);
    }
}

void app_wifi_test_at(void)
{
    char response[128];

    if (FSP_SUCCESS == app_wifi_init())
    {
        (void) drv_wifi_send_at_cmd("AT\r\n", response, sizeof(response), 2000U);
        printf("AT: %s\n", response);
    }
}

void app_wifi_test_http(void)
{
    char response[HTTP_RESP_MAX_LEN];

    if (FSP_SUCCESS == app_wifi_http_get("www.example.com", "/", response, sizeof(response)))
    {
        printf("%s\n", response);
    }
}

fsp_err_t app_wifi_connect_api(const char *ssid, const char *password)
{
    fsp_err_t err = app_wifi_init();

    if (FSP_SUCCESS != err)
    {
        return err;
    }

    return app_wifi_connect(ssid, password);
}

void app_wifi_send_message(wifi_state_t state, const char *message)
{
    char cmd_buf[128];
    const char *state_text = "UNKNOWN";

    switch (state)
    {
        case WIFI_STATE_IDLE:
            state_text = "WIFI: Idle";
            break;
        case WIFI_STATE_CONNECTING:
            state_text = "WIFI: Connecting";
            break;
        case WIFI_STATE_CONNECTED:
            state_text = "WIFI: Connected";
            break;
        case WIFI_STATE_DISCONNECTED:
            state_text = "WIFI: Disconnected";
            break;
        case WIFI_STATE_ERROR:
            state_text = "WIFI: Error";
            break;
        default:
            break;
    }

    snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"%s\"", state_text);
    uart2_printf(cmd_buf);
    uart2_putchar(0xFF);
    uart2_putchar(0xFF);
    uart2_putchar(0xFF);

    if (NULL != message)
    {
        snprintf(cmd_buf, sizeof(cmd_buf), "g1.txt=\"%s\"", message);
        uart2_printf(cmd_buf);
        uart2_putchar(0xFF);
        uart2_putchar(0xFF);
        uart2_putchar(0xFF);
    }
}

void app_wifi_translate_test(void)
{
    fsp_err_t err;
    char translate_result[TRANSLATE_RESULT_MAX_LEN];

    err = app_wifi_connect_api(APP_AI_WIFI_SSID, APP_AI_WIFI_PASSWORD);
    if (FSP_SUCCESS != err)
    {
        app_wifi_send_message(WIFI_STATE_ERROR, "Connection failed");
        return;
    }

    err = app_wifi_translate_baidu("Hello, how are you?", "en", "zh", translate_result, sizeof(translate_result));
    if (FSP_SUCCESS == err)
    {
        app_wifi_send_message(WIFI_STATE_CONNECTED, translate_result);
    }
    else
    {
        app_wifi_send_message(WIFI_STATE_ERROR, "Translation failed");
    }

    (void) app_wifi_disconnect();
}
