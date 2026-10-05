# cdict —— 构建脚本
#
#   make        构建 examples/demo
#   make test   构建并运行单元测试（失败时返回非零，可用于 CI）
#   make asan   用 AddressSanitizer + UBSan 构建并运行测试
#   make clean  清理构建产物
#
# 需要 GCC 或 Clang。Windows 用户请在 MSYS2/MinGW 环境下使用
# mingw32-make，或直接用 README 里的 gcc 命令。

CC       ?= cc
CFLAGS   ?= -std=c11 -Wall -Wextra -Wpedantic -O2
CPPFLAGS += -Iinclude

SRC := src/cdict.c
HDR := include/cdict.h

.PHONY: all test asan clean

all: demo

demo: examples/demo.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(CPPFLAGS) examples/demo.c $(SRC) -o $@

test: tests/test_cdict.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) $(CPPFLAGS) tests/test_cdict.c $(SRC) -o test_runner
	./test_runner

asan: tests/test_cdict.c $(SRC) $(HDR)
	$(CC) $(CFLAGS) -g -fno-omit-frame-pointer \
	      -fsanitize=address,undefined \
	      $(CPPFLAGS) tests/test_cdict.c $(SRC) -o test_asan
	./test_asan

clean:
	$(RM) demo test_runner test_asan
	$(RM) *.o *.obj *.exe
