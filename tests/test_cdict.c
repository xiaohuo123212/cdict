/*
 * cdict 单元测试
 *
 * 故意不依赖任何测试框架 —— 手写断言 + 计数就够了，
 * 而且失败时 main 返回非零，CI 能自动判定成败。
 */
#include <stdio.h>
#include <string.h>

#include "cdict.h"

static int g_passed = 0;
static int g_failed = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        if (cond) {                                                       \
            g_passed++;                                                   \
        } else {                                                          \
            g_failed++;                                                   \
            printf("  FAIL  %s:%d  %s\n", __FILE__, __LINE__, #cond);     \
        }                                                                 \
    } while (0)

/* ------------------------------------------------------------------ */
/* 基本功能                                                            */
/* ------------------------------------------------------------------ */
static void test_basic(void)
{
    puts("basic");

    cdict *d = cdict_new();
    CHECK(d != NULL);
    CHECK(cdict_count(d) == 0);
    CHECK(cdict_capacity(d) == 0);

    CHECK(cdict_init(d, 10));
    CHECK(cdict_count(d) == 0);
    CHECK(cdict_capacity(d) == 10);

    CHECK(cdict_add(d, "hello", 1));
    CHECK(cdict_add(d, "world", 2));
    CHECK(cdict_add(d, "c", 3));
    CHECK(cdict_count(d) == 3);

    /* 用 int 查 -> 得到字符串 */
    cdict_value r = cdict_search(d, cdv_int(2));
    CHECK(r.kind == CDV_STR);
    CHECK(r.kind == CDV_STR && strcmp(r.s, "world") == 0);

    /* 用字符串查 -> 得到 int */
    r = cdict_search(d, cdv_str("hello"));
    CHECK(r.kind == CDV_INT);
    CHECK(r.kind == CDV_INT && r.i == 1);

    /* 单字符的键也要能查到 */
    r = cdict_search(d, cdv_str("c"));
    CHECK(r.kind == CDV_INT && r.i == 3);

    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 覆盖：同一个键再插一次                                              */
/* ------------------------------------------------------------------ */
static void test_overwrite(void)
{
    puts("overwrite");

    cdict *d = cdict_new();
    cdict_init(d, 4);

    CHECK(cdict_add(d, "key", 1));
    CHECK(cdict_add(d, "key", 2));          /* 同一个键 */
    CHECK(cdict_count(d) == 2);             /* 目前是"允许重复"，见 DESIGN.md */

    /* 查找返回第一次命中的那条 */
    cdict_value r = cdict_search(d, cdv_str("key"));
    CHECK(r.kind == CDV_INT && r.i == 1);

    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 未命中：绝不能返回上一次的结果（这是本库最关键的行为）              */
/* ------------------------------------------------------------------ */
static void test_miss(void)
{
    puts("miss");

    cdict *d = cdict_new();
    cdict_init(d, 8);
    cdict_add(d, "a", 1);

    /* 先做一次命中的查找，让"上一次的结果"存在 */
    cdict_value r = cdict_search(d, cdv_str("a"));
    CHECK(r.kind == CDV_INT && r.i == 1);

    /* 紧接着做未命中的查找 —— 必须返回 NONE，而不是残留的旧结果 */
    r = cdict_search(d, cdv_int(999));
    CHECK(r.kind == CDV_NONE);

    r = cdict_search(d, cdv_str("nope"));
    CHECK(r.kind == CDV_NONE);

    /* 再命中一次，确认状态没被弄坏 */
    r = cdict_search(d, cdv_str("a"));
    CHECK(r.kind == CDV_INT && r.i == 1);

    /* 传一个 NONE 类型的 key 进去 */
    r = cdict_search(d, cdv_int(0));
    CHECK(r.kind == CDV_NONE);

    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 空字典                                                              */
/* ------------------------------------------------------------------ */
static void test_empty(void)
{
    puts("empty");

    cdict *d = cdict_new();
    cdict_init(d, 4);

    CHECK(cdict_search(d, cdv_int(1)).kind   == CDV_NONE);
    CHECK(cdict_search(d, cdv_str("x")).kind == CDV_NONE);
    CHECK(cdict_count(d) == 0);

    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 边界：容量、键长                                                    */
/* ------------------------------------------------------------------ */
static void test_capacity_limit(void)
{
    puts("capacity limit");

    cdict *d = cdict_new();
    CHECK(cdict_init(d, 2));

    CHECK(cdict_add(d, "a", 1));
    CHECK(cdict_add(d, "b", 2));
    CHECK(!cdict_add(d, "c", 3));           /* 满了 -> false，且不能越界 */
    CHECK(cdict_count(d) == 2);

    /* 满容量之后，已有的数据必须完好 */
    CHECK(cdict_search(d, cdv_str("a")).kind == CDV_INT);
    CHECK(cdict_search(d, cdv_int(2)).kind == CDV_STR);
    CHECK(cdict_search(d, cdv_str("c")).kind == CDV_NONE);

    cdict_free(d);
}

static void test_key_length(void)
{
    puts("key length");

    cdict *d = cdict_new();
    cdict_init(d, 8);

    /* 最长可接受的键：CDICT_KEY_MAX - 1 个字符 */
    char max_ok[CDICT_KEY_MAX];
    memset(max_ok, 'y', CDICT_KEY_MAX - 1);
    max_ok[CDICT_KEY_MAX - 1] = '\0';
    CHECK(cdict_add(d, max_ok, 111));

    /* 刚好超一个字符：CDICT_KEY_MAX 个字符，需要 CDICT_KEY_MAX+1 字节 */
    char too_long[CDICT_KEY_MAX + 1];
    memset(too_long, 'x', CDICT_KEY_MAX);
    too_long[CDICT_KEY_MAX] = '\0';
    CHECK(!cdict_add(d, too_long, 222));

    /* 超长键被拒绝后，不能污染已有数据 */
    cdict_value r = cdict_search(d, cdv_int(111));
    CHECK(r.kind == CDV_STR);
    CHECK(r.kind == CDV_STR && strcmp(r.s, max_ok) == 0);

    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 非法参数：全部要安全，不能崩                                        */
/* ------------------------------------------------------------------ */
static void test_invalid_args(void)
{
    puts("invalid args");

    cdict *d = cdict_new();

    /* 还没 init 就 add */
    CHECK(!cdict_add(d, "x", 1));
    CHECK(cdict_search(d, cdv_str("x")).kind == CDV_NONE);

    /* NULL 参数 */
    CHECK(!cdict_init(NULL, 10));
    CHECK(!cdict_init(d, 0));
    CHECK(!cdict_add(NULL, "x", 1));
    CHECK(!cdict_add(d, NULL, 1));

    cdict_init(d, 4);
    CHECK(!cdict_add(NULL, "x", 1));
    CHECK(!cdict_add(d, NULL, 1));
    CHECK(cdict_count(d) == 0);

    /* 对 NULL 字典查找、取计数 */
    CHECK(cdict_search(NULL, cdv_int(1)).kind == CDV_NONE);
    CHECK(cdict_count(NULL) == 0);
    CHECK(cdict_capacity(NULL) == 0);

    cdict_free(NULL);       /* 必须是安全的 no-op */
    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 重复 init：不能泄漏旧存储（靠 ASan 验证）                           */
/* ------------------------------------------------------------------ */
static void test_reinit(void)
{
    puts("re-init");

    cdict *d = cdict_new();

    CHECK(cdict_init(d, 4));
    CHECK(cdict_add(d, "a", 1));
    CHECK(cdict_count(d) == 1);

    /* 重新 init：旧存储要释放，count 要归零 */
    CHECK(cdict_init(d, 8));
    CHECK(cdict_capacity(d) == 8);
    CHECK(cdict_count(d) == 0);
    CHECK(cdict_search(d, cdv_str("a")).kind == CDV_NONE);

    cdict_free(d);
}

/* ------------------------------------------------------------------ */
/* 两个字典互不干扰                                                    */
/* ------------------------------------------------------------------ */
static void test_independent(void)
{
    puts("independent instances");

    cdict *a = cdict_new();
    cdict *b = cdict_new();
    cdict_init(a, 4);
    cdict_init(b, 4);

    cdict_add(a, "aaa", 1);
    cdict_add(b, "bbb", 2);

    CHECK(cdict_count(a) == 1);
    CHECK(cdict_count(b) == 1);

    CHECK(cdict_search(a, cdv_str("aaa")).kind == CDV_INT);
    CHECK(cdict_search(a, cdv_str("bbb")).kind == CDV_NONE);   /* b 的不该出现在 a 里 */
    CHECK(cdict_search(b, cdv_str("aaa")).kind == CDV_NONE);

    cdict_free(a);
    cdict_free(b);
}

/* ------------------------------------------------------------------ */
/* cdict_find：_Generic 泛型入口                                       */
/* ------------------------------------------------------------------ */
static void test_generic(void)
{
    puts("generic (_Generic dispatch)");

    cdict *d = cdict_new();
    cdict_init(d, 16);
    cdict_add(d, "hello", 1);
    cdict_add(d, "world", 2);
    cdict_add(d, "c", 3);

    /* int 分支 */
    cdict_value r = cdict_find(d, 2);
    CHECK(r.kind == CDV_STR);
    CHECK(r.kind == CDV_STR && strcmp(r.s, "world") == 0);

    /* 字符串【字面量】：类型是 char[6]，会退化成 char * */
    r = cdict_find(d, "hello");
    CHECK(r.kind == CDV_INT && r.i == 1);

    /* const char * 变量 —— 宏里少了这条分支就会在这里编译失败 */
    {
        const char *s = "c";
        r = cdict_find(d, s);
        CHECK(r.kind == CDV_INT && r.i == 3);
    }

    /* 非 const 的 char[] —— 同样退化成 char * */
    {
        char buf[] = "world";
        r = cdict_find(d, buf);
        CHECK(r.kind == CDV_INT && r.i == 2);
    }

    /* 已经构造好的 cdict_value：原样透传 */
    r = cdict_find(d, cdv_int(2));
    CHECK(r.kind == CDV_STR && strcmp(r.s, "world") == 0);

    /* 未命中 */
    CHECK(cdict_find(d, cdv_int(999)).kind == CDV_NONE);
    CHECK(cdict_find(d, "nope").kind       == CDV_NONE);

    /* 它只是语法糖：结果必须和 cdict_search 完全一致 */
    {
        cdict_value a = cdict_find(d, 2);
        cdict_value b = cdict_search(d, cdv_int(2));
        CHECK(a.kind == b.kind);
        CHECK(a.kind == CDV_STR && strcmp(a.s, b.s) == 0);

        cdict_value c = cdict_find(d, "hello");
        cdict_value e = cdict_search(d, cdv_str("hello"));
        CHECK(c.kind == e.kind);
        CHECK(c.kind == CDV_INT && c.i == e.i);
    }

    /* 注意：本文件【没有】覆盖「传 double / short / long 会编译报错」这种情况，
     * 因为那会让整个测试文件编译不过，它属于「编译失败测试」。
     * 手工验证：临时写一行 cdict_find(d, 3.14) 编译一下，应当报
     *     '_Generic' selector of type 'double' is not compatible with any association
     * 确认后删掉即可。 */

    cdict_free(d);
}

/* ------------------------------------------------------------------ */

int main(void)
{
    test_basic();
    test_overwrite();
    test_miss();
    test_empty();
    test_capacity_limit();
    test_key_length();
    test_invalid_args();
    test_reinit();
    test_independent();
    test_generic();

    printf("\n%d passed, %d failed\n", g_passed, g_failed);
    return g_failed ? 1 : 0;        /* 失败时返回非零，CI 才能抓到 */
}
