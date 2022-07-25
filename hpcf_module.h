#ifndef HPCF_MODULE
#define HPCF_MODULE

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include "hpcf_list_helper.h"

#define HPCF_MODULE_LIB_INIT_FUNC "hpcf_module_lib_init"

typedef int (*hpcf_module_processor_callback)(char *in, int in_size, char *out, int *out_size, void *data);

// 用来加载指定目录下的所有模块并注册到指定的模块管理器中

// 模块管理器用一个链表将所有的模块保存起来

// 每个模块应填充如下结构
struct hpcf_processor_module {
    struct list_head list;      // 由注册管理时添加到全局链表中，用户不需要管理
    char name[64];              // 模块名称,由模块管理填充
    int type;                   // 模块类型
    char path[256];             // 模块路径，由模块管理填充
    void *data;                 // 由模块开发者动态申请和释放，每次在调用模块回调时都会传入该指针
    hpcf_module_processor_callback callback; // 模块回调函数
};


typedef int (*hpcf_module_lib_init)(struct hpcf_processor_module *module);

typedef void* dlhandle_t;

typedef void* dlsym_t;



// 每个模块应实现统一的接口，用来注册到模块管理器中
// int lib_init(struct hpcf_processor_module *module);

dlhandle_t hpcf_load_library(char *path);

void hpcf_free_library(dlhandle_t handle);

dlsym_t hpcf_get_symbol(dlhandle_t handle, char *symbol);

// 添加模块到全局链表中
int hpcf_add_processor_module(struct hpcf_processor_module *module);

// 在全局链表中删除指定的模块
int hpcf_del_processor_module(struct hpcf_processor_module *module);

// 通过模块名获取模块结构
struct hpcf_processor_module *hpcf_get_processor_module_by_name(char *name);

// 通过模块类型获取模块结构
struct hpcf_processor_module *hpcf_get_processor_module_by_type(int type);

int hpcf_register_processor_module(struct hpcf_processor_module *module);

// 注册模块到模块管理器中
// 遍历目录下的所有模块，并注册到模块管理器中
int hpcf_register_processor_modules(char *dirname);

int hpcf_module_manager_init();

#endif /* HPCF_MODULE */
