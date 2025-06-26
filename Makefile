# Makefile for wolfSSL TLS Demo

# 编译器配置
CC = gcc
RISCV_CC = riscv64-linux-gnu-gcc

# 目录配置
BUILD_DIR = build
CERTS_DIR = certs

WOLFSSL_PATH = ../opt/wolfssl

# RISC-V 工具链路径（根据实际安装路径调整）
RISCV_SYSROOT = /usr/riscv64-linux-gnu
RISCV_WOLFSSL_PATH = ../opt/riscv-wolfssl

# 基本编译标志
CFLAGS = -Wall -Wextra -std=c99 \
	-I$(WOLFSSL_PATH)/include

LDFLAGS = -lwolfssl -lm -static \
	-L$(WOLFSSL_PATH)/lib

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
TARGETS = $(BUILD_DIR)/server $(BUILD_DIR)/client
RISCV_TARGETS = $(BUILD_DIR)/server-riscv $(BUILD_DIR)/client-riscv

# 默认目标（本地编译）
all: $(BUILD_DIR) $(TARGETS)

# 创建构建目录
$(BUILD_DIR):
	@mkdir -p $(BUILD_DIR)

# 创建证书目录
$(CERTS_DIR):
	@mkdir -p $(CERTS_DIR)

# 本地编译
$(BUILD_DIR)/server: server.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ server.c $(LDFLAGS)

$(BUILD_DIR)/client: client.c | $(BUILD_DIR)
	$(CC) $(CFLAGS) -o $@ client.c $(LDFLAGS)

# RISC-V 交叉编译目标
riscv: check-riscv-env $(BUILD_DIR) $(RISCV_TARGETS)

$(BUILD_DIR)/server-riscv: server.c | $(BUILD_DIR)
	$(RISCV_CC) $(RISCV_CFLAGS) -o $@ server.c $(RISCV_LDFLAGS)

$(BUILD_DIR)/client-riscv: client.c | $(BUILD_DIR)
	$(RISCV_CC) $(RISCV_CFLAGS) -o $@ client.c $(RISCV_LDFLAGS)

# 检查 RISC-V 环境
check-riscv-env:
	@echo "检查 RISC-V 编译环境..."
	@which $(RISCV_CC) > /dev/null || (echo "错误: 找不到 $(RISCV_CC)" && exit 1)
	@test -d $(RISCV_SYSROOT) || (echo "错误: RISC-V sysroot 不存在: $(RISCV_SYSROOT)" && exit 1)
	@echo "✓ RISC-V 编译环境检查通过"

# 生成证书
certs: $(CERTS_DIR)
	./generate_certs.sh

# 清理目标
clean:
	rm -rf $(BUILD_DIR)

clean-certs:
	rm -rf $(CERTS_DIR)

# 完全清理
clean-all: clean clean-certs

# 安装（可选）
install: all
	@echo "安装到系统目录..."
	cp $(BUILD_DIR)/server /usr/local/bin/ 2>/dev/null || echo "需要sudo权限安装到/usr/local/bin"
	cp $(BUILD_DIR)/client /usr/local/bin/ 2>/dev/null || echo "需要sudo权限安装到/usr/local/bin"

# 运行服务器
run-server: $(BUILD_DIR)/server certs
	cd $(CERTS_DIR) && ../$(BUILD_DIR)/server

# 运行客户端
run-client: $(BUILD_DIR)/client certs
	cd $(CERTS_DIR) && ../$(BUILD_DIR)/client

.PHONY: all riscv clean certs clean-certs clean-all check-riscv-env install run-server run-client
