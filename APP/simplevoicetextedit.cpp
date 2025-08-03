#include "simplevoicetextedit.h"
#include "voicerecognitionmanager.h"
#include <QUuid>
#include <QDebug>
#include <QApplication>

SimpleVoiceTextEdit::SimpleVoiceTextEdit(QWidget *parent)
    : QTextEdit(parent)
    , m_state(State::Idle)
    , m_longPressTimer(new QTimer(this))
    , m_hasFocus(false)
{
    // 生成唯一控件ID
    m_controlId = QUuid::createUuid().toString(QUuid::WithoutBraces);
    qDebug() << "📝 SimpleVoiceTextEdit 创建，ID:" << m_controlId;
    
    // 配置长按计时器
    m_longPressTimer->setSingleShot(true);
    m_longPressTimer->setInterval(LONG_PRESS_DURATION);
    connect(m_longPressTimer, &QTimer::timeout, this, &SimpleVoiceTextEdit::onLongPressTimeout);
    
    // 保存原始样式
    m_originalStyleSheet = styleSheet() + "QTextEdit { font-size: 40px; }";
    setStyleSheet(m_originalStyleSheet);
    
    // 设置焦点策略，确保能接收键盘事件
    setFocusPolicy(Qt::StrongFocus);
    
    // 设置初始提示
    setPlaceholderText("长按 'V' 键开始语音输入...");
    
    // 设置信号连接
    setupConnections();
}

SimpleVoiceTextEdit::~SimpleVoiceTextEdit()
{
    qDebug() << "📝 SimpleVoiceTextEdit 析构，ID:" << m_controlId;
}

void SimpleVoiceTextEdit::setupConnections()
{
    // 连接到语音识别管理器的信号
    VoiceRecognitionManager* manager = VoiceRecognitionManager::instance();
    
    connect(manager, &VoiceRecognitionManager::recognitionStarted,
            this, &SimpleVoiceTextEdit::onRecognitionStarted);
            
    connect(manager, &VoiceRecognitionManager::recognitionFinished,
            this, &SimpleVoiceTextEdit::onRecognitionFinished);
            
    connect(manager, &VoiceRecognitionManager::recognitionError,
            this, &SimpleVoiceTextEdit::onRecognitionError);
            
    connect(manager, &VoiceRecognitionManager::statusChanged,
            this, &SimpleVoiceTextEdit::onStatusChanged);
    
    qDebug() << "📝 信号连接已建立，ID:" << m_controlId;
}

void SimpleVoiceTextEdit::keyPressEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_V && !event->isAutoRepeat()) {
        if (m_state == State::Idle && m_hasFocus) {
            qDebug() << "📝 V键按下，开始等待长按确认，ID:" << m_controlId;
            setState(State::WaitingForLongPress);
            m_longPressTimer->start();
            return;
        }
    } else if (event->key() == Qt::Key_Escape) {
        if (m_state != State::Idle) {
            qDebug() << "📝 ESC键按下，取消录音，ID:" << m_controlId;
            VoiceRecognitionManager::instance()->cancelRecording();
            setState(State::Idle);
            return;
        }
    }
    
    // 如果不是语音输入相关的按键，且不在录音状态，正常处理
    if (m_state == State::Idle) {
        QTextEdit::keyPressEvent(event);
    }
}

void SimpleVoiceTextEdit::keyReleaseEvent(QKeyEvent *event)
{
    if (event->key() == Qt::Key_V && !event->isAutoRepeat()) {
        if (m_state == State::WaitingForLongPress) {
            // 短按，取消操作
            qDebug() << "📝 V键短按，取消操作，ID:" << m_controlId;
            m_longPressTimer->stop();
            setState(State::Idle);
            return;
        } else if (m_state == State::Recording) {
            // 结束录音
            qDebug() << "📝 V键释放，结束录音，ID:" << m_controlId;
            VoiceRecognitionManager::instance()->stopRecording();
            setState(State::Recognizing);
            return;
        }
    }
    
    // 如果不是语音输入相关的按键，且不在录音状态，正常处理
    if (m_state == State::Idle) {
        QTextEdit::keyReleaseEvent(event);
    }
}

void SimpleVoiceTextEdit::focusInEvent(QFocusEvent *event)
{
    QTextEdit::focusInEvent(event);
    m_hasFocus = true;
    qDebug() << "📝 获得焦点，ID:" << m_controlId;
}

void SimpleVoiceTextEdit::focusOutEvent(QFocusEvent *event)
{
    QTextEdit::focusOutEvent(event);
    m_hasFocus = false;
    qDebug() << "📝 失去焦点，ID:" << m_controlId;
    
    // 如果正在等待长按或录音中，取消操作
    if (m_state != State::Idle) {
        qDebug() << "📝 焦点丢失，取消当前语音操作，ID:" << m_controlId;
        m_longPressTimer->stop();
        VoiceRecognitionManager::instance()->cancelRecording();
        setState(State::Idle);
    }
}

void SimpleVoiceTextEdit::onLongPressTimeout()
{
    if (m_state == State::WaitingForLongPress && m_hasFocus) {
        qDebug() << "📝 长按确认，开始录音，ID:" << m_controlId;
        setState(State::Recording);
        // 通知管理器开始录音，传递控件ID
        VoiceRecognitionManager::instance()->startRecording(m_controlId);
    }
}

void SimpleVoiceTextEdit::onRecognitionStarted()
{
    qDebug() << "📝 收到录音开始信号，ID:" << m_controlId;
    // 如果当前控件有焦点，更新状态
    if (m_hasFocus && m_state == State::Recording) {
        // UI状态已在setState中处理
    }
}

void SimpleVoiceTextEdit::onRecognitionFinished(const QString &text, const QString &requestId)
{
    qDebug() << "📝 收到识别完成信号，文本:" << text << "，请求ID:" << requestId << "，当前ID:" << m_controlId;
    
    // 只有请求ID匹配或为空时才处理（为空表示兼容旧版本）
    if (requestId.isEmpty() || requestId == m_controlId) {
        // 只有当前有焦点的控件才插入文本
        if (m_hasFocus) {
            qDebug() << "📝 插入识别结果到当前控件，ID:" << m_controlId;
            insertPlainText(text);
        }
        setState(State::Idle);
    }
}

void SimpleVoiceTextEdit::onRecognitionError(const QString &error)
{
    qDebug() << "📝 收到识别错误信号:" << error << "，ID:" << m_controlId;
    
    // 所有控件都应该重置状态
    setState(State::Idle);
    
    // 如果当前控件有焦点，显示错误状态
    if (m_hasFocus) {
        emit statusChanged(error);
    }
}

void SimpleVoiceTextEdit::onStatusChanged(const QString &status)
{
    // 转发状态信号给外部UI
    if (m_hasFocus) {
        emit statusChanged(status);
    }
}

void SimpleVoiceTextEdit::setState(State newState)
{
    if (m_state == newState) {
        return;
    }
    
    qDebug() << "📝 状态变化，ID:" << m_controlId 
             << "，从" << static_cast<int>(m_state) 
             << "到" << static_cast<int>(newState);
    
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
        // 设置录音状态的视觉效果
        setStyleSheet(m_originalStyleSheet + 
                     "QTextEdit { background-color: #f0f0f0; color: #888888; }");
        setReadOnly(true);  // 使用只读模式
        break;
        
    case State::Recognizing:
        // 设置识别状态的视觉效果
        setStyleSheet(m_originalStyleSheet + 
                     "QTextEdit { background-color: #fff5e6; color: #666666; }");
        setReadOnly(true);  // 保持只读状态
        break;
    }
} 