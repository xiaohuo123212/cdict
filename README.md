# cdict

[![CI](https://github.com/xiaohuo123212/cdict/actions/workflows/ci.yml/badge.svg)](https://github.com/xiaohuo123212/cdict/actions/workflows/ci.yml)
![C11](https://img.shields.io/badge/C-C11-blue)
![License](https://img.shields.io/badge/license-MIT-green)

用 C11 写的小字典（映射）库，零依赖。支持双向查找：整数 ↔ 字符串。

> 这是一个**练手项目**，主要目的是熟悉 C 的内存管理、头文件组织、
> 不透明类型，以及 GitHub 的完整工作流。功能会继续补。

## 快速开始

```c
#include <stdio.h>
#include "cdict.h"

int main(void)
{
    cdict *d = cdict_new();
    cdict_init(d, 16);

    cdict_add(d, "hello", 1);

    cdict_value r = cdict_search(d, cdv_int(1));
    if (r.kind == CDV_STR) printf("%s\n", r.s);   /* hello */

    cdict_free(d);
    return 0;
}
```

集成只需要两个文件：`include/cdict.h` 和 `src/cdict.c`。

```sh
gcc -std=c11 -Wall -Wextra -O2 -Iinclude your_code.c src/cdict.c -o app
```

或者用仓库自带的示例和测试：

```sh
make        # 编译 examples/demo.c
make test   # 编译并运行测试
```

## API

| 函数 | 说明 | 失败时 |
|---|---|---|
| `cdict *cdict_new(void)` | 创建字典 | 返回 `NULL` |
| `void cdict_free(cdict *d)` | 释放全部内存 | 传 `NULL` 是安全的 |
| `bool cdict_init(cdict *d, size_t capacity)` | 分配容量 | 返回 `false` |
| `bool cdict_add(cdict *d, const char *word, int num)` | 插入一条映射 | 返回 `false` |
| `cdict_value cdict_search(const cdict *d, cdict_value key)` | 查找 | 返回 `kind == CDV_NONE` |
| `size_t cdict_count(const cdict *d)` | 已存条目数 | 传 `NULL` 返回 0 |

构造函数：`cdv_int(1)`、`cdv_str("hello")`

## 特点

- **不透明类型** —— 内部布局不对外暴露，使用者改不坏
- **标签联合的值类型** —— 不用 `void *`，类型信息跟着值一起走
- **完整的错误检查** —— 所有参数和边界都查过，失败用返回值报告，不崩溃
- **零警告** —— 在 `-Wall -Wextra -Wpedantic -Werror` 下编译通过

## 目前没做的

- 查找是 **O(n) 线性扫描**，条目多了应该换哈希表
- 容量固定，不能动态扩容
- 键最长 100 字节（`CDICT_KEY_MAX`）
- 非线程安全

更详细的设计取舍见 [DESIGN.md](DESIGN.md)。

## License

[MIT](LICENSE)
