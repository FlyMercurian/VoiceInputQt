#ifndef SIMPLEVOICETEXTEDIT_H
#define SIMPLEVOICETEXTEDIT_H

#include <QTextEdit>
#include <QTimer>
#include <QKeyEvent>

/**
 * 函数名称：`SimpleVoiceTextEdit`
 * 功能描述：简化版语音文本编辑器，专注于UI交互，语音识别逻辑交给VoiceRecognitionManager处理
 * 设计原则：单一职责 - 只管UI交互和状态显示，不处理语音识别业务逻辑
 */
class SimpleVoiceTextEdit : public QTextEdit
{
    Q_OBJECT

public:
    /**
     * 状态枚举
     */
    enum class State {
        Idle,                    // 空闲状态
        WaitingForLongPress,     // 等待长按确认
        Recording,               // 录音中
        Recognizing             // 识别中
    };

    explicit SimpleVoiceTextEdit(QWidget *parent = nullptr);
    ~SimpleVoiceTextEdit();

    /**
     * 函数名称：`getControlId`
     * 功能描述：获取控件唯一标识
     * 参数说明：无
     * 返回值：QString，控件ID
     */
    QString getControlId() const { return m_controlId; }

protected:
    /**
     * 函数名称：`keyPressEvent`
     * 功能描述：处理按键按下事件，监听V键长按
     * 参数说明：
     *     - event：QKeyEvent*，键盘事件
     * 返回值：void
     */
    void keyPressEvent(QKeyEvent *event) override;

    /**
     * 函数名称：`keyReleaseEvent`
     * 功能描述：处理按键释放事件，判断是否结束录音
     * 参数说明：
     *     - event：QKeyEvent*，键盘事件
     * 返回值：void
     */
    void keyReleaseEvent(QKeyEvent *event) override;

    /**
     * 函数名称：`focusInEvent`
     * 功能描述：获得焦点时的处理
     * 参数说明：
     *     - event：QFocusEvent*，焦点事件
     * 返回值：void
     */
    void focusInEvent(QFocusEvent *event) override;

    /**
     * 函数名称：`focusOutEvent`
     * 功能描述：失去焦点时的处理
     * 参数说明：
     *     - event：QFocusEvent*，焦点事件
     * 返回值：void
     */
    void focusOutEvent(QFocusEvent *event) override;

private slots:
    /**
     * 函数名称：`onLongPressTimeout`
     * 功能描述：长按超时处理，开始录音
     * 参数说明：无
     * 返回值：void
     */
    void onLongPressTimeout();

    /**
     * 函数名称：`onRecognitionStarted`
     * 功能描述：录音开始时的UI更新
     * 参数说明：无
     * 返回值：void
     */
    void onRecognitionStarted();

    /**
     * 函数名称：`onRecognitionFinished`
     * 功能描述：识别完成时的处理，插入识别结果
     * 参数说明：
     *     - text：QString，识别结果文本
     *     - requestId：QString，请求ID
     * 返回值：void
     */
    void onRecognitionFinished(const QString &text, const QString &requestId);

    /**
     * 函数名称：`onRecognitionError`
     * 功能描述：识别错误时的处理
     * 参数说明：
     *     - error：QString，错误信息
     * 返回值：void
     */
    void onRecognitionError(const QString &error);

    /**
     * 函数名称：`onStatusChanged`
     * 功能描述：状态变化时的UI更新
     * 参数说明：
     *     - status：QString，状态描述
     * 返回值：void
     */
    void onStatusChanged(const QString &status);

private:
    /**
     * 函数名称：`setState`
     * 功能描述：设置控件状态并更新UI
     * 参数说明：
     *     - newState：State，新状态
     * 返回值：void
     */
    void setState(State newState);

    /**
     * 函数名称：`setupConnections`
     * 功能描述：设置与VoiceRecognitionManager的信号连接
     * 参数说明：无
     * 返回值：void
     */
    void setupConnections();

private:
    State m_state;                      // 当前状态
    QTimer* m_longPressTimer;           // 长按计时器
    QString m_originalStyleSheet;       // 原始样式表
    QString m_controlId;                // 控件唯一标识
    bool m_hasFocus;                    // 是否拥有焦点

    // 常量
    static const int LONG_PRESS_DURATION = 500; // 长按持续时间(毫秒)

signals:
    /**
     * 信号名称：`statusChanged`
     * 功能描述：状态变化信号，可供外部UI显示
     * 参数说明：
     *     - status：QString，状态描述
     */
    void statusChanged(const QString &status);
};

#endif // SIMPLEVOICETEXTEDIT_H 