#ifndef HOTKEYCAPTURECONTROLLER_H
#define HOTKEYCAPTURECONTROLLER_H

#include <QObject>

#include "core/hotkey.h"
#include "core/hotkeyregistrar.h"

class QKeyEvent;

// HotkeyCaptureController —— 快捷键录制状态机
//
// 职责：统一协调「捕获 → 注册 → 持久化通知」的生命周期，是快捷键的唯一状态拥有者。
// 状态机：
//
//   Idle --beginCapture()--> Capturing
//   Capturing --onKeyPressed(成功)--> Idle
//   Capturing --onKeyPressed(失败)--> Failed
//   Failed --onKeyPressed(成功)--> Idle
//   Capturing/Failed --cancelCapture()--> Idle（回退旧热键）
//   Idle --clearHotkey()--> Idle（永久清除）
//
// 「注册成功才写配置」：只有提交（成功注册 / 永久清除 / 程序化设置）才发
// currentHotkeyChanged；取消（回退）不改变生效热键，因此不发该信号。
class HotkeyCaptureController : public QObject
{
    Q_OBJECT
public:
    explicit HotkeyCaptureController(IHotkeyRegistrar* registrar, QObject* parent = nullptr);

    // 当前生效热键
    Hotkey currentHotkey() const { return _current; }

    // 程序化设置（启动加载），成功才写配置
    void setHotkey(const Hotkey& hotkey);
    // 点击控件：进入录制态，旧热键停止触发
    void beginCapture();
    // 失焦且未成功注册：取消录制，回退并恢复旧热键
    void cancelCapture();
    // 收到一次按键；返回 true 表示已消费（UI 应 accept）
    bool onKeyPressed(const QKeyEvent* event);
    // 「Hotkey Clean」按钮：永久清除
    void clearHotkey();

signals:
    // 生效热键发生变化（提交 / 清除 / 程序化设置）
    void currentHotkeyChanged(const Hotkey& hotkey);
    // 进入录制态
    void captureStarted();
    // 录制成功提交
    void captureCommitted();
    // 录制取消（回退）
    void captureCancelled();
    // 注册失败（一次性事件）
    void registrationFailed(RegisterResult result);
    // 全局热键被触发（转发）
    void activated();

private:
    enum class CaptureState { Idle, Capturing, Failed };

    IHotkeyRegistrar* _registrar = nullptr;
    Hotkey _current;  // 生效热键（录制期间保持不变，用于取消时回退）
    CaptureState _state = CaptureState::Idle;

    void setState(CaptureState state);
};

#endif // HOTKEYCAPTURECONTROLLER_H
