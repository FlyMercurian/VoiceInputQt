#ifndef VOICERECOGNITIONMANAGER_H
#define VOICERECOGNITIONMANAGER_H

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QAudioInput>
#include <QBuffer>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QAudioFormat>
#include <QAudioDeviceInfo>

/**
 * 函数名称：`VoiceRecognitionManager`
 * 功能描述：语音识别管理器，运行在独立线程中处理所有语音识别逻辑
 * 设计模式：单例模式，全局唯一实例
 * 线程安全：所有操作通过信号槽机制保证线程安全
 */
class VoiceRecognitionManager : public QObject
{
    Q_OBJECT

public:
    /**
     * 函数名称：`instance`
     * 功能描述：获取单例实例
     * 参数说明：无
     * 返回值：VoiceRecognitionManager*，单例指针
     */
    static VoiceRecognitionManager* instance();

    /**
     * 函数名称：`initialize`
     * 功能描述：初始化管理器，设置工作线程
     * 参数说明：无
     * 返回值：void
     */
    void initialize();

    /**
     * 函数名称：`setServiceUrl`
     * 功能描述：设置语音识别服务URL
     * 参数说明：
     *     - url：QString，服务地址
     * 返回值：void
     */
    void setServiceUrl(const QString &url);

public slots:
    /**
     * 函数名称：`startRecording`
     * 功能描述：开始录音（由UI控件调用）
     * 参数说明：
     *     - requestId：QString，请求ID，用于标识来源控件
     * 返回值：void
     */
    void startRecording(const QString &requestId = "");

    /**
     * 函数名称：`stopRecording`
     * 功能描述：停止录音并开始识别
     * 参数说明：无
     * 返回值：void
     */
    void stopRecording();

    /**
     * 函数名称：`cancelRecording`
     * 功能描述：取消录音
     * 参数说明：无
     * 返回值：void
     */
    void cancelRecording();

signals:
    /**
     * 信号名称：`recognitionStarted`
     * 功能描述：录音开始信号
     * 参数说明：无
     */
    void recognitionStarted();

    /**
     * 信号名称：`recognitionFinished`
     * 功能描述：识别完成信号
     * 参数说明：
     *     - text：QString，识别结果文本
     *     - requestId：QString，请求ID
     */
    void recognitionFinished(const QString &text, const QString &requestId);

    /**
     * 信号名称：`recognitionError`
     * 功能描述：识别错误信号
     * 参数说明：
     *     - error：QString，错误信息
     */
    void recognitionError(const QString &error);

    /**
     * 信号名称：`statusChanged`
     * 功能描述：状态变化信号
     * 参数说明：
     *     - status：QString，状态描述
     */
    void statusChanged(const QString &status);

private slots:
    void onRecognitionReplyFinished();

private:
    explicit VoiceRecognitionManager(QObject *parent = nullptr);
    ~VoiceRecognitionManager();

    /**
     * 函数名称：`setupAudioFormat`
     * 功能描述：配置音频格式
     * 参数说明：无
     * 返回值：QAudioFormat，音频格式配置
     */
    QAudioFormat setupAudioFormat();

    /**
     * 函数名称：`sendRecognitionRequest`
     * 功能描述：发送识别请求到服务器
     * 参数说明：
     *     - audioData：QByteArray，音频数据
     * 返回值：void
     */
    void sendRecognitionRequest(const QByteArray &audioData);

    /**
     * 函数名称：`createWavHeader`
     * 功能描述：创建WAV文件头
     * 参数说明：
     *     - pcmData：QByteArray，PCM音频数据
     * 返回值：QByteArray，WAV文件头
     */
    QByteArray createWavHeader(const QByteArray &pcmData);

private:
    static VoiceRecognitionManager* m_instance;
    QThread* m_workerThread;
    QString m_serviceUrl;
    QString m_currentRequestId;
    
    // 音频相关
    QAudioInput* m_audioInput;
    QBuffer* m_audioBuffer;
    QByteArray m_audioData;
    
    // 网络相关
    QNetworkAccessManager* m_networkManager;
    
    // 常量
    static const int RECOGNITION_TIMEOUT = 10000; // 10秒超时
};

#endif // VOICERECOGNITIONMANAGER_H 