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

// 模拟传感器数据生成
function generateSensorData() {
  const now = new Date();
  const temperature = 20 + Math.random() * 5 + Math.sin(Date.now() / 60000) * 5; // 20-35°C范围
  const humidity = 40 + Math.random() * 10 + Math.cos(Date.now() / 80000) * 10; // 40-80%范围
  
  return {
    timestamp: now.toISOString(),
    time: now.toLocaleTimeString(),
    temperature: Math.round(temperature * 10) / 10,
    humidity: Math.round(humidity * 10) / 10
  };
}

// 从temp文件读取传感器数据
function readSensorDataFromFile() {
  try {
    const filePath = path.join(__dirname, 'temp');
    const fileContent = fs.readFileSync(filePath, 'utf8').trim();
    const [temperature, humidity] = fileContent.split(',').map(Number);
    
    const now = new Date();
    return {
      timestamp: now.toISOString(),
      time: now.toLocaleTimeString(),
      temperature: Math.round(temperature * 10) / 10,
      humidity: Math.round(humidity * 10) / 10
    };
  } catch (error) {
    console.error('读取temp文件失败:', error.message);
    // 如果读取失败，返回默认值
    const now = new Date();
    return {
      timestamp: now.toISOString(),
      time: now.toLocaleTimeString(),
      temperature: 0,
      humidity: 0
    };
  }
}

// 定期从文件读取传感器数据
setInterval(() => {
  const newData = generateSensorData();
  // const newData = readSensorDataFromFile();
  
  // 添加新数据
  sensorData.push(newData);
  
  // 保持数据点数量在限制内
  if (sensorData.length > MAX_DATA_POINTS) {
    sensorData.shift();
  }
  
  // 向所有连接的客户端发送新数据
  io.emit('sensorData', newData);
  
  console.log(`发送数据: 温度=${newData.temperature}°C, 湿度=${newData.humidity}%`);
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
  console.log(`服务器运行在 http://localhost:${PORT}`);
});

module.exports = app;