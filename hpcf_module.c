#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <dirent.h>
#include <dlfcn.h>

#include "hpcf_module.h"

struct list_head g_module_list;

dlhandle_t hpcf_load_library(char *path)
{
    dlhandle_t handle;
    handle = dlopen(path, RTLD_LAZY);
    if (handle == NULL) {
        fprintf(stderr, "dlopen failed: %s\n", dlerror());
        return NULL;
    }
    return handle;
}

void hpcf_free_library(dlhandle_t handle)
{
    dlclose(handle);
}

dlsym_t hpcf_get_symbol(dlhandle_t handle, char *symbol)
{
    dlsym_t sym;
    if (handle == NULL || symbol == NULL) {
        return NULL;
    }

    sym = dlsym(handle, symbol);
    if (sym == NULL) {
        fprintf(stderr, "dlsym failed: %s\n", dlerror());
        return NULL;
    }
    return sym;
}

// 添加模块到全局链表中
int hpcf_add_processor_module(struct hpcf_processor_module *module)
{
    struct list_head *pos;
    struct hpcf_processor_module *tmp;
    if (module == NULL) {
        return -1;
    }
    // 检查是否已经存在
    list_for_each(pos, &g_module_list) {
        tmp = list_entry(pos, struct hpcf_processor_module, list);
        if (strcmp(tmp->name, module->name) == 0) {
            // 已经存在
            return 0;
        }
    }
    list_add_tail(&module->list, &g_module_list);
    return 0;
}

// 在全局链表中删除指定的模块
int hpcf_del_processor_module(struct hpcf_processor_module *module)
{
    struct list_head *pos, *n;
    struct hpcf_processor_module *tmp;
    if (module == NULL) {
        return -1;
    }
    list_for_each_safe(pos, n, &g_module_list) {
        tmp = list_entry(pos, struct hpcf_processor_module, list);
        if (strcmp(tmp->name, module->name) == 0) {
            // 删除
            list_del(pos);
            return 0;
        }
    }
    return -1;
}

// 通过模块名获取模块结构
struct hpcf_processor_module *hpcf_get_processor_module_by_name(char *name)
{
    struct list_head *pos;
    struct hpcf_processor_module *tmp;
    if (name == NULL) {
        return NULL;
    }
    list_for_each(pos, &g_module_list) {
        tmp = list_entry(pos, struct hpcf_processor_module, list);
        if (strcmp(tmp->name, name) == 0) {
            return tmp;
        }
    }
    return NULL;
}

// 通过模块类型获取模块结构
struct hpcf_processor_module *hpcf_get_processor_module_by_type(int type)
{
    struct list_head *pos;
    struct hpcf_processor_module *tmp;

    list_for_each(pos, &g_module_list) {
        tmp = list_entry(pos, struct hpcf_processor_module, list);
        if (tmp->type == type) {
            return tmp;
        }
    }
    return NULL;
}

int hpcf_register_processor_module(struct hpcf_processor_module *module)
{
    dlhandle_t handle;
    dlsym_t sym;
    int ret = 0;

    if (module == NULL) {
        ret = -1;
        goto out;
    }

    handle = hpcf_load_library(module->path);
    if (handle == NULL) {
        ret = -1;
        goto out;
    }
    sym = hpcf_get_symbol(handle, HPCF_MODULE_LIB_INIT_FUNC);
    if (sym == NULL) {
        ret = -1;
        goto err1;
    }
    hpcf_module_lib_init l_init = (hpcf_module_lib_init)sym;

    ret = l_init((void *)module);
    if (ret < 0) {
        ret = -1;
        fprintf(stderr, "module init failed\n");
        goto err1;
    }

    // 添加到全局链表中
    hpcf_add_processor_module(module);

err1:
    hpcf_free_library(handle);
out:
    return ret;
}

// 注册模块到模块管理器中
// 遍历目录下的所有模块，并注册到模块管理器中
int hpcf_register_processor_modules(char *dirname)
{
    int ret = 0;
    // 判断dirname是否为空
    if (dirname == NULL) {
        ret = -1;
        goto out;
    }

    // 判断目录是否存在
    struct stat st;
    if (stat(dirname, &st) < 0) {
        ret = -1;
        goto out;
    }
    if (!S_ISDIR(st.st_mode)) {
        ret = -1;
        goto out;
    }

    // 遍历目录下的所有.so文件
    DIR *dir = opendir(dirname);
    if (dir == NULL) {
        ret = -1;
        goto out;
    }
    struct dirent *dirent = NULL;

    // read dir and register modules
    while ((dirent = readdir(dir)) != NULL) {
        // 跳过.和..
        if (strcmp(dirent->d_name, ".") == 0 || strcmp(dirent->d_name, "..") == 0) {
            continue;
        }
        int len = strlen(dirent->d_name);
        // 跳过非.so文件
        if (len < 4 || strcmp(dirent->d_name + len - 3, ".so") != 0) {
            continue;
        }
        // 动态申请一个模块结构
        struct hpcf_processor_module *module = (struct hpcf_processor_module *)malloc(sizeof(struct hpcf_processor_module));
        if (module == NULL) {
            ret = -1;
            goto out;
        }
        memset(module, 0, sizeof(struct hpcf_processor_module));
        // 获取模块路径
        strcpy(module->path, dirname);
        if (module->path[strlen(module->path) - 1] != '/') {
            strcat(module->path, "/");
        }
        strcat(module->path, dirent->d_name);

        // 获取模块名
        strncpy(module->name, dirent->d_name, len - 3);

        // 注册模块
        ret = hpcf_register_processor_module(module);
        if (ret != 0) {
            // 注册失败，释放模块结构
            // 说明这个不是一个模块动态库
            free(module);
            continue;
        }
    }

out:
    return ret;
}

int hpcf_module_manager_init()
{
    INIT_LIST_HEAD(&g_module_list);
    return 0;
}