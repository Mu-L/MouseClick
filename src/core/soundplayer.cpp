#include "soundplayer.h"

#include <windows.h>
#include <mmsystem.h>

// SND_ALIAS     ：第一个参数是系统声音别名（而非文件路径）
// SND_ASYNC     ：异步播放，立即返回不阻塞调用线程（连点器启停由热键触发，不可卡顿）
// SND_NODEFAULT ：找不到对应声音时保持静默，不回退到默认蜂鸣
void SoundPlayer::playStart()
{
    PlaySoundW(L"SystemAsterisk", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
}

void SoundPlayer::playStop()
{
    PlaySoundW(L"SystemExclamation", nullptr, SND_ALIAS | SND_ASYNC | SND_NODEFAULT);
}
