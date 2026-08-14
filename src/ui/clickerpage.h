#ifndef CLICKERPAGE_H
#define CLICKERPAGE_H

#include <QComboBox>
#include <QDoubleSpinBox>
#include <QLabel>
#include <QRadioButton>
#include <QWidget>

#include "pagebase.h"
#include "settingspage.h"

class ClickerPage : public PageBase
{
    Q_OBJECT
public:
    explicit ClickerPage(const QString& title, SettingsPage& settings_page, QWidget* parent = nullptr);
    ~ClickerPage();

protected:
    void changeEvent(QEvent *event) override;

private:
    Q_DISABLE_COPY_MOVE(ClickerPage)

    // 可翻译控�?
    QLabel* _page_title;
    QLabel* _click_type_desc;
    QComboBox* _click_type_list;
    QLabel* _interval_time_desc;
    QLabel* _random_interval_toggle_desc;
    QLabel* _random_interval_time_desc;
    QLabel* _memory_configuration_toggle_desc;

    void retranslateUi();
};

#endif // CLICKERPAGE_H
