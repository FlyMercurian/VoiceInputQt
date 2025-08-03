#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "voicetextedit.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_voiceEdit(nullptr)
{
    ui->setupUi(this);
    setupVoiceTextEdit();
    
    // 设置窗口标题
    setWindowTitle("语音输入演示 - VoiceTextEdit");
    
    // 检查服务状态
    QTimer::singleShot(1000, this, &MainWindow::onServiceStatusCheck);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::setupVoiceTextEdit()
{
    // 创建语音输入控件
    m_voiceEdit = new VoiceTextEdit(this);
    
    // 连接状态更新信号到状态栏 - 使用lambda表达式
    connect(m_voiceEdit, &VoiceTextEdit::statusChanged,
            [this](const QString &message) {
                statusBar()->showMessage(message);
            });
    
    // 将控件设置为中央控件
    setCentralWidget(m_voiceEdit);
    
    // 设置初始状态栏提示
    statusBar()->showMessage("长按 'V' 键开始语音输入", 5000);
}

void MainWindow::onServiceStatusCheck()
{
    if (m_voiceEdit && !m_voiceEdit->checkServiceAvailability()) {
        QMessageBox::warning(this, "服务提醒", 
            "语音识别服务未启动。\n\n"
            "请确保已启动 SenseVoice 服务：\n"
            "1. 进入 SenseVoice 目录\n"
            "2. 运行：python start_service.py\n"
            "3. 确保服务运行在 http://127.0.0.1:8000\n\n"
            "服务启动后即可使用语音输入功能。");
        statusBar()->showMessage("语音服务未就绪 - 请启动 SenseVoice 服务");
    } else {
        statusBar()->showMessage("语音服务已就绪 - 长按 'V' 键开始语音输入", 3000);
    }
}
