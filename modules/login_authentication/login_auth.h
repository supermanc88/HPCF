#ifndef MODULES_LOGIN_AUTHENTICATION_LOGIN_AUTH
#define MODULES_LOGIN_AUTHENTICATION_LOGIN_AUTH

#include "hpcf_module_header.h"

int hpcf_module_lib_init(struct hpcf_processor_module *module);

int login_auth_processor_callback(char *in, int in_size, char *out, int *out_size, void *data);

#endif /* MODULES_LOGIN_AUTHENTICATION_LOGIN_AUTH */
