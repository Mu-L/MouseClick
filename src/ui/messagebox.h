#ifndef MESSAGEBOX_H
#define MESSAGEBOX_H

#include <QDialog>
#include <QMessageBox>   // 仅复用 Icon / ButtonRole 枚举，保持调用点 API 不变

class QLabel;
class QPushButton;
class QHBoxLayout;
class QShowEvent;

namespace QWK
{
    class WidgetWindowAgent;
}

// ============================================================================
// MessageBox — 自定义对话框
// 基于 QDialog 重写，彻底摆脱 QMessageBox 的背景绘制逻辑（QMessageBox 不设置
// WA_StyledBackground，背景由 QWidgetPrivate 用 palette().brush(Window) 填充，
// 在深色主题下会错误地落到白色）。
// 背景通过 paintEvent 显式绘制：内容区（icon + 标题 + 正文）用 surfaceCard，
// 按钮区用 surfaceWindow，二者一上一下区分内容区与按钮区。
// 对外 API 与 QMessageBox 保持一致：setIcon / setText / setInformativeText /
// addButton / setDefaultButton / clickedButton / exec。
// 窗口外壳套用 QWindowKit 无边框 + 自定义标题栏（仅 icon + 标题 + close）。
// ============================================================================

class MessageBox : public QDialog
{
    Q_OBJECT
public:
    explicit MessageBox(QWidget* parent = nullptr);
    ~MessageBox() override;

    void setIcon(QMessageBox::Icon icon);
    void setText(const QString& text);
    void setInformativeText(const QString& text);
    QPushButton* addButton(const QString& text, QMessageBox::ButtonRole role);
    void setDefaultButton(QPushButton* button);
    QPushButton* clickedButton() const;

    int exec() override;   // 未添加任何按钮时自动补一个 OK

protected:
    void paintEvent(QPaintEvent* event) override;   // 绘制内容区背景（surfaceCard）
    void showEvent(QShowEvent* event) override;     // 显示时按内容自然尺寸锁定

private:
    QLabel* _icon_label          = nullptr;
    QLabel* _text_label          = nullptr;
    QLabel* _informative_label   = nullptr;
    QHBoxLayout* _button_layout  = nullptr;
    QPushButton* _clicked_button = nullptr;
    QPushButton* _default_button = nullptr;
    int _button_count = 0;

    QWK::WidgetWindowAgent* _window_agent = nullptr;   // QWindowKit 窗口代理（无边框外壳）

    Q_DISABLE_COPY_MOVE(MessageBox)
};

#endif // MESSAGEBOX_H
