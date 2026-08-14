#ifndef SEPPROXYSTYLE_H
#define SEPPROXYSTYLE_H

#include "tokens.h"

#include <QProxyStyle>
#include <QWidget>

// ============================================================================
// SepProxyStyle — 自定义 QProxyStyle
// 覆盖 ~20 个虚函数，接管项目中所有控件的绘制。
// 所有颜色和间距从 ThemeState::current() 动态读取，零 QSS 文件依赖。
// ============================================================================

class SepProxyStyle : public QProxyStyle
{
    Q_OBJECT
public:
    using QProxyStyle::QProxyStyle;   // 默认继承平台原生 QStyle

    // ── 几何 ──
    int pixelMetric(PixelMetric metric, const QStyleOption* opt = nullptr,
                    const QWidget* widget = nullptr) const override;

    QSize sizeFromContents(ContentsType ct, const QStyleOption* opt,
                           const QSize& contentsSize, const QWidget* widget = nullptr) const override;

    QRect subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                         SubControl sc, const QWidget* widget = nullptr) const override;

    // ── 绘制 ──
    void drawPrimitive(PrimitiveElement pe, const QStyleOption* opt,
                       QPainter* painter, const QWidget* widget = nullptr) const override;

    void drawControl(ControlElement ce, const QStyleOption* opt,
                     QPainter* painter, const QWidget* widget = nullptr) const override;

    void drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt,
                            QPainter* painter, const QWidget* widget = nullptr) const override;

    // ── 调色板 ──
    QPalette standardPalette() const override;

    // ── 行为 ──
    int styleHint(StyleHint sh, const QStyleOption* opt = nullptr,
                  const QWidget* widget = nullptr, QStyleHintReturn* ret = nullptr) const override;

    // ── 生命周期 ──
    void polish(QWidget* widget) override;
    void unpolish(QWidget* widget) override;

private:
    const ThemeTokens& token() const;
};

#endif // SEPPROXYSTYLE_H
