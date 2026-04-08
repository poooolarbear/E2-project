#include "wifi_module.h"

#include <stdio.h>
#include <string.h>

static wifi_module_state_t g_wifi_module_state = WIFI_MODULE_STATE_OFFLINE;

static fsp_err_t wifi_module_run_command(const char *cmd,
                                         const char *expect_ok,
                                         const char *expect_fail,
                                         char *response,
                                         uint16_t response_len,
                                         uint32_t timeout_ms)
{
    fsp_err_t err;

    if ((NULL == cmd) || (NULL == expect_ok))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    printf("[wifi_module] cmd=%s", cmd);

    err = drv_wifi_exec_at_transaction(cmd,
                                       expect_ok,
                                       expect_fail,
                                       response,
                                       response_len,
                                       timeout_ms);
    if (FSP_SUCCESS != err)
    {
        printf("[wifi_module] err=%d resp=%s\n", err, (NULL != response) ? response : "");
        g_wifi_module_state = WIFI_MODULE_STATE_ERROR;
    }
    else
    {
        printf("[wifi_module] ok resp=%s\n", (NULL != response) ? response : "");
    }

    return err;
}

fsp_err_t wifi_module_init(void)
{
    fsp_err_t err;
    char response[WIFI_BUFFER_SIZE];

    if ((WIFI_MODULE_STATE_READY == g_wifi_module_state) ||
        (WIFI_MODULE_STATE_AP_CONNECTED == g_wifi_module_state) ||
        (WIFI_MODULE_STATE_SOCKET_CONNECTED == g_wifi_module_state))
    {
        return FSP_SUCCESS;
    }

    err = drv_wifi_init();
    if (FSP_SUCCESS != err)
    {
        printf("[wifi_module] drv_wifi_init err=%d\n", err);
        g_wifi_module_state = WIFI_MODULE_STATE_ERROR;
        return err;
    }

    err = wifi_module_run_command("AT\r\n", "OK", "ERROR", response, sizeof(response), 2000U);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = wifi_module_run_command("AT+WMODE=STA\r\n", "OK", "ERROR", response, sizeof(response), WIFI_MODULE_DEFAULT_TIMEOUT_MS);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    drv_wifi_set_connected(false);
    g_wifi_module_state = WIFI_MODULE_STATE_READY;
    return FSP_SUCCESS;
}

fsp_err_t wifi_module_join_ap(const char *ssid, const char *password)
{
    char cmd[128];
    char response[WIFI_BUFFER_SIZE];
    fsp_err_t err;

    if ((NULL == ssid) || (NULL == password))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if (WIFI_MODULE_STATE_OFFLINE == g_wifi_module_state)
    {
        err = wifi_module_init();
        if (FSP_SUCCESS != err)
        {
            return err;
        }
    }

    snprintf(cmd, sizeof(cmd), "AT+SSID=%s\r\n", ssid);
    err = wifi_module_run_command(cmd, "OK", "ERROR", response, sizeof(response), WIFI_MODULE_DEFAULT_TIMEOUT_MS);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    snprintf(cmd, sizeof(cmd), "AT+KEY=1,0,%s\r\n", password);
    err = wifi_module_run_command(cmd, "OK", "ERROR", response, sizeof(response), WIFI_MODULE_DEFAULT_TIMEOUT_MS);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = wifi_module_run_command("AT+WJOIN\r\n",
                                  "OK",
                                  "FAIL",
                                  response,
                                  sizeof(response),
                                  WIFI_MODULE_CONNECT_TIMEOUT_MS);
    if (FSP_SUCCESS != err)
    {
        if ((NULL != strstr(response, "CONNECTED")) ||
            (NULL != strstr(response, "GOT IP")) ||
            (NULL != strstr(response, "OK")))
        {
            printf("[wifi_module] join accepted by async resp=%s\n", response);
            err = FSP_SUCCESS;
        }
        else
        {
            printf("wifi join response timeout/fail: %s\n", response);
            return err;
        }
    }

    drv_wifi_set_connected(true);
    g_wifi_module_state = WIFI_MODULE_STATE_AP_CONNECTED;
    return FSP_SUCCESS;
}

fsp_err_t wifi_module_leave_ap(void)
{
    char response[WIFI_BUFFER_SIZE];
    fsp_err_t err;

    err = wifi_module_run_command("AT+WQUT\r\n", "OK", "ERROR", response, sizeof(response), WIFI_MODULE_DEFAULT_TIMEOUT_MS);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    drv_wifi_set_connected(false);
    g_wifi_module_state = WIFI_MODULE_STATE_READY;
    return FSP_SUCCESS;
}

fsp_err_t wifi_module_open_tcp(const char *host, uint16_t port)
{
    char cmd[160];
    char response[WIFI_BUFFER_SIZE];
    fsp_err_t err;

    if (NULL == host)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if (!wifi_module_is_ap_connected())
    {
        return FSP_ERR_NOT_OPEN;
    }

    snprintf(cmd, sizeof(cmd), "AT+NETCONN=TCP,%s,%u\r\n", host, (unsigned int) port);
    err = wifi_module_run_command(cmd, "OK", "ERROR", response, sizeof(response), 10000U);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    g_wifi_module_state = WIFI_MODULE_STATE_SOCKET_CONNECTED;
    return FSP_SUCCESS;
}

fsp_err_t wifi_module_http_request(const char *host,
                                   uint16_t port,
                                   const char *request,
                                   char *response,
                                   uint16_t response_len,
                                   uint32_t timeout_ms)
{
    fsp_err_t err;

    if ((NULL == host) || (NULL == request) || (NULL == response) || (response_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    err = wifi_module_open_tcp(host, port);
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = drv_wifi_exec_at_transaction(request,
                                       "HTTP/1.1",
                                       "ERROR",
                                       response,
                                       response_len,
                                       timeout_ms);
    if (FSP_SUCCESS != err)
    {
        g_wifi_module_state = WIFI_MODULE_STATE_ERROR;
        return err;
    }

    g_wifi_module_state = WIFI_MODULE_STATE_AP_CONNECTED;
    return FSP_SUCCESS;
}

wifi_module_state_t wifi_module_get_state(void)
{
    return g_wifi_module_state;
}

bool wifi_module_is_ap_connected(void)
{
    return (WIFI_MODULE_STATE_AP_CONNECTED == g_wifi_module_state) ||
           (WIFI_MODULE_STATE_SOCKET_CONNECTED == g_wifi_module_state);
}
