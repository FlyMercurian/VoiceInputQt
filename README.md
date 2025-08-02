# VoiceTextEdit - 语音输入Qt控件

基于Qt和SenseVoice的语音输入控件，支持长按V键进行语音识别并自动插入文本。

## 功能特性

- 🎤 **语音输入**: 长按 `V` 键开始录音，松开后自动识别
- 🚫 **中断支持**: 按 `ESC` 键随时取消语音输入
- 🎨 **视觉反馈**: 录音时控件灰显，识别时显示处理状态
- 📊 **状态提示**: 状态栏实时显示"正在录音..."、"识别中..."等信息
- 🌐 **多语言**: 支持中文、英文、粤语、日语、韩语自动识别
- 🔧 **易集成**: 继承自QTextEdit，可直接替换现有文本控件

## 项目结构

```
VoiceInputQt/
├── APP/                    # Qt应用程序
│   ├── voicetextedit.h     # 语音输入控件头文件
│   ├── voicetextedit.cpp   # 语音输入控件实现
│   ├── mainwindow.h/cpp    # 主窗口
│   └── APP.pro             # Qt项目文件
├── SenseVoice/             # 语音识别服务
│   ├── api.py              # FastAPI服务接口
│   ├── start_service.py    # 服务启动脚本
│   ├── demo1.py            # 原始演示脚本
│   └── requirements.txt    # Python依赖
└── README.md               # 项目说明
```

## 快速开始

### 1. 环境准备

**Python环境**（SenseVoice服务）:
```bash
cd SenseVoice
pip install -r requirements.txt
```

**Qt环境**（应用程序）:
- Qt 5.15+ 或 Qt 6.x
- 支持 network、multimedia 模块
- C++11 或更高版本

### 2. 启动语音识别服务

```bash
cd SenseVoice
python start_service.py
```

服务启动后会显示：
```
==================================================
SenseVoice 语音识别服务
==================================================
✓ 依赖检查通过
✓ 模型文件检查通过
正在启动 SenseVoice 服务...
服务地址: http://127.0.0.1:8000
API文档: http://127.0.0.1:8000/docs
```

### 3. 编译运行Qt应用

```bash
cd APP
qmake APP.pro
make
./APP  # Windows下为 APP.exe
```

## 使用方法

1. **启动应用**: 确保SenseVoice服务已运行，然后启动Qt应用
2. **语音输入**: 
   - 长按 `V` 键开始录音（控件会变灰）
   - 说话内容
   - 松开 `V` 键结束录音，自动开始识别
   - 识别结果会自动插入到光标位置
3. **取消操作**: 在录音或识别过程中按 `ESC` 键取消
4. **状态监控**: 通过状态栏查看当前操作状态

## API接口

### VoiceTextEdit 类

```cpp
class VoiceTextEdit : public QTextEdit
{
public:
    VoiceTextEdit(QWidget *parent = nullptr);
    void setServiceUrl(const QString &url);  // 设置服务地址
    bool checkServiceAvailability();         // 检查服务状态

signals:
    void statusChanged(const QString &message); // 状态变化信号
};
```

### 集成示例

```cpp
// 在您的窗口中使用VoiceTextEdit
VoiceTextEdit *voiceEdit = new VoiceTextEdit(this);

// 连接状态更新
connect(voiceEdit, &VoiceTextEdit::statusChanged,
        statusBar(), &QStatusBar::showMessage);

// 设置为中央控件
setCentralWidget(voiceEdit);
```

## 配置选项

### SenseVoice服务配置

```bash
# 自定义主机和端口
python start_service.py --host 0.0.0.0 --port 8080

# 开发模式（热重载）
python start_service.py --reload

# 跳过依赖检查
python start_service.py --skip-checks
```

### Qt应用配置

```cpp
// 自定义服务地址
voiceEdit->setServiceUrl("http://192.168.1.100:8080");
```

## 技术架构

```
┌─────────────────┐    HTTP POST     ┌──────────────────┐
│   Qt Application │ ──────────────→ │  SenseVoice API  │
│                 │                 │   (FastAPI)      │
│  VoiceTextEdit  │ ←────────────── │                  │
│     控件        │    JSON响应      │   端口:8000      │
└─────────────────┘                 └──────────────────┘
```

## 故障排除

### 常见问题

**1. "语音服务未就绪"**
- 确认SenseVoice服务已启动
- 检查服务地址是否正确（默认：http://127.0.0.1:8000）
- 尝试访问 http://127.0.0.1:8000/health 检查服务状态

**2. "未找到音频输入设备"**
- 检查麦克风是否正确连接
- 确认系统音频权限设置
- 尝试在其他应用中测试麦克风

**3. "识别超时"**
- 检查网络连接
- 尝试缩短语音时长（建议10秒以内）
- 确认SenseVoice服务正常响应

**4. 编译错误**
- 确认Qt版本支持（5.15+）
- 检查是否包含network和multimedia模块
- 确认C++11支持

### 调试模式

启用Qt应用调试输出：
```cpp
// 在main.cpp中添加
QLoggingCategory::setFilterRules("*.debug=true");
```

启用SenseVoice服务详细日志：
```bash
python start_service.py --reload  # 开发模式，显示详细日志
```

## 性能优化

### 音频设置
- 采样率：16kHz（推荐）
- 声道：单声道
- 格式：16位PCM

### 网络优化
- 使用本地服务（127.0.0.1）获得最佳性能
- 识别超时设置：15秒（可调整）
- 支持请求取消和重试

## 扩展开发

### 自定义快捷键
```cpp
// 修改keyPressEvent中的按键检测
if (event->key() == Qt::Key_F1) {  // 使用F1键替代V键
    // 语音输入逻辑
}
```

### 多语言支持
```cpp
// 设置特定语言
voiceEdit->setLanguage("zh");  // 中文
voiceEdit->setLanguage("en");  // 英文
```

### 自定义样式
```cpp
// 录音时的自定义样式
voiceEdit->setRecordingStyle("background-color: #ffcccc;");
```

## 许可证

本项目基于 MIT 许可证开源。详见 LICENSE 文件。

## 贡献

欢迎提交Issue和Pull Request来改进这个项目！

## 更新日志

### v1.0.0 (2024-01-XX)
- ✨ 初始版本发布
- 🎤 支持长按V键语音输入
- 🚫 支持ESC键中断操作
- 📊 完整的状态提示系统
- 🌐 多语言自动识别
- 🔧 易于集成的Qt控件 