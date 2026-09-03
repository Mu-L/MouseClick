#include "mainwindow.h"
#include "theme/sepproxystyle.h"
#include "theme/themestate.h"

#include <QApplication>
#include <QFont>
#include <QFontDatabase>

#include "core/config.h"
#include "core/translation.h"

int main(int argc, char* argv[])
{
    QGuiApplication::setAttribute(Qt::AA_DontCreateNativeWidgetSiblings);
    qputenv("QT_WIN_DEBUG_CONSOLE", "attach");
    qputenv("QSG_INFO", "1");

    QApplication app(argc, argv);
    QApplication::setQuitOnLastWindowClosed(false);

    // 自定义 QStyle 接管全局控件渲染（替换 QSS 文件）
    app.setStyle(new SepProxyStyle);
    QApplication::setPalette(app.style()->standardPalette());

    // 主题切换 → 全局 repolish
    QObject::connect(&ThemeState::instance(), &ThemeState::themeChanged,
                     &app, []() {
        for (QWidget* w : QApplication::topLevelWidgets()) {
            w->style()->polish(w);
        }
        QApplication::setPalette(QApplication::style()->standardPalette());
    });

    // 设置字体：加载内嵌微软雅黑字体集（常规 msyh / 粗体 msyhbd / 细体 msyhl）。
    // 必须放在 Config::instance() 之前——Config 构造时若 config.ini 缺失会弹出一个
    // MessageBox，若字体尚未设置，该弹窗会退回系统默认字体（"Microsoft YaHei UI" 9pt）
    // 而非项目指定的 12pt 微软雅黑。
    const int font_id = QFontDatabase::addApplicationFont(
        QStringLiteral(":/fonts/Microsoft YaHei/msyh.ttc"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Microsoft YaHei/msyhbd.ttc"));
    QFontDatabase::addApplicationFont(QStringLiteral(":/fonts/Microsoft YaHei/msyhl.ttc"));

    if (font_id != -1) {
        const QStringList font_families = QFontDatabase::applicationFontFamilies(font_id);
        if (!font_families.isEmpty()) {
            // msyh.ttc 是字体集，同时注册 "Microsoft YaHei" 与 "Microsoft YaHei UI"，
            // 优先取标准族名（非 UI 变体），度量与系统一致
            const int idx = font_families.indexOf(QStringLiteral("Microsoft YaHei"));
            QFont font(font_families.at(idx >= 0 ? idx : 0), 12);
            app.setFont(font);
        }
    }

    Translation::instance().init();
    Config& app_settings = Config::instance();
    Translation::instance().switchLanguage(app_settings.Language());

    MainWindow window;
    window.show();

    return app.exec();
}
