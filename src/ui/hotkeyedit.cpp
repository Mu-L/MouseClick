#include "hotkeyedit.h"

#include <QFocusEvent>
#include <QKeyEvent>
#include <QMessageBox>
#include <QMouseEvent>

#include "core/hotkeycapturecontroller.h"
#include "ui/messagebox.h"

HotkeyEdit::HotkeyEdit(HotkeyCaptureController* controller, QWidget* parent)
    : QLineEdit{parent},
      _controller(controller)
{
    Q_ASSERT(controller);

    // 文字仅作展示，禁止编辑与选中
    setReadOnly(true);
    setPlaceholderText(tr("Please set a shortcut hotkey"));

    connect(_controller, &HotkeyCaptureController::currentHotkeyChanged,
            this, &HotkeyEdit::onCurrentHotkeyChanged);
    connect(_controller, &HotkeyCaptureController::captureStarted,
            this, &HotkeyEdit::onCaptureStarted);
    connect(_controller, &HotkeyCaptureController::captureCommitted,
            this, &HotkeyEdit::onCaptureCommitted);
    connect(_controller, &HotkeyCaptureController::captureCancelled,
            this, &HotkeyEdit::onCaptureCancelled);
    connect(_controller, &HotkeyCaptureController::registrationFailed,
            this, &HotkeyEdit::onRegistrationFailed);

    showCurrentHotkey();
}

void HotkeyEdit::focusInEvent(QFocusEvent* event)
{
    // 点击/聚焦：进入录制态（清空显示 + 旧热键停止触发）
    _controller->beginCapture();
    QLineEdit::focusInEvent(event);
}

void HotkeyEdit::focusOutEvent(QFocusEvent* event)
{
    // 失焦：若未成功注册，回退旧热键
    _controller->cancelCapture();
    QLineEdit::focusOutEvent(event);
}

void HotkeyEdit::keyPressEvent(QKeyEvent* event)
{
    // 纯修饰键等未消费的按键走默认（读取态下无副作用）
    if (_controller->onKeyPressed(event)) {
        event->accept();
    } else {
        QLineEdit::keyPressEvent(event);
    }
}

void HotkeyEdit::mousePressEvent(QMouseEvent* event)
{
    event->accept();  // 聚焦由 QApplication::notify 在分发前完成，这里只需阻止文本选中
}

void HotkeyEdit::mouseMoveEvent(QMouseEvent* event)
{
    event->accept();
}

void HotkeyEdit::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
}

void HotkeyEdit::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
}

void HotkeyEdit::onCurrentHotkeyChanged(const Hotkey& hotkey)
{
    Q_UNUSED(hotkey);
    showCurrentHotkey();
}

void HotkeyEdit::onCaptureStarted()
{
    // 进入录制：清空旧快捷键，显示 placeholder
    showPlaceholder();
}

void HotkeyEdit::onCaptureCommitted()
{
    // 注册成功：展示新快捷键并结束录制（此时状态机已回到 Idle，
    // 随后的 focusOut → cancelCapture 是 no-op，不会撤销本次提交）
    showCurrentHotkey();
    clearFocus();
}

void HotkeyEdit::onCaptureCancelled()
{
    // 失焦回退：恢复显示旧快捷键
    showCurrentHotkey();
}

void HotkeyEdit::onRegistrationFailed(RegisterResult result)
{
    Q_UNUSED(result);
    // 状态机进入 Failed，保持录制态等待重新录入
    showPlaceholder();

    MessageBox msgBox(this);
    msgBox.setIcon(QMessageBox::Critical);
    msgBox.setText(tr("Failed to register global hotkey."));
    msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
    msgBox.exec();

    setFocus();  // 继续录制
}

void HotkeyEdit::showPlaceholder()
{
    clear();  // 清空文字，让 placeholder 显示
}

void HotkeyEdit::showCurrentHotkey()
{
    const Hotkey current = _controller->currentHotkey();
    setText(current.isValid() ? current.toString() : QString());
}
