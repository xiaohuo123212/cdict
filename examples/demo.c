/*
 * cdict 使用示例：建一个 整数 <-> 字符串 的双向表
 *
 * 编译：
 *   gcc -std=c11 -Wall -Wextra -O2 -Iinclude examples/demo.c src/cdict.c -o demo
 */
#include <stdio.h>

#include "cdict.h"

int main(void)
{
    cdict *d = cdict_new();
    if (d == NULL) { perror("cdict_new"); return 1; }

    if (!cdict_init(d, 16)) {
        fprintf(stderr, "cdict_init 失败\n");
        cdict_free(d);
        return 1;
    }

    /* ---- 插入 ---- */
    static const char *words[] = { "zero", "one", "two", "three", "four" };
    for (int i = 0; i < 5; i++) {
        if (!cdict_add(d, words[i], i))
            fprintf(stderr, "插入 \"%s\" 失败\n", words[i]);
    }

    printf("count = %zu, capacity = %zu\n\n", cdict_count(d), cdict_capacity(d));

    /* ---- 用整数查字符串 ---- */
    puts("用整数查：");
    for (int i = 0; i < 6; i++) {
        cdict_value r = cdict_search(d, cdv_int(i));
        if (r.kind == CDV_STR) printf("  %d -> %s\n", i, r.s);
        else                   printf("  %d -> (未找到)\n", i);
    }

    /* ---- 用字符串查整数 ---- */
    puts("\n用字符串查：");
    static const char *probe[] = { "three", "zero", "nope" };
    for (int i = 0; i < 3; i++) {
        cdict_value r = cdict_search(d, cdv_str(probe[i]));
        if (r.kind == CDV_INT) printf("  \"%s\" -> %d\n", probe[i], r.i);
        else                   printf("  \"%s\" -> (未找到)\n", probe[i]);
    }

    /* ---- 容量满了会怎样 ---- */
    puts("\n超出容量：");
    cdict *small = cdict_new();
    cdict_init(small, 2);
    printf("  add(\"a\") = %d\n", cdict_add(small, "a", 1));
    printf("  add(\"b\") = %d\n", cdict_add(small, "b", 2));
    printf("  add(\"c\") = %d   <- 满了，安全地拒绝\n", cdict_add(small, "c", 3));
    cdict_free(small);

    cdict_free(d);
    return 0;
}
