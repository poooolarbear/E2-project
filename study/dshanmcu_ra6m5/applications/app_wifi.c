#include "app_wifi.h"

/* MD5算法实现 */
typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
} MD5_CTX;

static void MD5_Init(MD5_CTX *ctx);
static void MD5_Update(MD5_CTX *ctx, const uint8_t *input, uint32_t inputLen);
static void MD5_Final(uint8_t digest[16], MD5_CTX *ctx);
static void MD5_Transform(uint32_t state[4], const uint8_t block[64]);
static void MD5Encode(uint8_t *output, uint32_t *input, uint32_t len);
static void MD5Decode(uint32_t *output, const uint8_t *input, uint32_t len);

/* 常量定义 */
static uint8_t PADDING[64] = {
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint32_t S[4][4] = {
    {7, 12, 17, 22},
    {5, 9, 14, 20},
    {4, 11, 16, 23},
    {6, 10, 15, 21}
};

static uint32_t K[64] = {
    0xd76aa478, 0xe8c7b756, 0x242070db, 0xc1bdceee,
    0xf57c0faf, 0x4787c62a, 0xa8304613, 0xfd469501,
    0x698098d8, 0x8b44f7af, 0xffff5bb1, 0x895cd7be,
    0x6b901122, 0xfd987193, 0xa679438e, 0x49b40821,
    0xf61e2562, 0xc040b340, 0x265e5a51, 0xe9b6c7aa,
    0xd62f105d, 0x02441453, 0xd8a1e681, 0xe7d3fbc8,
    0x21e1cde6, 0xc33707d6, 0xf4d50d87, 0x455a14ed,
    0xa9e3e905, 0xfcefa3f8, 0x676f02d9, 0x8d2a4c8a,
    0xfffa3942, 0x8771f681, 0x6d9d6122, 0xfde5380c,
    0xa4beea44, 0x4bdecfa9, 0xf6bb4b60, 0xbebfbc70,
    0x289b7ec6, 0xeaa127fa, 0xd4ef3085, 0x04881d05,
    0xd9d4d039, 0xe6db99e5, 0x1fa27cf8, 0xc4ac5665,
    0xf4292244, 0x432aff97, 0xab9423a7, 0xfc93a039,
    0x655b59c3, 0x8f0ccc92, 0xffeff47d, 0x85845dd1,
    0x6fa87e4f, 0xfe2ce6e0, 0xa3014314, 0x4e0811a1,
    0xf7537e82, 0xbd3af235, 0x2ad7d2bb, 0xeb86d391
};

static void MD5_Init(MD5_CTX *ctx)
{
    ctx->count[0] = 0;
    ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

static void MD5_Update(MD5_CTX *ctx, const uint8_t *input, uint32_t inputLen)
{
    uint32_t i, index, partLen;
    index = (uint32_t)((ctx->count[0] >> 3) & 0x3F);
    partLen = 64 - index;
    if (inputLen >= partLen) {
        memcpy(&ctx->buffer[index], input, partLen);
        MD5_Transform(ctx->state, ctx->buffer);
        for (i = partLen; i + 63 < inputLen; i += 64) {
            MD5_Transform(ctx->state, &input[i]);
        }
        index = 0;
    } else {
        i = 0;
    }
    memcpy(&ctx->buffer[index], &input[i], inputLen - i);
    ctx->count[0] += inputLen << 3;
    if (ctx->count[0] < (inputLen << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += inputLen >> 29;
}

static void MD5_Final(uint8_t digest[16], MD5_CTX *ctx)
{
    uint8_t bits[8];
    uint32_t index, padLen;
    MD5Encode(bits, ctx->count, 8);
    index = (uint32_t)((ctx->count[0] >> 3) & 0x3F);
    padLen = (index < 56) ? (56 - index) : (120 - index);
    MD5_Update(ctx, PADDING, padLen);
    MD5_Update(ctx, bits, 8);
    MD5Encode(digest, ctx->state, 16);
}

static void MD5_Transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3], x[16];
    MD5Decode(x, block, 64);
    for (int i = 0; i < 64; i++) {
        uint32_t f, g;
        if (i < 16) {
            f = (b & c) | ((~b) & d);
            g = i;
        } else if (i < 32) {
            f = (d & b) | ((~d) & c);
            g = (5 * i + 1) % 16;
        } else if (i < 48) {
            f = b ^ c ^ d;
            g = (3 * i + 5) % 16;
        } else {
            f = c ^ (b | (~d));
            g = (7 * i) % 16;
        }
        uint32_t temp = d;
        d = c;
        c = b;
        b = b + ((a + f + K[i] + x[g]) << S[i/16][i%4]) | ((a + f + K[i] + x[g]) >> (32 - S[i/16][i%4]));
        a = temp;
    }
    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static void MD5Encode(uint8_t *output, uint32_t *input, uint32_t len)
{
    for (uint32_t i = 0, j = 0; j < len; i++, j += 4) {
        output[j] = (uint8_t)(input[i] & 0xFF);
        output[j+1] = (uint8_t)((input[i] >> 8) & 0xFF);
        output[j+2] = (uint8_t)((input[i] >> 16) & 0xFF);
        output[j+3] = (uint8_t)((input[i] >> 24) & 0xFF);
    }
}

static void MD5Decode(uint32_t *output, const uint8_t *input, uint32_t len)
{
    for (uint32_t i = 0, j = 0; j < len; i++, j += 4) {
        output[i] = ((uint32_t)input[j]) | (((uint32_t)input[j+1]) << 8) | (((uint32_t)input[j+2]) << 16) | (((uint32_t)input[j+3]) << 24);
    }
}

/* 计算MD5并返回十六进制字符串 */
void md5_calculate(const char *data, int len, char *md5_result)
{
    MD5_CTX ctx;
    uint8_t digest[16];
    MD5_Init(&ctx);
    MD5_Update(&ctx, (const uint8_t *)data, len);
    MD5_Final(digest, &ctx);
    for (int i = 0; i < 16; i++) {
        sprintf(&md5_result[i*2], "%02x", digest[i]);
    }
    md5_result[32] = '\0';
}

static wifi_state_t g_wifi_state = WIFI_STATE_IDLE;
static char g_wifi_ssid[WIFI_SSID_MAX_LEN] = {0};
static char g_wifi_pass[WIFI_PASS_MAX_LEN] = {0};

/***********************************************************************************************************************
 * WIFI应用层初始化
 **********************************************************************************************************************/

fsp_err_t app_wifi_init(void)
{
    fsp_err_t err;

    printf("Initializing WIFI module...\n");

    /* 初始化WIFI驱动 */
    err = drv_wifi_init();
    if(FSP_SUCCESS != err)
    {
        printf("WIFI init failed!\n");
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }

    printf("WIFI module initialized successfully!\n");
    g_wifi_state = WIFI_STATE_IDLE;

    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * WIFI连接管理
 **********************************************************************************************************************/

fsp_err_t app_wifi_connect(const char *ssid, const char *password)
{
    fsp_err_t err;
    char response[256];

    if (ssid == NULL || password == NULL)
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    /* 保存SSID和密码 */
    strncpy(g_wifi_ssid, ssid, WIFI_SSID_MAX_LEN - 1);
    strncpy(g_wifi_pass, password, WIFI_PASS_MAX_LEN - 1);

    printf("Connecting to WIFI: %s\n", ssid);
    g_wifi_state = WIFI_STATE_CONNECTING;

    /* 测试AT命令 */
    err = drv_wifi_send_at_cmd("AT\r\n", response, sizeof(response), 2000);
    if(FSP_SUCCESS != err)
    {
        printf("AT test failed!\n");
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }
    printf("AT response: %s\n", response);

    /* 设置WIFI模式为STA */
    err = drv_wifi_send_at_cmd("AT+WMODE=STA\r\n", response, sizeof(response), 5000);
    if(FSP_SUCCESS != err)
    {
        printf("Set STA mode failed!\n");
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }
    printf("STA mode set: %s\n", response);

    /* 连接WIFI - 使用W800模块正确的AT命令格式 (参考参考代码) */
    char cmd[128];
    
    printf("Attempting to connect to WIFI: %s\n", ssid);
    printf("Password: %s\n", password);
    
    /* 步骤1: 设置热点名称 */
    printf("Step 1: Setting SSID...\n");
    snprintf(cmd, sizeof(cmd), "AT+SSID=%s\r\n", ssid);
    err = drv_wifi_send_at_cmd(cmd, response, sizeof(response), 5000);
    if(FSP_SUCCESS != err || strstr(response, "OK") == NULL)
    {
        printf("Set SSID failed, response: %s\n", response);
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }
    printf("Set SSID response: %s\n", response);
    
    /* 步骤2: 设置密码 (格式: 1=ASCII格式, 0=密钥索引, 密码不用双引号包围) */
    printf("Step 2: Setting password...\n");
    snprintf(cmd, sizeof(cmd), "AT+KEY=1,0,%s\r\n", password);
    err = drv_wifi_send_at_cmd(cmd, response, sizeof(response), 5000);
    if(FSP_SUCCESS != err || strstr(response, "OK") == NULL)
    {
        printf("Set password failed, response: %s\n", response);
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }
    printf("Set password response: %s\n", response);
    
    /* 步骤3: 连接热点 (使用AT+WJOIN命令) */
    printf("Step 3: Connecting to WIFI...\n");
    snprintf(cmd, sizeof(cmd), "AT+WJOIN\r\n");
    err = drv_wifi_send_at_cmd(cmd, response, sizeof(response), 30000);
    if(FSP_SUCCESS != err)
    {
        printf("Connect to WIFI failed!\n");
        g_wifi_state = WIFI_STATE_ERROR;
        return err;
    }
    printf("Connect response: %s\n", response);

    /* 检查连接结果 */
    if (strstr(response, "OK") != NULL || strstr(response, "CONNECTED") != NULL)
    {
        printf("WIFI connected successfully!\n");
        g_wifi_state = WIFI_STATE_CONNECTED;
        drv_wifi_set_connected(true);
        return FSP_SUCCESS;
    }
    else
    {
        printf("WIFI connection failed!\n");
        g_wifi_state = WIFI_STATE_ERROR;
        return FSP_ERR_ABORTED;
    }
}

fsp_err_t app_wifi_disconnect(void)
{
    fsp_err_t err;
    char response[256];

    err = drv_wifi_send_at_cmd("AT+WQUT\r\n", response, sizeof(response), 5000);
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    g_wifi_state = WIFI_STATE_DISCONNECTED;
    drv_wifi_set_connected(false);

    printf("WIFI disconnected!\n");
    return FSP_SUCCESS;
}

wifi_state_t app_wifi_get_state(void)
{
    return g_wifi_state;
}

const char* app_wifi_get_state_string(void)
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

/***********************************************************************************************************************
 * HTTP通信
 **********************************************************************************************************************/

fsp_err_t app_wifi_http_get(const char *host, const char *path, char *response, uint16_t resp_len)
{
    fsp_err_t err;
    char cmd[256];
    char temp_resp[512];

    if (!drv_wifi_is_connected())
    {
        printf("WIFI not connected!\n");
        return FSP_ERR_NOT_OPEN;
    }

    /* 建立TCP连接 */
    snprintf(cmd, sizeof(cmd), "AT+NETCONN=TCP,%s,80\r\n", host);
    err = drv_wifi_send_at_cmd(cmd, temp_resp, sizeof(temp_resp), 10000);
    if(FSP_SUCCESS != err)
    {
        printf("TCP connect failed!\n");
        return err;
    }

    if (strstr(temp_resp, "OK") == NULL)
    {
        printf("TCP connect response error!\n");
        return FSP_ERR_ABORTED;
    }

    /* 发送HTTP GET请求 */
    char http_request[512];
    snprintf(http_request, sizeof(http_request),
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             path, host);

    err = drv_wifi_send_at_cmd(http_request, response, resp_len, 10000);
    if(FSP_SUCCESS != err)
    {
        printf("HTTP GET failed!\n");
        return err;
    }

    return FSP_SUCCESS;
}

fsp_err_t app_wifi_http_post(const char *host, const char *path, const char *data, char *response, uint16_t resp_len)
{
    fsp_err_t err;
    char cmd[256];
    char temp_resp[512];

    if (!drv_wifi_is_connected())
    {
        printf("WIFI not connected!\n");
        return FSP_ERR_NOT_OPEN;
    }

    /* 建立TCP连接 */
    snprintf(cmd, sizeof(cmd), "AT+NETCONN=TCP,%s,80\r\n", host);
    err = drv_wifi_send_at_cmd(cmd, temp_resp, sizeof(temp_resp), 10000);
    if(FSP_SUCCESS != err)
    {
        printf("TCP connect failed!\n");
        return err;
    }

    /* 发送HTTP POST请求 */
    char http_request[512];
    int data_len = strlen(data);
    snprintf(http_request, sizeof(http_request),
             "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
             path, host, data_len, data);

    err = drv_wifi_send_at_cmd(http_request, response, resp_len, 10000);
    if(FSP_SUCCESS != err)
    {
        printf("HTTP POST failed!\n");
        return err;
    }

    return FSP_SUCCESS;
}

/***********************************************************************************************************************
 * 翻译API
 **********************************************************************************************************************/

fsp_err_t app_wifi_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len)
{
    /* 百度翻译API - 使用实际的API密钥 */
    char path[512];
    char response[HTTP_RESP_MAX_LEN];
    char salt[] = "123456";
    char sign[33];
    char temp[1024];

    /* 用户提供的API凭证 */
    const char *appid = "20260327002581962";
    const char *secret_key = "gDUPXLeHP3oQ4qbVF2x";

    /* 计算签名: sign = MD5(appid + text + salt + secret_key) */
    snprintf(temp, sizeof(temp), "%s%s%s%s", appid, text, salt, secret_key);
    md5_calculate(temp, strlen(temp), sign);

    /* 构建请求路径 */
    snprintf(path, sizeof(path),
             "/api/trans/vip/translate?q=%s&from=%s&to=%s&appid=%s&salt=%s&sign=%s",
             text, from_lang, to_lang, appid, salt, sign);

    fsp_err_t err = app_wifi_http_get("https://fanyi-api.baidu.com/ait/api/aiTextTranslate", path, response, sizeof(response));
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    /* 解析JSON响应 - 简化处理 */
    char *start = strstr(response, "\"dst\":\"");
    if (start != NULL)
    {
        start += 7;
        char *end = strstr(start, "\"");
        if (end != NULL)
        {
            int len = end - start;
            if (len < result_len)
            {
                strncpy(result, start, len);
                result[len] = '\0';
                return FSP_SUCCESS;
            }
        }
    }

    return FSP_ERR_ABORTED;
}

fsp_err_t app_wifi_translate_google(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len)
{
    /* Google翻译API示例 */
    char path[512];
    char response[HTTP_RESP_MAX_LEN];

    /* 构建请求路径 */
    snprintf(path, sizeof(path),
             "/language/translate/v2?q=%s&source=%s&target=%s&key=YOUR_API_KEY",
             text, from_lang, to_lang);

    fsp_err_t err = app_wifi_http_get("translation.googleapis.com", path, response, sizeof(response));
    if(FSP_SUCCESS != err)
    {
        return err;
    }

    /* 解析JSON响应 */
    char *start = strstr(response, "\"translatedText\": \"");
    if (start != NULL)
    {
        start += 20;
        char *end = strstr(start, "\"");
        if (end != NULL)
        {
            int len = end - start;
            if (len < result_len)
            {
                strncpy(result, start, len);
                result[len] = '\0';
                return FSP_SUCCESS;
            }
        }
    }

    return FSP_ERR_ABORTED;
}

/***********************************************************************************************************************
 * 测试函数
 **********************************************************************************************************************/

void app_wifi_test(void)
{
    fsp_err_t err;

    printf("\n========== WIFI Test Start ==========\n");

    /* 初始化WIFI */
    err = app_wifi_init();
    if(FSP_SUCCESS != err)
    {
        printf("WIFI init failed!\n");
        return;
    }

    /* 测试AT命令 */
    app_wifi_test_at();

    printf("\n========== WIFI Test End ==========\n");
}

void app_wifi_test_at(void)
{
    fsp_err_t err;
    char response[256];

    printf("\n----- AT Command Test -----\n");

    /* 测试AT命令 */
    err = drv_wifi_send_at_cmd("AT\r\n", response, sizeof(response), 2000);
    if(FSP_SUCCESS == err)
    {
        printf("AT test: %s\n", response);
    }
    else
    {
        printf("AT test failed!\n");
    }

    /* 获取版本信息 */
    err = drv_wifi_send_at_cmd("AT+GMR\r\n", response, sizeof(response), 2000);
    if(FSP_SUCCESS == err)
    {
        printf("Version: %s\n", response);
    }

    /* 获取WIFI状态 */
    err = drv_wifi_send_at_cmd("AT+WSTATUS\r\n", response, sizeof(response), 2000);
    if(FSP_SUCCESS == err)
    {
        printf("WIFI Status: %s\n", response);
    }
}

void app_wifi_test_http(void)
{
    fsp_err_t err;
    char response[HTTP_RESP_MAX_LEN];

    printf("\n----- HTTP Test -----\n");

    if (!drv_wifi_is_connected())
    {
        printf("WIFI not connected, please connect first!\n");
        return;
    }

    /* 测试HTTP GET请求 */
    err = app_wifi_http_get("www.example.com", "/", response, sizeof(response));
    if(FSP_SUCCESS == err)
    {
        printf("HTTP GET success!\n");
        printf("Response:\n%s\n", response);
    }
    else
    {
        printf("HTTP GET failed!\n");
    }
}

/*******************************************************************************************************************//**
 * @brief WIFI连接管理函数
 *
 * @param ssid WIFI网络名称
 * @param password WIFI密码
 * @return fsp_err_t FSP_SUCCESS if successful, else error code
 **********************************************************************************************************************/
fsp_err_t app_wifi_connect_api(const char *ssid, const char *password)
{
    fsp_err_t err;

    printf("\n========== WIFI Connection Start ==========\n");

    /* 初始化WIFI */
    err = app_wifi_init();
    if(FSP_SUCCESS != err)
    {
        printf("WIFI init failed!\n");
        return err;
    }

    /* 连接WIFI */
    err = app_wifi_connect(ssid, password);
    if(FSP_SUCCESS != err)
    {
        printf("WIFI connect failed!\n");
        return err;
    }

    printf("WIFI connected successfully!\n");
    printf("\n========== WIFI Connection End ==========\n");

    return FSP_SUCCESS;
}

/*******************************************************************************************************************//**
 * @brief 根据WIFI状态通过串口2发送消息
 *
 * @param state WIFI状态
 * @param message 消息内容
 * @return void
 **********************************************************************************************************************/
void app_wifi_send_message(wifi_state_t state, const char *message)
{
    char cmd_buf[128];

    switch (state)
    {
        case WIFI_STATE_IDLE:
            snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"WIFI: Idle\"\xff\xff\xff");
            break;
        case WIFI_STATE_CONNECTING:
            snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"WIFI: Connecting...\"\xff\xff\xff");
            break;
        case WIFI_STATE_CONNECTED:
            snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"WIFI: Connected\"\xff\xff\xff");
            break;
        case WIFI_STATE_DISCONNECTED:
            snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"WIFI: Disconnected\"\xff\xff\xff");
            break;
        case WIFI_STATE_ERROR:
            snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"WIFI: Error\"\xff\xff\xff");
            break;
        default:
            snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"WIFI: Unknown\"\xff\xff\xff");
            break;
    }

    /* 发送状态到串口2 */
    uart2_printf(cmd_buf);
    printf("Sent to UART2: %s\n", cmd_buf);

    /* 如果有消息内容，发送消息 */
    if (message != NULL)
    {
        snprintf(cmd_buf, sizeof(cmd_buf), "g0.txt=\"%s\"\xff\xff\xff", message);
        uart2_printf(cmd_buf);
        printf("Sent to UART2: %s\n", cmd_buf);
    }
}

/*******************************************************************************************************************//**
 * @brief 测试百度翻译API并发送结果到串口2
 *
 * @return void
 **********************************************************************************************************************/
void app_wifi_translate_test(void)
{
    fsp_err_t err;
    char translate_result[TRANSLATE_RESULT_MAX_LEN];
    char test_text[] = "Hello, how are you?";

    printf("\n========== WIFI Translate Test Start ==========\n");

    /* 连接WIFI（请替换为实际的SSID和密码） */
    err = app_wifi_connect_api("your_ssid", "your_password");
    if(FSP_SUCCESS != err)
    {
        /* 发送错误状态 */
        app_wifi_send_message(WIFI_STATE_ERROR, "Connection failed");
        return;
    }

    /* 发送连接成功状态 */
    app_wifi_send_message(WIFI_STATE_CONNECTED, "Ready for translation");

    printf("Testing Baidu Translate API...\n");
    printf("Original text: %s\n", test_text);

    /* 调用百度翻译API（英文 -> 中文） */
    err = app_wifi_translate_baidu(test_text, "en", "zh", translate_result, sizeof(translate_result));
    if(FSP_SUCCESS == err)
    {
        printf("Translation result: %s\n", translate_result);

        /* 发送翻译结果到串口2 */
        app_wifi_send_message(WIFI_STATE_CONNECTED, translate_result);
    }
    else
    {
        printf("Translate failed!\n");
        app_wifi_send_message(WIFI_STATE_ERROR, "Translation failed");
    }

    /* 断开WIFI连接 */
    app_wifi_disconnect();
    app_wifi_send_message(WIFI_STATE_DISCONNECTED, "Disconnected");

    printf("\n========== WIFI Translate Test End ==========\n");
}
