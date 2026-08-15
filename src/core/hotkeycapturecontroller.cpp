#include "hotkeycapturecontroller.h"

#include <QKeyEvent>

#include "core/logger.h"

HotkeyCaptureController::HotkeyCaptureController(IHotkeyRegistrar* registrar, QObject* parent)
    : QObject{parent},
      _registrar(registrar)
{
    Q_ASSERT(registrar);
    connect(_registrar, &IHotkeyRegistrar::activated, this, &HotkeyCaptureController::activated);
}

void HotkeyCaptureController::setState(CaptureState state)
{
    _state = state;
}

void HotkeyCaptureController::setHotkey(const Hotkey& hotkey)
{
    if (!hotkey.isValid()) {
        _registrar->unregisterHotkey();
        _current = Hotkey();
        setState(CaptureState::Idle);
        emit currentHotkeyChanged(Hotkey());
        return;
    }

    const RegisterResult result = _registrar->registerHotkey(hotkey);
    if (result == RegisterResult::Success) {
        _current = hotkey;
        setState(CaptureState::Idle);
        emit currentHotkeyChanged(hotkey);
    } else {
        emit registrationFailed(result);
    }
}

void HotkeyCaptureController::beginCapture()
{
    if (_state != CaptureState::Idle)
        return;  // 已在录制中

    setState(CaptureState::Capturing);
    // 旧热键停止触发，等待新录入；生效热键 _current 保持不变，供取消时回退
    _registrar->unregisterHotkey();
    emit captureStarted();
}

void HotkeyCaptureController::cancelCapture()
{
    if (_state == CaptureState::Idle)
        return;  // 未在录制，无需回退

    // 回退：恢复旧热键（若此前有生效热键）
    if (_current.isValid()) {
        const RegisterResult result = _registrar->registerHotkey(_current);
        if (result != RegisterResult::Success) {
            LOG_WARNING(QString("Failed to restore hotkey %1 on capture cancel")
                            .arg(_current.toString()));
        }
    }

    setState(CaptureState::Idle);
    emit captureCancelled();
}

bool HotkeyCaptureController::onKeyPressed(const QKeyEvent* event)
{
    if (_state == CaptureState::Idle)
        return false;  // 非录制态，忽略

    const Hotkey hotkey = Hotkey::fromKeyEvent(event);
    if (!hotkey.isValid())
        return false;  // 纯修饰键，等待真正的按键

#ifdef QT_DEBUG
    LOG_DEBUG(QString("Register Global Hotkey: %1").arg(hotkey.toString()));
#endif

    const RegisterResult result = _registrar->registerHotkey(hotkey);
    if (result == RegisterResult::Success) {
        _current = hotkey;
        setState(CaptureState::Idle);
        emit currentHotkeyChanged(hotkey);
        emit captureCommitted();
        return true;
    }

    setState(CaptureState::Failed);
    emit registrationFailed(result);
    return true;
}

void HotkeyCaptureController::clearHotkey()
{
    _registrar->unregisterHotkey();
    _current = Hotkey();
    setState(CaptureState::Idle);
    emit currentHotkeyChanged(Hotkey());
}
