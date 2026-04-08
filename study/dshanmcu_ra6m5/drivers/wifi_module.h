#ifndef WIFI_MODULE_H
#define WIFI_MODULE_H

#include "drv_wifi.h"

#define WIFI_MODULE_HTTP_PORT           (80U)
#define WIFI_MODULE_DEFAULT_TIMEOUT_MS  (5000U)
#define WIFI_MODULE_CONNECT_TIMEOUT_MS  (30000U)

typedef enum e_wifi_module_state
{
    WIFI_MODULE_STATE_OFFLINE = 0,
    WIFI_MODULE_STATE_READY,
    WIFI_MODULE_STATE_AP_CONNECTED,
    WIFI_MODULE_STATE_SOCKET_CONNECTED,
    WIFI_MODULE_STATE_ERROR
} wifi_module_state_t;

fsp_err_t wifi_module_init(void);
fsp_err_t wifi_module_join_ap(const char *ssid, const char *password);
fsp_err_t wifi_module_leave_ap(void);
fsp_err_t wifi_module_open_tcp(const char *host, uint16_t port);
fsp_err_t wifi_module_http_request(const char *host,
                                   uint16_t port,
                                   const char *request,
                                   char *response,
                                   uint16_t response_len,
                                   uint32_t timeout_ms);
wifi_module_state_t wifi_module_get_state(void);
bool wifi_module_is_ap_connected(void);

#endif /* WIFI_MODULE_H */
