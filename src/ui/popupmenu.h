#ifndef POPUPMENU_H
#define POPUPMENU_H

#include <QMenu>

// QMenu 子类，在构造阶段设置 Frameless + TranslucentBackground，
// 确保 SepProxyStyle 的 PE_PanelMenu / CE_MenuItem 自定义渲染生效，
// 而非被 Windows 原生 Popup 窗口样式覆盖。
class PopupMenu : public QMenu
{
    Q_OBJECT
public:
    explicit PopupMenu(QWidget* parent = nullptr)
        : QMenu(parent)
    {
        setWindowFlags(windowFlags()
                       | Qt::FramelessWindowHint
                       | Qt::NoDropShadowWindowHint);
        setAttribute(Qt::WA_TranslucentBackground);
    }
};

#endif // POPUPMENU_H
