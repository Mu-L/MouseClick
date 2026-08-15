#ifndef QHOTKEYREGISTRAR_H
#define QHOTKEYREGISTRAR_H

#include "core/hotkeyregistrar.h"

class QHotkey;

// QHotkeyRegistrar —— 基于 QHotkey 的注册服务实现
class QHotkeyRegistrar : public IHotkeyRegistrar
{
    Q_OBJECT
public:
    explicit QHotkeyRegistrar(QObject* parent = nullptr);
    ~QHotkeyRegistrar() override;

    RegisterResult registerHotkey(const Hotkey& hotkey) override;
    void unregisterHotkey() override;

private:
    QHotkey* _hotkey = nullptr;
};

#endif // QHOTKEYREGISTRAR_H
