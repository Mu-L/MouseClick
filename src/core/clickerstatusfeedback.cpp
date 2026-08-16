#include "clickerstatusfeedback.h"

#include <QSystemTrayIcon>

#include "config.h"
#include "soundplayer.h"

ClickerStatusFeedback::ClickerStatusFeedback(QSystemTrayIcon* tray, QObject* parent)
    : QObject{parent},
      _tray(tray),
      _idle_icon(QStringLiteral(":/svg/favicon.svg")),
      _running_icon(QStringLiteral(":/svg/favicon-running.svg"))
{
    // 初始为停止态，与 setupSystemTray 创建的默认图标保持一致
    if (_tray) {
        _tray->setIcon(_idle_icon);
        _tray->setToolTip(tr("MouseClick"));
    }
}

void ClickerStatusFeedback::setRunning(bool running)
{
    _running = running;

    if (_tray) {
        _tray->setIcon(running ? _running_icon : _idle_icon);
        _tray->setToolTip(running ? tr("MouseClick — Clicker Running")
                                  : tr("MouseClick"));
    }

    if (Config::instance().SoundFeedback()) {
        if (running) {
            SoundPlayer::playStart();
        } else {
            SoundPlayer::playStop();
        }
    }
}

void ClickerStatusFeedback::retranslate()
{
    if (_tray) {
        _tray->setToolTip(_running ? tr("MouseClick — Clicker Running")
                                   : tr("MouseClick"));
    }
}
