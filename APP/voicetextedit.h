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
    QByteArray createWavHeader(const QByteArray &pcmData);

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