#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    /**
     * 函数名称：`initializeVoiceRecognitionManager`
     * 功能描述：初始化语音识别管理器
     * 参数说明：无
     * 返回值：无
     */
    void initializeVoiceRecognitionManager();

    /**
     * 函数名称：`setupVoiceTextEdit`
     * 功能描述：设置语音输入控件
     * 参数说明：无
     * 返回值：无
     */
    void setupVoiceTextEdit();

    Ui::MainWindow *ui;
};
#endif // MAINWINDOW_H
