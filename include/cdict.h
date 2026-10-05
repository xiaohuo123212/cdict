/*
 * cdict —— 一个用 C11 实现的小型字典（映射）库
 *
 * 设计要点：
 *   1. 不透明类型：cdict 的内部布局对使用者完全隐藏（定义在 src/cdict.c）
 *   2. 类型安全的值：标签联合（tagged union），不用 void *
 *   3. 未命中用 CDV_NONE 哨兵表示，不需要出参
 *   4. 所有函数都做参数与边界检查，只依赖 C 标准库
 *
 * 已知限制见 DESIGN.md。
 */
#ifndef CDICT_H
#define CDICT_H

#include <stdbool.h>
#include <stddef.h>

/* ------------------------------------------------------------------ *
 * 键（字符串）的最大长度，含结尾的 '\0'。
 * cdict_add 会拒绝长度 >= CDICT_KEY_MAX 的键。
 * ------------------------------------------------------------------ */
#define CDICT_KEY_MAX 100

/* ------------------------------------------------------------------ *
 * 值类型：标签 + 联合。
 * 定义在头文件里，所以它是【透明类型】—— 使用者看得见它的布局。
 * ------------------------------------------------------------------ */

enum cdv_kind {
    CDV_NONE = 0,   /* 未命中：唯一不代表有效值的标签 */
    CDV_INT,        /* 值是 int，读 .i */
    CDV_STR         /* 值是字符串，读 .s */
};

typedef struct cdict_value {
    enum cdv_kind kind;
    union {
        int         i;
        const char *s;   /* 指向字典内部存储，生命周期与字典相同 */
    };
} cdict_value;

/* 便捷构造。只依赖透明的 cdict_value，所以可以内联。 */
static inline cdict_value cdv_int(int v)
{
    cdict_value r;
    r.kind = CDV_INT;
    r.i    = v;
    return r;
}

static inline cdict_value cdv_str(const char *s)
{
    cdict_value r;
    r.kind = CDV_STR;
    r.s    = s;
    return r;
}

/* ------------------------------------------------------------------ *
 * 字典：不透明类型。完整定义在 src/cdict.c 里，使用者看不到。
 * ------------------------------------------------------------------ */

typedef struct cdict cdict;

/* ---- 生命周期 ---- */

/* 创建一个空字典。失败返回 NULL。此时还没有分配存储，需要再调 cdict_init。 */
cdict *cdict_new(void);

/* 释放字典及其全部内部存储。传 NULL 是安全的。 */
void cdict_free(cdict *d);

/* ---- 操作（全部返回 bool 表示成败）---- */

/* 分配容量为 capacity 条的内部存储。capacity 为 0 时失败。
 * 可以重复调用：旧存储会被正确释放，不会泄漏。 */
bool cdict_init(cdict *d, size_t capacity);

/* 插入一条 键 -> 值 的映射。以下情况返回 false：
 *   d 或 word 为 NULL / 尚未 cdict_init / 容量已满 / 键过长 */
bool cdict_add(cdict *d, const char *word, int num);

/* 查找。key 既可以是 int，也可以是字符串；返回对应的【另一半】。
 * 未命中时返回 kind == CDV_NONE 的值（.i / .s 无意义）。 */
cdict_value cdict_search(const cdict *d, cdict_value key);

/* ---- 类型化入口 ----
 * 把参数包成 cdict_value 再转发给 cdict_search。
 * 通常不用直接调用它们，而是通过下面的 cdict_find 宏自动选择。 */
cdict_value cdict_search_key_i(const cdict *d, int key);
cdict_value cdict_search_key_s(const cdict *d, const char *key);

/* ---- 泛型查找宏 ----
 * 按 key 的【静态类型】在编译期选择上面两个之一，零运行时开销。
 * 它只是语法糖 —— cdict_search 照常可用，行为完全一致。
 *
 * 只支持 int 和字符串。其他类型（double / short / long ...）会编译报错，
 * 这是有意为之：避免 long 之类被静默截断成 int 而算错键值。
 *
 * C++ 下不提供这个宏（_Generic 是 C11 特性），C++ 用户请直接用 cdict_search。 */
#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L && !defined(__cplusplus)
#  define cdict_find(d, key)                        \
        _Generic((key),                             \
            cdict_value:  cdict_search,             \
            int:          cdict_search_key_i,       \
            char *:       cdict_search_key_s,       \
            const char *: cdict_search_key_s        \
        )((d), (key))
#endif


/* ---- 只读访问器 ---- */

size_t cdict_count(const cdict *d);      /* 已存条目数 */
size_t cdict_capacity(const cdict *d);   /* 容量 */

#endif /* CDICT_H */
