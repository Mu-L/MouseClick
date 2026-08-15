#ifndef SETTINGSPAGE_H
#define SETTINGSPAGE_H

#include <QComboBox>
#include <QLabel>
#include <QWidget>

#include "pagebase.h"

class HotkeyCaptureController;
class HotkeyEdit;
class QHotkeyRegistrar;

class SettingsPage : public PageBase
{
    Q_OBJECT
public:
    explicit SettingsPage(const QString& title, QWidget* parent = nullptr);
    ~SettingsPage();

signals:
    void hotkeyActivated();

protected:
    void changeEvent(QEvent *event) override;

private:
    Q_DISABLE_COPY_MOVE(SettingsPage)

    QHotkeyRegistrar* _hotkey_registrar;
    HotkeyCaptureController* _hotkey_controller;
    HotkeyEdit* _hotkey_reader;
    QPushButton* _hotkey_clean;

    // 可翻译控�?
    QLabel* _page_title;
    QLabel* _hotkey_desc;
    QLabel* _theme_toggle_desc;
    QLabel* _language_switch_desc;
    QComboBox* _language_list;
    QLabel* _close_button_behavior_desc;
    QComboBox* _close_button_behavior_list;

    void retranslateUi();
};

#endif // SETTINGSPAGE_H
