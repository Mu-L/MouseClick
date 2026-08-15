#ifndef HOTKEYEDIT_H
#define HOTKEYEDIT_H

#include <QLineEdit>

#include "core/hotkey.h"
#include "core/hotkeyregistrar.h"

class HotkeyCaptureController;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;

// HotkeyEdit —— 纯 UI 转发控件
//
// 职责边界：只负责「展示 + 聚焦/失焦 + 按键转发」，不持有任何注册逻辑。
// 所有快捷键状态由 HotkeyCaptureController（状态机）拥有，本控件是它的只读投影。
//
// 行为约定：
//   1. 文字不可选中（只读 + 鼠标事件直接消费，不进默认选中逻辑）；
//   2. 点击/聚焦 → beginCapture()：清空旧快捷键，显示 placeholder，等待录入；
//   3. 失焦且未成功注册 → cancelCapture()：回退显示旧快捷键；
//   4. 注册成功 → captureCommitted()：展示新快捷键并 clearFocus() 结束录制。
class HotkeyEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit HotkeyEdit(HotkeyCaptureController* controller, QWidget* parent = nullptr);

protected:
    void focusInEvent(QFocusEvent* event) override;
    void focusOutEvent(QFocusEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

    // 只读控件无需文本选中，鼠标事件仅用于聚焦，直接消费
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;

private slots:
    void onCurrentHotkeyChanged(const Hotkey& hotkey);
    void onCaptureStarted();
    void onCaptureCommitted();
    void onCaptureCancelled();
    void onRegistrationFailed(RegisterResult result);

private:
    HotkeyCaptureController* _controller = nullptr;

    void showPlaceholder();
    void showCurrentHotkey();
};

#endif // HOTKEYEDIT_H
