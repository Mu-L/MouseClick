#ifndef TOKENS_H
#define TOKENS_H

#include <QColor>

// ============================================================================
// 语义化颜色令牌 — 按功能角色命名，不按使用控件命名
// ============================================================================

struct ColorTokens {
    // ── 表面色 Surface ──
    QColor surfaceWindow;          // 窗口底色
    QColor surfaceCard;            // 卡片/面板底色（#nav-page、QMenu …）
    QColor surfaceField;           // 输入框/弹出层底色

    // ── 文本色 Text ──
    QColor textPrimary;            // 正文
    QColor textSecondary;          // 副文（作者名等）
    QColor textDisabled;           // 禁用态文字
    QColor textOnAccent;           // 强调色背景上的文字

    // ── 品牌强调色 Accent ──
    QColor accentDefault;          // 主色
    QColor accentHover;            // 悬停
    QColor accentPressed;          // 按下
    QColor accentDisabled;         // 禁用态填充
    QColor accentEmphasis;         // 强调线（按钮底部、导航底部）
    QColor accentEmphasisDisabled; // 禁用态强调线

    // ── 边框与分隔 Border ──
    QColor borderDivider;          // 分隔线、中性边框

    // ── 交互叠加 Interaction Overlay ──
    QColor overlayHover;           // 卡片表面的悬停底色
    QColor overlayHoverSubtle;     // 字段表面的悬停底色（更微弱）
    QColor overlaySelected;        // 选中态底色

    // ── 窗口 Chrome（标题栏控件）──
    QColor chromeHover;            // 标题栏按钮悬停背景
    QColor chromeCloseHover;       // 关闭按钮悬停背景

    // ── 特殊 Special ──
    QColor focusField;             // 输入框聚焦时的背景
};

// ============================================================================
// 间距令牌
// ============================================================================

struct SpacingTokens {
    int radius = 4;                // 统一圆角半径
    int buttonHMargin = 12;        // 按钮水平内边距
    int buttonVMargin = 4;         // 按钮垂直内边距
    int inputPaddingLeft = 12;     // 输入控件文字左内边距
    int comboArrowWidth = 24;      // ComboBox 下拉箭头宽度
    int spinArrowWidth = 24;       // SpinBox 增减按钮宽度
    int menuHMargin = 4;           // 菜单水平边距
    int menuVMargin = 4;           // 菜单垂直边距
    int menuItemPaddingH = 12;     // 菜单项文字水平内边距
    int menuItemPaddingV = 4;      // 菜单项文字垂直内边距
    int menuSeparatorMargin = 8;   // 菜单分隔线左右边距
};

// ============================================================================
// 完整主题令牌包
// ============================================================================

struct ThemeTokens {
    ColorTokens colors;
    SpacingTokens spacing;
};

// ============================================================================
// 暗色主题
// ============================================================================

inline const ThemeTokens kDarkTokens = {
    .colors = {
        // Surface
        .surfaceWindow          = QColor(0x1F, 0x1F, 0x1F),
        .surfaceCard            = QColor(0x33, 0x33, 0x33),
        .surfaceField           = QColor(0x46, 0x46, 0x46),

        // Text
        .textPrimary            = QColor(0xFA, 0xFA, 0xFA),
        .textSecondary          = QColor(0x94, 0x94, 0x94),
        .textDisabled           = QColor(0xA5, 0xA5, 0xA5),
        .textOnAccent           = QColor(0xFA, 0xFA, 0xFA),

        // Accent
        .accentDefault          = QColor(0x3A, 0xA0, 0xF5),
        .accentHover            = QColor(0x4D, 0xAA, 0xF7),
        .accentPressed          = QColor(0x5C, 0xB0, 0xF7),
        .accentDisabled         = QColor(0x9B, 0xD2, 0xFF),
        .accentEmphasis         = QColor(0x29, 0x87, 0xD5),
        .accentEmphasisDisabled = QColor(0x95, 0xCB, 0xF5),

        // Border
        .borderDivider          = QColor(0x55, 0x55, 0x55),

        // Overlay
        .overlayHover           = QColor(0x59, 0x59, 0x59),
        .overlayHoverSubtle     = QColor(0x2A, 0x2D, 0x2E),
        .overlaySelected        = QColor(0x52, 0x52, 0x52),

        // Chrome
        .chromeHover            = QColor(0x3D, 0x3D, 0x3D),
        .chromeCloseHover       = QColor(0xE5, 0x3C, 0x3C),

        // Special
        .focusField             = QColor(0xEC, 0xF5, 0xFF),
    },
    .spacing = SpacingTokens{}   // 使用默认间距值
};

// ============================================================================
// 浅色主题
// ============================================================================

inline const ThemeTokens kLightTokens = {
    .colors = {
        // Surface
        .surfaceWindow          = QColor(0xEE, 0xEE, 0xF2),
        .surfaceCard            = QColor(0xFF, 0xFF, 0xFF),
        .surfaceField           = QColor(0xFF, 0xFF, 0xFF),

        // Text
        .textPrimary            = QColor(0x1E, 0x1E, 0x1E),
        .textSecondary          = QColor(0x94, 0x94, 0x94),
        .textDisabled           = QColor(0x78, 0x78, 0x78),
        .textOnAccent           = QColor(0xFA, 0xFA, 0xFA),

        // Accent（与暗色主题相同）
        .accentDefault          = QColor(0x3A, 0xA0, 0xF5),
        .accentHover            = QColor(0x4D, 0xAA, 0xF7),
        .accentPressed          = QColor(0x5C, 0xB0, 0xF7),
        .accentDisabled         = QColor(0x9B, 0xD2, 0xFF),
        .accentEmphasis         = QColor(0x29, 0x87, 0xD5),
        .accentEmphasisDisabled = QColor(0x95, 0xCB, 0xF5),

        // Border
        .borderDivider          = QColor(0xE0, 0xE0, 0xE0),

        // Overlay
        .overlayHover           = QColor(0xE5, 0xF3, 0xFF),
        .overlayHoverSubtle     = QColor(0xCC, 0xE8, 0xFF),
        .overlaySelected        = QColor(0xCC, 0xE8, 0xFF),

        // Chrome
        .chromeHover            = QColor(0xFC, 0xFC, 0xFD),
        .chromeCloseHover       = QColor(0xE5, 0x3C, 0x3C),

        // Special
        .focusField             = QColor(0xEC, 0xF5, 0xFF),
    },
    .spacing = SpacingTokens{}
};

#endif // TOKENS_H
