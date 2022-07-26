#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>

#include "hpcf_module_header.h"
#include "hpcf_module_type.h"
#include "login_auth.h"

int hpcf_module_lib_init(struct hpcf_processor_module *module)
{
    int ret = 0;

    // 本函数填充 type data 和 callback即可，其它的不用填
    module->type = HPCF_MODULE_TYPE_LOGIN_AUTH;
    module->data = (char *)malloc(4 * 1024);
    memset(module->data, 0, 4 * 1024);
    module->callback = &login_auth_processor_callback;

    return ret;
}

int login_auth_processor_callback(char *in, int in_size, char *out, int *out_size, void *data)
{
    int ret = 0;
    return ret;
}