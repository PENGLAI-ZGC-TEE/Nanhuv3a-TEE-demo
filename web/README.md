# 实时传感器数据可视化系统

一个基于Node.js和Socket.IO的实时传感器数据监控系统，能够实时显示温度和湿度数据，并通过动态折线图进行可视化展示。

## 功能特性

- 🌡️ **实时数据监控**: 每2秒自动更新温度和湿度数据
- 📈 **动态图表**: 使用Chart.js实现平滑的动态折线图
- 🔄 **实时通信**: 基于Socket.IO的WebSocket实时数据传输
- 📱 **响应式设计**: 支持桌面和移动设备
- 🎨 **现代UI**: 美观的渐变色彩和动画效果
- 📊 **数据历史**: 保存最近50个数据点的历史记录

## 技术栈

### 后端
- **Node.js**: 服务器运行环境
- **Express**: Web框架
- **Socket.IO**: 实时双向通信
- **CORS**: 跨域资源共享

### 前端
- **HTML5**: 页面结构
- **CSS3**: 样式和动画
- **JavaScript**: 交互逻辑
- **Chart.js**: 图表库
- **Socket.IO Client**: 客户端实时通信

## 项目结构

```
sensor-visual-web/
├── package.json          # 项目依赖配置
├── server.js            # 后端服务器
├── README.md           # 项目说明
└── public/
    └── index.html      # 前端页面
```

## 安装和运行

### 1. 安装依赖

```bash
npm install
```

### 2. 启动服务器

```bash
# 生产模式
npm start

# 开发模式（需要安装nodemon）
npm run dev
```

### 3. 访问应用

打开浏览器访问: `http://localhost:3000`

## 数据模拟

系统会自动模拟传感器数据：

- **温度**: 20-35°C范围，带有正弦波动
- **湿度**: 40-80%范围，带有余弦波动
- **更新频率**: 每2秒生成一次新数据

## API接口

### GET /api/data

获取所有历史传感器数据

**响应示例:**
```json
[
  {
    "timestamp": "2024-01-01T12:00:00.000Z",
    "time": "12:00:00",
    "temperature": 25.3,
    "humidity": 65.7
  }
]
```

## Socket.IO事件

### 客户端监听事件

- `connect`: 连接成功
- `disconnect`: 连接断开
- `historicalData`: 接收历史数据
- `sensorData`: 接收实时数据

### 数据格式

```javascript
{
  timestamp: "2024-01-01T12:00:00.000Z",  // ISO时间戳
  time: "12:00:00",                        // 本地时间字符串
  temperature: 25.3,                       // 温度值（°C）
  humidity: 65.7                           // 湿度值（%）
}
```

## 自定义配置

### 修改数据更新频率

在 `server.js` 中修改定时器间隔：

```javascript
setInterval(() => {
  // 数据生成逻辑
}, 2000); // 修改这个值（毫秒）
```

### 修改数据范围

在 `generateSensorData()` 函数中调整数据范围：

```javascript
const temperature = 20 + Math.random() * 15; // 20-35°C
const humidity = 40 + Math.random() * 40;    // 40-80%
```

### 修改历史数据保存数量

```javascript
const MAX_DATA_POINTS = 50; // 修改这个值
```

## 部署说明

### 本地部署

1. 确保已安装Node.js (版本 >= 14)
2. 克隆或下载项目代码
3. 运行 `npm install` 安装依赖
4. 运行 `npm start` 启动服务

### 生产环境部署

1. 设置环境变量 `PORT`
2. 使用PM2或其他进程管理器
3. 配置反向代理（如Nginx）
4. 启用HTTPS（推荐）

## 浏览器兼容性

- Chrome 60+
- Firefox 55+
- Safari 12+
- Edge 79+

## 许可证

MIT License

## 贡献

欢迎提交Issue和Pull Request来改进这个项目！