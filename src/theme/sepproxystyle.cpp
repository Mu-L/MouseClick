#include "sepproxystyle.h"
#include "themestate.h"

#include <QAbstractItemView>
#include <QAbstractScrollArea>
#include <QAbstractSpinBox>
#include <QComboBox>
#include <QLineEdit>
#include <QListView>
#include <QMenu>
#include <QPainter>
#include <QPainterPath>
#include <QPushButton>
#include <QStyleOptionButton>
#include <QStyleOptionComboBox>
#include <QStyleOptionMenuItem>
#include <QStyleOptionSpinBox>
#include <QStyleOptionViewItem>

// ============================================================================
// 辅助方法
// ============================================================================

const ThemeTokens& SepProxyStyle::token() const
{
    return ThemeState::instance().current();
}

// ============================================================================
// pixelMetric — 像素度量
// ============================================================================

int SepProxyStyle::pixelMetric(PixelMetric metric, const QStyleOption* opt,
                              const QWidget* widget) const
{
    switch (metric) {
    case PM_DefaultFrameWidth:          return 2;
    case PM_ComboBoxFrameWidth:         return 2;
    case PM_SpinBoxFrameWidth:          return 2;
    case PM_MenuHMargin:                return token().spacing.menuHMargin;
    case PM_MenuVMargin:                return token().spacing.menuVMargin;
    case PM_MenuPanelWidth:             return 0;  // PE_PanelMenu 自绘边框，无需原生 frame 宽度
    case PM_ExclusiveIndicatorWidth:    return 32;
    case PM_ExclusiveIndicatorHeight:   return 32;
    case PM_ButtonIconSize:             return 16;
    default:
        return QProxyStyle::pixelMetric(metric, opt, widget);
    }
}

// ============================================================================
// sizeFromContents — 尺寸计算
// ============================================================================

QSize SepProxyStyle::sizeFromContents(ContentsType ct, const QStyleOption* opt,
                                     const QSize& contentsSize, const QWidget* widget) const
{
    switch (ct) {
    case CT_ComboBox: {
        QSize sz = QProxyStyle::sizeFromContents(ct, opt, contentsSize, widget);
        sz.setHeight(qMax(sz.height(), 32));
        return sz;
    }
    case CT_LineEdit: {
        QSize sz = QProxyStyle::sizeFromContents(ct, opt, contentsSize, widget);
        // 跳过 SpinBox 内部 QLineEdit：其高度由 CC_SpinBox 统一决定，
        // 与 polish() 中的同名判断保持一致。
        if (widget) {
            const QWidget* pw = widget->parentWidget();
            if (pw && qobject_cast<const QAbstractSpinBox*>(pw))
                return sz;
        }
        sz.setHeight(qMax(sz.height(), 32));
        return sz;
    }
    case CT_MenuItem: {
        // 不再委托原生 Windows QStyle，避免其注入 checkmark/箭头
        // 区域空间（~36-40px），使菜单宽度仅由文字内容+padding决定。
        const auto* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
        const auto& sp = token().spacing;

        if (mi && mi->menuItemType == QStyleOptionMenuItem::Separator) {
            // 分隔线高度 = 上间距 + 1px 线 + 下间距，匹配原始 QSS 4+1+4=9px
            return {contentsSize.width(), sp.menuItemPaddingV * 2 + 1};
        }

        const int w = contentsSize.width() + sp.menuItemPaddingH * 2;
        const int h = contentsSize.height() + sp.menuItemPaddingV * 2;
        return {w, h};
    }
    case CT_PushButton: {
        QSize sz = QProxyStyle::sizeFromContents(ct, opt, contentsSize, widget);
        sz.setHeight(qMax(sz.height(), 32));
        return sz;
    }
    case CT_ItemViewItem: {
        QSize sz = QProxyStyle::sizeFromContents(ct, opt, contentsSize, widget);
        if (widget && widget->inherits("QComboBoxListView")) {
            sz.setHeight(qMax(sz.height(), 32));
        }
        return sz;
    }
    default:
        return QProxyStyle::sizeFromContents(ct, opt, contentsSize, widget);
    }
}

// ============================================================================
// subControlRect — 复合控件子区域几何
// ============================================================================

QRect SepProxyStyle::subControlRect(ComplexControl cc, const QStyleOptionComplex* opt,
                                   SubControl sc, const QWidget* widget) const
{
    switch (cc) {
    case CC_ComboBox: {
        const auto* cmb = qstyleoption_cast<const QStyleOptionComboBox*>(opt);
        if (!cmb) break;
        const int frame = 2;
        const int arrowW = token().spacing.comboArrowWidth;
        const QRect r = opt->rect.adjusted(frame, frame, -frame, -frame);

        switch (sc) {
        case SC_ComboBoxArrow:
            return QRect(r.right() - arrowW + 1, r.top(), arrowW, r.height());
        case SC_ComboBoxEditField: {
            const int iconPad = cmb->currentIcon.isNull() ? 0
                               : cmb->iconSize.width() + 4;
            return QRect(r.left() + token().spacing.inputPaddingLeft + iconPad,
                         r.top(),
                         r.width() - arrowW - token().spacing.inputPaddingLeft - iconPad,
                         r.height());
        }
        default: break;
        }
        break;
    }
    case CC_SpinBox: {
        const auto* sb = qstyleoption_cast<const QStyleOptionSpinBox*>(opt);
        if (!sb) break;
        const int frame = 2;
        const int btnW = token().spacing.spinArrowWidth;
        const QRect r = opt->rect.adjusted(frame, frame, -frame, -frame);

        switch (sc) {
        case SC_SpinBoxUp:
            return QRect(r.right() - btnW + 1, r.top(), btnW, r.height() / 2);
        case SC_SpinBoxDown:
            return QRect(r.right() - btnW + 1, r.top() + r.height() / 2,
                         btnW, r.height() / 2);
        case SC_SpinBoxEditField:
            return QRect(r.left() + token().spacing.inputPaddingLeft, r.top(),
                         r.width() - btnW - token().spacing.inputPaddingLeft,
                         r.height());
        default: break;
        }
        break;
    }
    default: break;
    }
    return QProxyStyle::subControlRect(cc, opt, sc, widget);
}

// ============================================================================
// drawPrimitive — 原始图形绘制
// ============================================================================

void SepProxyStyle::drawPrimitive(PrimitiveElement pe, const QStyleOption* opt,
                                 QPainter* painter, const QWidget* widget) const
{
    const auto& c = token().colors;
    const auto& sp = token().spacing;

    switch (pe) {

    // ── QPushButton 背景 ──
    // QStyle 不判断控件身份，只从 palette 读取并渲染。新增按钮样式不改此处一行代码
    case PE_PanelButtonCommand: {
        const bool isSystemBtn = widget && widget->property("system-button").toBool();

        if (isSystemBtn) {
            const QColor bg = opt->palette.color(QPalette::Button);
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(bg);
            painter->drawRoundedRect(opt->rect, 2, 2);   // chrome radius = 2px
            return;
        }

        // ── ActionButton / NavButton ──
        const bool themed = widget && widget->property("themed-button").toBool();
        if (themed) {
            const QColor bg     = opt->palette.color(QPalette::Button);
            const QColor border = opt->palette.color(QPalette::Shadow);

            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(bg);
            painter->drawRoundedRect(opt->rect, sp.radius, sp.radius);

            if (border.alpha() > 0) {
                painter->save();
                // 以按钮自身圆角矩形为 clip，底部强调线两端自然被圆角裁切
                QPainterPath buttonShape;
                buttonShape.addRoundedRect(QRectF(opt->rect), sp.radius, sp.radius);
                painter->setClipPath(buttonShape);
                painter->setPen(QPen(border, 2));
                const QRectF r = QRectF(opt->rect);
                painter->drawLine(QPointF(r.left(), r.bottom() - 1),
                                  QPointF(r.right(), r.bottom() - 1));
                painter->restore();
            }
            return;
        }

        // ── Fallback: 普通 QPushButton（QMessageBox::addButton 等）──
        // 非 ActionButton 子类，走 token 的状态色值映射
        {
            const bool down   = opt->state & State_Sunken;
            const bool hover  = opt->state & State_MouseOver;
            const bool enable = opt->state & State_Enabled;

            QColor bg, border;
            if (!enable) {
                bg     = c.accentDisabled;
                border = c.accentEmphasisDisabled;
            } else if (down) {
                bg     = c.accentPressed;
                border = c.accentPressed;
            } else if (hover) {
                bg     = c.accentHover;
                border = c.accentEmphasis;
            } else {
                bg     = c.accentDefault;
                border = c.accentEmphasis;
            }

            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(bg);
            painter->drawRoundedRect(opt->rect, sp.radius, sp.radius);

            if (border.alpha() > 0) {
                painter->save();
                QPainterPath buttonShape;
                buttonShape.addRoundedRect(QRectF(opt->rect), sp.radius, sp.radius);
                painter->setClipPath(buttonShape);
                painter->setPen(QPen(border, 2));
                const QRectF r = QRectF(opt->rect);
                painter->drawLine(QPointF(r.left(), r.bottom() - 1),
                                  QPointF(r.right(), r.bottom() - 1));
                painter->restore();
            }
            return;
        }
    }

    // ── 输入框边框 ──
    // 仅负责绘制边框线。背景填充由 PE_PanelLineEdit 负责（Qt QCommonStyle 委托模式）。
    case PE_FrameLineEdit: {
        const bool enabled = opt->state & State_Enabled;
        const QColor borderColor = enabled ? c.accentDefault : c.accentEmphasisDisabled;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setClipping(false);
        painter->setPen(QPen(borderColor, 2));
        painter->setBrush(Qt::NoBrush);
        // opt->rect = contentsRect()，adjusted(1,1,-1,-1) 补偿 2px 笔宽
        painter->drawRoundedRect(QRectF(opt->rect).adjusted(1, 1, -1, -1),
                                 sp.radius, sp.radius);
        painter->restore();
        return;
    }

    // ── 编辑区背景面板 ──
    // 遵循 Qt 官方委托模式（QCommonStyle）：填充背景后委托 PE_FrameLineEdit 画边框。
    // SpinBox 内部 QLineEdit 的背景和边框均由 CC_SpinBox 统一负责，此处完全跳过，
    // 避免 fillRect 覆盖 CC_SpinBox 已绘制的底部边框（造成局部变细）。
    case PE_PanelLineEdit: {
        if (widget) {
            const QWidget* pw = widget->parentWidget();
            if (pw && qobject_cast<const QAbstractSpinBox*>(pw))
                return;  // CC_SpinBox 负责背景 + 边框
        }

        painter->fillRect(opt->rect, c.surfaceField);
        drawPrimitive(PE_FrameLineEdit, opt, painter, widget);
        return;
    }

    // ── QComboBox 弹出窗口背景（QComboBoxPrivateContainer / QFrame）──
    case PE_Frame: {
        if (widget && widget->objectName() == QStringLiteral("nav-page")) {
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(c.surfaceCard);
            painter->drawRoundedRect(opt->rect, 8, 8);
            return;
        }
        if (widget && widget->inherits("QComboBoxPrivateContainer")) {
            // 容器窗口已由 polish() 设为半透明 + Frameless + NoDropShadow，
            // 窗口角落为真正透明（alpha=0），此处只需在圆角区域内绘制背景和边框。
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(QPen(c.accentDefault, 2));
            painter->setBrush(c.surfaceCard);
            painter->drawRoundedRect(QRectF(opt->rect).adjusted(1, 1, -1, -1),
                                     sp.radius, sp.radius);
            return;
        }
        // QListWidget / QTreeWidget 等列表控件：抑制 Windows 原生 3D 边框
        // 实现平坦设计（等价于 QSS 的 border: none）
        if (widget && qobject_cast<const QAbstractItemView*>(widget))
            return;
        break;
        // QFrame 由平台原生绘制
    }

    // ── QMenu 背景（系统托盘菜单等）──
    // PE_PanelMenu 已包含完整的背景+边框绘制，PE_FrameMenu 空处理避免
    // 基类 QProxyStyle 绘制 Windows 原生菜单 3D 边框覆盖主题渲染。
    case PE_FrameMenu:
        return;

    case PE_PanelMenu: {
        // 半透明窗口模式下由 PE_Frame 统一绘制圆角内容，此处无需再画背景
        if (widget && widget->inherits("QComboBoxPrivateContainer"))
            return;
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(c.surfaceCard);
        painter->setPen(QPen(c.borderDivider, 1));
        painter->drawRoundedRect(QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                 6, 6);
        return;
    }

    // ── Tooltip 面板背景（QToolTip）──
    // QToolTip 的底色由 PE_PanelTipLabel 绘制；基类 QProxyStyle 委托到系统原生样式，
    // 使用系统 tooltip 配色（白底灰框），忽略主题 ToolTipBase，深色模式下白底白字不可读。
    // 这里与 PE_PanelMenu 一致，按主题 surfaceCard + borderDivider 绘制。
    case PE_PanelTipLabel: {
        painter->setRenderHint(QPainter::Antialiasing);
        painter->setBrush(c.surfaceCard);
        painter->setPen(QPen(c.borderDivider, 1));
        painter->drawRoundedRect(QRectF(opt->rect).adjusted(0.5, 0.5, -0.5, -0.5),
                                 sp.radius, sp.radius);
        return;
    }

    // ── 列表/树项面板背景（由 CE_ItemViewItem 接管，此处跳过避免重复）──
    case PE_PanelItemViewItem:
        return;

    // ── RadioButton 指示器（Toggle Switch）──
    case PE_IndicatorRadioButton: {
        const bool checked = opt->state & State_On;
        const bool enabled = opt->state & State_Enabled;

        QString iconPath;
        if (checked) {
            iconPath = enabled ? ":/svg/toggle-right-fill-blue.svg"
                               : ":/svg/toggle-right-fill-light-blue.svg";
        } else {
            iconPath = enabled ? ":/svg/toggle-left-line-gray.svg"
                               : ":/svg/toggle-left-line-light-gray.svg";
        }
        QIcon(iconPath).paint(painter, opt->rect);
        return;
    }

    // ── 箭头指示器（ComboBox 下拉箭头，主题色）──
    case PE_IndicatorArrowDown:
    case PE_IndicatorArrowUp: {
        const bool enabled = opt->state & State_Enabled;
        const QString base = (pe == PE_IndicatorArrowUp) ? "up-line" : "down-line";
        const QString colorKey = ThemeState::instance().isDark() ? "white" : "black";
        QIcon(QString(":/svg/%1-%2.svg").arg(base, enabled ? colorKey : "light-gray"))
            .paint(painter, opt->rect, Qt::AlignCenter);
        return;
    }

    // ── 箭头指示器（SpinBox 增减箭头，三态变色）──
    case PE_IndicatorSpinUp:
    case PE_IndicatorSpinDown: {
        const bool enabled = opt->state & State_Enabled;
        const bool pressed = opt->state & State_Sunken;
        const QString base = (pe == PE_IndicatorSpinUp) ? "up-line" : "down-line";
        QString colorKey;
        if (!enabled)
            colorKey = "light-gray";
        else if (pressed)
            colorKey = "gray";
        else
            colorKey = ThemeState::instance().isDark() ? "white" : "black";
        QIcon(QString(":/svg/%1-%2.svg").arg(base, colorKey))
            .paint(painter, opt->rect, Qt::AlignCenter);
        return;
    }

    default:
        break;
    }

    QProxyStyle::drawPrimitive(pe, opt, painter, widget);
}

// ============================================================================
// drawControl — 控件级元素绘制
// ============================================================================

void SepProxyStyle::drawControl(ControlElement ce, const QStyleOption* opt,
                               QPainter* painter, const QWidget* widget) const
{
    const auto& c = token().colors;
    const auto& sp = token().spacing;

    switch (ce) {

    // ── QPushButton 整体绘制 ──
    // QWindowsVistaStyle 的 CE_PushButton 使用 uxtheme.dll 原生渲染
    // 完全绕过 PE_PanelButtonCommand，所以必须在此拦截并手动串联
    case CE_PushButtonBevel:
    case CE_PushButton: {
        drawPrimitive(PE_PanelButtonCommand, opt, painter, widget);

        // Label 使用 SE_PushButtonContents 计算内容区（Qt 官方模式），
        // 保证文字/图标在按钮边框内部正确居中。
        if (const auto* btn = qstyleoption_cast<const QStyleOptionButton*>(opt)) {
            QStyleOptionButton subopt = *btn;
            subopt.rect = subElementRect(SE_PushButtonContents, btn, widget);
            drawControl(CE_PushButtonLabel, &subopt, painter, widget);
        } else {
            drawControl(CE_PushButtonLabel, opt, painter, widget);
        }

        // 键盘焦点矩形（Qt 官方模式）
        if (opt->state & State_HasFocus) {
            QStyleOptionFocusRect fropt;
            fropt.QStyleOption::operator=(*opt);
            fropt.rect = subElementRect(SE_PushButtonFocusRect, opt, widget);
            drawPrimitive(PE_FrameFocusRect, &fropt, painter, widget);
        }
        return;
    }

    // ── 按钮文字 + icon ──
    case CE_PushButtonLabel: {
        if (widget && widget->property("system-button").toBool())
            break;

        const bool themed = widget && widget->property("themed-button").toBool();
        if (!themed) {
            QProxyStyle::drawControl(ce, opt, painter, widget);
            return;
        }

        const auto* btnOpt = qstyleoption_cast<const QStyleOptionButton*>(opt);
        if (!btnOpt) break;

        const int alignProp = widget->property("button-text-align").toInt();
        const Qt::Alignment alignment = (alignProp != 0)
            ? static_cast<Qt::Alignment>(alignProp)
            : (Qt::AlignCenter | Qt::AlignVCenter);

        const bool enabled = opt->state & State_Enabled;
        const auto& sp = token().spacing;

        // 左对齐按钮（NavButton）：图标+文字 12px 左内边距
        // 居中按钮（StyledButton）：4px 左内边距，保证图标不贴边
        const bool leftAligned = alignment & Qt::AlignLeft;
        const int padL = leftAligned ? sp.inputPaddingLeft : 4;
        const int iconW = btnOpt->icon.isNull() ? padL
                                                : padL + btnOpt->iconSize.width() + 4;

        if (!btnOpt->icon.isNull()) {
            const QSize is = btnOpt->iconSize;
            const QRect iconRect(btnOpt->rect.left() + padL,
                                 btnOpt->rect.center().y() - is.height() / 2,
                                 is.width(), is.height());
            btnOpt->icon.paint(painter, iconRect, Qt::AlignCenter,
                               enabled ? QIcon::Normal : QIcon::Disabled);
        }

        const QRect textRect = btnOpt->rect.adjusted(iconW, 0, -4, 0);
        drawItemText(painter, textRect,
                     alignment | Qt::TextShowMnemonic,
                     btnOpt->palette, enabled,
                     btnOpt->text, QPalette::ButtonText);
        return;
    }

    // ── QComboBox 当前选中项的文字 + icon ──
    // 遵循 Qt 官方模式：CC_ComboBox 绘制背景/边框后委托此处绘制标签，
    // 同时兼容 Qt 内部独立调用 CE_ComboBoxLabel 的路径。
    // ── QComboBox 当前选中项的文字 + icon ──
    // 遵循 Qt 官方委托模式：CC_ComboBox 绘制背景/边框/箭头后委托此处绘制标签。
    // 注意：我们的 SC_ComboBoxEditField 不含 icon 区域（icon 在 editRect 左侧外侧），
    // 因此此处不使用 setClipRect——icon 与文字物理分离，无需 clip 保护。
    case CE_ComboBoxLabel:
        if (const auto* cb = qstyleoption_cast<const QStyleOptionComboBox*>(opt)) {
            const bool enabled = opt->state & State_Enabled;
            const QRect editRect = subControlRect(CC_ComboBox, cb,
                                                   SC_ComboBoxEditField, widget);

            if (!cb->currentIcon.isNull()) {
                const QSize is = cb->iconSize;
                const int iconX = pixelMetric(PM_ComboBoxFrameWidth, opt, widget)
                                  + sp.inputPaddingLeft;
                const QRect iconRect(iconX,
                                     (opt->rect.height() - is.height()) / 2,
                                     is.width(), is.height());
                cb->currentIcon.paint(painter, iconRect, Qt::AlignCenter);
            }

            painter->save();
            painter->setPen(enabled ? c.textPrimary : c.textDisabled);
            const QString elided = painter->fontMetrics().elidedText(
                cb->currentText, Qt::ElideRight, editRect.width() - 8);
            painter->drawText(editRect.adjusted(0, 0, -4, 0),
                              Qt::AlignVCenter | Qt::AlignLeft, elided);
            painter->restore();
            return;
        }
        break;

    // ── QListWidget / QTreeWidget item ──
    case CE_ItemViewItem: {
        const auto* ivi = qstyleoption_cast<const QStyleOptionViewItem*>(opt);
        if (!ivi) break;

        const bool isComboPopup = widget && widget->inherits("QComboBoxListView");

        painter->save();

        // ── 裁剪路径：防止直角 item 穿透容器圆角边框 ──
        // ListView 填充容器内容区（0 边距），其 items 的直角 fillRect
        // 会在圆角弧线区域产生视觉穿透。设置与容器边框内缘匹配的圆角
        // 裁剪路径（radius = sp.radius），从几何层约束所有绘制。
        if (isComboPopup) {
            QPainterPath clipPath;
            clipPath.addRoundedRect(QRectF(widget->rect()), sp.radius, sp.radius);
            painter->setClipPath(clipPath);
        }

        if (ivi->state & State_Selected) {
            painter->fillRect(ivi->rect, c.overlaySelected);
        } else if (ivi->state & State_MouseOver) {
            painter->fillRect(ivi->rect, isComboPopup ? c.overlayHoverSubtle : c.overlayHover);
        }

        // QSS padding-left: 12px 左右内边距
        if (isComboPopup) {
            // 不委托 QProxyStyle（其内部 viewItemDrawText 会加
            // PM_FocusFrameHMargin+1 的额外文本边距），直接绘制 icon 与文本，
            // 确保文本与 CE_ComboBoxLabel 的文本左起始位置一致
            const bool itemEnabled = ivi->state & State_Enabled;

            // 图标（若有）
            int iconPad = 0;
            if (!ivi->icon.isNull()) {
                QSize is = ivi->decorationSize;
                if (is.width() <= 0 || is.height() <= 0)
                    is = QSize(16, 16);
                const QRect iconRect(ivi->rect.left() + sp.inputPaddingLeft,
                                     ivi->rect.top() + (ivi->rect.height() - is.height()) / 2,
                                     is.width(), is.height());
                ivi->icon.paint(painter, iconRect, Qt::AlignCenter);
                iconPad = is.width() + 4;
            }

            // 文本
            painter->setPen(itemEnabled ? c.textPrimary : c.textDisabled);
            const QRect textRect = ivi->rect.adjusted(
                sp.inputPaddingLeft + iconPad, 0, -sp.inputPaddingLeft, 0);
            QString text = ivi->text;
            text.remove(QLatin1Char('&'));
            const QString elided = painter->fontMetrics().elidedText(
                text, Qt::ElideRight, textRect.width());
            painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, elided);
        } else {
            QProxyStyle::drawControl(ce, opt, painter, widget);
        }

        painter->restore();
        return;
    }

    // ── QMenu 空区域背景——由 PE_PanelMenu 统一绘制，此处空返回 ──
    // 避免基类 QProxyStyle 以原生背景色填充，覆盖主题背景。
    case CE_MenuEmptyArea:
        return;

    // ── QMenu item（系统托盘菜单）──
    case CE_MenuItem: {
        const auto* mi = qstyleoption_cast<const QStyleOptionMenuItem*>(opt);
        if (!mi) break;

        const auto& sp = token().spacing;

        if (mi->menuItemType == QStyleOptionMenuItem::Separator) {
            const int y = mi->rect.center().y();
            painter->setPen(QPen(c.borderDivider, 1));
            painter->drawLine(mi->rect.left() + sp.menuSeparatorMargin, y,
                              mi->rect.right() - sp.menuSeparatorMargin, y);
            return;
        }

        const bool selected = mi->state & State_Selected;
        const bool enabled  = mi->state & State_Enabled;

        // 选中背景（圆角）
        if (selected && enabled) {
            painter->setRenderHint(QPainter::Antialiasing);
            painter->setPen(Qt::NoPen);
            painter->setBrush(c.accentDefault);
            const QRect selRect = mi->rect.adjusted(sp.radius - 1, sp.radius - 1,
                                                       -(sp.radius - 1), -(sp.radius - 1));
            painter->drawRoundedRect(QRectF(selRect), sp.radius, sp.radius);
        }

        // 文字
        painter->setPen(enabled ? (selected ? c.textOnAccent : c.textPrimary)
                                : c.textDisabled);
        QFont f = mi->font;
        f.setPixelSize(12);
        painter->setFont(f);

        const QRect textRect = mi->rect.adjusted(sp.menuItemPaddingH, sp.menuItemPaddingV,
                                                  -sp.menuItemPaddingH, -sp.menuItemPaddingV);
        QString text = mi->text;
        text.remove(QLatin1Char('&'));  // 去除助记符
        painter->drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, text);
        return;
    }

    default:
        break;
    }

    QProxyStyle::drawControl(ce, opt, painter, widget);
}

// ============================================================================
// drawComplexControl — 复合控件绘制
// ============================================================================

void SepProxyStyle::drawComplexControl(ComplexControl cc, const QStyleOptionComplex* opt,
                                      QPainter* painter, const QWidget* widget) const
{
    const auto& c = token().colors;
    const auto& sp = token().spacing;

    switch (cc) {

    // ── QComboBox ──
    case CC_ComboBox: {
        const auto* cmbOpt = qstyleoption_cast<const QStyleOptionComboBox*>(opt);
        if (!cmbOpt) break;

        const bool enabled = opt->state & State_Enabled;
        const bool popupOn = opt->state & State_On;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 背景 + 边框
        painter->setBrush(c.surfaceField);
        painter->setPen(QPen(enabled ? c.accentDefault
                                     : c.accentEmphasisDisabled, 2));
        painter->drawRoundedRect(QRectF(opt->rect).adjusted(1, 1, -1, -1),
                                 sp.radius, sp.radius);

        // 委托 CE_ComboBoxLabel 绘制 icon + 文字（Qt 官方模式）
        drawControl(CE_ComboBoxLabel, opt, painter, widget);

        // 下拉箭头：委托 PE_IndicatorArrowDown / PE_IndicatorArrowUp
        {
            QStyleOption indicatorOpt;
            indicatorOpt.QStyleOption::operator=(*opt);
            const QRect arrowRect = subControlRect(CC_ComboBox, opt,
                                                    SC_ComboBoxArrow, widget);
            indicatorOpt.rect = arrowRect.adjusted(4, 7, -4, -7);
            drawPrimitive(popupOn ? PE_IndicatorArrowUp : PE_IndicatorArrowDown,
                          &indicatorOpt, painter, widget);
        }

        painter->restore();
        return;
    }

    // ── QDoubleSpinBox ──
    case CC_SpinBox: {
        const auto* sbOpt = qstyleoption_cast<const QStyleOptionSpinBox*>(opt);
        if (!sbOpt) break;

        const bool enabled = opt->state & State_Enabled;

        painter->save();
        painter->setRenderHint(QPainter::Antialiasing);

        // 整体边框
        painter->setBrush(c.surfaceField);
        painter->setPen(QPen(enabled ? c.accentDefault
                                     : c.accentEmphasisDisabled, 2));
        painter->drawRoundedRect(QRectF(opt->rect).adjusted(1, 1, -1, -1),
                                 sp.radius, sp.radius);

        // Up 按钮
        if (sbOpt->subControls & SC_SpinBoxUp) {
            const QRect upRect = subControlRect(CC_SpinBox, opt,
                                                 SC_SpinBoxUp, widget);
            const bool upHover = (sbOpt->activeSubControls == SC_SpinBoxUp);

            if (upHover && enabled) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(c.overlayHoverSubtle);
                painter->drawRoundedRect(QRectF(upRect).adjusted(1, 2, -1, 0),
                                         sp.radius, sp.radius);
            }

            // Arrow: 委托 PE_IndicatorSpinUp
            {
                QStyleOption indicatorOpt;
                indicatorOpt.QStyleOption::operator=(*opt);
                indicatorOpt.state = enabled ? QStyle::State_Enabled : QStyle::State_None;
                if (upHover && (opt->state & State_Sunken))
                    indicatorOpt.state |= State_Sunken;
                indicatorOpt.rect = upRect.adjusted(3, 0, -4, 0);
                drawPrimitive(PE_IndicatorSpinUp, &indicatorOpt, painter, widget);
            }
        }

        // Down 按钮
        if (sbOpt->subControls & SC_SpinBoxDown) {
            const QRect downRect = subControlRect(CC_SpinBox, opt,
                                                   SC_SpinBoxDown, widget);
            const bool downHover = (sbOpt->activeSubControls == SC_SpinBoxDown);

            if (downHover && enabled) {
                painter->setPen(Qt::NoPen);
                painter->setBrush(c.overlayHoverSubtle);
                painter->drawRoundedRect(QRectF(downRect).adjusted(1, 0, -1, -2),
                                         sp.radius, sp.radius);
            }

            // Arrow: 委托 PE_IndicatorSpinDown
            {
                QStyleOption indicatorOpt;
                indicatorOpt.QStyleOption::operator=(*opt);
                indicatorOpt.state = enabled ? QStyle::State_Enabled : QStyle::State_None;
                if (downHover && (opt->state & State_Sunken))
                    indicatorOpt.state |= State_Sunken;
                indicatorOpt.rect = downRect.adjusted(3, 0, -4, 0);
                drawPrimitive(PE_IndicatorSpinDown, &indicatorOpt, painter, widget);
            }
        }

        painter->restore();
        return;
    }

    default:
        break;
    }

    QProxyStyle::drawComplexControl(cc, opt, painter, widget);
}

// ============================================================================
// standardPalette — 全局调色板
// ============================================================================

QPalette SepProxyStyle::standardPalette() const
{
    const auto& c = token().colors;
    QPalette p;

    p.setColor(QPalette::Window,           c.surfaceWindow);
    p.setColor(QPalette::WindowText,       c.textPrimary);
    p.setColor(QPalette::Base,             c.surfaceCard);
    p.setColor(QPalette::AlternateBase,    c.surfaceCard);
    p.setColor(QPalette::Text,             c.textPrimary);
    p.setColor(QPalette::Button,           c.accentDefault);
    p.setColor(QPalette::ButtonText,       c.textOnAccent);
    p.setColor(QPalette::Shadow,           Qt::transparent);   // 底部强调线默认隐藏
    p.setColor(QPalette::BrightText,       c.textOnAccent);
    p.setColor(QPalette::Highlight,        c.accentDefault);
    p.setColor(QPalette::HighlightedText,  c.textOnAccent);

    // Disabled
    p.setColor(QPalette::Disabled, QPalette::WindowText,       c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::Text,             c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::Button,           c.accentDisabled);
    p.setColor(QPalette::Disabled, QPalette::ButtonText,       c.textDisabled);
    p.setColor(QPalette::Disabled, QPalette::HighlightedText,  c.textDisabled);

    // Tooltip
    p.setColor(QPalette::ToolTipBase, c.surfaceCard);
    p.setColor(QPalette::ToolTipText, c.textPrimary);

    return p;
}

// ============================================================================
// styleHint — 行为提示
// ============================================================================

int SepProxyStyle::styleHint(StyleHint sh, const QStyleOption* opt,
                            const QWidget* widget, QStyleHintReturn* ret) const
{
    switch (sh) {
    case SH_EtchDisabledText:
        return 0;
    case SH_DitherDisabledText:
        return 0;
    case SH_ToolTip_Mask:
        // tooltip 圆角改由 polish() 中的 WA_TranslucentBackground 逐像素透明实现，
        // 不再用 setMask（SetWindowRgn 仅 1-bit 锯齿，且 DWM 下对 tooltip 无效）。
        return 0;
    default:
        return QProxyStyle::styleHint(sh, opt, widget, ret);
    }
}

// ============================================================================
// polish — Widget 初始化钩子
// ============================================================================

void SepProxyStyle::polish(QWidget* widget)
{
    // QMenu frameless+translucent 需在构造阶段设置（PopupMenu 子类负责），
    // 此处不能通过 setWindowFlags 修改——polish() 在 native window 创建后才调用，时序太晚。

    // ── 字体层级 ──
    // 页面内容区容器：14px 基准，所有子控件默认继承
    if (widget->objectName() == QStringLiteral("page-content")) {
        QFont f = widget->font();
        f.setPixelSize(14);
        widget->setFont(f);
    }
    // 页面标题：30px
    if (widget->objectName() == QStringLiteral("page-title")) {
        QFont f = widget->font();
        f.setPixelSize(30);
        widget->setFont(f);
    }
    // 光标名称：加粗
    if (widget->objectName() == QStringLiteral("cursor-title")) {
        QFont f = widget->font();
        f.setBold(true);
        widget->setFont(f);
    }
    // 光标作者：灰色副文（复用 textSecondary 令牌，与原始 QSS #949494 一致）
    // 注意：该 QLabel 位于 QListWidget 的 itemWidget 内，其 viewport 的 backgroundRole 为 Base
    //（QAbstractItemView 构造时设置），使 QLabel 的 foregroundRole() 解析为 QPalette::Text 而非
    // 默认的 WindowText。故副文灰色必须写到 Text 角色，写 WindowText 无效。
    if (widget->objectName() == QStringLiteral("cursor-author")) {
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Text, token().colors.textSecondary);
        widget->setPalette(pal);
    }

    // ── QLineEdit 左内边距（还原 QSS padding-left: 12px）──
    // PM_DefaultFrameWidth 提供 2px，setTextMargins 补足 10px = 总计 12px
    if (qobject_cast<QLineEdit*>(widget)) {
        const QWidget* pw = widget->parentWidget();
        const bool isSpinBoxInternal = pw && qobject_cast<const QAbstractSpinBox*>(pw);
        if (!isSpinBoxInternal) {
            QLineEdit* le = static_cast<QLineEdit*>(widget);
            le->setTextMargins(10, 0, 2, 0);
        } else {
            // SpinBox 内部 QLineEdit 的背景由 CC_SpinBox 统一填充，
            // 关闭自填充避免覆盖父控件已绘制的边框。
            widget->setAutoFillBackground(false);
        }
    }

    // ── QComboBoxListView viewport：关闭自填充，避免直角矩形覆盖容器圆角边框弧线 ──
    if (widget->inherits("QComboBoxListView")) {
        if (auto* area = qobject_cast<QAbstractScrollArea*>(widget)) {
            area->viewport()->setAutoFillBackground(false);
        }
    }

    // ── QComboBoxPrivateContainer 半透明窗口 ──
    // 参考 QWindows11Style 的五属性组合：将 Popup 容器从原生矩形窗口
    // 转为逐像素透明的无边框窗口，圆角由 PE_Frame 在透明表面上绘制。
    // 五者缺一不可：缺少任一个都会导致角落出现黑/灰直角区域。
    if (widget->inherits("QComboBoxPrivateContainer")) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint);
        QPalette pal = widget->palette();
        pal.setColor(widget->backgroundRole(), Qt::transparent);
        widget->setPalette(pal);
    }

    QProxyStyle::polish(widget);

    // ── QToolTip 半透明窗口 ──
    // QToolTip(QTipLabel) 是矩形不透明顶层窗口，原生底色(palette().window()=surfaceWindow)
    // 会露出圆角外四角（与 QComboBoxPrivateContainer 下拉层同源问题）。
    //
    // 必须放在 QProxyStyle::polish(widget) 之后：平台样式(QWindowsVistaStyle) 对 QTipLabel
    // 有硬编码分支，会 setPalette(resolveMask=0) 覆盖本样式已设置的调色板。resolveMask=0
    // 使 QEvent::Polish 处理函数里的 resolvePalette() 把 Window 角色回填为不透明的
    // surfaceWindow，四角因此露出直角区域。放在基础样式之后再 setColor 会置位对应的
    // resolve 位，resolvePalette() 便不再回填 Window，透明得以保留。
    //
    // 光有 WA_TranslucentBackground 还不够：QWindowsWindow::setWindowLayered 只有在
    //   needsLayered = (WindowTransparentForInput) || (hasAlpha && hasNoNativeFrame) || opacity<1
    // 成立时才给 HWND 加 WS_EX_LAYERED。QTipLabel 构造标志只有 Qt::ToolTip|BypassGraphicsProxyWidget，
    // 无 FramelessWindowHint → hasNoNativeFrame=false → 窗口非 layered，逐像素 alpha 被系统忽略，
    // 四角仍是不透明直角。故必须补 FramelessWindowHint（与 QComboBoxPrivateContainer 同源方案）。
    if (widget->windowType() == Qt::ToolTip) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, false);
        widget->setAttribute(Qt::WA_TranslucentBackground);
        widget->setWindowFlag(Qt::FramelessWindowHint);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint);
        QPalette pal = widget->palette();
        pal.setColor(QPalette::Window, Qt::transparent);
        pal.setColor(QPalette::ToolTipBase, Qt::transparent);
        pal.setColor(QPalette::ToolTipText, token().colors.textPrimary);
        widget->setPalette(pal);
    }
}

void SepProxyStyle::unpolish(QWidget* widget)
{
    if (widget->inherits("QComboBoxPrivateContainer")) {
        widget->setAttribute(Qt::WA_OpaquePaintEvent, true);
        widget->setAttribute(Qt::WA_TranslucentBackground, false);
        widget->setWindowFlag(Qt::FramelessWindowHint, false);
        widget->setWindowFlag(Qt::NoDropShadowWindowHint, false);
        QPalette pal = widget->palette();
        pal.setColor(widget->backgroundRole(), QPalette().color(widget->backgroundRole()));
        widget->setPalette(pal);
    }

    QProxyStyle::unpolish(widget);
}
