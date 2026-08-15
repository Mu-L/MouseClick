#ifndef HOTKEYREGISTRAR_H
#define HOTKEYREGISTRAR_H

#include <QMetaType>
#include <QObject>

#include "core/hotkey.h"

// RegisterResult —— 注册结果
enum class RegisterResult {
    Success,      // 注册成功
    Conflict,     // 被其它程序占用（RegisterHotKey 返回失败）
    Invalid,      // 非法组合（空 / 纯修饰键）
    NotSupported  // 当前平台不支持全局热键
};

Q_DECLARE_METATYPE(RegisterResult)

// IHotkeyRegistrar —— 全局热键注册服务抽象
//
// 依赖倒置：上层（控制器）只依赖此接口，便于单元测试注入 mock。
class IHotkeyRegistrar : public QObject
{
    Q_OBJECT
public:
    using QObject::QObject;

    // 注册（或替换）当前热键
    virtual RegisterResult registerHotkey(const Hotkey& hotkey) = 0;
    // 注销当前热键
    virtual void unregisterHotkey() = 0;

signals:
    // 全局热键在系统任意位置被触发
    void activated();
};

#endif // HOTKEYREGISTRAR_H
