#include <bits/types/struct_timeval.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/time.h>
#include <time.h>

#include "hpcf_module_header.h"
#include "hpcf_module_type.h"
#include "login_auth.h"
#include "parson.h"
#include "sm3.h"
#include "sm4.h"
#include "base64.h"

struct list_head g_sessionid_list;
struct session_data {
    struct list_head list;
    char sessionid[128];
    struct timeval time;    // 保存sessionid的时间
};

int hpcf_module_lib_init(struct hpcf_processor_module *module)
{
    int ret = 0;

    // 本函数填充 type data 和 callback即可，其它的不用填
    module->type = HPCF_MODULE_TYPE_LOGIN_AUTH;
    INIT_LIST_HEAD(&g_sessionid_list);
    module->data = &g_sessionid_list;
    module->callback = &login_auth_processor_callback;

    return ret;
}

// 如果json header中sessionid不为空的话，则验证sessionid是否有效，如果有效则返回0，否则返回-1
// 如果json header中sessionid为空的话，则认为是请求sessionid的流程，最终返回一个sessionid
int login_auth_processor_callback(char *in, int in_size, char *out, int *out_size, void **module_data, void **conn_data)
{
    int ret = 0;
    struct login_auth_req req = {0};

    if ( *module_data == NULL) {
        *module_data = (void *)malloc(8 * 1024);
        memset(*module_data, 0, (8 * 1024));
    }
    if ( *conn_data == NULL) {
        *conn_data = (void *)malloc(1024);
        memset(*conn_data, 0, 1024);
    }

    // 解析json
    ret = login_auth_parse_req(in, in_size, &req);
    if (ret != 0) {
        goto out;
    }

    if (strlen(req.h.session_id) != 0) {
        // 验证sessionid是否有效
        ret = login_auth_verify_sessionid(&req, req.h.session_id, *module_data, out, out_size);
        if (ret != 0) {
            goto out;
        }
    } else {
        // 获取sessionid
        ret = login_auth_get_sessionid(&req, *module_data, *conn_data, out, out_size);
        if (ret != 0) {
            goto out;
        }
    }
out:
    return ret;
}

// 解析json数据
int login_auth_parse_req(char *in, int in_size, struct login_auth_req *req)
{
    int ret = 0;
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Object *header_object = NULL;
    JSON_Object *body_object = NULL;

    root_value = json_parse_string(in);
    if (root_value == NULL) {
        ret = -1;
        goto out;
    }
    root_object = json_value_get_object(root_value);
    if (root_object == NULL) {
        ret = -1;
        goto out;
    }
    header_object = json_object_get_object(root_object, "Header");
    if (header_object == NULL) {
        ret = -1;
        goto out;
    }
    body_object = json_object_get_object(root_object, "Body");
    if (body_object == NULL) {
        ret = -1;
        goto out;
    }
    int n = json_object_get_number(header_object, "Version");
    req->h.version = n;
    char *s = json_object_get_string(header_object, "User");
    memcpy(req->h.user, s, strlen(s));
    s = json_object_get_string(header_object, "Announce");
    memcpy(req->h.announce, s, strlen(s));
    s = json_object_get_string(header_object, "SessionID");
    memcpy(req->h.session_id, s, strlen(s));
    s = json_object_get_string(header_object, "RequestType");
    memcpy(req->h.request_type, s, strlen(s));

    s = json_object_get_string(body_object, "Processor");
    memcpy(req->b.processor, s, strlen(s));
    s = json_object_get_string(body_object, "RequestMessage");
    if (s != NULL) {
        memcpy(req->b.d.req_msg, s, strlen(s));
    }

    json_value_free(root_value);

#if 1
    // print
    printf("version: %d\n", req->h.version);
    printf("user: %s\n", req->h.user);
    printf("announce: %s\n", req->h.announce);
    printf("session_id: %s\n", req->h.session_id);
    printf("request_type: %s\n", req->h.request_type);
    printf("processor: %s\n", req->b.processor);
    printf("request_msg: %s\n", req->b.d.req_msg);
#endif 
out:
    return ret;
}

// 输出数据内存由函数内分配
int login_auth_construct_result(struct login_auth_result *result, char **out, int *out_size)
{
    int ret = 0;

    // 以json形式输出到out
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Value *header_value = NULL;
    JSON_Object *header_object = NULL;
    JSON_Value *body_value = NULL;
    JSON_Object *body_object = NULL;
    JSON_Value *res_value = NULL;
    JSON_Object *res_object = NULL;
    JSON_Value *status_value = NULL;
    JSON_Object *status_object = NULL;

    root_value = json_value_init_object();
    if (root_value == NULL) {
        ret = -1;
        goto out;
    }
    root_object = json_value_get_object(root_value);
    if (root_object == NULL) {
        ret = -1;
        goto out;
    }
    header_value = json_value_init_object();
    if (header_value == NULL) {
        ret = -1;
        goto out;
    }
    header_object = json_value_get_object(header_value);
    if (header_object == NULL) {
        ret = -1;
        goto out;
    }
    body_value = json_value_init_object();
    if (body_value == NULL) {
        ret = -1;
        goto out;
    }
    body_object = json_value_get_object(body_value);
    if (body_object == NULL) {
        ret = -1;
        goto out;
    }
    res_value = json_value_init_object();
    if (res_value == NULL) {
        ret = -1;
        goto out;
    }
    res_object = json_value_get_object(res_value);
    if (res_object == NULL) {
        ret = -1;
        goto out;
    }
    status_value = json_value_init_object();
    if (status_value == NULL) {
        ret = -1;
        goto out;
    }
    status_object = json_value_get_object(status_value);
    if (status_object == NULL) {
        ret = -1;
        goto out;
    }

    // 填充header
    json_object_set_number(header_object, "Version", result->h.version);
    json_object_set_string(header_object, "User", result->h.user);
    json_object_set_string(header_object, "Announce", result->h.announce);
    json_object_set_string(header_object, "SessionID", result->h.session_id);
    json_object_set_string(header_object, "RequestType", result->h.request_type);

    // 填充status
    json_object_set_number(status_object, "Code", result->b.s.result);
    json_object_set_string(status_object, "Message", result->b.s.result_msg);
    // 填充responsemessage
    json_object_set_value(res_object, "Status", status_value);
    json_object_set_string(res_object, "Data", result->b.d.ran_or_sess);

    // 填充body
    json_object_set_string(body_object, "Processor", result->b.processor);
    json_object_set_value(body_object, "ResponseMessage", res_value);

    // 填充root
    json_object_set_value(root_object, "Header", header_value);
    json_object_set_value(root_object, "Body", body_value);

    // 输出json
    char *s = json_serialize_to_string(root_value);

    // 分配内存
    *out = (char *)malloc(strlen(s) + 10);
    if (*out == NULL) {
        ret = -1;
        goto out;
    }
    memset(*out, 0, strlen(s) + 10);
    memcpy(*out, s, strlen(s));
    *out_size = strlen(s);

    // 释放json内存
    json_value_free(root_value);

out:
    return ret;
}

int login_auth_get_sessionid(struct login_auth_req *req, void *module_data, void *conn_data, char *out, int *out_size)
{
    int ret = 0;
    if (strcmp(req->b.processor, "SessionRequest1") == 0) {
        // 根据用户名和挑战码生成一个随机数，此流程暂略
        // 假设生成的是1111111111111111
        // 生成8字节的随机数，并转换成16进制字符串，这样就不会因为0导致字符串截断了
        // 把生成的sessionid放到conn_data中
        struct login_auth_result result;
        memset(&result, 0, sizeof(result));

        char ran_str[33] = {0};
        ret = login_auth_gen_32_random_string(ran_str);
        // header 不用变，直接复制
        memcpy(&result.h, &req->h, sizeof(struct login_auth_header));
        strcpy(result.b.processor, "SessionResponse1");
        result.b.s.result = 0;
        strcpy(result.b.s.result_msg, "OK");
        strcpy(result.b.d.ran_or_sess, ran_str);

        // 生成的随机数记到conn_data中
        strcpy((char *)conn_data, ran_str);

        // 输出json
        char *out_json = NULL;
        ret = login_auth_construct_result(&result, &out_json, out_size);
        if (ret != 0) {
            goto out;
        }
        // 输出到out
        memcpy(out, out_json, *out_size);
        free(out_json);

    } else if (strcmp(req->b.processor, "SessionRequest2") == 0) {
        // 需要将req->b.d.req_msg中的数据先解密，看是否和1中的随机数一致
        // 通过后，生成一个sessionid

        // 加密的数据格式为：
        // |random_str|username|password|padding
        // 应该先base64解码
        // 我这里解密(sm4)出来后，直接验证random_str是不是之前发送的就可以了
        // sessionid的生成方式为：
        // 使用sm3对|random_str|username|进行hash，得到sessionid
        int req_msg_len = strlen(req->b.d.req_msg);
        unsigned char *enc_msg = (unsigned char *)malloc(req_msg_len);
        memset(enc_msg, 0, req_msg_len);
        ret = base64_decode(req->b.d.req_msg, req_msg_len, enc_msg);

        // 解密
        unsigned char key[16] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35};
        unsigned char iv[16] = {0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x30, 0x31, 0x32, 0x33, 0x34, 0x35};
        unsigned char *dec_msg = (unsigned char *)malloc(ret);
        sm4_context ctx;
        sm4_setkey_dec(&ctx, key);
        sm4_crypt_cbc(&ctx, SM4_DECRYPT, ret, iv, enc_msg, dec_msg);

        // 使用 | 作为分隔符，分割字符串
        char *random_str = strtok((char *)dec_msg, "|");
        char *username = strtok(NULL, "|");

        struct login_auth_result result;
        memset(&result, 0, sizeof(result));

        // 验证random_str
        if (strcmp((char *)conn_data, random_str)) {
            // 验证失败
            // header 不用变，直接复制
            memcpy(&result.h, &req->h, sizeof(struct login_auth_header));
            strcpy(result.b.processor, "SessionResponse2");
            result.b.s.result = -1;
            strcpy(result.b.s.result_msg, "Login verify failed");
        } else {
            // 生成sessionid
            char sessionid[65] = {0};
            char hash[32] = {0};
            char data[128] = {0};
            sprintf(data, "%s|%s", random_str, username);
            sm3((unsigned char *)data, strlen(data), (unsigned char *)hash);
            int i;
            for (i = 0; i < 32; i++) {
                sprintf(sessionid + i * 2, "%02x", (unsigned char)hash[i]);
            }
            // header 不用变，直接复制
            memcpy(&result.h, &req->h, sizeof(struct login_auth_header));
            strcpy(result.b.processor, "SessionResponse2");
            result.b.s.result = 0;
            strcpy(result.b.s.result_msg, "OK");
            strcpy(result.b.d.ran_or_sess, sessionid);

            // 将此次产生的sessionid记录到模块数据中
            struct session_data *sess_data = (struct session_data *)malloc(sizeof(struct session_data));
            memset(sess_data, 0, sizeof(struct session_data));
            strcpy(sess_data->sessionid, sessionid);
            gettimeofday(&sess_data->time, NULL);
            list_add_tail(&sess_data->list, &g_sessionid_list);
        }

        // 输出json
        char *out_json = NULL;
        ret = login_auth_construct_result(&result, &out_json, out_size);
        if (ret != 0) {
            goto out;
        }
        // 输出到out
        memcpy(out, out_json, *out_size);
        free(out_json);

        ret = 0;

    } else {
        // error
        struct login_auth_result result;
        memset(&result, 0, sizeof(result));
        // header 不用变，直接复制
        memcpy(&result.h, &req->h, sizeof(struct login_auth_header));
        strcpy(result.b.processor, "SessionRequestError");
        result.b.s.result = -1;
        strcpy(result.b.s.result_msg, "unknown processor");
        // strcpy(result.b.d.ran_or_sess, "");

        // 输出json
        char *out_json = NULL;
        ret = login_auth_construct_result(&result, &out_json, out_size);
        if (ret != 0) {
            goto out;
        }
        // 输出到out
        memcpy(out, out_json, *out_size);
        free(out_json);
    }

out:
    return ret;
}

int login_auth_gen_32_random_string(char *data)
{
    int ret = 0; 
    unsigned char ran[16];
    memset(ran, 0, sizeof(ran));

    srand(time(NULL));
    int i;
    for (i = 0; i < 16; i++) {
        ran[i] = rand() % 256;
    }
    // 将ran转换成16进制字符串
    for (i = 0; i < 16; i++) {
        sprintf(data + i * 2, "%02x", ran[i]);
    }

    return ret;
}

int login_auth_verify_sessionid(struct login_auth_req *req, char *session_id, void *module_data, char *out, int *out_size)
{
    int ret = -1;

    // 遍历g_sessionid_list，查找sessionid
    struct list_head *pos;
    struct session_data *sess_data;
    list_for_each(pos, &g_sessionid_list) {
        sess_data = list_entry(pos, struct session_data, list);
        if (strcmp(sess_data->sessionid, session_id) == 0) {
            // 找到了
            // 判断是否超时
            struct timeval now;
            gettimeofday(&now, NULL);
            if (now.tv_sec - sess_data->time.tv_sec > 3600) {
                // 超时
                ret = -1;
            } else {
                // 更新时间
                sess_data->time = now;
                ret = 0;
            }
            break;
        }
    }
    // 如果没有找到，ret = -1
    if (ret != 0) {
        // 外面就直接返回了，需要填充out
        struct login_auth_result result;
        memset(&result, 0, sizeof(result));
        // header 不用变，直接复制
        memcpy(&result.h, &req->h, sizeof(struct login_auth_header));
        strcpy(result.b.processor, "SessionResponse");
        result.b.s.result = -1;
        strcpy(result.b.s.result_msg, "Sessionid verify failed");
        // strcpy(result.b.d.ran_or_sess, "");

        // 输出json
        char *out_json = NULL;
        login_auth_construct_result(&result, &out_json, out_size);

        // 输出到out
        memcpy(out, out_json, *out_size);
        free(out_json);
    }

    return ret;
}