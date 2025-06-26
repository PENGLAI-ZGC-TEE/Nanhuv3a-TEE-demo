# wolfSSL TLS 双向认证 Demo

这是一个使用 wolfSSL 库实现的 TLS 双向认证（mutual authentication）示例项目，包含服务端和模块化客户端代码。项目模拟核电厂监控系统，提供实时数据传输、Web界面和API接口。

## 项目结构

```
tls-demo/
├── Makefile              # 编译配置文件
├── README.md             # 项目说明文档
├── generate_certs.sh     # 证书生成脚本
├── server.c              # TLS 服务端代码
├── client/               # 模块化客户端目录
│   ├── README.md         # 客户端详细说明
│   ├── client.h          # 头文件和数据结构
│   ├── main.c            # 主程序入口
│   ├── tls_client.c      # TLS客户端模块
│   ├── http_server.c     # HTTP服务器模块
│   └── data_manager.c    # 数据管理模块
└── public/               # Web界面静态文件
    └── index.html        # 核电厂监控界面
```

## 功能特性

### 核心功能
- **TLS 双向认证**: 服务端和客户端都需要验证对方的证书
- **安全通信**: 使用 TLS 1.2 协议进行加密通信
- **证书验证**: 基于 CA 证书验证通信双方的身份
- **模块化架构**: 客户端采用模块化设计，易于维护和扩展
- **错误处理**: 完善的错误处理和状态检查

### 新增功能
- **实时数据监控**: 模拟核电厂传感器数据（离心机转速、发电量）
- **Web界面**: 提供现代化的数据可视化界面
- **HTTP API**: RESTful API接口，支持数据查询
- **数据管理**: 内存中存储和管理传感器数据
- **多线程处理**: 支持并发的TLS连接和HTTP服务
- **动态端口**: HTTP服务器支持端口自动递增（从8080开始）
- **实时图表**: 使用Chart.js显示实时数据趋势

## 依赖要求

### 系统要求
- Linux 或 macOS 系统
- GCC 编译器
- OpenSSL 工具（用于生成证书）

### 编译 wolfSSL
```bash
./autogen.sh
./configure --enable-distro --enable-oldtls --enable-pkcs11
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

服务端将在端口 8443 上监听连接，并开始发送模拟的核电厂传感器数据。

#### 启动客户端

在另一个终端窗口中运行：

```bash
# 使用默认服务器地址 (127.0.0.1)
./build/client

# 或指定服务器地址
./build/client 192.168.1.100

# 查看帮助信息
./build/client --help
```

客户端将：
1. 连接到TLS服务端并进行握手
2. 启动HTTP服务器（默认端口8080，如被占用则自动递增）
3. 开始接收和存储传感器数据

### 4. 访问Web界面

客户端启动后，在浏览器中访问：

```
http://localhost:8080
```

你将看到核电厂监控界面，包括：
- 实时离心机转速图表
- 实时总发电量图表
- 自动数据更新（每2秒）
- 响应式设计

### 5. API接口测试

可以通过API接口获取JSON格式的传感器数据：

```bash
curl http://localhost:8080/api/data
```

响应示例：
```json
{
  "data": [
    {
      "centrifugeSpeed": 60000.0,
      "powerOutput": 1000.0,
      "timestamp": "2024-01-01 12:00:00"
    }
  ],
  "count": 1,
  "capacity": 1000,
  "message": "Data retrieved successfully"
}
```

## 架构说明

### 服务端 (server.c)

- 创建 SSL 上下文并配置 TLS 1.2
- 加载服务端证书和私钥
- 加载 CA 证书用于验证客户端
- 设置双向认证模式
- 监听客户端连接并处理 TLS 握手
- 发送模拟的核电厂传感器数据（离心机转速、发电量）

### 模块化客户端 (client/)

#### main.c - 主程序
- 程序入口点和命令行参数处理
- 初始化所有模块（数据管理、HTTP服务器、TLS客户端）
- 信号处理和优雅关机
- 多线程协调

#### tls_client.c - TLS客户端模块
- 创建 SSL 上下文并配置 TLS 1.2
- 加载客户端证书和私钥
- 连接服务端并进行 TLS 握手
- 在独立线程中接收传感器数据
- 解析数据并存储到数据管理器

#### http_server.c - HTTP服务器模块
- 提供HTTP服务器功能（支持端口自动递增）
- 服务静态文件（Web界面）
- 提供RESTful API接口 `/api/data`
- 支持多种MIME类型和CORS
- 多线程处理HTTP请求

#### data_manager.c - 数据管理模块
- 线程安全的传感器数据存储
- 自动管理数据容量（最多1000个数据点）
- 提供JSON格式的数据导出
- 支持数据查询和统计

#### client.h - 头文件
- 定义共享数据结构（sensor_data_t）
- 声明所有模块的函数接口
- 配置常量和宏定义

## 安全特性

1. **双向认证**: 服务端和客户端都必须提供有效的证书
2. **证书链验证**: 所有证书都由同一个 CA 签发并验证
3. **加密通信**: 所有传感器数据传输都经过 TLS 加密
4. **协议安全**: 使用 TLS 1.2 协议确保通信安全
5. **线程安全**: 数据管理模块使用互斥锁保护共享数据
6. **输入验证**: HTTP服务器对请求进行基本验证
7. **资源管理**: 自动清理SSL连接和内存资源

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

5. **HTTP端口被占用**
   ```
   HTTP server failed to bind to port 8080
   ```
   解决方案：客户端会自动尝试递增端口号，或手动释放端口

6. **Web界面无法访问**
   ```
   404 Not Found
   ```
   解决方案：确保 `public/` 目录存在且包含 `index.html` 文件

7. **API返回空数据**
   ```
   {"data": [], "count": 0}
   ```
   解决方案：检查TLS连接是否正常，服务端是否在发送数据

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

3. 测试HTTP API：
   ```bash
   curl -v http://localhost:8080/api/data
   curl -I http://localhost:8080/
   ```

4. 检查端口占用：
   ```bash
   lsof -i :8080
   lsof -i :8443
   ```

5. 查看客户端日志：
   客户端会输出详细的连接和数据接收信息

## 性能优化

1. **数据存储优化**：默认存储1000个数据点，可在 `client.h` 中调整 `MAX_DATA_POINTS`
2. **HTTP轮询间隔**：Web界面默认2秒轮询，可在 `public/index.html` 中调整
3. **线程优化**：使用合理的线程数量，避免过度创建线程
4. **内存管理**：自动清理旧数据，防止内存泄漏

## 扩展功能

1. **数据持久化**：可扩展为将数据保存到数据库
2. **多客户端支持**：服务端可支持多个客户端连接
3. **实时告警**：可添加数据阈值监控和告警功能
4. **历史数据查询**：可添加时间范围查询API
5. **用户认证**：可为Web界面添加用户登录功能

## 清理

```bash
# 清理编译文件
make clean

# 清理证书文件
make clean-certs

# 停止所有相关进程
pkill -f "./server"
pkill -f "./build/client"
```

## 许可证

本项目仅供学习和演示使用。请根据实际需求调整安全配置。

## 贡献

欢迎提交问题报告和功能请求。

## 更新日志

### v2.0 (最新)
- 重构客户端为模块化架构
- 新增HTTP服务器和Web界面
- 添加实时数据可视化
- 支持RESTful API
- 实现动态端口分配
- 添加线程安全的数据管理

### v1.0
- 基础TLS双向认证功能
- 简单的客户端-服务端通信