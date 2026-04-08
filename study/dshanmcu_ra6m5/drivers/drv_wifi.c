#include "drv_wifi.h"

static volatile int  g_uart6_tx_complete = 0;
static volatile int  g_uart6_rx_complete = 0;
static volatile bool g_wifi_connected = false;
static volatile bool g_wifi_ready = false;

static uint8_t  g_wifi_rx_buffer[WIFI_BUFFER_SIZE];
static uint16_t g_wifi_rx_index = 0;

static void drv_wifi_copy_response(char *response, uint16_t resp_len)
{
    uint16_t copy_len;

    if ((NULL == response) || (0U == resp_len))
    {
        return;
    }

    copy_len = g_wifi_rx_index;
    if (copy_len >= resp_len)
    {
        copy_len = (uint16_t) (resp_len - 1U);
    }

    memcpy(response, g_wifi_rx_buffer, copy_len);
    response[copy_len] = '\0';
}

fsp_err_t drv_uart6_init(void)
{
    fsp_err_t err;

    err = g_uart6.p_api->open(g_uart6.p_ctrl, g_uart6.p_cfg);
    if (FSP_SUCCESS != err)
    {
        __BKPT();
    }

    return err;
}

fsp_err_t drv_uart6_send(uint8_t *p_msg, uint16_t len)
{
    fsp_err_t err;

    err = g_uart6.p_api->write(g_uart6.p_ctrl, p_msg, len);
    drv_uart6_wait_for_tx();
    return err;
}

fsp_err_t drv_uart6_receive(uint8_t *p_msg, uint16_t len, uint32_t timeout_ms)
{
    fsp_err_t err;
    uint32_t elapsed_ms = 0;
    uint16_t received = 0;

    while (received < len)
    {
        if (elapsed_ms > timeout_ms)
        {
            return FSP_ERR_TIMEOUT;
        }

        err = g_uart6.p_api->read(g_uart6.p_ctrl, &p_msg[received], 1);
        if (FSP_SUCCESS == err)
        {
            received++;
        }
        else
        {
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            elapsed_ms++;
        }
    }

    return FSP_SUCCESS;
}

void drv_uart6_wait_for_tx(void)
{
    while (!g_uart6_tx_complete)
    {
    }
    g_uart6_tx_complete = 0;
}

void drv_uart6_wait_for_rx(void)
{
    while (!g_uart6_rx_complete)
    {
    }
    g_uart6_rx_complete = 0;
}

void uart6_callback(uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE:
        {
            g_uart6_tx_complete = 1;
            break;
        }

        case UART_EVENT_RX_COMPLETE:
        {
            g_uart6_rx_complete = 1;
            break;
        }

        case UART_EVENT_RX_CHAR:
        {
            if (g_wifi_rx_index < (WIFI_BUFFER_SIZE - 1U))
            {
                g_wifi_rx_buffer[g_wifi_rx_index++] = (uint8_t) p_args->data;
                g_wifi_rx_buffer[g_wifi_rx_index] = '\0';
            }
            break;
        }

        default:
        {
            break;
        }
    }
}

fsp_err_t drv_wifi_init(void)
{
    fsp_err_t err;

    err = drv_uart6_init();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = drv_wifi_reset();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = drv_wifi_wakeup();
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    drv_wifi_clear_rx_buffer();
    g_wifi_ready = true;
    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_reset(void)
{
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_08, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_08, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(2000, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_PinAccessDisable();
    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_wakeup(void)
{
    R_BSP_PinAccessEnable();
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_07, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_07, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);
    R_BSP_PinAccessDisable();
    return FSP_SUCCESS;
}

void drv_wifi_clear_rx_buffer(void)
{
    g_wifi_rx_index = 0;
    memset(g_wifi_rx_buffer, 0, sizeof(g_wifi_rx_buffer));
}

fsp_err_t drv_wifi_send_at_cmd(const char *cmd, char *response, uint16_t resp_len, uint32_t timeout_ms)
{
    return drv_wifi_exec_at_transaction(cmd, "OK", "ERROR", response, resp_len, timeout_ms);
}

fsp_err_t drv_wifi_exec_at_transaction(const char *cmd,
                                       const char *expect_ok,
                                       const char *expect_fail,
                                       char *response,
                                       uint16_t resp_len,
                                       uint32_t timeout_ms)
{
    fsp_err_t err;
    int recv_len;

    if ((NULL == cmd) || (NULL == expect_ok))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    drv_wifi_clear_rx_buffer();

    err = drv_uart6_send((uint8_t *) cmd, (uint16_t) strlen(cmd));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    recv_len = drv_wifi_wait_response_ex(expect_ok, expect_fail, response, resp_len, timeout_ms);
    if (-2 == recv_len)
    {
        return FSP_ERR_ABORTED;
    }

    if (recv_len < 0)
    {
        return FSP_ERR_TIMEOUT;
    }

    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_send_data(const uint8_t *data, uint16_t len)
{
    return drv_uart6_send((uint8_t *) data, len);
}

int drv_wifi_wait_response(char *response, uint16_t resp_len, uint32_t timeout_ms)
{
    return drv_wifi_wait_response_ex("OK", "ERROR", response, resp_len, timeout_ms);
}

int drv_wifi_wait_response_ex(const char *expect_ok,
                              const char *expect_fail,
                              char *response,
                              uint16_t resp_len,
                              uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        if ((NULL != expect_fail) && (NULL != strstr((char *) g_wifi_rx_buffer, expect_fail)))
        {
            drv_wifi_copy_response(response, resp_len);
            return -2;
        }

        if ((NULL != expect_ok) && (NULL != strstr((char *) g_wifi_rx_buffer, expect_ok)))
        {
            drv_wifi_copy_response(response, resp_len);
            return (int) strlen(response);
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        elapsed_ms += 10;
    }

    drv_wifi_copy_response(response, resp_len);
    return -1;
}

bool drv_wifi_is_connected(void)
{
    return g_wifi_connected;
}

bool drv_wifi_is_ready(void)
{
    return g_wifi_ready;
}

void drv_wifi_set_connected(bool connected)
{
    g_wifi_connected = connected;
}
