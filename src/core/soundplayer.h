#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

// ============================================================================
// SoundPlayer — 声音反馈
// 用 Win32 PlaySound 播放 Windows 系统提示音，用于连点器启动/停止反馈。
// 无状态静态方法，不引入 Qt Multimedia 模块（仅链接 winmm 系统库），
// 音色跟随用户系统的「声音方案」，可在 Windows 声音设置里自定义。
// ============================================================================

class SoundPlayer
{
public:
    SoundPlayer() = delete;

    static void playStart();
    static void playStop();
};

#endif // SOUNDPLAYER_H
