#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "theme/themestate.h"

#include <QMainWindow>
#include <QPushButton>
#include <QSystemTrayIcon>

class NavButton;
class PopupMenu;
class QFrame;

namespace QWK
{
    class WidgetWindowAgent;
}

class QStackedWidget;
class SettingsPage;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

protected:
    bool event(QEvent *event) override;
    void changeEvent(QEvent *event) override;
    void closeEvent(QCloseEvent *event) override;

signals:
    void windowStateChanged(Qt::WindowStates newState);

private:
    Q_DISABLE_COPY_MOVE(MainWindow)

    QWK::WidgetWindowAgent* _window_agent;

    SettingsPage* _settings_page = nullptr;
    QStackedWidget* _navigation_pages = nullptr;
    QFrame* _nav_page_card = nullptr;  // QFrame wrapper for nav-page visual styling

    // 系统托盘
    QSystemTrayIcon* _tray_icon = nullptr;
    PopupMenu* _tray_menu = nullptr;
    QAction* _tray_open_action = nullptr;
    QAction* _tray_website_action = nullptr;
    QAction* _tray_exit_action = nullptr;
    bool _force_quit = false;
    bool _was_maximized_before_tray = false;
    bool _was_hidden_before_clicker = false;

    // 导航按钮（语言切换需要重新设置文本）
    NavButton* _nav_mouse_click;
    NavButton* _nav_beautify_cursor;
    NavButton* _nav_settings;

    QWidget* _nav_widget = nullptr;  // QVBoxLayout + QButtonGroup，替代原 QTreeWidget

    void windowInit(const QString& title, const QIcon& icon);
    void UIWidgetInit();
    void connectInit();
    void applyBackgroundPalettes();
    void setupSystemTray();
    void retranslateUi();
};

#endif // MAINWINDOW_H
