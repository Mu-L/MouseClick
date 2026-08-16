#ifndef CLICKERSTATUSFEEDBACK_H
#define CLICKERSTATUSFEEDBACK_H

#include <QIcon>
#include <QObject>

class QSystemTrayIcon;

// ============================================================================
// ClickerStatusFeedback — 连点器运行状态反馈
// 将「运行/停止」状态投影为 托盘图标 + tooltip + 声音 三路反馈。
// 依赖注入 QSystemTrayIcon；声音是否播放由 Config::SoundFeedback() 决定。
// 取代原先 QSystemTrayIcon::showMessage 的系统通知（高频启停时会被节流/丢失）。
// ============================================================================

class ClickerStatusFeedback : public QObject
{
    Q_OBJECT
public:
    explicit ClickerStatusFeedback(QSystemTrayIcon* tray, QObject* parent = nullptr);

    void setRunning(bool running);
    void retranslate();   // 语言切换后重设 tooltip，保持运行态文案正确

private:
    QSystemTrayIcon* _tray = nullptr;
    bool _running = false;
    QIcon _idle_icon;
    QIcon _running_icon;
};

#endif // CLICKERSTATUSFEEDBACK_H
