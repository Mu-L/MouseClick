#include "messagebox.h"

#include "theme/themestate.h"
#include "ui/button.h"
#include "vendor/qwk/windowbar.h"

#include <QWKWidgets/widgetwindowagent.h>

#include <QHBoxLayout>
#include <QLabel>
#include <QMap>
#include <QPainter>
#include <QPushButton>
#include <QSizePolicy>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

namespace {

// 纯色背景容器：用 paintEvent 直接填充颜色，完全不依赖 palette 继承，
// 从根本上规避 QMessageBox「深色主题下背景落到白色」的问题。
class SolidColorWidget : public QWidget
{
public:
    SolidColorWidget(const QColor& color, QWidget* parent = nullptr)
        : QWidget(parent)
        , _color(color)
    {}

protected:
    void paintEvent(QPaintEvent*) override
    {
        QPainter painter(this);
        painter.fillRect(rect(), _color);
    }

private:
    QColor _color;
};

} // namespace

MessageBox::MessageBox(QWidget* parent)
    : QDialog(parent, Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint
                          | Qt::WindowSystemMenuHint | Qt::WindowCloseButtonHint)
{
    const ColorTokens& c = ThemeState::instance().current().colors;

    // ── 内容区（扁平结构：label 直接挂在顶层布局，保证 heightForWidth 传播）──
    auto* content_row = new QHBoxLayout;
    content_row->setContentsMargins(20, 20, 20, 20);
    content_row->setSpacing(16);

    _icon_label = new QLabel(this);
    _icon_label->setAlignment(Qt::AlignTop | Qt::AlignLeft);
    _icon_label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    _icon_label->hide();   // 默认无图标
    content_row->addWidget(_icon_label);

    auto* text_column = new QVBoxLayout;
    text_column->setSpacing(8);

    _text_label = new QLabel(this);
    _text_label->setWordWrap(false);   // 不自动换行：文字严格按开发者写的 \n 显示
    _text_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    text_column->addWidget(_text_label);

    _informative_label = new QLabel(this);
    _informative_label->setWordWrap(false);   // 不自动换行：文字严格按开发者写的 \n 显示
    _informative_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    _informative_label->hide();   // 默认无正文，setInformativeText 有内容时再显示
    text_column->addWidget(_informative_label);

    content_row->addLayout(text_column, 1);

    // 标题与正文初始颜色均为 textPrimary（标题颜色随后由 setIcon 覆盖为图标色）
    QPalette primary = _text_label->palette();
    primary.setColor(QPalette::WindowText, c.textPrimary);
    _text_label->setPalette(primary);

    QPalette informative = _informative_label->palette();
    informative.setColor(QPalette::WindowText, c.textPrimary);
    _informative_label->setPalette(informative);

    // ── 按钮区（surfaceWindow 由 SolidColorWidget 承载）──
    auto* button_strip = new SolidColorWidget(c.surfaceWindow, this);
    _button_layout = new QHBoxLayout(button_strip);
    _button_layout->setContentsMargins(12, 12, 12, 12);
    _button_layout->setSpacing(8);
    _button_layout->addStretch();   // 按钮靠右对齐

    // ── 无边框外壳 + 自定义标题栏（QWindowKit）──
    // 复用主窗口同一套 IconButton / CloseButton，主题与悬停态由 SepProxyStyle 接管。
    // QDialog 无 setMenuWidget()，标题栏作为根布局首行加入。
    _window_agent = new QWK::WidgetWindowAgent(this);
    _window_agent->setup(this);

    auto* titlebar_label = new QLabel();
    titlebar_label->setAlignment(Qt::AlignVCenter | Qt::AlignLeft);
    titlebar_label->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);

    auto* icon_btn = new IconButton();
    auto* close_btn = new CloseButton();

    auto* titlebar = new QWK::WindowBar();
    titlebar->setIconButton(icon_btn);
    titlebar->setTitleLabel(titlebar_label);
    titlebar->setCloseButton(close_btn);
    titlebar->setHostWidget(this);

    _window_agent->setTitleBar(titlebar);
    _window_agent->setSystemButton(QWK::WindowAgentBase::WindowIcon, icon_btn);
    _window_agent->setSystemButton(QWK::WindowAgentBase::Close, close_btn);

    // 关闭按钮 → 等价 Esc（clickedButton() 返回 nullptr，调用方走 else 分支）
    connect(titlebar, &QWK::WindowBar::closeRequested, this, &QDialog::reject);

    // 图标按钮：单击弹系统菜单，双击关闭（与主窗口一致）
    connect(icon_btn, &QAbstractButton::clicked, _window_agent, [this, icon_btn]() {
        icon_btn->setProperty("double-click-close", false);
        QTimer::singleShot(80, _window_agent, [this, icon_btn]() {
            if (icon_btn->property("double-click-close").toBool())
                return;
            _window_agent->showSystemMenu(icon_btn->mapToGlobal(QPoint{0, icon_btn->height()}));
        });
    });
    connect(icon_btn, &QWK::WindowButton::doubleClicked, this, [this, icon_btn]() {
        icon_btn->setProperty("double-click-close", true);
        close();
    });

    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);
    root->setSpacing(0);
    root->addWidget(titlebar);
    root->addLayout(content_row);
    root->addWidget(button_strip);

    // 应用标识：任务栏图标 + 默认标题，与主窗口一致（调用方可 setWindowTitle 覆盖）
    setWindowIcon(QIcon(":/svg/favicon.svg"));
    setWindowTitle(tr("MouseClick"));
}

MessageBox::~MessageBox() = default;

void MessageBox::setIcon(QMessageBox::Icon icon)
{
    static const QMap<QMessageBox::Icon, QColor> iconColors = {
        {QMessageBox::Information, QColor("#0099CC")},
        {QMessageBox::Warning,     QColor("#E6A23C")},
        {QMessageBox::Critical,    QColor("#F56C6C")},
        {QMessageBox::Question,    QColor("#409EFF")},
    };

    QStyle::StandardPixmap sp = QStyle::SP_CustomBase;
    switch (icon) {
    case QMessageBox::Information: sp = QStyle::SP_MessageBoxInformation; break;
    case QMessageBox::Warning:     sp = QStyle::SP_MessageBoxWarning;     break;
    case QMessageBox::Critical:    sp = QStyle::SP_MessageBoxCritical;    break;
    case QMessageBox::Question:    sp = QStyle::SP_MessageBoxQuestion;    break;
    default: break;
    }

    if (sp != QStyle::SP_CustomBase) {
        _icon_label->setPixmap(style()->standardIcon(sp).pixmap(40, 40));
        _icon_label->show();
    } else {
        _icon_label->clear();
        _icon_label->hide();
    }

    // 标题栏标题 = 应用名 + 类型提示：MouseClick[信息/警告/错误/询问]
    QString typeName;
    switch (icon) {
    case QMessageBox::Information: typeName = tr("Information"); break;
    case QMessageBox::Warning:     typeName = tr("Warning");     break;
    case QMessageBox::Critical:    typeName = tr("Error");       break;
    case QMessageBox::Question:    typeName = tr("Question");    break;
    default: break;
    }
    QString title = tr("MouseClick");
    if (!typeName.isEmpty())
        title += QStringLiteral("[") + typeName + QStringLiteral("]");
    setWindowTitle(title);

    // 标题文字按图标类型上色（保留现有做法）
    const ColorTokens& c = ThemeState::instance().current().colors;
    QPalette pal = _text_label->palette();
    pal.setColor(QPalette::WindowText, iconColors.value(icon, c.textPrimary));
    _text_label->setPalette(pal);
}

void MessageBox::setText(const QString& text)
{
    _text_label->setText(text);
}

void MessageBox::setInformativeText(const QString& text)
{
    _informative_label->setText(text);
    _informative_label->setVisible(!text.isEmpty());   // 空正文隐藏，避免标题与按钮间多一行空白
}

QPushButton* MessageBox::addButton(const QString& text, QMessageBox::ButtonRole role)
{
    Q_UNUSED(role)

    auto* button = new QPushButton(text, this);
    button->setMinimumWidth(70);   // 与原 messagebox.qss 的 min-width 保持一致
    button->setAutoDefault(false);
    _button_layout->addWidget(button);
    ++_button_count;

    connect(button, &QPushButton::clicked, this, [this, button]() {
        _clicked_button = button;
        accept();
    });

    // 首个按钮自动成为默认按钮（Enter 触发）
    if (!_default_button)
        setDefaultButton(button);

    return button;
}

void MessageBox::setDefaultButton(QPushButton* button)
{
    _default_button = button;
    if (button)
        button->setDefault(true);
}

QPushButton* MessageBox::clickedButton() const
{
    return _clicked_button;
}

int MessageBox::exec()
{
    if (_button_count == 0)
        addButton(tr("OK"), QMessageBox::AcceptRole);
    return QDialog::exec();
}

void MessageBox::paintEvent(QPaintEvent*)
{
    // 内容区背景 = surfaceCard；按钮区 surfaceWindow 由按钮条子 widget 覆盖其上。
    QPainter painter(this);
    painter.fillRect(rect(), ThemeState::instance().current().colors.surfaceCard);
}

void MessageBox::showEvent(QShowEvent* event)
{
    // 对话框按内容自然尺寸锁定（不可调大小），sizeHint() 即内容的自然尺寸。
    setFixedSize(sizeHint());
    QDialog::showEvent(event);
}
