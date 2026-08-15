#include "hotkey.h"

#include <QKeyEvent>

Hotkey::Hotkey(Qt::Key key, Qt::KeyboardModifiers modifiers)
    : _key(key), _modifiers(modifiers)
{
}

Hotkey Hotkey::fromKeyEvent(const QKeyEvent* event)
{
    if (!event)
        return Hotkey();

    const int key = event->key();
    // 纯修饰键不构成组合，等待真正的按键
    if (key == Qt::Key_unknown || key == Qt::Key_Control || key == Qt::Key_Shift
        || key == Qt::Key_Alt || key == Qt::Key_Meta) {
        return Hotkey();
    }

    // 关键：用 event->key()（底层键码）+ event->modifiers()（当前按住修饰键），
    // 不做 QKeyMapper 的 Shift 归一化，Shift+1 精确保留为 "Shift+1" 而非 "!"
    return Hotkey(static_cast<Qt::Key>(key), event->modifiers());
}

Hotkey Hotkey::fromString(const QString& text)
{
    QKeySequence sequence = QKeySequence::fromString(text, QKeySequence::PortableText);
    if (sequence.isEmpty()) {
        // 兼容旧配置里以 NativeText 保存的序列
        sequence = QKeySequence::fromString(text, QKeySequence::NativeText);
    }

    if (sequence.isEmpty())
        return Hotkey();

    // 全局热键只支持单键组合，取第一段
    const QKeyCombination combination = sequence[0];
    return Hotkey(combination.key(), combination.keyboardModifiers());
}

bool Hotkey::isValid() const
{
    return _key != Qt::Key_unknown
        && _key != Qt::Key_Control
        && _key != Qt::Key_Shift
        && _key != Qt::Key_Alt
        && _key != Qt::Key_Meta;
}

QString Hotkey::toString() const
{
    return toKeySequence().toString(QKeySequence::PortableText);
}

QKeySequence Hotkey::toKeySequence() const
{
    // QKeyCombination 的构造顺序是 (modifiers, key)
    return QKeySequence(QKeyCombination(_modifiers, _key));
}

bool Hotkey::operator==(const Hotkey& other) const
{
    return _key == other._key && _modifiers == other._modifiers;
}

bool Hotkey::operator!=(const Hotkey& other) const
{
    return !(*this == other);
}
