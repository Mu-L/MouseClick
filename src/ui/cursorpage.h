#ifndef CURSORPAGE_H
#define CURSORPAGE_H

#include <QLabel>
#include <QPushButton>
#include <QTimer>
#include <QWidget>

#include "pagebase.h"

class CursorPage : public PageBase
{
    Q_OBJECT
public:
    explicit CursorPage(const QString &title, QWidget *parent = nullptr);
    ~CursorPage();

protected:
    void changeEvent(QEvent *event) override;

private:
    Q_DISABLE_COPY_MOVE(CursorPage)

    QMap<QString, QJsonObject> _cursors;

    QLabel* _page_title;

    void loadCursorInfo(const QString &path);
    QJsonObject getConfigJsonObj(const QString &file_path);
    void retranslateUi();
};

class CursorListItem : public QWidget
{
    Q_OBJECT
public:
    explicit CursorListItem(const QString &image_path,
                            const QString &author_name,
                            const QString &description,
                            const QString &source_url,
                            QWidget *parent = nullptr);

    QPushButton* installButton() const { return _install_btn; }
    QPushButton* applyButton() const { return _apply_btn; }
    QPushButton* uninstallButton() const { return _uninstall_btn; }

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void changeEvent(QEvent *event) override;

private:
    QPushButton* _install_btn;
    QPushButton* _apply_btn;
    QPushButton* _uninstall_btn;
    QLabel* _author_label;
    QString _source_url;
    QTimer* _hover_timer;

    void openSourceUrl();
    void retranslateUi();
};

#endif // CURSORPAGE_H
