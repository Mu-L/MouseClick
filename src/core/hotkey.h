#ifndef HOTKEY_H
#define HOTKEY_H

#include <QKeySequence>
#include <QMetaType>
#include <QString>

class QKeyEvent;

// Hotkey —— 全局热键值对象（不可变）
//
// 表示「修饰键 + 键码」的单一键组合，是快捷键在程序内的唯一数据源。
//
// 关键设计：只从 Qt::Key + Qt::KeyboardModifiers 直接构造，绝不经过
// QKeySequenceEdit 内部的 QKeyMapper（后者会把 Shift+1 归一化成 "!"，
// 从而丢失 Shift 修饰符）。字符串只是本对象的投影，禁止反解析字符串。
class Hotkey
{
public:
    Hotkey() = default;

    Hotkey(Qt::Key key, Qt::KeyboardModifiers modifiers = Qt::NoModifier);

    // 从按键事件构造：使用底层键码 + 当前按住的修饰键；纯修饰键返回无效值
    static Hotkey fromKeyEvent(const QKeyEvent* event);

    // 从持久化字符串还原（PortableText 优先，NativeText 兜底）
    static Hotkey fromString(const QString& text);

    Qt::Key key() const { return _key; }
    Qt::KeyboardModifiers modifiers() const { return _modifiers; }

    // 是否为有效组合（有实际键码，且不是纯修饰键）
    bool isValid() const;

    // 持久化 / 展示字符串（PortableText，可安全往返）
    QString toString() const;

    // 交给 QHotkey 注册的 QKeySequence
    QKeySequence toKeySequence() const;

    bool operator==(const Hotkey& other) const;
    bool operator!=(const Hotkey& other) const;

private:
    Qt::Key _key = Qt::Key_unknown;
    Qt::KeyboardModifiers _modifiers = Qt::NoModifier;
};

Q_DECLARE_METATYPE(Hotkey)

#endif // HOTKEY_H
