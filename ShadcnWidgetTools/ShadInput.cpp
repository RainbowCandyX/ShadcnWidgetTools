#include "ShadInput.h"

#include <QColor>
#include <QDynamicPropertyChangeEvent>
#include <QEvent>
#include <QFocusEvent>
#include <QHelpEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QRectF>
#include <QSizePolicy>

#include "ShadInputPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"

namespace
{
    // h-9 可视高度
    constexpr int kInputHeight = 36;
} // namespace

ShadInput::ShadInput(QWidget* parent)
    : QLineEdit(parent), d_ptr(new ShadInputPrivate())
{
    Q_D(ShadInput);
    d->q_ptr = this;
    setFrame(false);
    setFocusPolicy(Qt::StrongFocus);
#ifdef Q_OS_MACOS
    // setFrame(false) 会重新开启该属性，必须放在其后关闭，否则 macOS 样式会在获得焦点时额外绘制系统焦点框；焦点环由组件自绘
    setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    // 尾部按钮需要悬停态
    setMouseTracking(true);
    // 尾部按钮有单独的提示，提示事件由 event 自行处理
    setProperty(ShadStyleHelper::kCustomToolTipProperty, true);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    d->applyThemeColors();
    // 清空按钮的可见性随文字变化
    connect(this, &QLineEdit::textChanged, this, [this]() { update(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->applyThemeColors(); });
}

ShadInput::~ShadInput()
{
}

void ShadInput::setInvalid(bool invalid)
{
    Q_D(ShadInput);
    if (d->_isInvalid == invalid)
    {
        return;
    }
    d->_isInvalid = invalid;
    update();
    Q_EMIT invalidChanged();
}

bool ShadInput::isInvalid() const
{
    Q_D(const ShadInput);
    return d->_isInvalid;
}

void ShadInput::setLeadingIcon(ShadIconType::IconName icon)
{
    Q_D(ShadInput);
    if (d->_leadingIcon == icon)
    {
        return;
    }
    d->_leadingIcon = icon;
    d->applyThemeColors();
}

ShadIconType::IconName ShadInput::getLeadingIcon() const
{
    Q_D(const ShadInput);
    return d->_leadingIcon;
}

void ShadInput::setTrailingAction(ShadInputType::TrailingAction action)
{
    Q_D(ShadInput);
    if (d->_trailingAction == action)
    {
        return;
    }
    d->_trailingAction = action;
    d->_isTrailingHovered = false;
    d->_isTrailingPressed = false;
    if (action == ShadInputType::PasswordToggleAction)
    {
        d->_isPasswordVisible = false;
        setEchoMode(QLineEdit::Password);
    }
    d->applyThemeColors();
}

ShadInputType::TrailingAction ShadInput::getTrailingAction() const
{
    Q_D(const ShadInput);
    return d->_trailingAction;
}

QSize ShadInput::sizeHint() const
{
    return QSize(QLineEdit::sizeHint().width(), kInputHeight + 2 * ShadStyleHelper::kFocusRingWidth);
}

QSize ShadInput::minimumSizeHint() const
{
    return QSize(QLineEdit::minimumSizeHint().width(), kInputHeight + 2 * ShadStyleHelper::kFocusRingWidth);
}

bool ShadInput::event(QEvent* event)
{
    Q_D(ShadInput);
    // 布局方向变化时内边距需要左右对调；放入输入组后（嵌入模式）内边距与外观由输入组负责
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->applyThemeColors();
    }
    if (event->type() == QEvent::DynamicPropertyChange && static_cast<QDynamicPropertyChangeEvent*>(event)->propertyName() == ShadStyleHelper::kEmbeddedProperty)
    {
        d->applyThemeColors();
        updateGeometry();
    }
    // 悬停在尾部按钮上时显示按钮说明，其余位置显示组件自身的 toolTip（两者都通过 ShadTooltip 显示）
    if (event->type() == QEvent::ToolTip)
    {
        QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        if (d->isTrailingVisible() && d->trailingButtonRect().contains(helpEvent->pos()))
        {
            shadTooltip->showText(this, d->trailingButtonRect().toAlignedRect(), d->trailingToolTip());
            return true;
        }
        if (!toolTip().isEmpty())
        {
            shadTooltip->showText(this, rect(), toolTip());
            return true;
        }
        event->ignore();
        return false;
    }
    return QLineEdit::event(event);
}

void ShadInput::focusInEvent(QFocusEvent* event)
{
    QLineEdit::focusInEvent(event);
    update();
}

void ShadInput::focusOutEvent(QFocusEvent* event)
{
    QLineEdit::focusOutEvent(event);
    update();
}

void ShadInput::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadInput);
    if (event->button() == Qt::LeftButton && d->isTrailingVisible() && d->trailingButtonRect().contains(event->position()))
    {
        // 点击尾部按钮时不移动文字光标
        d->_isTrailingPressed = true;
        event->accept();
        return;
    }
    QLineEdit::mousePressEvent(event);
}

void ShadInput::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadInput);
    d->updateTrailingHover(event->position().toPoint());
    if (d->_isTrailingPressed)
    {
        event->accept();
        return;
    }
    QLineEdit::mouseMoveEvent(event);
}

void ShadInput::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadInput);
    if (event->button() == Qt::LeftButton && d->_isTrailingPressed)
    {
        d->_isTrailingPressed = false;
        if (d->isTrailingVisible() && d->trailingButtonRect().contains(event->position()))
        {
            d->triggerTrailingAction();
        }
        d->updateTrailingHover(event->position().toPoint());
        event->accept();
        return;
    }
    QLineEdit::mouseReleaseEvent(event);
}

void ShadInput::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_D(ShadInput);
    // 双击尾部按钮时不触发全选；快速连点时第二次按下以双击事件送达，按一次普通按下处理，松开时照常触发
    if (d->isTrailingVisible() && d->trailingButtonRect().contains(event->position()))
    {
        if (event->button() == Qt::LeftButton)
        {
            d->_isTrailingPressed = true;
        }
        event->accept();
        return;
    }
    QLineEdit::mouseDoubleClickEvent(event);
}

void ShadInput::leaveEvent(QEvent* event)
{
    Q_D(ShadInput);
    d->updateTrailingHover(QPoint(-1, -1));
    QLineEdit::leaveEvent(event);
}

void ShadInput::paintEvent(QPaintEvent* event)
{
    Q_D(ShadInput);
    using namespace ShadStyleHelper;
    {
        // 先绘制背景、边框、焦点环与图标，再交给 QLineEdit 绘制文字与光标
        const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
        const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
        const bool isFocused = hasFocus();
        // 放入 ShadButtonGroup 时与相邻组件拼接的边为直角，起始侧不描边
        const GroupEdges edges = groupEdges(this);
        QPainter painter(this);
        painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
        painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
        // 嵌入输入组时外框、焦点环由输入组绘制（InputGroupInput：border-0 bg-transparent shadow-none ring-0）
        if (!isEmbedded(this))
        {
            drawShadowXs(&painter, bodyRect, radius, edges);
            if (shadTheme->isDark())
            {
                fillRoundedRect(&painter, bodyRect, radius, withAlpha(shadTheme->getColor(ShadThemeType::Input), 0.3), edges);
            }
            QColor borderColor = shadTheme->getColor(isFocused ? ShadThemeType::Ring : ShadThemeType::Input);
            if (d->_isInvalid)
            {
                borderColor = shadTheme->getColor(ShadThemeType::Destructive);
            }
            strokeRoundedRect(&painter, bodyRect, radius, borderColor, edges);
            // 输入框获得焦点即显示焦点环（浏览器对文本框的 :focus-visible 行为）；错误态常驻 destructive 焦点环
            if (isFocused || d->_isInvalid)
            {
                drawFocusRing(&painter, bodyRect, radius, focusRingColor(d->_isInvalid), edges);
            }
        }
        d->paintDecorations(&painter);
    }
    QLineEdit::paintEvent(event);
}
