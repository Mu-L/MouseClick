#include "themestate.h"

ThemeState& ThemeState::instance()
{
    static ThemeState inst;
    return inst;
}

ThemeState::ThemeState(QObject* parent)
    : QObject{parent}
{
}

const ThemeTokens& ThemeState::current() const
{
    return *_current;
}

bool ThemeState::isDark() const
{
    return _current == &kDarkTokens;
}

void ThemeState::setDarkMode(bool dark)
{
    const ThemeTokens* next = dark ? &kDarkTokens : &kLightTokens;
    if (_current != next) {
        _current = next;
        emit themeChanged();
    }
}
