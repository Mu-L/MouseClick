#ifndef UI_BUTTON_H
#define UI_BUTTON_H

#include "theme/themestate.h"

#include <QEvent>
#include <QIcon>
#include <QPushButton>
#include <QStyleOptionButton>

// ============================================================================
// 按钮家族 — 配色值对象 + 操作按钮 + 导航按钮 + 标题栏系统按钮
//
// 设计：
//   ButtonPalette   — 按钮在各交互态下的颜色集合（值对象，无行为）
//   ActionButton    — 品牌强调色操作按钮（安装 / 应用 / 取消等）
//   NavButton       — 侧边导航栏按钮（checked → accent, unchecked → subtle）
//   SystemButton 们 — 标题栏 Chrome 按钮（Icon / Minimize / Maximize / Close）
//
// 颜色流：
//   tokens → ButtonPalette 工厂 → initStyleOption() → opt→palette → SepProxyStyle 渲染
// ============================================================================

// ============================================================================
// ButtonPalette — 按钮颜色值对象
// ============================================================================

struct ButtonPalette {
    // 背景色
    QColor bgNormal, bgHover, bgPressed, bgDisabled;
    // 文字色
    QColor textNormal, textHover, textPressed, textDisabled;
    // 底部强调线色
    QColor borderNormal, borderHover, borderPressed, borderDisabled;

    /// 品牌强调色按钮（操作按钮、选中态导航按钮）
    static ButtonPalette accent()
    {
        const auto& c = ThemeState::instance().current().colors;
        return {
            .bgNormal     = c.accentDefault,
            .bgHover      = c.accentHover,
            .bgPressed    = c.accentPressed,
            .bgDisabled   = c.accentDisabled,
            .textNormal   = c.textOnAccent,
            .textHover    = c.textOnAccent,
            .textPressed  = c.textOnAccent,
            .textDisabled = c.textDisabled,
            .borderNormal   = c.accentEmphasis,
            .borderHover    = c.accentEmphasis,
            .borderPressed  = c.accentPressed,
            .borderDisabled = c.accentEmphasisDisabled,
        };
    }

    /// 弱化按钮（未选中导航按钮）
    static ButtonPalette subtle()
    {
        const auto& c = ThemeState::instance().current().colors;
        return {
            .bgNormal     = Qt::transparent,
            .bgHover      = c.accentHover,
            .bgPressed    = c.accentPressed,
            .bgDisabled   = Qt::transparent,
            .textNormal   = c.textPrimary,
            .textHover    = c.textOnAccent,
            .textPressed  = c.textOnAccent,
            .textDisabled = c.textDisabled,
            .borderNormal   = Qt::transparent,
            .borderHover    = c.accentEmphasis,
            .borderPressed  = c.accentPressed,
            .borderDisabled = Qt::transparent,
        };
    }
};

// ============================================================================
// ActionButton — 品牌强调色操作按钮
// ============================================================================

class ActionButton : public QPushButton
{
    Q_OBJECT
public:
    explicit ActionButton(const QString& text, QWidget* parent = nullptr)
        : QPushButton(text, parent)
    {
        QFont f = font();
        f.setPixelSize(14);
        setFont(f);
        setProperty("themed-button", true);
        setProperty("button-text-align", static_cast<int>(Qt::AlignCenter | Qt::AlignVCenter));
    }

    explicit ActionButton(QWidget* parent = nullptr)
        : QPushButton(parent)
    {
        QFont f = font();
        f.setPixelSize(14);
        setFont(f);
        setProperty("themed-button", true);
        setProperty("button-text-align", static_cast<int>(Qt::AlignCenter | Qt::AlignVCenter));
    }

protected:
    void initStyleOption(QStyleOptionButton* opt) const override
    {
        QPushButton::initStyleOption(opt);
        applyPalette(ButtonPalette::accent(), opt);
    }

    /// 将 ButtonPalette 中当前状态对应的颜色写入 QStyleOption 的 palette
    static void applyPalette(const ButtonPalette& p, QStyleOptionButton* opt)
    {
        const bool enabled = opt->state & QStyle::State_Enabled;
        const bool down    = opt->state & QStyle::State_Sunken;
        const bool hover   = opt->state & QStyle::State_MouseOver;

        QColor bg, text, border;
        if (!enabled) {
            bg = p.bgDisabled; text = p.textDisabled; border = p.borderDisabled;
        } else if (down) {
            bg = p.bgPressed; text = p.textPressed; border = p.borderPressed;
        } else if (hover) {
            bg = p.bgHover; text = p.textHover; border = p.borderHover;
        } else {
            bg = p.bgNormal; text = p.textNormal; border = p.borderNormal;
        }

        opt->palette.setColor(QPalette::Button,     bg);
        opt->palette.setColor(QPalette::ButtonText, text);
        opt->palette.setColor(QPalette::Shadow,     border);
    }
};

// ============================================================================
// NavButton — 侧边栏导航按钮
// ============================================================================

class NavButton : public ActionButton
{
    Q_OBJECT
public:
    explicit NavButton(const QString& text, QWidget* parent = nullptr)
        : ActionButton(text, parent)
    {
        setProperty("button-text-align", static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
    }

    explicit NavButton(QWidget* parent = nullptr)
        : ActionButton(parent)
    {
        setProperty("button-text-align", static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter));
    }

    void setNavIcon(const QString& baseName) { _iconBaseName = baseName; }

protected:
    void initStyleOption(QStyleOptionButton* opt) const override
    {
        QPushButton::initStyleOption(opt);
        const auto& p = isChecked() ? ButtonPalette::accent() : ButtonPalette::subtle();
        applyPalette(p, opt);

        if (!_iconBaseName.isEmpty()) {
            const bool dark   = ThemeState::instance().isDark();
            const bool active = isChecked() || (opt->state & QStyle::State_MouseOver);
            const QString suffix = (dark || active) ? "white" : "black";
            opt->icon = QIcon(QStringLiteral(":/svg/%1-%2.svg")
                                .arg(_iconBaseName, suffix));
        }
    }

private:
    QString _iconBaseName;
};

// ============================================================================
// 标题栏 Chrome 按钮（继承 QWK::WindowButton）
// ============================================================================

#include "vendor/qwk/windowbutton.h"

class IconButton : public QWK::WindowButton
{
    Q_OBJECT
public:
    explicit IconButton(QWidget* parent = nullptr)
        : QWK::WindowButton(parent)
    {
        setIconSize(QSize(18, 18));
        setFixedSize(32, 32);
        setProperty("system-button", true);
        setIconNormal(QIcon(QStringLiteral(":/svg/favicon.svg")));
    }

protected:
    void initStyleOption(QStyleOptionButton* opt) const override
    {
        QPushButton::initStyleOption(opt);
        opt->palette.setColor(QPalette::Button, Qt::transparent);
    }
};

class MinimizeButton : public QWK::WindowButton
{
    Q_OBJECT
public:
    explicit MinimizeButton(QWidget* parent = nullptr)
        : QWK::WindowButton(parent)
    {
        setIconSize(QSize(16, 16));
        setFixedSize(32, 30);
        setProperty("system-button", true);
        reloadIcons();
    }

protected:
    void initStyleOption(QStyleOptionButton* opt) const override
    {
        QPushButton::initStyleOption(opt);
        injectChrome(opt);
    }

    void changeEvent(QEvent* e) override
    {
        QWK::WindowButton::changeEvent(e);
        if (e->type() == QEvent::PaletteChange)
            reloadIcons();
    }

private:
    void injectChrome(QStyleOptionButton* opt) const
    {
        const auto& c = ThemeState::instance().current().colors;
        const bool dark  = ThemeState::instance().isDark();
        const bool hover = opt->state & QStyle::State_MouseOver;

        opt->icon = hover
            ? QIcon(QStringLiteral(":/svg/minimize-blue.svg"))
            : QIcon(dark ? QStringLiteral(":/svg/minimize-white.svg")
                         : QStringLiteral(":/svg/minimize-black.svg"));

        opt->palette.setColor(QPalette::Button,
            hover ? c.chromeHover : Qt::transparent);
    }

    void reloadIcons()
    {
        const bool dark = ThemeState::instance().isDark();
        setIconNormal(QIcon(dark ? QStringLiteral(":/svg/minimize-white.svg")
                                 : QStringLiteral(":/svg/minimize-black.svg")));
    }
};

class MaximizeButton : public QWK::WindowButton
{
    Q_OBJECT
public:
    explicit MaximizeButton(QWidget* parent = nullptr)
        : QWK::WindowButton(parent)
    {
        setCheckable(true);
        setIconSize(QSize(16, 16));
        setFixedSize(32, 30);
        setProperty("system-button", true);
        reloadIcons();
    }

protected:
    void initStyleOption(QStyleOptionButton* opt) const override
    {
        QPushButton::initStyleOption(opt);
        injectChrome(opt);
    }

    void changeEvent(QEvent* e) override
    {
        QWK::WindowButton::changeEvent(e);
        if (e->type() == QEvent::PaletteChange)
            reloadIcons();
    }

private:
    void injectChrome(QStyleOptionButton* opt) const
    {
        const auto& c = ThemeState::instance().current().colors;
        const bool dark    = ThemeState::instance().isDark();
        const bool hover   = opt->state & QStyle::State_MouseOver;
        const bool checked = opt->state & QStyle::State_On;

        if (checked) {
            opt->icon = QIcon(hover
                ? QStringLiteral(":/svg/restore-blue.svg")
                : (dark ? QStringLiteral(":/svg/restore-white.svg")
                        : QStringLiteral(":/svg/restore-black.svg")));
        } else if (hover) {
            opt->icon = QIcon(QStringLiteral(":/svg/maximize-blue.svg"));
        } else {
            opt->icon = QIcon(dark ? QStringLiteral(":/svg/maximize-white.svg")
                                   : QStringLiteral(":/svg/maximize-black.svg"));
        }

        opt->palette.setColor(QPalette::Button,
            hover ? c.chromeHover : Qt::transparent);
    }

    void reloadIcons()
    {
        const bool dark = ThemeState::instance().isDark();
        setIconNormal(QIcon(dark ? QStringLiteral(":/svg/maximize-white.svg")
                                 : QStringLiteral(":/svg/maximize-black.svg")));
        setIconChecked(QIcon(dark ? QStringLiteral(":/svg/restore-white.svg")
                                  : QStringLiteral(":/svg/restore-black.svg")));
    }
};

class CloseButton : public QWK::WindowButton
{
    Q_OBJECT
public:
    explicit CloseButton(QWidget* parent = nullptr)
        : QWK::WindowButton(parent)
    {
        setIconSize(QSize(16, 16));
        setFixedSize(32, 30);
        setProperty("system-button", true);
        reloadIcons();
    }

protected:
    void initStyleOption(QStyleOptionButton* opt) const override
    {
        QPushButton::initStyleOption(opt);
        injectChrome(opt);
    }

    void changeEvent(QEvent* e) override
    {
        QWK::WindowButton::changeEvent(e);
        if (e->type() == QEvent::PaletteChange)
            reloadIcons();
    }

private:
    void injectChrome(QStyleOptionButton* opt) const
    {
        const auto& c = ThemeState::instance().current().colors;
        const bool dark  = ThemeState::instance().isDark();
        const bool hover = opt->state & QStyle::State_MouseOver;

        opt->icon = hover
            ? QIcon(QStringLiteral(":/svg/close-white.svg"))
            : QIcon(dark ? QStringLiteral(":/svg/close-white.svg")
                         : QStringLiteral(":/svg/close-black.svg"));

        opt->palette.setColor(QPalette::Button,
            hover ? c.chromeCloseHover : Qt::transparent);
    }

    void reloadIcons()
    {
        const bool dark = ThemeState::instance().isDark();
        setIconNormal(QIcon(dark ? QStringLiteral(":/svg/close-white.svg")
                                 : QStringLiteral(":/svg/close-black.svg")));
    }
};

#endif // UI_BUTTON_H
