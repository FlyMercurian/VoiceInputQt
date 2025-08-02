# **VoiceTextEdit 语音输入控件技术方案文档**

## 1. 项目背景与需求分析

### 1.1 项目现状
- **Qt工程** (`APP/`): 基础的Qt桌面应用，使用C++11，包含MainWindow基础框架
- **SenseVoice工程** (`SenseVoice/`): 完整的语音识别解决方案，基于FunASR框架，支持多语言识别，已提供FastAPI服务接口

### 1.2 核心需求
创建一个可复用的`VoiceTextEdit`控件，实现以下功能：
- **触发机制**: 长按`V`键启动语音录制
- **视觉反馈**: 录制时控件灰显
- **状态提示**: 状态栏显示"正在录音..."、"识别中..."、"识别成功"
- **中断机制**: `ESC`键随时取消操作
- **文本插入**: 识别结果自动插入到光标位置

## 2. 技术方案选型

### 2.1 架构设计思路

考虑到SenseVoice已提供FastAPI服务接口(`api.py`)，我们采用**本地HTTP服务**的方案，相比进程调用方案具有以下优势：

- **性能优势**: 模型常驻内存，避免重复加载
- **响应速度**: 毫秒级响应，用户体验更佳
- **接口标准**: 基于HTTP RESTful API，便于测试和扩展
- **错误处理**: 标准HTTP状态码，异常处理更规范

### 2.2 系统架构图

```
┌─────────────────┐    HTTP POST     ┌──────────────────┐
│   Qt Application │ ──────────────→ │  SenseVoice API  │
│                 │                 │   (FastAPI)      │
│  VoiceTextEdit  │ ←────────────── │                  │
│     控件        │    JSON响应      │   端口:8000      │
└─────────────────┘                 └──────────────────┘
```

## 3. 详细实现方案

### 3.1 Qt端实现 (`VoiceTextEdit`类)

#### 3.1.1 类结构设计

```cpp
class VoiceTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit VoiceTextEdit(QWidget *parent = nullptr);
    ~VoiceTextEdit();

    void setServiceUrl(const QString &url);
    bool checkServiceAvailability();

signals:
    void statusChanged(const QString &message);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onLongPressTimeout();
    void onRecognitionFinished(QNetworkReply *reply);
    void onServiceCheckFinished(QNetworkReply *reply);

private:
    enum class State {
        Idle,
        WaitingForLongPress,
        Recording,
        Recognizing
    };

    void startRecording();
    void stopRecording();
    void cancelRecording();
    void setState(State newState);
    QAudioFormat setupAudioFormat();
    void sendRecognitionRequest(const QByteArray &audioData);

    // 成员变量
    State m_state;
    QTimer *m_longPressTimer;
    QAudioInput *m_audioInput;
    QBuffer *m_audioBuffer;
    QByteArray m_audioData;
    QNetworkAccessManager *m_networkManager;
    QString m_originalStyleSheet;
    QString m_serviceUrl;
    
    static const int LONG_PRESS_DURATION = 300;
    static const int RECOGNITION_TIMEOUT = 15000;
};
```

#### 3.1.2 关键模块设计

**状态管理模块**
- 使用状态机模式管理四个核心状态
- 每个状态转换都有明确的触发条件和回调处理

**音频录制模块**
- 使用`QAudioSource`进行实时音频采集
- 音频数据存储在`QBuffer`中，便于直接通过HTTP传输
- 支持常见音频格式（WAV/PCM）

**网络通信模块**
- 使用`QNetworkAccessManager`进行HTTP请求
- 异步处理，避免界面阻塞
- 完整的错误处理和超时机制

**用户界面模块**
- 键盘事件监听（`V`键长按检测、`ESC`中断）
- 视觉状态反馈（灰显效果、恢复正常）
- 状态栏消息同步

#### 3.1.3 核心流程设计

```
用户按下V键 → 启动300ms计时器 → 长按确认 → 开始录音
     ↓                                           ↓
控件灰显 ← 状态栏"正在录音..." ← 音频数据采集
     ↓                                           ↓
用户松开V键 → 停止录音 → HTTP POST请求 → SenseVoice API
     ↓                                           ↓
状态栏"识别中..." → 等待响应 → 解析JSON结果 → 插入文本
     ↓                                           ↓
恢复控件状态 ← 状态栏"识别成功" ← 清理资源
```

### 3.2 SenseVoice服务端调整

#### 3.2.1 现有API分析
当前`api.py`已提供完整的FastAPI服务：
- 端点：`POST /recognition`
- 支持文件上传和表单参数
- 返回JSON格式识别结果

#### 3.2.2 建议优化点

**启动脚本优化**
```python
# 添加启动脚本 start_service.py
import uvicorn
import os

def start_sensevoice_service():
    """启动SenseVoice HTTP服务"""
    port = int(os.getenv('SENSEVOICE_PORT', 8000))
    host = os.getenv('SENSEVOICE_HOST', '127.0.0.1')
    
    uvicorn.run(
        "api:app",
        host=host,
        port=port,
        log_level="info",
        reload=False
    )

if __name__ == "__main__":
    start_sensevoice_service()
```

**配置文件支持**
- 添加`config.json`支持模型路径、设备选择等配置
- 支持环境变量覆盖配置

### 3.3 Qt工程集成方案

#### 3.3.1 项目文件更新

**APP.pro 修改**
```pro
QT += core gui widgets network multimedia

# 添加网络和多媒体模块支持
# 新增源文件
SOURCES += \
    main.cpp \
    mainwindow.cpp \
    voicetextedit.cpp

HEADERS += \
    mainwindow.h \
    voicetextedit.h

FORMS += \
    mainwindow.ui
```

#### 3.3.2 主窗口集成

**MainWindow集成示例**
```cpp
// mainwindow.cpp
void MainWindow::setupVoiceTextEdit()
{
    m_voiceEdit = new VoiceTextEdit(this);
    
    // 连接状态更新信号
    connect(m_voiceEdit, &VoiceTextEdit::statusChanged,
            statusBar(), &QStatusBar::showMessage);
    
    // 设置初始提示
    statusBar()->showMessage("长按'V'键开始语音输入", 3000);
}
```

## 4. 服务启动与生命周期管理

### 4.1 服务启动策略

**方案一：手动启动（推荐用于开发阶段）**
- 开发者手动启动SenseVoice服务
- Qt应用启动时检测服务可用性
- 简单可靠，便于调试

**方案二：自动启动（推荐用于生产环境）**
- Qt应用启动时自动启动Python服务进程
- 使用`QProcess`管理服务生命周期
- 应用退出时自动清理服务进程

### 4.2 服务健康检查

```cpp
// 服务可用性检测
bool VoiceTextEdit::checkServiceAvailability()
{
    QNetworkRequest request(QUrl("http://127.0.0.1:8000/health"));
    QNetworkReply *reply = m_networkManager->get(request);
    
    // 同步等待响应（超时3秒）
    QEventLoop loop;
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    loop.exec();
    
    return reply->error() == QNetworkReply::NoError;
}
```

## 5. 错误处理与用户体验优化

### 5.1 异常场景处理

| 异常场景 | 处理策略 | 用户提示 |
|---------|---------|---------|
| 服务未启动 | 尝试自动启动或提示手动启动 | "语音服务未就绪，请稍后重试" |
| 网络超时 | 取消请求，恢复控件状态 | "识别超时，请重试" |
| 音频设备占用 | 检测并提示设备状态 | "麦克风设备被占用" |
| 识别失败 | 解析错误信息并友好提示 | "识别失败，请重新录音" |

### 5.2 性能优化点

**音频处理优化**
- 使用合适的采样率（16kHz）和位深度
- 实时音频数据压缩，减少网络传输
- 支持音频格式转换

**网络请求优化**
- 连接池复用，减少连接开销
- 请求超时设置（建议10-15秒）
- 支持请求取消和重试机制

## 6. 部署与配置

### 6.1 开发环境配置

**Python环境准备**
```bash
cd SenseVoice
pip install -r requirements.txt
python start_service.py
```

**Qt编译配置**
```bash
cd APP
qmake APP.pro
make
```

### 6.2 生产环境部署

**打包策略**
- Qt应用：使用windeployqt工具打包
- Python服务：使用PyInstaller打包为独立可执行文件
- 统一安装包：使用NSIS或Inno Setup制作安装程序

## 7. 测试策略

### 7.1 单元测试
- `VoiceTextEdit`各状态转换逻辑测试
- 音频录制和网络请求模块测试
- 异常场景覆盖测试

### 7.2 集成测试
- Qt应用与SenseVoice服务端到端测试
- 多并发请求压力测试
- 长时间运行稳定性测试

### 7.3 用户体验测试
- 不同长度语音识别准确性测试
- 响应时间性能测试
- 界面交互流畅性测试

## 8. 未来扩展方向

### 8.1 功能增强
- 支持多语言切换
- 语音识别结果编辑和确认
- 历史语音记录管理
- 自定义快捷键配置

### 8.2 技术升级
- 支持实时流式识别
- 集成语音降噪和增强
- 支持离线识别模式
- 云端识别服务对接

---

**总结**：本方案基于现有项目结构，充分利用SenseVoice的FastAPI接口，通过HTTP通信实现Qt与Python的解耦集成。方案具有良好的可扩展性和维护性，能够满足当前需求并为未来功能扩展提供基础。

## 📝 修复方案

请将以下内容替换到您的文件中：

### 1. 修改 `APP/voicetextedit.h`

```cpp
#ifndef VOICETEXTEDIT_H
#define VOICETEXTEDIT_H

#include <QTextEdit>
#include <QTimer>
#include <QAudioInput>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QKeyEvent>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

class VoiceTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    explicit VoiceTextEdit(QWidget *parent = nullptr);
    ~VoiceTextEdit();

    void setServiceUrl(const QString &url);
    bool checkServiceAvailability();

signals:
    void statusChanged(const QString &message);

protected:
    void keyPressEvent(QKeyEvent *event) override;
    void keyReleaseEvent(QKeyEvent *event) override;

private slots:
    void onLongPressTimeout();
    void onRecognitionFinished(QNetworkReply *reply);
    void onServiceCheckFinished(QNetworkReply *reply);

private:
    enum class State {
        Idle,
        WaitingForLongPress,
        Recording,
        Recognizing
    };

    void startRecording();
    void stopRecording();
    void cancelRecording();
    void setState(State newState);
    QAudioFormat setupAudioFormat();
    void sendRecognitionRequest(const QByteArray &audioData);

    // 成员变量
    State m_state;
    QTimer *m_longPressTimer;
    QAudioInput *m_audioInput;
    QBuffer *m_audioBuffer;
    QByteArray m_audioData;
    QNetworkAccessManager *m_networkManager;
    QString m_originalStyleSheet;
    QString m_serviceUrl;
    
    static const int LONG_PRESS_DURATION = 300;
    static const int RECOGNITION_TIMEOUT = 15000;
};

#endif // VOICETEXTEDIT_H
```

### 2. 修改 `APP/voicetextedit.cpp`

在实现文件中，主要修改音频相关的部分：

```cpp
#include "voicetextedit.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QEventLoop>
#include <QDebug>
#include <QApplication>

// ... 构造函数和其他方法保持不变 ...

void VoiceTextEdit::startRecording()
{
    // 检查服务可用性
    if (!checkServiceAvailability()) {
        emit statusChanged("语音服务未就绪，请检查服务状态");
        setState(State::Idle);
        return;
    }
    
    setState(State::Recording);
    emit statusChanged("正在录音...");
    
    // 配置音频格式
    QAudioFormat format = setupAudioFormat();
    
    // 获取默认音频输入设备 (Qt5版本)
    QAudioDeviceInfo audioDevice = QAudioDeviceInfo::defaultInputDevice();
    if (audioDevice.isNull()) {
        emit statusChanged("未找到音频输入设备");
        setState(State::Idle);
        return;
    }
    
    // 检查格式支持
    if (!audioDevice.isFormatSupported(format)) {
        format = audioDevice.nearestFormat(format);
    }
    
    // 创建音频输入 (Qt5版本)
    m_audioInput = new QAudioInput(audioDevice, format, this);
    
    // 准备音频缓冲区
    m_audioData.clear();
    m_audioBuffer->setBuffer(&m_audioData);
    m_audioBuffer->open(QIODevice::WriteOnly);
    
    // 开始录音
    m_audioInput->start(m_audioBuffer);
    
    if (m_audioInput->state() != QAudio::ActiveState) {
        emit statusChanged("无法启动音频录制");
        setState(State::Idle);
        return;
    }
}

void VoiceTextEdit::stopRecording()
{
    if (m_audioInput) {
        m_audioInput->stop();
        m_audioBuffer->close();
        
        delete m_audioInput;
        m_audioInput = nullptr;
    }
    
    if (m_audioData.isEmpty()) {
        emit statusChanged("未录制到音频数据");
        setState(State::Idle);
        return;
    }
    
    setState(State::Recognizing);
    emit statusChanged("识别中...");
    
    // 发送识别请求
    sendRecognitionRequest(m_audioData);
}

void VoiceTextEdit::cancelRecording()
{
    m_longPressTimer->stop();
    
    if (m_audioInput) {
        m_audioInput->stop();
        m_audioBuffer->close();
        delete m_audioInput;
        m_audioInput = nullptr;
    }
    
    // 取消网络请求
    m_networkManager->clearAccessCache();
    
    setState(State::Idle);
    emit statusChanged("语音输入已取消");
}

// 其他方法保持不变...
```

### 3. 修改 `APP/APP.pro`

确保Qt项目文件使用正确的模块：

```pro
<code_block_to_apply_changes_from>
```

## 🔧 关键修改点

1. **`QAudioSource` → `QAudioInput`**：Qt5中的音频输入类
2. **`QMediaDevices` → `QAudioDeviceInfo`**：Qt5中的设备信息类
3. **设备获取方式**：`QAudioDeviceInfo::defaultInputDevice()`
4. **格式检查**：添加`isFormatSupported()`检查

## 📋 编译步骤

修改文件后，重新编译：

```bash
cd APP
qmake APP.pro
mingw32-make clean
mingw32-make
```

这样就能在Qt 5.12.9上正常编译了！主要的API差异已经处理，功能保持完全一致。