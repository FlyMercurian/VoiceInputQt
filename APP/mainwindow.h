#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class VoiceTextEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    /**
     * 函数名称：`onServiceStatusCheck`
     * 功能描述：检查语音服务状态
     * 参数说明：无
     * 返回值：无
     */
    void onServiceStatusCheck();

private:
    /**
     * 函数名称：`setupVoiceTextEdit`
     * 功能描述：设置语音输入控件
     * 参数说明：无
     * 返回值：无
     */
    void setupVoiceTextEdit();

    Ui::MainWindow *ui;
    VoiceTextEdit *m_voiceEdit;
};
#endif // MAINWINDOW_H
