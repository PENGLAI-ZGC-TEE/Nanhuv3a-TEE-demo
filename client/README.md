# 模块化客户端架构

## 概述

客户端已重构为模块化架构，提供以下功能：
1. **TLS客户端** - 连接到服务器接收传感器数据
2. **HTTP服务器** - 提供网页界面和API接口
3. **数据管理** - 存储和管理传感器数据
4. **Web界面** - 实时显示核电厂监控数据

## 文件结构

```
client/
├── client.h          # 头文件，包含所有声明和数据结构
├── main.c            # 主程序入口
├── tls_client.c      # TLS客户端模块
├── http_server.c     # HTTP服务器模块
├── data_manager.c    # 数据管理模块
└── README.md         # 本文件
```

## 模块说明

### 1. client.h
- 定义了所有共享的数据结构和常量
- 包含传感器数据结构 `sensor_data_t`
- 声明所有模块的函数接口

### 2. main.c
- 程序主入口点
- 处理命令行参数
- 初始化所有模块
- 信号处理和优雅关机

### 3. tls_client.c
- 负责与TLS服务器的连接
- 处理wolfSSL的初始化和握手
- 接收传感器数据并解析
- 在独立线程中运行数据接收循环

### 4. http_server.c
- 提供HTTP服务器功能
- 服务静态文件（网页界面）
- 提供API接口 `/api/data`
- 支持多种MIME类型

### 5. data_manager.c
- 管理传感器数据的存储
- 提供线程安全的数据访问
- 生成JSON格式的API响应
- 自动管理数据容量（最多1000个数据点）

## 功能特性

### TLS连接
- 使用wolfSSL进行安全通信
- 支持客户端证书认证
- 自动重连机制
- 实时数据接收

### HTTP服务器
- 监听端口：8080
- 静态文件服务：`public/` 目录
- API端点：`/api/data`
- 跨域支持（CORS）

### 数据管理
- 内存中存储最多1000个数据点
- 自动滚动删除旧数据
- 线程安全的数据访问
- JSON格式的数据导出

### Web界面
- 实时数据可视化
- Chart.js图表显示
- 响应式设计
- 自动数据轮询（每2秒）

## 使用方法

### 编译
```bash
make build/client
```

### 运行
```bash
# 使用默认服务器地址 (127.0.0.1)
./build/client

# 指定服务器地址
./build/client 192.168.1.100

# 查看帮助
./build/client --help
```

### 访问Web界面
启动客户端后，在浏览器中访问：
- 主页面：http://localhost:8080/
- API接口：http://localhost:8080/api/data

## API接口

### GET /api/data
返回所有传感器数据的JSON格式：

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

## 配置参数

可以在 `client.h` 中修改以下配置：

```c
#define TLS_PORT 8443          // TLS服务器端口
#define HTTP_PORT 8080         // HTTP服务器端口
#define MAX_DATA_POINTS 1000   // 最大数据点数量
#define BUFFER_SIZE 1024       // 缓冲区大小
```

## 依赖项

- wolfSSL：用于TLS连接
- pthread：用于多线程
- 标准C库：socket、文件操作等

## 注意事项

1. 确保证书文件存在于 `certs/` 目录中
2. HTTP服务器需要访问 `public/` 目录中的静态文件
3. 程序使用多线程，确保系统支持pthread
4. 数据存储在内存中，程序重启后数据会丢失
5. 使用Ctrl+C可以优雅关闭程序

## 故障排除

### 编译错误
- 检查wolfSSL是否正确安装
- 确保所有头文件路径正确

### 连接问题
- 检查TLS服务器是否运行
- 验证证书文件是否存在和有效
- 检查网络连接和防火墙设置

### Web界面问题
- 确保HTTP端口8080未被占用
- 检查public目录是否存在
- 查看浏览器控制台的错误信息