#include "translation.h"

#include <QCoreApplication>

Translation& Translation::instance()
{
    static Translation instance;
    return instance;
}

Translation::Translation(QObject* parent)
    : QObject{parent}
{}

Translation::~Translation()
{
    if (_loaded && qApp) {
        QCoreApplication::removeTranslator(_translator);
    }
}

void Translation::init()
{
    _translator = new QTranslator(this);
}

void Translation::switchLanguage(const QString& language)
{
    const QString qmPath = ":/i18n/MouseClick_" + language;

    if (!_loaded) {
        _loaded = _translator->load(qmPath);
        if (_loaded) {
            QCoreApplication::installTranslator(_translator);
        }
    } else {
        QCoreApplication::removeTranslator(_translator);
        QTranslator* newTranslator = new QTranslator(this);
        if (newTranslator->load(qmPath)) {
            delete _translator;
            _translator = newTranslator;
        } else {
            delete newTranslator;
        }
        QCoreApplication::installTranslator(_translator);
    }
}
