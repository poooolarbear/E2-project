#include "drv_uart.h"

static volatile int g_uart7_tx_complete = 0;
static volatile int g_uart7_rx_complete = 0;
static volatile int g_uart2_tx_complete = 0;
static volatile int g_uart2_rx_complete = 0;

/* 环形缓冲区 */
static ring_buffer_t g_uart2_ring_buffer;


/***********************************************************************************************************************
 * UART7 Functions
 **********************************************************************************************************************/

fsp_err_t drv_uart7_init(void)
{
    fsp_err_t err;

    /* 打开串口 */
    err = g_uart7.p_api->open(g_uart7.p_ctrl, g_uart7.p_cfg);
    if(FSP_SUCCESS != err) __BKPT();

    return err;
}

fsp_err_t drv_uart7_test(uint8_t *p_msg)
{
    fsp_err_t err;
    uint8_t msg_len = 0;
    char *p_temp_ptr = (char *)p_msg;

    /* 计算长度 */
    msg_len = ((uint8_t)(strlen((char *)p_temp_ptr)));

    /* 启动发送 */
    err = g_uart7.p_api->write(g_uart7.p_ctrl, p_msg, msg_len);
    /* 等待发送完毕 */
    drv_uart7_wait_for_tx();

    return err;
}

void drv_uart7_wait_for_tx(void)
{
    while (!g_uart7_tx_complete); // 阻塞等待
    g_uart7_tx_complete = 0;
}

void drv_uart7_wait_for_rx(void)
{
    while (!g_uart7_rx_complete); // 阻塞等待
    g_uart7_rx_complete = 0;
}

void uart7_callback(uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE:
        {
            g_uart7_tx_complete  = 1;
            break;
        }
        case UART_EVENT_RX_COMPLETE:
        {
            g_uart7_rx_complete = 1;
            break;
        }
        default:
        {
            break;
        }
    }
}

/***********************************************************************************************************************
 * UART2 Functions
 **********************************************************************************************************************/

fsp_err_t drv_uart2_init(void)
{
    fsp_err_t err;

    /* 初始化环形缓冲区 */
    ring_buffer_init(&g_uart2_ring_buffer);

    /* 打开串口 */
    err = g_uart2.p_api->open(g_uart2.p_ctrl, g_uart2.p_cfg);
    if(FSP_SUCCESS != err) __BKPT();

    return err;
}

fsp_err_t drv_uart2_test(uint8_t *p_msg)
{
    fsp_err_t err;
    uint8_t msg_len = 0;
    char *p_temp_ptr = (char *)p_msg;

    /* 计算长度 */
    msg_len = ((uint8_t)(strlen((char *)p_temp_ptr)));

    /* 启动发送 */
    err = g_uart2.p_api->write(g_uart2.p_ctrl, p_msg, msg_len);
    /* 等待发送完毕 */
    drv_uart2_wait_for_tx();

    return err;
}

void drv_uart2_wait_for_tx(void)
{
    while (!g_uart2_tx_complete); // 阻塞等待
    g_uart2_tx_complete = 0;
}

void drv_uart2_wait_for_rx(void)
{
    while (!g_uart2_rx_complete); // 阻塞等待
    g_uart2_rx_complete = 0;
}

void uart2_callback(uart_callback_args_t * p_args)
{
    switch (p_args->event)
    {
        case UART_EVENT_TX_COMPLETE:
        {
            g_uart2_tx_complete  = 1;
            break;
        }
        case UART_EVENT_RX_COMPLETE:
        {
            g_uart2_rx_complete = 1;
            break;
        }
        case UART_EVENT_RX_CHAR:
        {
            // 接收到单个字符，存入环形缓冲区
            ring_buffer_put(&g_uart2_ring_buffer, (uint8_t)p_args->data);
            break;
        }
        default:
        {
            break;
        }
    }
}



/*printf输出重定向到串口 (默认使用UART7)*/
int __io_putchar(int ch)
{
    fsp_err_t err = FSP_SUCCESS;

    err = g_uart7.p_api->write(g_uart7.p_ctrl, (uint8_t*)&ch, 1);

    if(FSP_SUCCESS != err) __BKPT();
    drv_uart7_wait_for_tx();

    return ch;
}

int _write(int fd, char *pBuffer, int size)
{
    ((void)fd);

    for(int i=0;i<size;i++)
    {
        __io_putchar(*pBuffer++);
    }
    return size;
}

/*******************************************************************************************************************//**
 * @brief 串口2字符输出函数
 *
 * @param ch 要输出的字符
 * @return int 输出的字符
 **********************************************************************************************************************/
int uart2_putchar(int ch)
{
    fsp_err_t err = FSP_SUCCESS;

    err = g_uart2.p_api->write(g_uart2.p_ctrl, (uint8_t*)&ch, 1);

    if(FSP_SUCCESS != err) __BKPT();
    drv_uart2_wait_for_tx();

    return ch;
}

/*******************************************************************************************************************//**
 * @brief 串口2字符串输出函数
 *
 * @param p_str 要输出的字符串
 * @return int 输出的字符数
 **********************************************************************************************************************/
int uart2_printf(const char *p_str)
{
    int count = 0;
    
    while(*p_str)
    {
        uart2_putchar(*p_str++);
        count++;
    }
    
    return count;
}

/***********************************************************************************************************************
 * 环形队列函数实现
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief 初始化环形队列
 *
 * @param rb 环形队列指针
 **********************************************************************************************************************/
void ring_buffer_init(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

/*******************************************************************************************************************//**
 * @brief 向环形队列中添加数据
 *
 * @param rb 环形队列指针
 * @param data 要添加的数据
 * @return int 成功返回1，失败返回0
 **********************************************************************************************************************/
int ring_buffer_put(ring_buffer_t *rb, uint8_t data)
{
    if (rb->count >= RING_BUFFER_SIZE)
    {
        return 0; // 队列已满
    }
    
    rb->buffer[rb->tail] = data;
    rb->tail = (rb->tail + 1) % RING_BUFFER_SIZE;
    rb->count++;
    
    return 1;
}

/*******************************************************************************************************************//**
 * @brief 从环形队列中取出数据
 *
 * @param rb 环形队列指针
 * @param data 存储取出数据的指针
 * @return int 成功返回1，失败返回0
 **********************************************************************************************************************/
int ring_buffer_get(ring_buffer_t *rb, uint8_t *data)
{
    if (rb->count == 0)
    {
        return 0; // 队列为空
    }
    
    *data = rb->buffer[rb->head];
    rb->head = (rb->head + 1) % RING_BUFFER_SIZE;
    rb->count--;
    
    return 1;
}

/*******************************************************************************************************************//**
 * @brief 获取环形队列中的数据数量
 *
 * @param rb 环形队列指针
 * @return uint16_t 队列中的数据数量
 **********************************************************************************************************************/
uint16_t ring_buffer_count(ring_buffer_t *rb)
{
    return rb->count;
}

/*******************************************************************************************************************//**
 * @brief 清空环形队列
 *
 * @param rb 环形队列指针
 **********************************************************************************************************************/
void ring_buffer_clear(ring_buffer_t *rb)
{
    rb->head = 0;
    rb->tail = 0;
    rb->count = 0;
}

/***********************************************************************************************************************
 * 串口2数据处理函数
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief 处理串口2接收到的数据
 *
 * @return void
 **********************************************************************************************************************/
void uart2_process_data(void)
{
    static uint8_t frame_buffer[64];
    static uint8_t frame_index = 0;
    static uint8_t state = 0;
    
    uint8_t data;
    
    while (ring_buffer_count(&g_uart2_ring_buffer) > 0)
    {
        // 从环形缓冲区中取出一个字节
        if (ring_buffer_get(&g_uart2_ring_buffer, &data))
        {
            // 处理数据帧
            switch (state)
            {
                case 0: // 等待帧头
                    if (data == 0x5A) // 假设帧头是0x5A
                    {
                        frame_buffer[0] = data;
                        frame_index = 1;
                        state = 1;
                    }
                    break;
                    
                case 1: // 接收数据
                    frame_buffer[frame_index++] = data;
                    
                    // 检查帧尾（假设帧尾是0xFF 0xFF 0xFF）
                    if (frame_index >= 3 && 
                        frame_buffer[frame_index-3] == 0xFF &&
                        frame_buffer[frame_index-2] == 0xFF &&
                        frame_buffer[frame_index-1] == 0xFF)
                    {
                        // 帧接收完成
                        frame_buffer[frame_index] = '\0'; // 添加结束符
                        
                        // 打印接收到的数据到UART7
                        printf("Received from UART2: ");
                        for (int i = 0; i < frame_index; i++)
                        {
                            printf("%02X ", frame_buffer[i]);
                        }
                        printf("\n");
                        
                        // 解析数据（这里可以根据实际协议进行解析）
                        // TODO: 解析数据的代码
                        
                        // 重置状态
                        frame_index = 0;
                        state = 0;
                    }
                    else if (frame_index >= sizeof(frame_buffer))
                    {
                        // 帧过长，重置
                        frame_index = 0;
                        state = 0;
                    }
                    break;
            }
        }
    }
}


