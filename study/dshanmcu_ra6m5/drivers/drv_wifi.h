#ifndef DRV_WIFI_H
#define DRV_WIFI_H

#include "hal_data.h"
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#define WIFI_BUFFER_SIZE        (512U)
#define WIFI_CMD_TIMEOUT        (5000U)
#define WIFI_RESP_TIMEOUT       (10000U)
#define WIFI_CONNECT_TIMEOUT    (30000U)

fsp_err_t drv_uart6_init(void);
fsp_err_t drv_uart6_send(uint8_t *p_msg, uint16_t len);
fsp_err_t drv_uart6_receive(uint8_t *p_msg, uint16_t len, uint32_t timeout_ms);
void drv_uart6_wait_for_tx(void);
void drv_uart6_wait_for_rx(void);
void uart6_callback(uart_callback_args_t * p_args);

fsp_err_t drv_wifi_init(void);
fsp_err_t drv_wifi_reset(void);
fsp_err_t drv_wifi_wakeup(void);
void drv_wifi_clear_rx_buffer(void);
fsp_err_t drv_wifi_send_at_cmd(const char *cmd, char *response, uint16_t resp_len, uint32_t timeout_ms);
fsp_err_t drv_wifi_exec_at_transaction(const char *cmd,
                                       const char *expect_ok,
                                       const char *expect_fail,
                                       char *response,
                                       uint16_t resp_len,
                                       uint32_t timeout_ms);
fsp_err_t drv_wifi_send_data(const uint8_t *data, uint16_t len);
int drv_wifi_wait_response(char *response, uint16_t resp_len, uint32_t timeout_ms);
int drv_wifi_wait_response_ex(const char *expect_ok,
                              const char *expect_fail,
                              char *response,
                              uint16_t resp_len,
                              uint32_t timeout_ms);

bool drv_wifi_is_connected(void);
bool drv_wifi_is_ready(void);
void drv_wifi_set_connected(bool connected);

#endif /* DRV_WIFI_H */
