#include "mainwindow.h"

#include <QApplication>
#include <QBoxLayout>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QEvent>
#include <QGuiApplication>
#include <QMessageBox>
#include <QSettings>
#include <QTimer>
#include <QHBoxLayout>
#include <QFrame>
#include <QLabel>
#include <QButtonGroup>
#include <QDesktopServices>
#include <QListWidget>
#include <QStackedWidget>
#include <QThread>
#include <QTime>
#include <QUrl>
#include <QMenu>

#include <dwmapi.h>

#include <QWKWidgets/widgetwindowagent.h>
#include "ui/cursorpage.h"
#include "vendor/qwk/windowbar.h"
#include "vendor/qwk/windowbutton.h"

#include "ui/clickerpage.h"
#include "ui/settingspage.h"
#include "core/config.h"
#include "core/clickerstatusfeedback.h"
#include "ui/button.h"
#include "ui/popupmenu.h"
#include "theme/themestate.h"

static inline void emulateLeaveEvent(QWidget* widget)
{
    Q_ASSERT(widget);
    if (!widget) {
        return;
    }
    QTimer::singleShot(0, widget, [widget]() {
#if (QT_VERSION >= QT_VERSION_CHECK(5, 14, 0))
        const QScreen* screen = widget->screen();
#else
        const QScreen* screen = widget->windowHandle()->screen();
#endif
        const QPoint globalPos = QCursor::pos(screen);
        if (!QRect(widget->mapToGlobal(QPoint{0, 0}), widget->size()).contains(globalPos)) {
            QCoreApplication::postEvent(widget, new QEvent(QEvent::Leave));
            if (widget->testAttribute(Qt::WA_Hover)) {
                const QPoint localPos = widget->mapFromGlobal(globalPos);
                const QPoint scenePos = widget->window()->mapFromGlobal(globalPos);
                static constexpr const auto oldPos = QPoint{};
                const Qt::KeyboardModifiers modifiers = QGuiApplication::keyboardModifiers();
#if (QT_VERSION >= QT_VERSION_CHECK(6, 4, 0))
                const auto event =
                    new QHoverEvent(QEvent::HoverLeave, scenePos, globalPos, oldPos, modifiers);
                Q_UNUSED(localPos);
#elif (QT_VERSION >= QT_VERSION_CHECK(6, 3, 0))
                const auto event =  new QHoverEvent(QEvent::HoverLeave, localPos, globalPos, oldPos, modifiers);
                Q_UNUSED(scenePos);
#else
                const auto event =  new QHoverEvent(QEvent::HoverLeave, localPos, oldPos, modifiers);
                Q_UNUSED(scenePos);
#endif
                QCoreApplication::postEvent(widget, event);
            }
        }
    });
}

// Qt 6.7.3 的 QWindowsWindow::setDarkBorder 在收到 ApplicationPaletteChange 时，
// 会把 DWMWA_USE_IMMERSIVE_DARK_MODE 重置为浅色（shouldApplyDarkFrame 读不到
// widget 窗口的 palette，恒判为浅色），从而把 QWindowKit 露出的 1px 顶部边框刷白。
// 这里由应用接管，始终恢复深色边框，与 QWindowKit 的 dark-mode=true 默认一致：
// 若改成跟随 app 主题（浅色主题→浅色边框），浅色主题下仍会露出白色边框。
static void applyDarkFrameColor(HWND hwnd, bool dark)
{
    // MinGW 的 dwmapi.h 未定义沉浸式深色模式属性，这里显式声明。
    constexpr DWORD immersiveDarkMode = 20;        // DWMWA_USE_IMMERSIVE_DARK_MODE
    constexpr DWORD immersiveDarkModeLegacy = 19;  // DWMWA_USE_IMMERSIVE_DARK_MODE_BEFORE_20H1

    const BOOL value = dark ? TRUE : FALSE;
    if (FAILED(DwmSetWindowAttribute(hwnd, immersiveDarkMode, &value, sizeof(value))))
        DwmSetWindowAttribute(hwnd, immersiveDarkModeLegacy, &value, sizeof(value));

    // 通知 DWM 按新属性重绘非客户区边框。
    SetWindowPos(hwnd, nullptr, 0, 0, 0, 0,
                 SWP_FRAMECHANGED | SWP_NOMOVE | SWP_NOSIZE | SWP_NOZORDER | SWP_NOACTIVATE);
}


MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
{
    Config& app_settings = Config::instance();

    setWindowState(app_settings.WindowState());

    // 初始化 ThemeState 并同步到 Config 的主题状态
    ThemeState::instance().setDarkMode(app_settings.ThemeMode() == Theme::Dark);

    /******************/

    setMinimumSize(QSize(800, 600));
    resize(QSize(800, 600));
    windowInit(tr("MouseClick"), QIcon(":/svg/favicon.svg"));

    UIWidgetInit();

    // 连点运行时禁用页面内容区（导航栏保持可交互），最小化至系统托盘 / 恢复窗口
    connect(_settings_page, &SettingsPage::hotkeyActivated, this, [this]() {
        bool running = PageBase::clickerThread()->isRunning();
        _navigation_pages->setEnabled(!running);

        if (running) {
            // 连点已启动：最小化到系统托盘
            _was_maximized_before_tray = isMaximized();
            _was_hidden_before_clicker = !isVisible();
            hide();
        } else {
            // 连点已停止：仅在启动前为显示状态时才恢复窗口
            if (!_was_hidden_before_clicker) {
                if (_was_maximized_before_tray) {
                    showMaximized();
                } else {
                    showNormal();
                }
                raise();
                activateWindow();
            }
        }

        // 托盘图标 + tooltip + 声音 三路状态反馈（替代系统通知）
        if (_status_feedback) {
            _status_feedback->setRunning(running);
        }
    });

    /******************/

    connectInit();

    setupSystemTray();
}

void MainWindow::windowInit(const QString& title, const QIcon& icon)
{
    setObjectName(QStringLiteral("mainwindow"));

    // set the window agent
    _window_agent = new QWK::WidgetWindowAgent(this);
    _window_agent->setup(this);

    QLabel* titlebar_label = new QLabel();
    titlebar_label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    titlebar_label->setObjectName(QStringLiteral("titlebar-label"));
    titlebar_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    QWK::WindowButton* icon_btn = new IconButton();
    icon_btn->setObjectName(QStringLiteral("titlebar-icon-button"));
    icon_btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QWK::WindowButton* min_btn = new MinimizeButton();
    min_btn->setObjectName(QStringLiteral("titlebar-min-button"));
    min_btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QWK::WindowButton* max_btn = new MaximizeButton();
    max_btn->setObjectName(QStringLiteral("titlebar-max-button"));
    max_btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QWK::WindowButton* close_btn = new CloseButton();
    close_btn->setObjectName(QStringLiteral("titlebar-close-button"));
    close_btn->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QWK::WindowBar* titlebar = new QWK::WindowBar();
    titlebar->setIconButton(icon_btn);
    titlebar->setMinButton(min_btn);
    titlebar->setMaxButton(max_btn);
    titlebar->setCloseButton(close_btn);
    titlebar->setTitleLabel(titlebar_label);
    titlebar->setHostWidget(this);

    _window_agent->setTitleBar(titlebar);
    _window_agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, icon_btn);
    _window_agent->setSystemButton(QWK::WindowAgentBase::Minimize, min_btn);
    _window_agent->setSystemButton(QWK::WindowAgentBase::Maximize, max_btn);
    _window_agent->setSystemButton(QWK::WindowAgentBase::Close, close_btn);

    setMenuWidget(titlebar);

    // Adds simulated mouse events to the title bar buttons
    connect(icon_btn, &QAbstractButton::clicked, _window_agent, [this, icon_btn]() {
        icon_btn->setProperty("double-click-close", false);

        QTimer::singleShot(80, _window_agent, [this, icon_btn]() {
            if (icon_btn->property("double-click-close").toBool())
                return;
            _window_agent->showSystemMenu(icon_btn->mapToGlobal(QPoint{0, icon_btn->height()}));
        });
    });
    connect(icon_btn, &QWK::WindowButton::doubleClicked, this, [this, icon_btn]() {
        icon_btn->setProperty("double-click-close", true);
        close();
    });
    connect(titlebar, &QWK::WindowBar::minimizeRequested, this, &QWidget::showMinimized);
    connect(titlebar, &QWK::WindowBar::maximizeRequested, this, [this, max_btn](bool max) {
        if (max) {
            showMaximized();
        } else {
            showNormal();
        }

        // It's a Qt issue that if a QAbstractButton::clicked triggers a window's maximization,
        // the button remains to be hovered until the mouse move. As a result, we need to
        // manually send leave events to the button.
        emulateLeaveEvent(max_btn);
    });
    connect(titlebar, &QWK::WindowBar::closeRequested, this, &QWidget::close);

    // set the window title
    setWindowTitle(title);

    // set the window icon
    setWindowIcon(icon);
}

void MainWindow::UIWidgetInit()
{
    QWidget* central_widget = new QWidget(this);
    central_widget->setObjectName(QStringLiteral("central-widget"));
    central_widget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    QHBoxLayout* central_layout = new QHBoxLayout(central_widget);
    central_layout->setSpacing(0);
    central_layout->setContentsMargins(QMargins(0, 4, 8, 8));

    // ── side-nav：QWidget + QVBoxLayout 替代 QTreeWidget ──
    // QTreeWidget + setItemWidget() 的行布局管线将 itemWidget 完全封闭。
    //   - updateGeometries() 始终将 widget resize 为 visualRect 并填满整行
    //   - 无 API 可控制行内 margin"
    // QVBoxLayout 直接控制间距，无需对抗任何内部机制。
    QWidget* navigation = new QWidget(central_widget);
    navigation->setObjectName(QStringLiteral("side-nav"));
    navigation->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Preferred);
    navigation->setMaximumWidth(240);
    QVBoxLayout* nav_layout = new QVBoxLayout(navigation);
    nav_layout->setContentsMargins(12, 12, 12, 12);  // Design Token 标准间距 12px
    nav_layout->setSpacing(4);                        // button 间距保持不变

    QButtonGroup* navigation_item_btn_group = new QButtonGroup(navigation);
    navigation_item_btn_group->setExclusive(true); // 互斥：同一时刻仅一个按钮 checked

    const QString mouse_click_page_title = tr("Mouse Click");
    const QString beautify_cursor_page_title = tr("Beautify Cursor");
    const QString settings_page_title = tr("Settings");

    _nav_mouse_click = new NavButton(mouse_click_page_title, navigation);
    _nav_beautify_cursor = new NavButton(beautify_cursor_page_title, navigation);
    _nav_settings = new NavButton(settings_page_title, navigation);

    _nav_mouse_click->setCheckable(true);
    _nav_beautify_cursor->setCheckable(true);
    _nav_settings->setCheckable(true);

    _nav_mouse_click->setObjectName(QStringLiteral("nav-item-mouse-click"));
    _nav_beautify_cursor->setObjectName(QStringLiteral("nav-item-beautify-cursor"));
    _nav_settings->setObjectName(QStringLiteral("nav-item-settings"));

    // 导航图标（基础文件名，不含路径）-black/-white 后缀。
    _nav_mouse_click->setNavIcon(QStringLiteral("mouse-click-item"));
    _nav_beautify_cursor->setNavIcon(QStringLiteral("beautify-cursor-item"));
    _nav_settings->setNavIcon(QStringLiteral("settings-item"));

    navigation_item_btn_group->addButton(_nav_mouse_click, 0);
    navigation_item_btn_group->addButton(_nav_beautify_cursor, 1);
    navigation_item_btn_group->addButton(_nav_settings, 2);

    nav_layout->addWidget(_nav_mouse_click);
    nav_layout->addWidget(_nav_beautify_cursor);
    nav_layout->addWidget(_nav_settings);
    nav_layout->addStretch();  // 按钮靠上对齐

    // set Default selected
    _nav_mouse_click->setChecked(true);

    // ── nav-page 视觉容器：圆角卡片背景 + 24px 内边距 ──
    // 外层 QFrame 负责视觉效果，QStackedWidget 仅负责页面切换。
    // QFrame::StyledPanel 触发 PE_Frame 渲染路径绘制圆角背景。
    QFrame* nav_page_card = new QFrame(central_widget);
    nav_page_card->setObjectName(QStringLiteral("nav-page"));
    nav_page_card->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    QVBoxLayout* nav_card_layout = new QVBoxLayout(nav_page_card);
    nav_card_layout->setSpacing(0);
    nav_card_layout->setContentsMargins(QMargins(24, 24, 24, 24));

    QStackedWidget* navigation_pages = new QStackedWidget();
    navigation_pages->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    navigation_pages->setContentsMargins(QMargins());
    navigation_pages->setAutoFillBackground(false);

    // SettingsPage 需要优先声明，这里的设计以后会改进
    _settings_page = new SettingsPage(settings_page_title, navigation_pages);
    CursorPage* beautify_cursor_page = new CursorPage(beautify_cursor_page_title, navigation_pages);
    ClickerPage* mouse_click_page = new ClickerPage(mouse_click_page_title, *_settings_page, navigation_pages);

    navigation_pages->addWidget(mouse_click_page);
    navigation_pages->addWidget(beautify_cursor_page);
    navigation_pages->addWidget(_settings_page);

    // set Default page
    navigation_pages->setCurrentIndex(0);

    _nav_widget = navigation;

    nav_card_layout->addWidget(navigation_pages);
    central_layout->addWidget(navigation);
    central_layout->addWidget(nav_page_card);

    _nav_page_card = nav_page_card;
    _navigation_pages = navigation_pages;

    // 设置独立于全局 palette 的 widget 背景。
    applyBackgroundPalettes();

    // QButtonGroup::idClicked 直接传递 button id 到 page index 映射
    connect(navigation_item_btn_group, &QButtonGroup::idClicked,
            navigation_pages, &QStackedWidget::setCurrentIndex);

    setCentralWidget(central_widget);
}

void MainWindow::applyBackgroundPalettes()
{
    const auto& c = ThemeState::instance().current().colors;

    // side-nav：透明背景，显示 MainWindow 底色：#EEEEF2 / #1F1F1F。
    if (_nav_widget) {
        _nav_widget->setAutoFillBackground(false);
    }

    // nav-page 视觉容器：圆角卡片色背景（light=#FFF, dark=#333, radius=8px）
    // PE_Frame 绘制圆角矩形背景。autoFillBackground 必须为 false，否则会用
    // 纯色矩形填充整个 QFrame，覆盖圆角外的"透明"区域→圆角视觉消失。
    // StyledPanel 保证 PE_Frame 一定被触发绘制。
    if (_nav_page_card) {
        _nav_page_card->setFrameShape(QFrame::StyledPanel);
        _nav_page_card->setAutoFillBackground(false);
        _nav_page_card->update();
    }
}

void MainWindow::retranslateUi()
{
    setWindowTitle(tr("MouseClick"));
    _nav_mouse_click->setText(tr("Mouse Click"));
    _nav_beautify_cursor->setText(tr("Beautify Cursor"));
    _nav_settings->setText(tr("Settings"));

    // 更新系统托盘菜单文本
    if (_tray_open_action) {
        _tray_open_action->setText(tr("Open Main Interface"));
    }
    if (_tray_website_action) {
        _tray_website_action->setText(tr("Official Website"));
    }
    if (_tray_exit_action) {
        _tray_exit_action->setText(tr("Exit"));
    }
    if (_status_feedback) {
        _status_feedback->retranslate();
    }
}

void MainWindow::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QMainWindow::changeEvent(event);
}

void MainWindow::connectInit()
{
    // 主题切换：Config（持久化）→ ThemeState（QStyle 运行时）
    connect(&Config::instance(), &Config::currentThemeChanged,
            this, [](Theme::ThemeMode mode) {
        ThemeState::instance().setDarkMode(mode == Theme::Dark);
    });

    // 主题切换后重新应用独立于全局 palette 的 widget 背景。
    connect(&ThemeState::instance(), &ThemeState::themeChanged,
            this, &MainWindow::applyBackgroundPalettes);

    connect(this, &MainWindow::windowStateChanged, &Config::instance(), &Config::setWindowState);
}

MainWindow::~MainWindow()
{}

bool MainWindow::event(QEvent *event)
{
    if (event->type() == QEvent::WindowStateChange) {
        Qt::WindowStates newState = windowState();
        emit windowStateChanged(newState);
    }
    if (event->type() == QEvent::ApplicationPaletteChange) {
        // 必须排队到 Qt 的 QWindowsWindow::setDarkBorder 之后执行，否则会被它覆盖回浅色。
        // 始终设为深色（DWMWA=1），与 QWindowKit 的 dark-mode=true 默认一致，避免浅色主题露白边。
        QTimer::singleShot(0, this, [this]() {
            applyDarkFrameColor(reinterpret_cast<HWND>(winId()), true);
        });
    }
    return QWidget::event(event); // 保留其他事件处理
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (_force_quit || QCoreApplication::closingDown()) {
        event->accept();
        QMainWindow::closeEvent(event);
        return;
    }

    if (Config::instance().CloseButtonBehavior() == "minimize") {
        // 最小化至系统托盘
        _was_maximized_before_tray = isMaximized();
        hide();
        event->ignore();
    } else {
        // 正常退出
        event->accept();
        // 停止连点（如果正在运行）
        if (PageBase::clickerThread()->isRunning()) {
            PageBase::clicker()->stop();
            PageBase::clickerThread()->quit();
            PageBase::clickerThread()->wait();
        }
        QApplication::quit();
    }
}

void MainWindow::setupSystemTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable()) {
        return;
    }

    _tray_icon = new QSystemTrayIcon(QIcon(":/svg/favicon.svg"), this);

    // 状态反馈：统一管理托盘图标 / tooltip / 声音，取代系统通知
    _status_feedback = new ClickerStatusFeedback(_tray_icon, this);

    _tray_menu = new PopupMenu(this);

    _tray_open_action = _tray_menu->addAction(tr("Open Main Interface"));
    _tray_menu->addSeparator();
    _tray_website_action = _tray_menu->addAction(tr("Official Website"));
    _tray_menu->addSeparator();
    _tray_exit_action = _tray_menu->addAction(tr("Exit"));

    // 托盘图标交互：左键双击恢复窗口，右键弹出菜单
    connect(_tray_icon, &QSystemTrayIcon::activated, this,
            [this](QSystemTrayIcon::ActivationReason reason) {
        if (reason == QSystemTrayIcon::Trigger ||
            reason == QSystemTrayIcon::DoubleClick) {
            if (_was_maximized_before_tray) {
                showMaximized();
            } else {
                showNormal();
            }
            raise();
            activateWindow();
        } else if (reason == QSystemTrayIcon::Context) {
            _tray_menu->popup(QCursor::pos());
        }
    });

    // 打开主界面
    connect(_tray_open_action, &QAction::triggered, this, [this]() {
        if (_was_maximized_before_tray) {
            showMaximized();
        } else {
            showNormal();
        }
        raise();
        activateWindow();
    });

    // 官网
    connect(_tray_website_action, &QAction::triggered, this, []() {
        QDesktopServices::openUrl(
            QUrl("https://github.com/SeaEpoch/MouseClick"));
    });

    // 退出
    connect(_tray_exit_action, &QAction::triggered, this, [this]() {
        _force_quit = true;
        // 停止连点（如果正在运行）
        if (PageBase::clickerThread()->isRunning()) {
            PageBase::clicker()->stop();
            PageBase::clickerThread()->quit();
            PageBase::clickerThread()->wait();
        }
        qApp->quit();
    });

    _tray_icon->show();
}
