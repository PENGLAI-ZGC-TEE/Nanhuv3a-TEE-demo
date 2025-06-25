const express = require('express');
const http = require('http');
const socketIo = require('socket.io');
const cors = require('cors');
const path = require('path');
const fs = require('fs');

const app = express();
const server = http.createServer(app);
const io = socketIo(server, {
  cors: {
    origin: "*",
    methods: ["GET", "POST"]
  }
});

app.use(cors());
app.use(express.static(path.join(__dirname, 'public')));

// 存储最近的传感器数据（最多保存50个数据点）
let sensorData = [];
const MAX_DATA_POINTS = 50;

// 模拟核电厂数据生成
function generateSensorData() {
  const now = new Date();
  
  // 铀浓缩离心机转速 (典型范围: 50,000-70,000 RPM)
  // 基础转速 + 随机波动 + 周期性变化模拟运行状态
  const baseSpeed = 60000; // 基础转速 60,000 RPM
  const randomVariation = (Math.random() - 0.5) * 2000; // ±1000 RPM随机波动
  const cyclicVariation = Math.sin(Date.now() / 300000) * 1500; // 5分钟周期的±1500 RPM变化
  const centrifugeSpeed = baseSpeed + randomVariation + cyclicVariation;
  
  // 核电厂总发电量 (典型范围: 800-1200 MW)
  // 基础功率 + 随机波动 + 周期性变化模拟负载变化
  const basePower = 1000; // 基础功率 1000 MW
  const powerRandomVariation = (Math.random() - 0.5) * 100; // ±50 MW随机波动
  const powerCyclicVariation = Math.cos(Date.now() / 240000) * 80; // 4分钟周期的±80 MW变化
  const powerOutput = basePower + powerRandomVariation + powerCyclicVariation;
  
  return {
    timestamp: now.toISOString(),
    time: now.toLocaleTimeString(),
    centrifugeSpeed: Math.round(centrifugeSpeed),
    powerOutput: Math.round(powerOutput * 10) / 10
  };
}

// 从temp文件读取核电厂数据
function readNuclearDataFromFile() {
  try {
    const filePath = path.join(__dirname, 'temp');
    const fileContent = fs.readFileSync(filePath, 'utf8').trim();
    const [centrifugeSpeed, powerOutput] = fileContent.split(',').map(Number);
    
    const now = new Date();
    return {
      timestamp: now.toISOString(),
      time: now.toLocaleTimeString(),
      centrifugeSpeed: Math.round(centrifugeSpeed * 10) / 10,
      powerOutput: Math.round(powerOutput * 10) / 10
    };
  } catch (error) {
    console.error('读取temp文件失败:', error.message);
    // 如果读取失败，返回默认值
    const now = new Date();
    return {
      timestamp: now.toISOString(),
      time: now.toLocaleTimeString(),
      centrifugeSpeed: 0,
      powerOutput: 0
    };
  }
}

// 定期从文件读取核电厂数据
setInterval(() => {
  // const newData = readNuclearDataFromFile();
  const newData = generateSensorData();
  
  // 添加新数据
  sensorData.push(newData);
  
  // 保持数据点数量在限制内
  if (sensorData.length > MAX_DATA_POINTS) {
    sensorData.shift();
  }
  
  // 向所有连接的客户端发送新数据
  io.emit('sensorData', newData);
  
  console.log(`发送数据: 离心机转速=${newData.centrifugeSpeed}RPM, 总发电量=${newData.powerOutput}MW`);
}, 2000); // 每2秒读取一次数据

// Socket.IO连接处理
io.on('connection', (socket) => {
  console.log('客户端已连接:', socket.id);
  
  // 向新连接的客户端发送历史数据
  socket.emit('historicalData', sensorData);
  
  socket.on('disconnect', () => {
    console.log('客户端已断开连接:', socket.id);
  });
});

// API路由
app.get('/api/data', (req, res) => {
  res.json(sensorData);
});

app.get('/', (req, res) => {
  res.sendFile(path.join(__dirname, 'public', 'index.html'));
});

const PORT = process.env.PORT || 3000;
server.listen(PORT, () => {
  console.log(`核电厂监控系统运行在 http://localhost:${PORT}`);
});

module.exports = app;