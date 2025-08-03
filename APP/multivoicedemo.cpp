#include "multivoicedemo.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QSplitter>

MultiVoiceDemo::MultiVoiceDemo(QWidget *parent)
    : QWidget(parent)
{
    setupUI();
}

void MultiVoiceDemo::setupUI()
{
    setWindowTitle("多控件语音输入演示 - 新架构");
    resize(800, 600);
    
    // 创建主布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    
    // 添加说明标签
    QLabel* titleLabel = new QLabel("🎤 多控件语音输入演示", this);
    titleLabel->setStyleSheet("font-size: 20px; font-weight: bold; color: #2c3e50; margin: 10px;");
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);
    
    QLabel* instructionLabel = new QLabel(
        "💡 使用说明：\n"
        "1. 点击任意文本框获得焦点\n"
        "2. 长按 'V' 键开始语音输入\n"
        "3. 说话完毕释放 'V' 键\n"
        "4. 识别结果将出现在有焦点的文本框中\n"
        "5. 支持多个控件共享同一个语音识别服务", this);
    instructionLabel->setStyleSheet("background-color: #ecf0f1; padding: 10px; border-radius: 5px; color: #34495e;");
    instructionLabel->setWordWrap(true);
    mainLayout->addWidget(instructionLabel);
    
    // 创建分割器
    QSplitter* splitter = new QSplitter(Qt::Horizontal, this);
    
    // 左侧文本框
    QWidget* leftWidget = new QWidget();
    QVBoxLayout* leftLayout = new QVBoxLayout(leftWidget);
    
    QLabel* leftLabel = new QLabel("📝 文档编辑区", leftWidget);
    leftLabel->setStyleSheet("font-weight: bold; color: #27ae60; margin-bottom: 5px;");
    leftLayout->addWidget(leftLabel);
    
    m_leftTextEdit = new SimpleVoiceTextEdit(leftWidget);
    m_leftTextEdit->setPlaceholderText("这里可以输入文档内容...\n长按 'V' 键开始语音输入");
    leftLayout->addWidget(m_leftTextEdit);
    
    // 右侧分为上下两个文本框
    QWidget* rightWidget = new QWidget();
    QVBoxLayout* rightLayout = new QVBoxLayout(rightWidget);
    
    // 上方文本框
    QLabel* topLabel = new QLabel("💬 聊天消息区", rightWidget);
    topLabel->setStyleSheet("font-weight: bold; color: #3498db; margin-bottom: 5px;");
    rightLayout->addWidget(topLabel);
    
    m_topTextEdit = new SimpleVoiceTextEdit(rightWidget);
    m_topTextEdit->setPlaceholderText("这里可以输入聊天消息...\n长按 'V' 键开始语音输入");
    rightLayout->addWidget(m_topTextEdit);
    
    // 下方文本框
    QLabel* bottomLabel = new QLabel("📋 备注说明区", rightWidget); 
    bottomLabel->setStyleSheet("font-weight: bold; color: #e67e22; margin-bottom: 5px;");
    rightLayout->addWidget(bottomLabel);
    
    m_bottomTextEdit = new SimpleVoiceTextEdit(rightWidget);
    m_bottomTextEdit->setPlaceholderText("这里可以输入备注说明...\n长按 'V' 键开始语音输入");
    rightLayout->addWidget(m_bottomTextEdit);
    
    // 添加到分割器
    splitter->addWidget(leftWidget);
    splitter->addWidget(rightWidget);
    splitter->setStretchFactor(0, 1);
    splitter->setStretchFactor(1, 1);
    
    mainLayout->addWidget(splitter);
    
    // 状态显示区域
    m_statusLabel = new QLabel("✅ 多控件语音输入已就绪", this);
    m_statusLabel->setStyleSheet("background-color: #d5dbdb; padding: 8px; border-radius: 3px; color: #2c3e50;");
    mainLayout->addWidget(m_statusLabel);
    
    // 连接状态信号
    connectStatusSignals();
}

void MultiVoiceDemo::connectStatusSignals()
{
    // 连接所有文本框的状态信号
    connect(m_leftTextEdit, &SimpleVoiceTextEdit::statusChanged,
            this, &MultiVoiceDemo::onStatusChanged);
            
    connect(m_topTextEdit, &SimpleVoiceTextEdit::statusChanged,
            this, &MultiVoiceDemo::onStatusChanged);
            
    connect(m_bottomTextEdit, &SimpleVoiceTextEdit::statusChanged,
            this, &MultiVoiceDemo::onStatusChanged);
}

void MultiVoiceDemo::onStatusChanged(const QString &status)
{
    if (status.isEmpty()) {
        m_statusLabel->setText("✅ 多控件语音输入已就绪");
        m_statusLabel->setStyleSheet("background-color: #d5dbdb; padding: 8px; border-radius: 3px; color: #2c3e50;");
    } else {
        m_statusLabel->setText("🎤 " + status);
        m_statusLabel->setStyleSheet("background-color: #f39c12; padding: 8px; border-radius: 3px; color: white;");
    }
} 