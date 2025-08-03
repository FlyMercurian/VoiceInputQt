#include "voicerecognitionmanager.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHttpMultiPart>
#include <QNetworkRequest>
#include <QDebug>
#include <QApplication>

// 静态成员初始化
VoiceRecognitionManager* VoiceRecognitionManager::m_instance = nullptr;

VoiceRecognitionManager::VoiceRecognitionManager(QObject *parent)
    : QObject(parent)
    , m_workerThread(nullptr)
    , m_serviceUrl("http://127.0.0.1:8000")
    , m_audioInput(nullptr)
    , m_audioBuffer(new QBuffer(this))
    , m_networkManager(new QNetworkAccessManager(this))
{
    qDebug() << "🎤 VoiceRecognitionManager 构造函数";
}

VoiceRecognitionManager::~VoiceRecognitionManager()
{
    qDebug() << "🎤 VoiceRecognitionManager 析构函数";
    
    if (m_audioInput) {
        m_audioInput->stop();
        delete m_audioInput;
    }
    
    if (m_workerThread) {
        m_workerThread->quit();
        m_workerThread->wait();
        delete m_workerThread;
    }
}

VoiceRecognitionManager* VoiceRecognitionManager::instance()
{
    if (!m_instance) {
        m_instance = new VoiceRecognitionManager();
    }
    return m_instance;
}

void VoiceRecognitionManager::initialize()
{
    qDebug() << "🎤 初始化 VoiceRecognitionManager";
    
    // 创建工作线程
    m_workerThread = new QThread();
    
    // 将当前对象移到工作线程
    this->moveToThread(m_workerThread);
    
    // 启动工作线程
    m_workerThread->start();
    
    qDebug() << "🎤 工作线程已启动，线程ID:" << m_workerThread->currentThreadId();
}

void VoiceRecognitionManager::setServiceUrl(const QString &url)
{
    m_serviceUrl = url;
    qDebug() << "🎤 设置服务URL:" << url;
}

void VoiceRecognitionManager::startRecording(const QString &requestId)
{
    qDebug() << "🎤 开始录音，请求ID:" << requestId;
    m_currentRequestId = requestId;
    
    emit statusChanged("正在录音...");
    emit recognitionStarted();
    
    // 配置音频格式
    QAudioFormat format = setupAudioFormat();
    
    // 获取默认音频输入设备
    QAudioDeviceInfo audioDevice = QAudioDeviceInfo::defaultInputDevice();
    if (audioDevice.isNull()) {
        emit recognitionError("未找到音频输入设备");
        return;
    }
    
    // 检查格式支持
    if (!audioDevice.isFormatSupported(format)) {
        format = audioDevice.nearestFormat(format);
    }

    // 创建音频输入
    if (m_audioInput) {
        delete m_audioInput;
    }
    m_audioInput = new QAudioInput(audioDevice, format, this);
    
    // 准备音频缓冲区
    m_audioData.clear();
    m_audioBuffer->setBuffer(&m_audioData);
    m_audioBuffer->open(QIODevice::WriteOnly);
    
    // 开始录音
    m_audioInput->start(m_audioBuffer);
    
    if (m_audioInput->state() != QAudio::ActiveState) {
        emit recognitionError("无法启动音频录制");
        return;
    }
    
    qDebug() << "🎤 录音已开始，音频格式:" << format;
}

void VoiceRecognitionManager::stopRecording()
{
    qDebug() << "🎤 停止录音";
    
    if (m_audioInput) {
        m_audioInput->stop();
        m_audioBuffer->close();
        
        delete m_audioInput;
        m_audioInput = nullptr;
    }
    
    if (m_audioData.isEmpty()) {
        emit recognitionError("未录制到音频数据");
        return;
    }
    
    emit statusChanged("识别中...");
    
    // 发送识别请求
    sendRecognitionRequest(m_audioData);
}

void VoiceRecognitionManager::cancelRecording()
{
    qDebug() << "🎤 取消录音";
    
    if (m_audioInput) {
        m_audioInput->stop();
        m_audioBuffer->close();
        delete m_audioInput;
        m_audioInput = nullptr;
    }
    
    // 取消网络请求
    m_networkManager->clearAccessCache();
    
    emit statusChanged("语音输入已取消");
}

QAudioFormat VoiceRecognitionManager::setupAudioFormat()
{
    QAudioFormat format;
    format.setSampleRate(16000);        // 16kHz采样率
    format.setChannelCount(1);          // 单声道
    format.setSampleSize(16);           // Qt5: 使用setSampleSize
    format.setCodec("audio/pcm");       // Qt5: 设置编解码器
    format.setByteOrder(QAudioFormat::LittleEndian);  // Qt5: 字节序
    format.setSampleType(QAudioFormat::SignedInt);    // Qt5: 采样类型
    return format;
}

void VoiceRecognitionManager::sendRecognitionRequest(const QByteArray &audioData)
{
    qDebug() << "🎤 发送识别请求，音频数据大小:" << audioData.size();
    
    // 将PCM数据转换为WAV格式
    QByteArray wavData = createWavHeader(audioData) + audioData;
    
    // 创建多部分表单数据
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    
    // 添加音频文件部分
    QHttpPart audioPart;
    audioPart.setHeader(QNetworkRequest::ContentTypeHeader, QVariant("audio/wav"));
    audioPart.setHeader(QNetworkRequest::ContentDispositionHeader, 
                       QVariant("form-data; name=\"files\"; filename=\"audio.wav\""));
    audioPart.setBody(wavData);
    multiPart->append(audioPart);
    
    // 添加语言参数
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
    
    // 创建请求
    QNetworkRequest request(QUrl(m_serviceUrl + "/api/v1/asr"));
    request.setRawHeader("User-Agent", "VoiceRecognitionManager");
    
    // 发送POST请求
    QNetworkReply *reply = m_networkManager->post(request, multiPart);
    multiPart->setParent(reply);
    
    // 设置超时
    QTimer *timeoutTimer = new QTimer(this);
    timeoutTimer->setSingleShot(true);
    timeoutTimer->setInterval(RECOGNITION_TIMEOUT);
    
    connect(timeoutTimer, &QTimer::timeout, [reply, this]() {
        reply->abort();
        emit recognitionError("识别超时，请重试");
    });
    
    connect(reply, &QNetworkReply::finished, this, [this, reply, timeoutTimer]() {
        timeoutTimer->stop();
        timeoutTimer->deleteLater();
        onRecognitionReplyFinished();
    });
    
    // 存储reply引用，供finished槽函数使用
    reply->setProperty("voiceReply", true);
    
    timeoutTimer->start();
}

void VoiceRecognitionManager::onRecognitionReplyFinished()
{
    // 获取reply对象
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    if (!reply) {
        // 如果从sender()获取失败，从networkManager获取
        auto replies = m_networkManager->findChildren<QNetworkReply*>();
        for (auto r : replies) {
            if (r->property("voiceReply").toBool()) {
                reply = r;
                break;
            }
        }
    }
    
    if (!reply) {
        qDebug() << "🎤 错误：无法获取网络响应对象";
        emit recognitionError("网络响应错误");
        return;
    }
    
    // 获取HTTP状态码
    int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    qDebug() << "🎤 HTTP状态码:" << statusCode;
    
    // 读取响应数据
    QByteArray responseData = reply->readAll();
    qDebug() << "🎤 响应数据:" << responseData;
    
    // 确保reply被正确删除
    reply->deleteLater();
    
    // 检查网络错误
    if (reply->error() != QNetworkReply::NoError) {
        qDebug() << "🎤 网络错误:" << reply->error() << reply->errorString();
        emit recognitionError("识别失败: " + reply->errorString());
        return;
    }
    
    // 检查HTTP状态码
    if (statusCode != 200) {
        qDebug() << "🎤 HTTP错误，状态码:" << statusCode;
        emit recognitionError("服务器错误: HTTP " + QString::number(statusCode));
        return;
    }
    
    // 检查响应数据是否为空
    if (responseData.isEmpty()) {
        qDebug() << "🎤 空响应数据";
        emit recognitionError("服务器返回空数据");
        return;
    }
    
    // 解析JSON响应
    QJsonParseError parseError;
    QJsonDocument doc = QJsonDocument::fromJson(responseData, &parseError);
    
    if (parseError.error != QJsonParseError::NoError) {
        qDebug() << "🎤 JSON解析错误:" << parseError.errorString();
        emit recognitionError("响应解析失败: " + parseError.errorString());
        return;
    }
    
    QJsonObject obj = doc.object();
    QString recognizedText;
    
    qDebug() << "🎤 =========== 识别管理器解析结果 ===========";
    qDebug() << "🎤 JSON键:" << obj.keys();
    
    // 解析识别结果
    if (obj.contains("result")) {
        QJsonArray resultArray = obj["result"].toArray();
        qDebug() << "🎤 结果数组大小:" << resultArray.size();
        
        if (!resultArray.isEmpty()) {
            QJsonObject firstResult = resultArray[0].toObject();
            qDebug() << "🎤 第一个结果的键:" << firstResult.keys();
            
            QString rawText = firstResult["raw_text"].toString();
            QString cleanText = firstResult["clean_text"].toString();
            QString finalText = firstResult["text"].toString();
            
            qDebug() << "🎤 🔤 原始文本:" << rawText;
            qDebug() << "🎤 🧹 清理文本:" << cleanText;
            qDebug() << "🎤 ✨ 最终文本:" << finalText;
            
            recognizedText = finalText;
        }
    }
    
    qDebug() << "🎤 =============================================";
    
    if (recognizedText.isEmpty()) {
        emit recognitionError("未识别到有效内容");
    } else {
        qDebug() << "🎤 ✅ 识别成功，发送结果:" << recognizedText;
        emit recognitionFinished(recognizedText, m_currentRequestId);
        emit statusChanged("识别成功");
        
        // 3秒后清除状态消息
        QTimer::singleShot(3000, [this]() {
            emit statusChanged("");
        });
    }
}

QByteArray VoiceRecognitionManager::createWavHeader(const QByteArray &pcmData)
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