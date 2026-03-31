/***********************************************************************************************************************
 * Includes
 **********************************************************************************************************************/
#include "app.h"
#include <stdarg.h>

/**********************************************************************************************************************
 * Macro definitions
 **********************************************************************************************************************/
// 陶晶驰串口屏指令结束符
#define TJC_END_CMD         "\xff\xff\xff"

// 控件类型
#define TJC_CONTROL_TEXT    "txt"
#define TJC_CONTROL_VAL     "val"
#define TJC_CONTROL_PIC     "pic"


/**********************************************************************************************************************
 * Typedef definitions
 **********************************************************************************************************************/


/***********************************************************************************************************************
 * Private function prototypes
 **********************************************************************************************************************/
static fsp_err_t tjc_send_command(uint8_t *p_cmd);
static fsp_err_t tjc_build_command(char *p_buf, size_t buf_size, const char *p_format, ...);
static void tjc_send_end_cmd(void);

/***********************************************************************************************************************
 * Private global variables
 **********************************************************************************************************************/

/***********************************************************************************************************************
 * Functions
 **********************************************************************************************************************/

/*******************************************************************************************************************//**
 * @brief 初始化陶晶驰串口屏通信
 *
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_init(void)
{
    fsp_err_t err;
    
    /* 初始化UART2 */
    err = drv_uart2_init();
    if(FSP_SUCCESS != err) __BKPT();
    
    /* 发送初始化指令，清屏 */
    uint8_t clear_cmd[] = "cls" TJC_END_CMD;
    err = tjc_send_command(clear_cmd);
    
    return err;
}

/*******************************************************************************************************************//**
 * @brief 设置文本控件内容
 *
 * @param p_control 控件名称（如"t0"）
 * @param p_text 文本内容
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_set_text(const char *p_control, const char *p_text)
{
    fsp_err_t err;
    char cmd_buf[128];
    
    /* 构建指令：t0.txt="Hello"FFFFFF */
    err = tjc_build_command(cmd_buf, sizeof(cmd_buf), "%s.%s=\"%s\"" TJC_END_CMD, 
                           p_control, TJC_CONTROL_TEXT, p_text);
    if(FSP_SUCCESS != err) return err;
    
    /* 发送指令 */
    return tjc_send_command((uint8_t *)cmd_buf);
}

/*******************************************************************************************************************//**
 * @brief 设置数值控件值
 *
 * @param p_control 控件名称（如"n0"）
 * @param value 数值
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_set_value(const char *p_control, int value)
{
    fsp_err_t err;
    char cmd_buf[128];
    
    /* 构建指令：n0.val=123FFFFFF */
    err = tjc_build_command(cmd_buf, sizeof(cmd_buf), "%s.%s=%d" TJC_END_CMD, 
                           p_control, TJC_CONTROL_VAL, value);
    if(FSP_SUCCESS != err) return err;
    
    /* 发送指令 */
    return tjc_send_command((uint8_t *)cmd_buf);
}

/*******************************************************************************************************************//**
 * @brief 设置图片控件
 *
 * @param p_control 控件名称（如"p0"）
 * @param pic_id 图片ID
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_set_picture(const char *p_control, int pic_id)
{
    fsp_err_t err;
    char cmd_buf[128];
    
    /* 构建指令：p0.pic=1FFFFFF */
    err = tjc_build_command(cmd_buf, sizeof(cmd_buf), "%s.%s=%d" TJC_END_CMD, 
                           p_control, TJC_CONTROL_PIC, pic_id);
    if(FSP_SUCCESS != err) return err;
    
    /* 发送指令 */
    return tjc_send_command((uint8_t *)cmd_buf);
}

/*******************************************************************************************************************//**
 * @brief 在文本控件中显示数值
 *
 * @param p_control 控件名称（如"t0"）
 * @param value 数值
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_set_text_int(const char *p_control, int value)
{
    fsp_err_t err;
    char cmd_buf[128];
    
    /* 构建指令：t0.txt= "123"FFFFFF */
    err = tjc_build_command(cmd_buf, sizeof(cmd_buf), "%s.%s=\"%d\"" TJC_END_CMD, 
                           p_control, TJC_CONTROL_TEXT, value);
    if(FSP_SUCCESS != err) return err;
    
    /* 发送指令 */
    return tjc_send_command((uint8_t *)cmd_buf);
}

/*******************************************************************************************************************//**
 * @brief 读取控件的值
 *
 * @param p_control 控件名称（如"g0"）
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_get_value(const char *p_control)
{
    fsp_err_t err;
    char cmd_buf[128];
    
    /* 构建指令：get g0.valFFFFFF */
    err = tjc_build_command(cmd_buf, sizeof(cmd_buf), "get %s.val" TJC_END_CMD, p_control);
    if(FSP_SUCCESS != err) return err;
    
    /* 发送指令 */
    return tjc_send_command((uint8_t *)cmd_buf);
}

/*******************************************************************************************************************//**
 * @brief 清屏
 *
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_tjc_display_clear(void)
{
    uint8_t clear_cmd[] = "cls" TJC_END_CMD;
    return tjc_send_command(clear_cmd);
}

/*******************************************************************************************************************//**
 * @brief 测试陶晶驰串口屏通信
 *
 * @return void
 **********************************************************************************************************************/
void app_tjc_display_test(void)
{
    fsp_err_t err;
    int counter = 0;
    
    /* 初始化串口屏 */
    err = app_tjc_display_init();
    if(FSP_SUCCESS != err) __BKPT();
    
    /* 显示欢迎信息 */
    err = app_tjc_display_set_text("t0", "陶晶驰串口屏测试");
    if(FSP_SUCCESS != err) __BKPT();
    
    /* 循环测试 */
    while (1)
    {
        /* 更新计数器（使用新的文本数值显示函数） */
        err = app_tjc_display_set_text_int("t1", counter);
        if(FSP_SUCCESS != err) __BKPT();
        
        /* 更新数值显示 */
        err = app_tjc_display_set_value("n0", counter);
        if(FSP_SUCCESS != err) __BKPT();
        
        /* 延迟1秒 */
        R_BSP_SoftwareDelay(1000, BSP_DELAY_UNITS_MILLISECONDS);
        
        /* 增加计数器 */
        counter++;
        if(counter > 999) counter = 0;
    }
}

/*******************************************************************************************************************//**
 * @brief 发送指令到陶晶驰串口屏
 *
 * @param p_cmd 指令内容
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
static fsp_err_t tjc_send_command(uint8_t *p_cmd)
{
    int written = uart2_printf((const char *) p_cmd);

    if (written <= 0)
    {
        return FSP_ERR_ABORTED;
    }

    if (strstr((const char *) p_cmd, TJC_END_CMD) == NULL)
    {
        tjc_send_end_cmd();
    }

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief 构建陶晶驰指令
 *
 * @param p_buf 缓冲区
 * @param buf_size 缓冲区大小
 * @param p_format 格式字符串
 * @param ... 可变参数
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
static fsp_err_t tjc_build_command(char *p_buf, size_t buf_size, const char *p_format, ...)
{
    va_list args;
    int ret;
    
    va_start(args, p_format);
    ret = vsnprintf(p_buf, buf_size, p_format, args);
    va_end(args);
    
    if(ret < 0 || (size_t)ret >= buf_size)
    {
        return FSP_ERR_INVALID_SIZE;
    }
    
    return FSP_SUCCESS;
}

static void tjc_send_end_cmd(void)
{
    uart2_putchar(0xFF);
    uart2_putchar(0xFF);
    uart2_putchar(0xFF);
}
