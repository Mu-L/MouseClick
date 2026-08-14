#include "pagebase.h"

#include <QThread>

std::unique_ptr<Clicker> PageBase::_clicker = std::make_unique<Clicker>();
std::unique_ptr<QThread> PageBase::_clicker_thread = std::make_unique<QThread>();
bool PageBase::_is_thread_initialized = false;

Clicker* PageBase::clicker()
{
    return _clicker.get();
}

QThread* PageBase::clickerThread()
{
    return _clicker_thread.get();
}

PageBase::PageBase(QWidget* parent)
    : QWidget{parent}
{
    if (!_is_thread_initialized) {
        _clicker->moveToThread(_clicker_thread.get());
        connect(_clicker_thread.get(), &QThread::started,
                _clicker.get(), &Clicker::start);
        _is_thread_initialized = true;
    }
}

PageBase::~PageBase()
{
}
