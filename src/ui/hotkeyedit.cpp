#include "hotkeyedit.h"
#include "core/logger.h"
#include "ui/messagebox.h"

#include <QWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QKeySequence>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QFocusEvent>
#include <QHotkey>

HotkeyEdit::HotkeyEdit(QWidget* parent)
    : QLineEdit{parent},
      _hotkey(new QHotkey())
{
    setReadOnly(true);
    setPlaceholderText(tr("Please set a shortcut hotkey"));
    connect(_hotkey, &QHotkey::activated, this, &HotkeyEdit::hotkeyActivated);
}

HotkeyEdit::~HotkeyEdit()
{
    unregisterGlobalHotkey();

    delete _hotkey;
}

void HotkeyEdit::cleanHotKey()
{
    clear();
    unregisterGlobalHotkey();
}

const QString HotkeyEdit::getHotkey() const
{
    return _key_sequence;
}

void HotkeyEdit::setHotkey(const QString& key_sequence)
{
    _key_sequence = key_sequence;

    setText(_key_sequence);

    if (!_key_sequence.isEmpty()) {
        registerGlobalHotkey();
    }
}

void HotkeyEdit::mousePressEvent(QMouseEvent* event)
{
    if (!hasFocus()) {
        setFocus();
    }
    QLineEdit::mousePressEvent(event);
}

void HotkeyEdit::focusInEvent(QFocusEvent* event)
{
    // 清空之前的内容
    cleanHotKey();
    // 设置焦点时监听键盘事件
    grabKeyboard();
    QLineEdit::focusInEvent(event);
}

void HotkeyEdit::focusOutEvent(QFocusEvent* event)
{
    // 失去焦点时取消监听
    releaseKeyboard();
    QLineEdit::focusOutEvent(event);
}

void HotkeyEdit::keyPressEvent(QKeyEvent* event)
{
    int key = event->key(); // 获取按键编码
    Qt::KeyboardModifiers modifiers = event->modifiers();

    if (key != Qt::Key_unknown && key != Qt::Key_Control && key != Qt::Key_Shift &&
        key != Qt::Key_Alt && key != Qt::Key_Meta) {
        _key_sequence = QKeySequence(modifiers | key).toString();
    } else {
        _key_sequence = QKeySequence(modifiers).toString();
    }

    setText(_key_sequence);

    _pressed_keys.insert(key);

    QLineEdit::keyPressEvent(event);
}

void HotkeyEdit::keyReleaseEvent(QKeyEvent* event)
{
    int key = event->key();

    _pressed_keys.remove(key);

    if (_pressed_keys.isEmpty()) {
        clearFocus();

        registerGlobalHotkey();
    }

    QLineEdit::keyReleaseEvent(event);
}

void HotkeyEdit::registerGlobalHotkey()
{
#ifdef QT_DEBUG
    LOG_DEBUG(QString("Register Global Hotkey: %1").arg(_key_sequence));
#endif

    unregisterGlobalHotkey(); // 先注销之前的快捷键

    QStringList keys = _key_sequence.split('+');
    Qt::KeyboardModifiers modifiers = Qt::NoModifier;
    int key = 0;

    for (const QString& k : keys) {
        if (k == "Ctrl") {
            modifiers |= Qt::ControlModifier;
        } else if (k == "Shift") {
            modifiers |= Qt::ShiftModifier;
        } else if (k == "Alt") {
            modifiers |= Qt::AltModifier;
        } else if (k == "Meta") {
            modifiers |= Qt::MetaModifier;
        } else {
            QKeySequence keySequence(k, QKeySequence::PortableText);
            if (!keySequence.isEmpty()) {
                key = keySequence[0].key();
            }
        }
    }

    if (key != 0) {
        if (!_hotkey->setShortcut(QKeySequence(modifiers | key), true)) {
            // 设置失败
            MessageBox msgBox(this);
            msgBox.setIcon(QMessageBox::Critical);
            msgBox.setText(tr("Failed to register global hotkey."));
            msgBox.addButton(tr("OK"), QMessageBox::AcceptRole);
            msgBox.exec();

            // 清空
            _key_sequence.clear();
            clear();
        } else {
            emit currentHotkeyChanged(_key_sequence);   // 发送信号，快捷键已更改
        }
    }
}

void HotkeyEdit::unregisterGlobalHotkey()
{
    _hotkey->setShortcut(QKeySequence(), true);
}
