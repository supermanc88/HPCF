#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#include "parson.h"

int construct_json()
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
    json_object_set_number(header_object, "Version", 1);
    json_object_set_string(header_object, "User", "abc");
    json_object_set_string(header_object, "Announce", "1234565");
    json_object_set_string(header_object, "SessionID", "");
    json_object_set_string(header_object, "RequestType", "Session");

    // 填充status
    json_object_set_number(status_object, "Code", 0);
    json_object_set_string(status_object, "Message", "OK");
    // 填充responsemessage
    json_object_set_value(res_object, "Status", status_value);
    json_object_set_string(res_object, "Data", "11111111");

    // 填充body
    json_object_set_string(body_object, "Processor", "Request1");
    json_object_set_value(body_object, "ResponseMessage", res_value);

    // 填充root
    json_object_set_value(root_object, "Header", header_value);
    json_object_set_value(root_object, "Body", body_value);

    // 输出json
    char *s = json_serialize_to_string(root_value);
    printf("%s\n", s);
out:
    return ret;
}

int main(int argc, char *argv[])
{
    int ret = 0;

#if 0
    JSON_Value *root_value = NULL;
    JSON_Object *root_object = NULL;
    JSON_Object *header_object = NULL;
    JSON_Object *body_object = NULL;

    root_value = json_parse_file("./a.json");
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
    int version = json_object_get_number(header_object, "Version");
    printf("version: %d\n", version);
    char *user = json_object_get_string(header_object, "User");
    printf("user: %s\n", user);
    char *announce = json_object_get_string(header_object, "Announce");
    printf("announce: %s\n", announce);
    char *sessionid = json_object_get_string(header_object, "SessionID");
    printf("sessionid: %s, len: %d\n", sessionid, strlen(sessionid));
    char *requesttype = json_object_get_string(header_object, "RequestType");
    printf("requesttype: %s\n", requesttype);

    char *processor = json_object_get_string(body_object, "Processor");
    printf("processor: %s\n", processor);
    char *requestmessage = json_object_get_string(body_object, "RequestMessage");
    printf("requestmessage: %s\n", requestmessage);
    json_value_free(root_value);

#endif

    construct_json();

out:
    return ret;
}