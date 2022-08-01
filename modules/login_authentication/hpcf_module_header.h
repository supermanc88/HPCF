#ifndef MODULES_LOGIN_AUTHENTICATION_HPCF_MODULE_HEADER
#define MODULES_LOGIN_AUTHENTICATION_HPCF_MODULE_HEADER

#include "hpcf_list_helper.h"
// struct list_head {
//     struct list_head *next;
//     struct list_head *prev;
// };

typedef int (*hpcf_module_processor_callback)(char *in, int in_size, char *out, int *out_size,
                void **module_data, void **conn_data);

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

// 必须实现的接口
// int hpcf_module_lib_init(struct hpcf_processor_module *module);

#endif /* MODULES_LOGIN_AUTHENTICATION_HPCF_MODULE_HEADER */
