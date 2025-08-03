#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "simplevoicetextedit.h"
#include "voicerecognitionmanager.h"
#include <QVBoxLayout>
#include <QWidget>
#include <QStatusBar>
#include <QTimer>
#include <QMessageBox>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    
    // 初始化语音识别管理器
    initializeVoiceRecognitionManager();
    
    // 设置语音文本编辑器
    setupVoiceTextEdit();
    
    // 设置窗口标题
    setWindowTitle("语音录入应用-本地部署版");
    
    // 设置初始状态
    statusBar()->showMessage("语音识别管理器已初始化 - 长按 'V' 键开始语音输入", 3000);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::initializeVoiceRecognitionManager()
{
    // 获取语音识别管理器实例
    VoiceRecognitionManager* manager = VoiceRecognitionManager::instance();
    
    // 设置服务URL
    manager->setServiceUrl("http://127.0.0.1:8000");
    
    // 初始化管理器（启动工作线程）
    manager->initialize();
    
    qDebug() << "🏠 语音识别管理器已初始化";
}

void MainWindow::setupVoiceTextEdit()
{   
    // 连接状态更新信号到状态栏
    connect(ui->voicetextEdit_1, &SimpleVoiceTextEdit::statusChanged,
            [this](const QString &message) {
                statusBar()->showMessage(message);
            });

    connect(ui->voicetextEdit_2, &SimpleVoiceTextEdit::statusChanged,
    [this](const QString &message) {
        statusBar()->showMessage(message);
    });

}
