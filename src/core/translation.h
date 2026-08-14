#ifndef TRANSLATION_H
#define TRANSLATION_H

#include <QObject>
#include <QTranslator>

class Translation : public QObject
{
    Q_OBJECT
public:
    static Translation& instance();
    void init();
    void switchLanguage(const QString& language);

private:
    explicit Translation(QObject* parent = nullptr);
    ~Translation();
    Q_DISABLE_COPY_MOVE(Translation)

    QTranslator* _translator = nullptr;
    bool _loaded = false;
};

#endif // TRANSLATION_H
