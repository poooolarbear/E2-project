#include "drv_wifi.h"

static volatile int g_uart6_tx_complete = 0;
static volatile int g_uart6_rx_complete = 0;
static volatile bool g_wifi_connected = false;
static volatile bool g_wifi_ready = false;

/* 接收缓冲区 */
static uint8_t g_wifi_rx_buffer[WIFI_BUFFER_SIZE];
static uint16_t g_wifi_rx_index = 0;

/***********************************************************************************************************************
 * UART6 Functions (WIFI模块通信)
 **********************************************************************************************************************/

fsp_err_t drv_uart6_init(void)
{
    fsp_err_t err;

    /* 打开串口 */
    err = g_uart6.p_api->open(g_uart6.p_ctrl, g_uart6.p_cfg);
    if(FSP_SUCCESS != err) __BKPT();

    return err;
}

fsp_err_t drv_uart6_send(uint8_t *p_msg, uint16_t len)
{
    fsp_err_t err;

    /* 启动发送 */
    err = g_uart6.p_api->write(g_uart6.p_ctrl, p_msg, len);
    /* 等待发送完毕 */
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

        /* 尝试读取一个字符 */
        err = g_uart6.p_api->read(g_uart6.p_ctrl, &p_msg[received], 1);
        if (FSP_SUCCESS == err)
        {
            received++;
        }
        else if (err == FSP_ERR_TIMEOUT)
        {
            /* 超时是正常的，继续等待 */
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            elapsed_ms++;
        }
        else
        {
            /* 其他错误，继续等待 */
            R_BSP_SoftwareDelay(1, BSP_DELAY_UNITS_MILLISECONDS);
            elapsed_ms++;
        }
    }

    return FSP_SUCCESS;
}

void drv_uart6_wait_for_tx(void)
{
    while (!g_uart6_tx_complete); // 阻塞等待
    g_uart6_tx_complete = 0;
}

void drv_uart6_wait_for_rx(void)
{
    while (!g_uart6_rx_complete); // 阻塞等待
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
            // 接收到单个字符，存入缓冲区
            if (g_wifi_rx_index < WIFI_BUFFER_SIZE - 1)
            {
                g_wifi_rx_buffer[g_wifi_rx_index++] = (uint8_t)p_args->data;
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

/***********************************************************************************************************************
 * WIFI基础驱动函数
 **********************************************************************************************************************/

fsp_err_t drv_wifi_init(void)
{
    fsp_err_t err;

    /* 初始化UART6 */
    err = drv_uart6_init();
    if(FSP_SUCCESS != err) return err;

    /* 复位WIFI模块 */
    err = drv_wifi_reset();
    if(FSP_SUCCESS != err) return err;

    /* 唤醒WIFI模块 */
    err = drv_wifi_wakeup();
    if(FSP_SUCCESS != err) return err;

    /* 清空接收缓冲区 */
    g_wifi_rx_index = 0;
    memset(g_wifi_rx_buffer, 0, WIFI_BUFFER_SIZE);

    g_wifi_ready = true;

    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_reset(void)
{
    /* 使能PFS寄存器访问 */
    R_BSP_PinAccessEnable();

    printf("WIFI reset: pulling pin low...\n");
    /* 拉低复位引脚 */
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_08, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(500, BSP_DELAY_UNITS_MILLISECONDS);

    printf("WIFI reset: pulling pin high...\n");
    /* 拉高复位引脚 */
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_08, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(2000, BSP_DELAY_UNITS_MILLISECONDS);

    /* 禁用PFS寄存器访问 */
    R_BSP_PinAccessDisable();

    printf("WIFI reset completed\n");
    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_wakeup(void)
{
    /* 使能PFS寄存器访问 */
    R_BSP_PinAccessEnable();

    printf("WIFI wakeup: pulling pin high...\n");
    /* 拉高唤醒引脚 */
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_07, BSP_IO_LEVEL_HIGH);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);

    printf("WIFI wakeup: pulling pin low...\n");
    /* 拉低唤醒引脚 */
    R_BSP_PinWrite(BSP_IO_PORT_05_PIN_07, BSP_IO_LEVEL_LOW);
    R_BSP_SoftwareDelay(200, BSP_DELAY_UNITS_MILLISECONDS);

    /* 禁用PFS寄存器访问 */
    R_BSP_PinAccessDisable();

    printf("WIFI wakeup completed\n");
    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_send_at_cmd(const char *cmd, char *response, uint16_t resp_len, uint32_t timeout_ms)
{
    fsp_err_t err;
    uint16_t cmd_len = strlen(cmd);

    /* 清空接收缓冲区 */
    g_wifi_rx_index = 0;
    memset(g_wifi_rx_buffer, 0, WIFI_BUFFER_SIZE);

    /* 发送命令 */
    err = drv_uart6_send((uint8_t *)cmd, cmd_len);
    if(FSP_SUCCESS != err) return err;

    /* 等待响应 */
    int recv_len = drv_wifi_wait_response(response, resp_len, timeout_ms);
    if (recv_len < 0)
    {
        return FSP_ERR_TIMEOUT;
    }

    return FSP_SUCCESS;
}

fsp_err_t drv_wifi_send_data(const uint8_t *data, uint16_t len)
{
    return drv_uart6_send((uint8_t *)data, len);
}

int drv_wifi_wait_response(char *response, uint16_t resp_len, uint32_t timeout_ms)
{
    uint32_t elapsed_ms = 0;

    while (elapsed_ms < timeout_ms)
    {
        /* 检查是否收到OK或ERROR */
        if (strstr((char *)g_wifi_rx_buffer, "OK") != NULL ||
            strstr((char *)g_wifi_rx_buffer, "ERROR") != NULL ||
            strstr((char *)g_wifi_rx_buffer, "FAIL") != NULL ||
            strstr((char *)g_wifi_rx_buffer, "CONNECTED") != NULL ||
            strstr((char *)g_wifi_rx_buffer, "DISCONNECTED") != NULL)
        {
            /* 复制响应 */
            uint16_t copy_len = g_wifi_rx_index;
            if (copy_len > resp_len - 1)
            {
                copy_len = resp_len - 1;
            }
            memcpy(response, g_wifi_rx_buffer, copy_len);
            response[copy_len] = '\0';

            return copy_len;
        }

        R_BSP_SoftwareDelay(10, BSP_DELAY_UNITS_MILLISECONDS);
        elapsed_ms += 10;
    }

    return -1; // 超时
}

/***********************************************************************************************************************
 * WIFI状态检查
 **********************************************************************************************************************/

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
