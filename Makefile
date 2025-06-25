# Makefile for wolfSSL TLS Demo

# 编译器配置
CC = gcc
RISCV_CC = riscv64-linux-gnu-gcc

# 基本编译标志
CFLAGS = -Wall -Wextra -std=c99
LDFLAGS = -lwolfssl

# RISC-V 工具链路径（根据实际安装路径调整）
RISCV_SYSROOT = /usr/riscv64-linux-gnu
RISCV_WOLFSSL_PATH = /opt/riscv-wolfssl

# RISC-V 特定配置
RISCV_CFLAGS = $(CFLAGS) \
    -march=rv64gc -mabi=lp64d \
    --sysroot=$(RISCV_SYSROOT) \
    -I$(RISCV_SYSROOT)/include \
    -I$(RISCV_WOLFSSL_PATH)/include

RISCV_LDFLAGS = \
    --sysroot=$(RISCV_SYSROOT) \
    -L$(RISCV_SYSROOT)/lib \
    -L$(RISCV_WOLFSSL_PATH)/lib \
    -lwolfssl -lm -static

# 目标文件
TARGETS = server client
RISCV_TARGETS = server-riscv client-riscv

# 默认目标（本地编译）
all: $(TARGETS)

# 本地编译
server: server.c
	$(CC) $(CFLAGS) -o server server.c $(LDFLAGS)

client: client.c
	$(CC) $(CFLAGS) -o client client.c $(LDFLAGS)

# RISC-V 交叉编译目标
riscv: check-riscv-env $(RISCV_TARGETS)

server-riscv: server.c
	$(RISCV_CC) $(RISCV_CFLAGS) -o server-riscv server.c $(RISCV_LDFLAGS)

client-riscv: client.c
	$(RISCV_CC) $(RISCV_CFLAGS) -o client-riscv client.c $(RISCV_LDFLAGS)

# 检查 RISC-V 环境
check-riscv-env:
	@echo "检查 RISC-V 编译环境..."
	@which $(RISCV_CC) > /dev/null || (echo "错误: 找不到 $(RISCV_CC)" && exit 1)
	@test -d $(RISCV_SYSROOT) || (echo "错误: RISC-V sysroot 不存在: $(RISCV_SYSROOT)" && exit 1)
	@echo "✓ RISC-V 编译环境检查通过"

# 生成证书
certs:
	./generate_certs.sh

# 清理目标
clean:
	rm -f $(TARGETS) $(RISCV_TARGETS)

clean-certs:
	rm -f *.pem *.key *.crt *.srl

.PHONY: all riscv clean certs clean-certs check-riscv-env
