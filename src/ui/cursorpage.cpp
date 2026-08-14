#include "cursorpage.h"

#include "core/config.h"
#include "ui/button.h"
#include "core/cursormanager.h"
#include "core/logger.h"

#include <QBoxLayout>
#include <QCoreApplication>
#include <QCursor>
#include <QDesktopServices>
#include <QDir>
#include <QEvent>
#include <QFontMetrics>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLabel>
#include <QListView>
#include <QListWidgetItem>
#include "ui/messagebox.h"
#include <QResizeEvent>
#include <QScrollBar>
#include <QTimer>
#include <QToolTip>
#include <QUrl>

// 单行文本标签：文本超出可用宽度时自动省略为 "..."，
// 避免长光标名把条目自然宽度撑过列表视口（触发水平滚动条）。
class ElidedLabel : public QLabel {
public:
    explicit ElidedLabel(const QString& text, QWidget* parent = nullptr)
        : QLabel(parent), _full_text(text)
    {
        setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Preferred);
        setWordWrap(false);
        QLabel::setText(_full_text);
        setToolTip(_full_text);   // 截断时悬停显示完整名字
    }

protected:
    void resizeEvent(QResizeEvent* event) override
    {
        QLabel::resizeEvent(event);
        refresh();
    }

    void changeEvent(QEvent* event) override
    {
        QLabel::changeEvent(event);
        if (event->type() == QEvent::FontChange)
            refresh();
    }

private:
    void refresh()
    {
        if (width() <= 0) {
            QLabel::setText(_full_text);
        } else {
            QLabel::setText(fontMetrics().elidedText(_full_text, Qt::ElideRight, width()));
        }
    }

    QString _full_text;
};

CursorPage::CursorPage(const QString &title, QWidget *parent)
    : PageBase{parent},
      _page_title(nullptr) {
    Config &app_settings = Config::instance();

    QVBoxLayout *central_layout = new QVBoxLayout(this);
    central_layout->setSpacing(0);
    central_layout->setContentsMargins(QMargins());

    _page_title = new QLabel(this);
    _page_title->setObjectName(QStringLiteral("page-title"));
    _page_title->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    _page_title->setFocusPolicy(Qt::NoFocus);
    _page_title->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);
    _page_title->setText(title);
    _page_title->setMaximumHeight(42);

    QWidget *page_content = new QWidget(this);
    page_content->setObjectName(QStringLiteral("page-content"));
    page_content->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

    QVBoxLayout *page_content_layout = new QVBoxLayout(page_content);
    page_content_layout->setSpacing(12);
    page_content_layout->setContentsMargins(QMargins(0, 0, 0, 0));

    /********************/

    QListWidget *cursors_style_list = new QListWidget(page_content);
    cursors_style_list->setObjectName(QStringLiteral("cursor-style-list"));
    cursors_style_list->setSizePolicy(QSizePolicy::Preferred,
                                      QSizePolicy::Preferred);
    cursors_style_list->setMinimumSize(QSize(480, 64));
    cursors_style_list->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    cursors_style_list->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    cursors_style_list->verticalScrollBar()->setSingleStep(30);

    // 获取程序根目录
    const QString root_dir = QCoreApplication::applicationDirPath();

    // 导入鼠标样式信息
    const QString cursors_dir = root_dir + "/cursors/";
    loadCursorInfo(cursors_dir);

    // 添加自定义控件到列表
    for (auto it = _cursors.begin(); it != _cursors.end(); ++it) {
        const QString cursor_name = it.key();
        const QJsonObject cursor_config = it.value();
        const QString cursor_logo_path =
            cursors_dir + cursor_name + "/logo.png";
        const QString cursor_author = cursor_config.value("author").toString();
        const QString cursor_display_name = cursor_config.value("name").toString();
        const QString source_url = cursor_config.value("source").toString();

        QListWidgetItem *item = new QListWidgetItem(cursors_style_list);
        CursorListItem *cursor_list_item = new CursorListItem(cursor_logo_path, cursor_author, cursor_display_name, source_url);

        item->setSizeHint(cursor_list_item->sizeHint());
        cursors_style_list->setItemWidget(item, cursor_list_item);

        // 存储光标名称到 item data，方便后续按钮回调使用
        item->setData(Qt::UserRole, cursor_name);

        // 连接 Install / Apply / Uninstall 按钮
        connect(cursor_list_item->installButton(), &QPushButton::clicked, this, [this, cursor_name, cursor_config]() {
            CursorManager mgr;

            // 校验是否已安装
            if (mgr.isInstalled(cursor_config)) {
                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(tr("Already Installed"));
                msgBox.setInformativeText(tr("Cursor '%1' is already installed.\n"
                                             "No action needed.")
                                              .arg(cursor_config["name"].toString()));
                msgBox.exec();
                return;
            }

            if (mgr.installCursor(cursor_name, cursor_config)) {
                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(tr("Install Success"));
                msgBox.setInformativeText(tr("Cursor '%1' files copied to C:\\Windows\\Cursors\\%1 successfully.")
                                              .arg(cursor_config["name"].toString()));
                msgBox.exec();
            } else {
                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText(tr("Install Failed"));
                msgBox.setInformativeText(tr("Failed to copy cursor files to C:\\Windows\\Cursors.\n\n"
                                             "Please try running the program as Administrator."));
                msgBox.exec();
            }
        });

        connect(cursor_list_item->applyButton(), &QPushButton::clicked, this, [this, cursor_name, cursor_config]() {
            CursorManager mgr;
            if (mgr.applyCursor(cursor_name, cursor_config)) {
                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(tr("Apply Success"));
                msgBox.setInformativeText(tr("Cursor '%1' applied successfully.")
                                              .arg(cursor_config["name"].toString()));
                msgBox.exec();
            } else {
                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Warning);
                msgBox.setText(tr("Apply Failed"));
                msgBox.setInformativeText(tr("Cursor '%1' has not been installed yet.\n"
                                             "Please install it first before applying.")
                                              .arg(cursor_config["name"].toString()));
                msgBox.exec();
            }
        });

        connect(cursor_list_item->uninstallButton(), &QPushButton::clicked, this, [this, cursor_name, cursor_config]() {
            CursorManager mgr;
            const QString themeName = cursor_config["name"].toString();
            const bool filesExist = mgr.isInstalled(cursor_config);
            const bool schemeExists = mgr.isSchemeRegistered(cursor_config);

            // 文件和注册表均不存在，无需卸载
            if (!filesExist && !schemeExists) {
                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(tr("Not Installed"));
                msgBox.setInformativeText(tr("Cursor '%1' has not been installed.\n"
                                             "Nothing to uninstall.")
                                              .arg(themeName));
                msgBox.exec();
                return;
            }

            // 构建确认对话框的描述信息（汇总已安装项）
            QStringList installedItems;
            if (filesExist) {
                installedItems << tr("Cursor files (C:\\Windows\\Cursors\\%1)").arg(themeName);
            }
            if (schemeExists) {
                installedItems << tr("Registry scheme");
            }

            MessageBox confirmBox;
            confirmBox.setIcon(QMessageBox::Question);
            confirmBox.setText(tr("Confirm Uninstall"));
            confirmBox.setInformativeText(tr("The following items will be removed:\n  - %1\n\n"
                                             "This will restore the default Windows cursor theme.\n"
                                             "Are you sure you want to continue?")
                                              .arg(installedItems.join("\n  - ")));
            QPushButton* yesBtn = confirmBox.addButton(tr("Yes"), QMessageBox::YesRole);
            confirmBox.addButton(tr("No"), QMessageBox::NoRole);
            confirmBox.setDefaultButton(yesBtn);
            confirmBox.exec();

            if (confirmBox.clickedButton() == yesBtn) {
                mgr.uninstallCursor(cursor_name, cursor_config);

                // 汇总实际清理结果
                QStringList cleanedItems;
                if (filesExist) {
                    cleanedItems << tr("Cursor files removed");
                }
                if (schemeExists) {
                    cleanedItems << tr("Registry scheme removed");
                }

                MessageBox msgBox;
                msgBox.setIcon(QMessageBox::Information);
                msgBox.setText(tr("Uninstall Complete"));
                msgBox.setInformativeText(tr("Cursor '%1' uninstalled.\n  - %2\n\nSystem default cursor restored.")
                                              .arg(themeName, cleanedItems.join("\n  - ")));
                msgBox.exec();
            }
        });

#ifdef QT_DEBUG
        LOG_DEBUG(QString("Loaded:%1").arg(cursor_logo_path));
#endif
    }

    /********************/

    page_content_layout->addWidget(cursors_style_list);

    central_layout->addWidget(_page_title);
    central_layout->addWidget(page_content);

    setLayout(central_layout);
}

CursorPage::~CursorPage() {}

void CursorPage::retranslateUi()
{
    _page_title->setText(tr("Beautify Cursor"));
}

void CursorPage::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    PageBase::changeEvent(event);
}

void CursorPage::loadCursorInfo(const QString &path) {
    QDir dir(path);

    // 检查目录是否存在
    if (!dir.exists()) {
        LOG_WARNING(QString("The directory does not exist: %1").arg(path));
        return;
    }

    // 获取子文件夹列表
    QStringList cursors_name = dir.entryList(QDir::Dirs | QDir::NoDotAndDotDot);

    foreach (QString cursor_name, cursors_name) {
        const QString cursor_config_path =
            QCoreApplication::applicationDirPath() + "/cursors/" + cursor_name + "/config.json";

        _cursors[cursor_name] = getConfigJsonObj(cursor_config_path);
    }
}

QJsonObject CursorPage::getConfigJsonObj(const QString &file_path)
{
    QFile config_file(file_path);
    if (!config_file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        LOG_WARNING(QString("Cannot open file: %1").arg(file_path));
        return QJsonObject();
    }

    QByteArray file_data = config_file.readAll();
    config_file.close();

    QJsonDocument json_doc = QJsonDocument::fromJson(file_data);
    if (!json_doc.isObject()) {
        LOG_WARNING(QString("Invalid JSON file: %1").arg(file_path));
        return QJsonObject();
    }

    return json_doc.object();
}

CursorListItem::CursorListItem(const QString &imagePath, const QString &author_name,
                               const QString &description, const QString &source_url,
                               QWidget *parent)
    : QWidget(parent),
      _install_btn(nullptr),
      _apply_btn(nullptr),
      _uninstall_btn(nullptr),
      _author_label(nullptr),
      _source_url(source_url),
      _hover_timer(new QTimer(this))
{
    this->setMinimumSize(QSize(480, 64));
    QHBoxLayout *main_layout = new QHBoxLayout(this);

    QLabel *cursor_logo_area = new QLabel(this);
    cursor_logo_area->setFixedSize(QSize(64, 64));
    cursor_logo_area->setPixmap(QPixmap(imagePath).scaled(
        64, 64, Qt::KeepAspectRatio, Qt::SmoothTransformation));

    QWidget *cursor_desc_area = new QWidget(this);
    cursor_desc_area->setMinimumSize(QSize(100, 30));
    QVBoxLayout *cursor_desc_area_layout = new QVBoxLayout(cursor_desc_area);
    auto *cursor_title = new ElidedLabel(description, cursor_desc_area);
    cursor_title->setObjectName(QStringLiteral("cursor-title"));
    _author_label = new QLabel(cursor_desc_area);
    _author_label->setObjectName(QStringLiteral("cursor-author"));
    _author_label->setWordWrap(true);
    _author_label->setText(author_name);
    cursor_desc_area_layout->addWidget(cursor_title);
    cursor_desc_area_layout->addWidget(_author_label);
    cursor_desc_area->setLayout(cursor_desc_area_layout);

    QWidget *btn_area = new QWidget(this);
    QVBoxLayout *btn_area_layout = new QVBoxLayout(btn_area);
    _install_btn = new ActionButton(tr("Install"), btn_area);
    _apply_btn = new ActionButton(tr("Apply"), btn_area);
    _uninstall_btn = new ActionButton(tr("Uninstall"), btn_area);
    _install_btn->setMinimumSize(QSize(70, 30));
    _apply_btn->setMinimumSize(QSize(70, 30));
    _uninstall_btn->setMinimumSize(QSize(70, 30));
    btn_area_layout->addWidget(_install_btn);
    btn_area_layout->addWidget(_apply_btn);
    btn_area_layout->addWidget(_uninstall_btn);
    btn_area->setLayout(btn_area_layout);

    main_layout->addWidget(cursor_logo_area);
    main_layout->addWidget(cursor_desc_area);
    main_layout->addWidget(btn_area);
    main_layout->setStretch(1, 1);

    // 作者标签交互：悬浮 1 秒显示来源链接，双击打开
    if (!_source_url.isEmpty()) {
        _author_label->setCursor(Qt::PointingHandCursor);
        _author_label->setMouseTracking(true);
        _author_label->installEventFilter(this);

        _hover_timer->setSingleShot(true);
        _hover_timer->setInterval(1000);  // 1 秒延迟
        connect(_hover_timer, &QTimer::timeout, this, [this]() {
            QToolTip::showText(QCursor::pos(), _source_url, _author_label);
        });
    }
}

bool CursorListItem::eventFilter(QObject *obj, QEvent *event)
{
    if (obj == _author_label) {
        switch (event->type()) {
        case QEvent::Enter:
            _hover_timer->start();
            break;
        case QEvent::Leave:
            _hover_timer->stop();
            QToolTip::hideText();
            break;
        case QEvent::MouseButtonDblClick:
            _hover_timer->stop();
            QToolTip::hideText();
            openSourceUrl();
            return true;
        default:
            break;
        }
    }
    return QWidget::eventFilter(obj, event);
}

void CursorListItem::openSourceUrl()
{
    if (_source_url.isEmpty()) {
        return;
    }

    MessageBox confirmBox;
    confirmBox.setIcon(QMessageBox::Question);
    confirmBox.setText(tr("Open Link"));
    confirmBox.setInformativeText(tr("Do you want to open this link in your browser?\n\n%1")
                                      .arg(_source_url));
    QPushButton* openBtn = confirmBox.addButton(tr("Open Link"), QMessageBox::YesRole);
    confirmBox.addButton(tr("Cancel"), QMessageBox::NoRole);
    confirmBox.setDefaultButton(openBtn);
    confirmBox.exec();

    if (confirmBox.clickedButton() == openBtn) {
        QDesktopServices::openUrl(QUrl(_source_url));
    }
}

void CursorListItem::retranslateUi()
{
    _install_btn->setText(tr("Install"));
    _apply_btn->setText(tr("Apply"));
    _uninstall_btn->setText(tr("Uninstall"));
}

void CursorListItem::changeEvent(QEvent *event)
{
    if (event->type() == QEvent::LanguageChange) {
        retranslateUi();
    }
    QWidget::changeEvent(event);
}
