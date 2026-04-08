#include "net_client.h"
#include "app_ai_config.h"

#include <stdio.h>
#include <string.h>

typedef struct
{
    uint32_t state[4];
    uint32_t count[2];
    uint8_t  buffer[64];
} md5_ctx_t;

static uint8_t g_md5_padding[64] = {
    0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

static uint32_t g_md5_shift[4][4] = {
    {7, 12, 17, 22},
    {5, 9, 14, 20},
    {4, 11, 16, 23},
    {6, 10, 15, 21}
};

static uint32_t g_md5_k[64] = {
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

static void md5_init(md5_ctx_t *ctx);
static void md5_update(md5_ctx_t *ctx, const uint8_t *input, uint32_t input_len);
static void md5_final(uint8_t digest[16], md5_ctx_t *ctx);
static void md5_transform(uint32_t state[4], const uint8_t block[64]);
static void md5_encode(uint8_t *output, uint32_t *input, uint32_t len);
static void md5_decode(uint32_t *output, const uint8_t *input, uint32_t len);
static void md5_calculate(const char *data, int len, char *md5_result);
static fsp_err_t net_client_url_encode(const char *input, char *output, uint16_t output_len);
static fsp_err_t net_client_extract_json_value(const char *json, const char *key, char *value, uint16_t value_len);

static void md5_init(md5_ctx_t *ctx)
{
    ctx->count[0] = 0;
    ctx->count[1] = 0;
    ctx->state[0] = 0x67452301;
    ctx->state[1] = 0xefcdab89;
    ctx->state[2] = 0x98badcfe;
    ctx->state[3] = 0x10325476;
}

static void md5_update(md5_ctx_t *ctx, const uint8_t *input, uint32_t input_len)
{
    uint32_t i;
    uint32_t index;
    uint32_t part_len;

    index = (uint32_t)((ctx->count[0] >> 3) & 0x3FU);
    part_len = 64U - index;

    if (input_len >= part_len)
    {
        memcpy(&ctx->buffer[index], input, part_len);
        md5_transform(ctx->state, ctx->buffer);
        for (i = part_len; i + 63U < input_len; i += 64U)
        {
            md5_transform(ctx->state, &input[i]);
        }
        index = 0;
    }
    else
    {
        i = 0;
    }

    memcpy(&ctx->buffer[index], &input[i], input_len - i);
    ctx->count[0] += input_len << 3;
    if (ctx->count[0] < (input_len << 3))
    {
        ctx->count[1]++;
    }
    ctx->count[1] += input_len >> 29;
}

static void md5_final(uint8_t digest[16], md5_ctx_t *ctx)
{
    uint8_t bits[8];
    uint32_t index;
    uint32_t pad_len;

    md5_encode(bits, ctx->count, 8U);
    index = (uint32_t)((ctx->count[0] >> 3) & 0x3FU);
    pad_len = (index < 56U) ? (56U - index) : (120U - index);
    md5_update(ctx, g_md5_padding, pad_len);
    md5_update(ctx, bits, 8U);
    md5_encode(digest, ctx->state, 16U);
}

static void md5_transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t x[16];

    md5_decode(x, block, 64U);

    for (int i = 0; i < 64; i++)
    {
        uint32_t f;
        uint32_t g;
        uint32_t rotate_input;
        uint32_t temp;

        if (i < 16)
        {
            f = (b & c) | ((~b) & d);
            g = (uint32_t) i;
        }
        else if (i < 32)
        {
            f = (d & b) | ((~d) & c);
            g = (uint32_t) ((5 * i + 1) % 16);
        }
        else if (i < 48)
        {
            f = b ^ c ^ d;
            g = (uint32_t) ((3 * i + 5) % 16);
        }
        else
        {
            f = c ^ (b | (~d));
            g = (uint32_t) ((7 * i) % 16);
        }

        rotate_input = a + f + g_md5_k[i] + x[g];
        temp = d;
        d = c;
        c = b;
        b = b + ((rotate_input << g_md5_shift[i / 16][i % 4]) |
                (rotate_input >> (32U - g_md5_shift[i / 16][i % 4])));
        a = temp;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

static void md5_encode(uint8_t *output, uint32_t *input, uint32_t len)
{
    for (uint32_t i = 0, j = 0; j < len; i++, j += 4)
    {
        output[j] = (uint8_t)(input[i] & 0xFFU);
        output[j + 1] = (uint8_t)((input[i] >> 8) & 0xFFU);
        output[j + 2] = (uint8_t)((input[i] >> 16) & 0xFFU);
        output[j + 3] = (uint8_t)((input[i] >> 24) & 0xFFU);
    }
}

static void md5_decode(uint32_t *output, const uint8_t *input, uint32_t len)
{
    for (uint32_t i = 0, j = 0; j < len; i++, j += 4)
    {
        output[i] = ((uint32_t) input[j]) |
                    (((uint32_t) input[j + 1]) << 8) |
                    (((uint32_t) input[j + 2]) << 16) |
                    (((uint32_t) input[j + 3]) << 24);
    }
}

static void md5_calculate(const char *data, int len, char *md5_result)
{
    md5_ctx_t ctx;
    uint8_t digest[16];

    md5_init(&ctx);
    md5_update(&ctx, (const uint8_t *) data, (uint32_t) len);
    md5_final(digest, &ctx);

    for (int i = 0; i < 16; i++)
    {
        sprintf(&md5_result[i * 2], "%02x", digest[i]);
    }

    md5_result[32] = '\0';
}

static fsp_err_t net_client_url_encode(const char *input, char *output, uint16_t output_len)
{
    static const char hex[] = "0123456789ABCDEF";
    uint16_t in_index = 0;
    uint16_t out_index = 0;

    if ((NULL == input) || (NULL == output) || (0U == output_len))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    while ('\0' != input[in_index])
    {
        uint8_t ch = (uint8_t) input[in_index++];
        bool is_unreserved = (((ch >= 'a') && (ch <= 'z')) ||
                              ((ch >= 'A') && (ch <= 'Z')) ||
                              ((ch >= '0') && (ch <= '9')) ||
                              (ch == '-') || (ch == '_') || (ch == '.') || (ch == '~'));

        if (is_unreserved)
        {
            if ((out_index + 1U) >= output_len)
            {
                return FSP_ERR_INVALID_SIZE;
            }

            output[out_index++] = (char) ch;
        }
        else
        {
            if ((out_index + 3U) >= output_len)
            {
                return FSP_ERR_INVALID_SIZE;
            }

            output[out_index++] = '%';
            output[out_index++] = hex[(ch >> 4) & 0x0FU];
            output[out_index++] = hex[ch & 0x0FU];
        }
    }

    output[out_index] = '\0';
    return FSP_SUCCESS;
}

static fsp_err_t net_client_extract_json_value(const char *json, const char *key, char *value, uint16_t value_len)
{
    const char *key_pos;
    const char *value_start;
    const char *value_end;
    uint16_t copy_len;

    if ((NULL == json) || (NULL == key) || (NULL == value) || (value_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    key_pos = strstr(json, key);
    if (NULL == key_pos)
    {
        return FSP_ERR_ABORTED;
    }

    value_start = strchr(key_pos, ':');
    if (NULL == value_start)
    {
        return FSP_ERR_ABORTED;
    }

    value_start = strchr(value_start, '"');
    if (NULL == value_start)
    {
        return FSP_ERR_ABORTED;
    }

    value_start++;
    value_end = strchr(value_start, '"');
    if (NULL == value_end)
    {
        return FSP_ERR_ABORTED;
    }

    copy_len = (uint16_t) (value_end - value_start);
    if (copy_len >= value_len)
    {
        return FSP_ERR_INVALID_SIZE;
    }

    memcpy(value, value_start, copy_len);
    value[copy_len] = '\0';
    return FSP_SUCCESS;
}

fsp_err_t net_client_init(void)
{
    return wifi_module_init();
}

fsp_err_t net_client_connect_wifi(const char *ssid, const char *password)
{
    return wifi_module_join_ap(ssid, password);
}

fsp_err_t net_client_disconnect_wifi(void)
{
    return wifi_module_leave_ap();
}

fsp_err_t net_client_http_get(const char *host, const char *path, char *response, uint16_t response_len)
{
    char request[512];

    if ((NULL == host) || (NULL == path) || (NULL == response) || (response_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    snprintf(request,
             sizeof(request),
             "GET %s HTTP/1.1\r\nHost: %s\r\nConnection: close\r\n\r\n",
             path,
             host);

    return wifi_module_http_request(host,
                                    WIFI_MODULE_HTTP_PORT,
                                    request,
                                    response,
                                    response_len,
                                    12000U);
}

fsp_err_t net_client_http_post(const char *host, const char *path, const char *data, char *response, uint16_t response_len)
{
    char request[512];

    if ((NULL == host) || (NULL == path) || (NULL == data) || (NULL == response) || (response_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    snprintf(request,
             sizeof(request),
             "POST %s HTTP/1.1\r\nHost: %s\r\nContent-Type: application/x-www-form-urlencoded\r\nContent-Length: %u\r\nConnection: close\r\n\r\n%s",
             path,
             host,
             (unsigned int) strlen(data),
             data);

    return wifi_module_http_request(host,
                                    WIFI_MODULE_HTTP_PORT,
                                    request,
                                    response,
                                    response_len,
                                    12000U);
}

fsp_err_t net_client_translate_baidu(const char *text, const char *from_lang, const char *to_lang, char *result, uint16_t result_len)
{
    char encoded_text[512];
    char path[768];
    char response[NET_CLIENT_HTTP_RESPONSE_MAX_LEN];
    char sign_input[1024];
    char sign[33];
    const char *appid = APP_AI_BAIDU_APP_ID;
    const char *secret_key = APP_AI_BAIDU_SECRET_KEY;
    const char *salt = APP_AI_BAIDU_SALT;
    fsp_err_t err;

    if ((NULL == text) || (NULL == from_lang) || (NULL == to_lang) || (NULL == result) || (result_len < 2U))
    {
        return FSP_ERR_INVALID_ARGUMENT;
    }

    if ((0 == strcmp(appid, "your_baidu_appid")) || (0 == strcmp(secret_key, "your_baidu_secret")))
    {
        return FSP_ERR_NOT_OPEN;
    }

    err = net_client_url_encode(text, encoded_text, sizeof(encoded_text));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    snprintf(sign_input, sizeof(sign_input), "%s%s%s%s", appid, text, salt, secret_key);
    md5_calculate(sign_input, (int) strlen(sign_input), sign);

    snprintf(path,
             sizeof(path),
             "/api/trans/vip/translate?q=%s&from=%s&to=%s&appid=%s&salt=%s&sign=%s",
             encoded_text,
             from_lang,
             to_lang,
             appid,
             salt,
             sign);

    err = net_client_http_get("fanyi-api.baidu.com", path, response, sizeof(response));
    if (FSP_SUCCESS != err)
    {
        return err;
    }

    err = net_client_extract_json_value(response, "\"dst\"", result, result_len);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    err = net_client_extract_json_value(response, "\"trans_result\"", result, result_len);
    if (FSP_SUCCESS == err)
    {
        return FSP_SUCCESS;
    }

    return FSP_ERR_ABORTED;
}
