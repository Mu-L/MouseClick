#ifndef PAGEBASE_H
#define PAGEBASE_H

#include <memory>
#include <QPushButton>
#include <QWidget>

#include "core/clicker.h"

class PageBase : public QWidget
{
    Q_OBJECT
public:
    explicit PageBase(QWidget* parent = nullptr);
    ~PageBase();

    static Clicker* clicker();
    static QThread* clickerThread();

Q_SIGNALS:
    void ThemeChanged();

private:
    static std::unique_ptr<Clicker> _clicker;
    static std::unique_ptr<QThread> _clicker_thread;
    static bool _is_thread_initialized;
};

#endif // PAGEBASE_H
