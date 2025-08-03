#ifndef MULTIVOICEDEMO_H
#define MULTIVOICEDEMO_H

#include <QWidget>
#include "simplevoicetextedit.h"

class QLabel;

/**
 * 函数名称：`MultiVoiceDemo`
 * 功能描述：多控件语音输入演示窗口，展示新架构的优势
 * 设计特点：
 *   - 多个SimpleVoiceTextEdit控件共享一个VoiceRecognitionManager
 *   - 基于焦点的智能文本投递
 *   - 统一的状态显示
 */
class MultiVoiceDemo : public QWidget
{
    Q_OBJECT

public:
    explicit MultiVoiceDemo(QWidget *parent = nullptr);

private slots:
    /**
     * 函数名称：`onStatusChanged`
     * 功能描述：处理状态变化，更新状态显示
     * 参数说明：
     *     - status：QString，状态描述
     * 返回值：void
     */
    void onStatusChanged(const QString &status);

private:
    /**
     * 函数名称：`setupUI`
     * 功能描述：设置用户界面
     * 参数说明：无
     * 返回值：void
     */
    void setupUI();

    /**
     * 函数名称：`connectStatusSignals`
     * 功能描述：连接所有控件的状态信号
     * 参数说明：无
     * 返回值：void
     */
    void connectStatusSignals();

private:
    SimpleVoiceTextEdit* m_leftTextEdit;      // 左侧文本编辑器
    SimpleVoiceTextEdit* m_topTextEdit;       // 右上文本编辑器
    SimpleVoiceTextEdit* m_bottomTextEdit;    // 右下文本编辑器
    QLabel* m_statusLabel;                    // 状态显示标签
};

#endif // MULTIVOICEDEMO_H 