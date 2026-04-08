#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include "wifi_module.h"

#define NET_CLIENT_HTTP_RESPONSE_MAX_LEN    (1024U)
#define NET_CLIENT_TRANSLATE_RESULT_MAX_LEN (256U)

fsp_err_t net_client_init(void);
fsp_err_t net_client_connect_wifi(const char *ssid, const char *password);
fsp_err_t net_client_disconnect_wifi(void);
fsp_err_t net_client_http_get(const char *host, const char *path, char *response, uint16_t response_len);
fsp_err_t net_client_http_post(const char *host, const char *path, const char *data, char *response, uint16_t response_len);
fsp_err_t net_client_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len);

#endif /* NET_CLIENT_H */
