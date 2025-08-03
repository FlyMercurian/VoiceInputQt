#include "voicetextedit.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QHttpMultiPart>
#include <QEventLoop>
#include <QDebug>
#include <QApplication>
#include <QJsonArray>        // 添加这一行

VoiceTextEdit::VoiceTextEdit(QWidget *parent)
    : QTextEdit(parent)
    , m_state(State::Idle)
    , m_longPressTimer(new QTimer(this))
    , m_audioInput(nullptr)
    , m_audioBuffer(new QBuffer(this))
    , m_networkManager(new QNetworkAccessManager(this))
    , m_serviceUrl("http://127.0.0.1:8000")
{
    // 配置长按计时器
    m_longPressTimer->setSingleShot(true);
    m_longPressTimer->setInterval(LONG_PRESS_DURATION);
    connect(m_longPressTimer, &QTimer::timeout, this, &VoiceTextEdit::onLongPressTimeout);
    
    // 保存原始样式
    m_originalStyleSheet = styleSheet() + "QTextEdit { font-size: 40px; }";
    
    // 设置焦点策略，确保能接收键盘事件
    setFocusPolicy(Qt::StrongFocus);

    setStyleSheet(m_originalStyleSheet);
    
    // 设置初始提示
    setPlaceholderText("长按 'V' 键开始语音输入...");
}

VoiceTextEdit::~VoiceTextEdit()
{
    if (m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
    }
}

void VoiceTextEdit::setServiceUrl(const QString &url)
{
    m_serviceUrl = url;
}

bool VoiceTextEdit::checkServiceAvailability()
{
    QNetworkRequest request(QUrl(m_serviceUrl + "/health"));
    request.setRawHeader("User-Agent", "VoiceTextEdit");
    
    QNetworkReply *reply = m_networkManager->get(request);
    
    // 设置超时
    QTimer timeoutTimer;
    timeoutTimer.setSingleShot(true);
    timeoutTimer.setInterval(3000);
    
    QEventLoop loop;
    connect(reply, &QNetworkReply::finished, &loop, &QEventLoop::quit);
    connect(&timeoutTimer, &QTimer::timeout, &loop, &QEventLoop::quit);
    
    timeoutTimer.start();
    loop.exec();
    
    bool available = (reply->error() == QNetworkReply::NoError) && !timeoutTimer.isActive();
    reply->deleteLater();
    
    return available;
}

void VoiceTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_V && !event->isAutoRepeat()) {
        if (m_state == State::Idle) {
            setState(State::WaitingForLongPress);
            m_longPressTimer->start();
            return;
        }
    } else if (event->key() == Qt::Key_Escape) {
        if (m_state != State::Idle) {
            cancelRecording();
            return;
        }
    }
    
    // 如果不是语音输入相关的按键，且不在录音状态，正常处理
    if (m_state == State::Idle) {
        QTextEdit::keyPressEvent(event);
    }
}

void VoiceTextEdit::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_V && !event->isAutoRepeat()) {
        if (m_state == State::WaitingForLongPress) {
            // 短按，取消操作
            m_longPressTimer->stop();
            setState(State::Idle);
            return;
        } else if (m_state == State::Recording) {
            // 结束录音
            stopRecording();
            return;
        }
    }
    
    // 如果不是语音输入相关的按键，且不在录音状态，正常处理
    if (m_state == State::Idle) {
        QTextEdit::keyReleaseEvent(event);
    }
}

void VoiceTextEdit::onLongPressTimeout()
{
    if (m_state == State::WaitingForLongPress) {
        startRecording();
    }
}

void VoiceTextEdit::startRecording()
{
    // // 检查服务可用性
    // if (!checkServiceAvailability()) {
    //     emit statusChanged("语音服务未就绪，请检查服务状态");
    //     setState(State::Idle);
    //     return;
    // }
    
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

void VoiceTextEdit::setState(State newState)
{
    m_state = newState;
    
    switch (m_state) {
    case State::Idle:
        setStyleSheet(m_originalStyleSheet);
        setReadOnly(false);  // 恢复可编辑状态
        break;
        
    case State::WaitingForLongPress:
        // 保持正常状态，等待长按确认
        break;
        
    case State::Recording:
        // 设置灰显效果，但保持键盘事件接收
        setStyleSheet(m_originalStyleSheet + 
                     "QTextEdit { background-color: #f0f0f0; color: #888888; }");
        setReadOnly(true);  // 使用只读模式而不是禁用控件
        break;
        
    case State::Recognizing:
        // 保持灰显，但显示处理中状态
        setStyleSheet(m_originalStyleSheet + 
                     "QTextEdit { background-color: #fff5e6; color: #666666; }");
        setReadOnly(true);  // 保持只读状态
        break;
    }
}

QAudioFormat VoiceTextEdit::setupAudioFormat()
{
    QAudioFormat format;
    format.setSampleRate(16000);        // 16kHz采样率
    format.setChannelCount(1);          // 单声道
    format.setSampleSize(16);           // Qt5: 使用setSampleSize而不是setSampleFormat
    format.setCodec("audio/pcm");       // Qt5: 设置编解码器
    format.setByteOrder(QAudioFormat::LittleEndian);  // Qt5: 字节序
    format.setSampleType(QAudioFormat::SignedInt);    // Qt5: 采样类型
    return format;
}

void VoiceTextEdit::sendRecognitionRequest(const QByteArray &audioData)
{
    // 将PCM数据转换为WAV格式
    QByteArray wavData = createWavHeader(audioData) + audioData;
    
    // 创建多部分表单数据
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // 添加音频文件部分 - 修改参数名为files
    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant("form-data; name=\"files\"; filename=\"audio.wav\""));
    audioPart.setBody(wavData);
    multiPart->append(audioPart);
    
    // 添加语言参数 - 修改参数名为lang
    QHttpPart languagePart;
    languagePart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                          QVariant("form-data; name=\"lang\""));
    languagePart.setBody("auto");
    multiPart->append(languagePart);
    
    // 添加keys参数
    QHttpPart keysPart;
    keysPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                      QVariant("form-data; name=\"keys\""));
    keysPart.setBody("audio_input");
    multiPart->append(keysPart);
    
    // 创建请求 - 修改端点URL
    QNetworkRequest request(QUrl(m_serviceUrl + "/api/v1/asr"));
    request.setRawHeader("User-Agent", "VoiceTextEdit");
    
    // 发送POST请求
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply); // 确保multiPart随reply一起删除
    
    // 设置超时
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(RECOGNITION_TIMEOUT);
    
    connect(timeoutTimer, &QTimer::timeout, [reply, this]() {
        reply->abort();
        emit statusChanged("识别超时，请重试");
        setState(State::Idle);
    });
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer]() {
        timeoutTimer->stop();
        timeoutTimer->deleteLater();
        onRecognitionFinished(reply);
    });
    
    timeoutTimer->start();
}

void VoiceTextEdit::onRecognitionFinished(QNetworkReply *reply)
{
    // 安全检查：确保reply不为空
    if (!reply) {
        qDebug() << "Error: reply is null";
        emit statusChanged("网络响应错误");
        setState(State::Idle);
        return;
    }
    
    // 获取HTTP状态码
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "HTTP Status Code:" << statusCode;
    
    // 读取响应数据（在检查错误之前）
    QByteArray responseData = reply->readAll();
    qDebug() << "Response data:" << responseData;
    
    // 确保reply被正确删除
    reply->deleteLater();
    
    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "Network error:" << reply->error() << reply->errorString();
        emit statusChanged("识别失败: " + reply->errorString());
        setState(State::Idle);
        return;
    }
    
    // 检查HTTP状态码
    if (statusCode != 200) {
        qDebug() << "HTTP error, status code:" << statusCode;
        emit statusChanged("服务器错误: HTTP " + QString::number(statusCode));
        setState(State::Idle);
        return;
    }
    
    // 检查响应数据是否为空
    if (responseData.isEmpty()) {
        qDebug() << "Empty response data";
        emit statusChanged("服务器返回空数据");
        setState(State::Idle);
        return;
    }
    
    // 解析JSON响应
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "JSON parse error:" << parseError.errorString();
        emit statusChanged("响应解析失败: " + parseError.errorString());
        setState(State::Idle);
        return;
    }
    
    QJsonObject obj = doc.object();
    QString recognizedText;
    
    qDebug() << "=========== Qt客户端解析调试信息 ===========";
    qDebug() << "JSON object keys:" << obj.keys();
    
    // 根据SenseVoice API响应格式解析
    if (obj.contains("result")) {
        QJsonArray resultArray = obj["result"].toArray();
        qDebug() << "Result array size:" << resultArray.size();
        
        if (!resultArray.isEmpty()) {
            QJsonObject firstResult = resultArray[0].toObject();
            qDebug() << "First result keys:" << firstResult.keys();
            
            // 打印所有可用的文本字段
            QString rawText = firstResult["raw_text"].toString();
            QString cleanText = firstResult["clean_text"].toString();
            QString finalText = firstResult["text"].toString();
            
            qDebug() << "🔤 原始文本 (raw_text):" << rawText;
            qDebug() << "🧹 清理文本 (clean_text):" << cleanText;
            qDebug() << "✨ 最终文本 (text):" << finalText;
            
            // 使用最终处理的text字段
            recognizedText = finalText;
            qDebug() << "📝 将要插入的文本:" << recognizedText;
            qDebug() << "📏 文本长度:" << recognizedText.length();
        }
    } else if (obj.contains("text")) {
        recognizedText = obj["text"].toString();
        qDebug() << "Direct text:" << recognizedText;
    }
    
    qDebug() << "=============================================";
    
    if (recognizedText.isEmpty()) {
        emit statusChanged("未识别到有效内容");
    } else {
        // 插入识别的文本到光标位置
        insertPlainText(recognizedText);
        emit statusChanged("识别成功");
    }
    
    setState(State::Idle);
    
    // 3秒后清除状态消息
    QTimer::singleShot(3000, [this]() {
        emit statusChanged("");
    });
}

void VoiceTextEdit::onServiceCheckFinished(QNetworkReply *reply)
{
    // 此方法预留用于异步服务检查
    reply->deleteLater();
}

QByteArray VoiceTextEdit::createWavHeader(const QByteArray &pcmData)
{
    QByteArray header;
    
    // WAV文件参数
    quint32 sampleRate = 16000;
    quint16 channels = 1;
    quint16 bitsPerSample = 16;
    quint32 dataSize = pcmData.size();
    quint32 fileSize = 36 + dataSize;
    
    // RIFF头
    header.append("RIFF");
    header.append(reinterpret_cast<const char*>(&fileSize), 4);
    header.append("WAVE");
    
    // fmt子块
    header.append("fmt ");
    quint32 fmtSize = 16;
    header.append(reinterpret_cast<const char*>(&fmtSize), 4);
    
    quint16 audioFormat = 1; // PCM
    header.append(reinterpret_cast<const char*>(&audioFormat), 2);
    header.append(reinterpret_cast<const char*>(&channels), 2);
    header.append(reinterpret_cast<const char*>(&sampleRate), 4);
    
    quint32 byteRate = sampleRate * channels * bitsPerSample / 8;
    header.append(reinterpret_cast<const char*>(&byteRate), 4);
    
    quint16 blockAlign = channels * bitsPerSample / 8;
    header.append(reinterpret_cast<const char*>(&blockAlign), 2);
    header.append(reinterpret_cast<const char*>(&bitsPerSample), 2);
    
    // data子块
    header.append("data");
    header.append(reinterpret_cast<const char*>(&dataSize), 4);
    
    return header;
}