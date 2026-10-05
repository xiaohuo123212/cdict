/*
 * cdict —— 实现
 *
 * struct cdict 的完整定义只出现在本文件里。使用者包含 cdict.h 时
 * 只看到一个不完整类型，因此无法依赖内部布局，也无法破坏内部状态。
 */
#include "cdict.h"

#include <stdint.h>     /* SIZE_MAX */
#include <stdlib.h>     /* malloc / calloc / free */
#include <string.h>     /* strlen / memcpy / strcmp */

/* ------------------------------------------------------------------ *
 * 内部布局（使用者永远看不到）
 *
 *   numarr  : capacity 个 int，紧凑排列 —— 按整数查找时缓存友好
 *   chararr : capacity × CDICT_KEY_MAX 字节的扁平字符串区
 *
 * 注意 count 与 capacity 是两回事：
 *   capacity 是"分配了多少格"，count 是"实际写了几格"。
 *   遍历已有条目必须用 count —— 用 capacity 会读到未初始化的内存。
 * ------------------------------------------------------------------ */
struct cdict {
    size_t  count;      /* 已存条目数 */
    size_t  capacity;   /* 容量 */
    int    *numarr;     /* 值数组 */
    char   *chararr;    /* 字符串区 */
};

/* ------------------------------------------------------------------ *
 * 生命周期
 * ------------------------------------------------------------------ */

cdict *cdict_new(void)
{
    /* calloc 把 count / capacity 清零、指针置 NULL，省掉逐字段初始化 */
    return calloc(1, sizeof(cdict));
}

void cdict_free(cdict *d)
{
    if (d == NULL) return;      /* 让 cdict_free(NULL) 成为安全的 no-op */

    free(d->numarr);
    free(d->chararr);
    free(d);
}

/* ------------------------------------------------------------------ *
 * 初始化
 * ------------------------------------------------------------------ */

bool cdict_init(cdict *d, size_t capacity)
{
    if (d == NULL || capacity == 0) return false;

    /* 防乘法溢出：capacity * 每格字节数 必须能用 size_t 表示 */
    if (capacity > SIZE_MAX / sizeof(int))     return false;
    if (capacity > SIZE_MAX / CDICT_KEY_MAX)   return false;

    int  *nums = malloc(capacity * sizeof *nums);
    char *strs = malloc(capacity * CDICT_KEY_MAX);

    if (nums == NULL || strs == NULL) {
        free(nums);             /* 部分成功也要清干净，不能泄漏 */
        free(strs);
        return false;
    }

    /* 先分配成功、再释放旧的 —— 这样失败时旧数据不会被破坏。
     * 也正因如此，重复调用 cdict_init 不会泄漏。
     * free(NULL) 是安全的，所以第一次调用也没问题。 */
    free(d->numarr);
    free(d->chararr);

    d->numarr   = nums;
    d->chararr  = strs;
    d->capacity = capacity;
    d->count    = 0;
    return true;
}

/* ------------------------------------------------------------------ *
 * 插入
 * ------------------------------------------------------------------ */

bool cdict_add(cdict *d, const char *word, int num)
{
    if (d == NULL || word == NULL)          return false;
    if (d->numarr == NULL || d->chararr == NULL) return false;  /* 尚未 init */
    if (d->count >= d->capacity)            return false;       /* 容量已满 */
    if (strlen(word) >= CDICT_KEY_MAX)      return false;       /* 键过长 */

    size_t len = strlen(word) + 1;      /* 含结尾 '\0' */

    d->numarr[d->count] = num;
    memcpy(d->chararr + d->count * CDICT_KEY_MAX, word, len);
    d->count++;
    return true;
}

/* ------------------------------------------------------------------ *
 * 查找
 * ------------------------------------------------------------------ */

cdict_value cdict_search(const cdict *d, cdict_value key)
{
    /* 默认结果就是"未命中"。分支未覆盖时也返回它。 */
    cdict_value result = { .kind = CDV_NONE };

    if (d == NULL || d->numarr == NULL) return result;

    if (key.kind == CDV_INT) {
        /* 用整数查 -> 返回对应的字符串 */
        for (size_t i = 0; i < d->count; i++) {
            if (d->numarr[i] == key.i) {
                result.kind = CDV_STR;
                result.s    = d->chararr + i * CDICT_KEY_MAX;
                break;                  /* 命中即停，不再扫完剩下的 */
            }
        }
    } else if (key.kind == CDV_STR) {
        /* 用字符串查 -> 返回对应的整数 */
        for (size_t i = 0; i < d->count; i++) {
            if (strcmp(d->chararr + i * CDICT_KEY_MAX, key.s) == 0) {
                result.kind = CDV_INT;
                result.i    = d->numarr[i];
                break;
            }
        }
    }
    /* key.kind == CDV_NONE：直接返回未命中 */

    return result;
}

/* ------------------------------------------------------------------ *
 * 只读访问器
 * ------------------------------------------------------------------ */

size_t cdict_count(const cdict *d)
{
    return (d != NULL) ? d->count : 0;
}

size_t cdict_capacity(const cdict *d)
{
    return (d != NULL) ? d->capacity : 0;
}
