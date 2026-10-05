# 设计笔记

写这个库时做过的几个决定，以及踩过的坑。随项目演进会继续补。

---

## 1. 不透明类型

`cdict` 的完整定义放在 `src/cdict.c` 里，头文件只有一个前向声明：

```c
typedef struct cdict cdict;      /* 使用者只见其名，不见其形 */
```

使用者只能拿着指针用，看不到内部，也就改不坏。

代价：必须走 `cdict_new` / `cdict_free`，不能在栈上定义。

（C 标准库的 `FILE` 就是这个套路。）

## 2. 值类型用标签联合，不用 void *

```c
typedef struct cdict_value {
    enum cdv_kind kind;              /* 我现在是什么类型 */
    union {
        int         i;
        const char *s;
    };
} cdict_value;
```

`union` 本身**不记录**当前存的是哪个成员 —— 所有成员共享同一块内存。
所以必须自己加一个 `kind`，而且要和 union 打包在一起传递。

`kind` 里特意留了一个 `CDV_NONE` 表示"没找到"。这样查找函数可以直接返回
结果，不需要额外的出参。（Python 的 `None`、Rust 的 `Option` 是同一个思路。）

> **踩过的坑**：早期版本用函数内的 `static cdict_value` 当返回值，结果
> 第二次调用会把第一次的结果覆盖掉。现在改成局部变量，每次都是独立的。

## 3. capacity 和 count 是两回事

```c
size_t count;      /* 实际写了几格 */
size_t capacity;   /* 分配了多少格 */
```

**遍历已经存了的条目必须用 `count`。** 用 `capacity` 会读到从没写过的内存 ——
轻则拿到 `malloc` 的垃圾值，重则越界读。

> **踩过的坑**：早期版本的查找循环写的是 `i < capacity`。

## 4. 去掉了函数指针表

一开始我把 `init` / `add` / `search` 做成了结构体成员（像虚函数表）：

```c
struct cdict {
    ...
    void (*init)(cdict *, int);
    bool (*add)(cdict *, const char *, int);
};
```

后来想明白了：函数指针成员**唯一的价值**是"同一套接口、多种实现"。
这个库只有一种实现，用不上这个能力，却要付全部代价 ——
结构体白白多 4 个指针、每次调用都不能内联、还得记得在 `cdict_new` 里初始化。

改成直接调函数 `cdict_add(d, ...)` 之后，`cdict_new` 从 4 行变成 1 行：

```c
cdict *cdict_new(void) { return calloc(1, sizeof(cdict)); }
```

## 5. 库不替调用者决定生死

所有函数用返回值报告失败，**不 `exit`、不打印、不崩溃**。
`cdict_free(NULL)` 和 `cdict_search(NULL, ...)` 都是安全的，
这样调用者的清理代码可以无条件写。

`cdict_init` 里是**先分配新的、成功了再释放旧的**：

```c
int *nums = malloc(...);
if (nums == NULL) { ...; return false; }   /* 失败时旧数据完好 */
free(d->numarr);                           /* 到这里才动旧的 */
d->numarr = nums;
```

顺序反了的话，一旦分配失败，字典就会处于"指针已释放但还指着旧地址"的
状态 —— 再 `cdict_free` 就是 double free。

---

## 已知限制

- 查找 O(n) 线性扫描
- 容量固定，不支持动态扩容
- 键长上限 `CDICT_KEY_MAX`（100 字节）
- 同一个键可以重复插入，查找返回第一次命中的那条
- 非线程安全

## 以后想做的

- [ ] 哈希表实现，和现在的线性版本做性能对比
- [ ] 动态扩容
- [ ] 迭代器（`cdict_foreach`）
- [ ] 值类型支持 `double`
