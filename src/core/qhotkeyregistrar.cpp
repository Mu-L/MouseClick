#include "qhotkeyregistrar.h"

#include <QHotkey>

QHotkeyRegistrar::QHotkeyRegistrar(QObject* parent)
    : IHotkeyRegistrar{parent},
      _hotkey(new QHotkey(this))
{
    connect(_hotkey, &QHotkey::activated, this, &QHotkeyRegistrar::activated);
}

QHotkeyRegistrar::~QHotkeyRegistrar() = default;

RegisterResult QHotkeyRegistrar::registerHotkey(const Hotkey& hotkey)
{
    if (!hotkey.isValid())
        return RegisterResult::Invalid;

    if (!_hotkey->setShortcut(hotkey.toKeySequence(), true))
        return RegisterResult::Conflict;

    return RegisterResult::Success;
}

void QHotkeyRegistrar::unregisterHotkey()
{
    _hotkey->setRegistered(false);
}
