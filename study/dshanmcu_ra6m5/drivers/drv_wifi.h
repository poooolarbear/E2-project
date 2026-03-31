#ifndef DRV_WIFI_H
#define DRV_WIFI_H

/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "hal_data.h"
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
#define WIFI_BUFFER_SIZE        512
#define WIFI_CMD_TIMEOUT        5000
#define WIFI_RESP_TIMEOUT       10000
#define WIFI_CONNECT_TIMEOUT    30000

/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Exported global functions (to be accessed by other files)
 **********************************************************************************************************************/

/* UART6 Functions (WIFI模块通信) */
fsp_err_t drv_uart6_init(void);
fsp_err_t drv_uart6_send(uint8_t *p_msg, uint16_t len);
fsp_err_t drv_uart6_receive(uint8_t *p_msg, uint16_t len, uint32_t timeout_ms);
void drv_uart6_wait_for_tx(void);
void drv_uart6_wait_for_rx(void);
void uart6_callback(uart_callback_args_t * p_args);

/* WIFI基础驱动函数 */
fsp_err_t drv_wifi_init(void);
fsp_err_t drv_wifi_reset(void);
fsp_err_t drv_wifi_wakeup(void);
fsp_err_t drv_wifi_send_at_cmd(const char *cmd, char *response, uint16_t resp_len, uint32_t timeout_ms);
fsp_err_t drv_wifi_send_data(const uint8_t *data, uint16_t len);
int drv_wifi_wait_response(char *response, uint16_t resp_len, uint32_t timeout_ms);

/* WIFI状态检查 */
bool drv_wifi_is_connected(void);
bool drv_wifi_is_ready(void);

#endif /* DRV_WIFI_H */
