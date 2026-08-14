#ifndef HOTKEYEDIT_H
#define HOTKEYEDIT_H

#include <QLineEdit>

class QWidget;
class QKeyEvent;
class QMouseEvent;
class QFocusEvent;
class QHotkey;

class HotkeyEdit : public QLineEdit
{
    Q_OBJECT
public:
    explicit HotkeyEdit(QWidget* parent = nullptr);
    ~HotkeyEdit();

    void cleanHotKey();

    const QString getHotkey() const;
    void setHotkey(const QString& key_sequence);

Q_SIGNALS:
    void hotkeyActivated();
    void currentHotkeyChanged(const QString& key_sequence);

protected:
    void mousePressEvent(QMouseEvent* event) override;

    void focusInEvent(QFocusEvent* event) override;

    void focusOutEvent(QFocusEvent* event) override;

    void keyPressEvent(QKeyEvent* event) override;

    void keyReleaseEvent(QKeyEvent* event) override;

private:
    QString _key_sequence;

    QSet<int> _pressed_keys;    // 当前被按下的所有按键

    QHotkey* _hotkey = nullptr;

    void registerGlobalHotkey();
    void unregisterGlobalHotkey();
};

#endif // HOTKEYEDIT_H
