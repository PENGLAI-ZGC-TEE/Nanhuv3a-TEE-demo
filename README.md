# wolfSSL TLS 双向认证 Demo

这是一个使用 wolfSSL 库实现的 TLS 双向认证（mutual authentication）示例项目，包含服务端和客户端代码。

## 项目结构

```
tls-demo/
├── Makefile              # 编译配置文件
├── README.md             # 项目说明文档
├── generate_certs.sh     # 证书生成脚本
├── server.c              # TLS 服务端代码
└── client.c              # TLS 客户端代码
```

## 功能特性

- **TLS 双向认证**: 服务端和客户端都需要验证对方的证书
- **安全通信**: 使用 TLS 1.2 协议进行加密通信
- **证书验证**: 基于 CA 证书验证通信双方的身份
- **交互式客户端**: 支持用户输入消息与服务端交互
- **错误处理**: 完善的错误处理和状态检查

## 依赖要求

### 系统要求
- Linux 或 macOS 系统
- GCC 编译器
- OpenSSL 工具（用于生成证书）

### wolfSSL 库安装

#### macOS (使用 Homebrew)
```bash
brew install wolfssl
```

#### Ubuntu/Debian
```bash
sudo apt-get update
sudo apt-get install libwolfssl-dev
```

#### 从源码编译 wolfSSL
```bash
wget https://github.com/wolfSSL/wolfssl/archive/v5.6.4-stable.tar.gz
tar -xzf v5.6.4-stable.tar.gz
cd wolfssl-5.6.4-stable
./autogen.sh
./configure --enable-opensslextra --enable-opensslall
make
sudo make install
```

## 快速开始

### 1. 生成证书

首先需要生成 TLS 双向认证所需的证书：

```bash
# 给脚本添加执行权限
chmod +x generate_certs.sh

# 生成证书
./generate_certs.sh
```

这将生成以下证书文件：
- `ca-cert.pem` - CA 根证书
- `ca-key.pem` - CA 私钥
- `server-cert.pem` - 服务端证书
- `server-key.pem` - 服务端私钥
- `client-cert.pem` - 客户端证书
- `client-key.pem` - 客户端私钥

### 2. 编译程序

```bash
# 编译服务端和客户端
make all

# 或者分别编译
make server
make client
```

### 3. 运行程序

#### 启动服务端

在一个终端窗口中运行：

```bash
./server
```

服务端将在端口 8443 上监听连接。

#### 启动客户端

在另一个终端窗口中运行：

```bash
./client
```

客户端将连接到本地服务端并进行 TLS 握手。

### 4. 测试通信

客户端启动后，你可以输入消息发送给服务端：

```
> Hello, Server!
Server response: Server echo: Hello, Server!

> This is a secure message
Server response: Server echo: This is a secure message

> quit
Closing connection...
```

## 代码说明

### 服务端 (server.c)

- 创建 SSL 上下文并配置 TLS 1.2
- 加载服务端证书和私钥
- 加载 CA 证书用于验证客户端
- 设置双向认证模式
- 监听客户端连接并处理 TLS 握手
- 回显客户端发送的消息

### 客户端 (client.c)

- 创建 SSL 上下文并配置 TLS 1.2
- 加载客户端证书和私钥
- 加载 CA 证书用于验证服务端
- 连接服务端并进行 TLS 握手
- 提供交互式界面发送消息
- 显示服务端响应

## 安全特性

1. **双向认证**: 服务端和客户端都必须提供有效的证书
2. **证书链验证**: 所有证书都由同一个 CA 签发并验证
3. **加密通信**: 所有数据传输都经过 TLS 加密
4. **协议安全**: 使用 TLS 1.2 协议确保通信安全

## 故障排除

### 常见错误

1. **证书文件不存在**
   ```
   Error loading server certificate
   ```
   解决方案：运行 `./generate_certs.sh` 生成证书

2. **wolfSSL 库未找到**
   ```
   fatal error: wolfssl/ssl.h: No such file or directory
   ```
   解决方案：安装 wolfSSL 开发库

3. **连接被拒绝**
   ```
   Connection to server failed: Connection refused
   ```
   解决方案：确保服务端正在运行

4. **TLS 握手失败**
   ```
   TLS handshake failed
   ```
   解决方案：检查证书文件是否正确生成和配置

### 调试技巧

1. 使用 `openssl s_client` 测试服务端：
   ```bash
   openssl s_client -connect localhost:8443 -cert client-cert.pem -key client-key.pem -CAfile ca-cert.pem
   ```

2. 检查证书有效性：
   ```bash
   openssl x509 -in server-cert.pem -text -noout
   openssl x509 -in client-cert.pem -text -noout
   ```

## 清理

```bash
# 清理编译文件
make clean

# 清理证书文件
make clean-certs
```

## 许可证

本项目仅用于学习和演示目的。请根据实际需求调整安全配置。