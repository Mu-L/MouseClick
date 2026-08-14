#ifndef THEMESTATE_H
#define THEMESTATE_H

#include "tokens.h"

#include <QObject>

// ============================================================================
// Theme 枚举 — 原 shared.h 中的主题枚举，移入主题系统
// ============================================================================

namespace Theme {
    enum ThemeMode {
        Light = 0,
        Dark  = 1,
    };

    inline bool isValidThemeMode(int mode)
    {
        return mode == Light || mode == Dark;
    }
}

// ============================================================================
// ThemeState — 运行时主题状态持有者
// 本身不持久化（持久化由 Config 负责），仅作为 QStyle 与 Config
// 之间的桥梁，提供零开销的 Token 查询接口。
// ============================================================================

class ThemeState : public QObject
{
    Q_OBJECT
public:
    static ThemeState& instance();

    /// 返回当前主题的 Token 常量引用（零拷贝）
    const ThemeTokens& current() const;

    bool isDark() const;

public slots:
    /// 切换暗色 / 浅色模式，并发射 themeChanged 信号
    void setDarkMode(bool dark);

signals:
    void themeChanged();

private:
    explicit ThemeState(QObject* parent = nullptr);
    Q_DISABLE_COPY_MOVE(ThemeState)

    const ThemeTokens* _current = &kLightTokens;
};

#endif // THEMESTATE_H
