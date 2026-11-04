#include "ShadButtonGroup.h"

#include <QEvent>
#include <QFont>
#include <QIcon>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtMath>

#include <utility>

#include "ShadButtonGroupPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // ButtonGroupText：px-4 gap-2，图标 size-4，高度与默认按钮 h-9 一致（在组中会拉伸到组内最高的组件）
    constexpr int kTextPaddingX = 16;
    constexpr int kTextGap = 8;
    constexpr int kTextIconSize = 16;
    constexpr int kTextHeight = 36;
} // namespace

ShadButtonGroupText::ShadButtonGroupText(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadButtonGroupTextPrivate())
{
    Q_D(ShadButtonGroupText);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Medium);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadButtonGroupText::ShadButtonGroupText(const QString& text, QWidget* parent)
    : ShadButtonGroupText(parent)
{
    setText(text);
}

ShadButtonGroupText::~ShadButtonGroupText()
{
}

void ShadButtonGroupText::setText(const QString& text)
{
    Q_D(ShadButtonGroupText);
    if (d->_text == text)
    {
        return;
    }
    d->_text = text;
    setAccessibleName(text);
    updateGeometry();
    update();
}

QString ShadButtonGroupText::getText() const
{
    Q_D(const ShadButtonGroupText);
    return d->_text;
}

void ShadButtonGroupText::setShadIcon(ShadIconType::IconName icon)
{
    Q_D(ShadButtonGroupText);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadButtonGroupText::getShadIcon() const
{
    Q_D(const ShadButtonGroupText);
    return d->_icon;
}

QSize ShadButtonGroupText::sizeHint() const
{
    Q_D(const ShadButtonGroupText);
    const int margin = 2 * ShadStyleHelper::kFocusRingWidth;
    const int contentWidth = qCeil(ShadStyleHelper::iconTextWidth(font(), d->_text, d->_icon != ShadIconType::None, kTextGap, kTextIconSize));
    return QSize(contentWidth + 2 * kTextPaddingX + margin, kTextHeight + margin);
}

QSize ShadButtonGroupText::minimumSizeHint() const
{
    return sizeHint();
}

void ShadButtonGroupText::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    QWidget::changeEvent(event);
}

void ShadButtonGroupText::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadButtonGroupText);
    using namespace ShadStyleHelper;
    const QRectF bodyRect = insetRect(rect(), kFocusRingWidth);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const GroupEdges edges = groupEdges(this);
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);
    drawShadowXs(&painter, bodyRect, radius, edges);
    fillRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Muted), edges);
    strokeRoundedRect(&painter, bodyRect, radius, shadTheme->getColor(ShadThemeType::Border), edges);
    const QRectF contentRect = bodyRect.adjusted(kTextPaddingX, 0, -kTextPaddingX, 0);
    drawIconText(&painter, contentRect, d->_icon, QIcon(), d->_text, font(), shadTheme->getColor(ShadThemeType::Foreground), kTextGap, Qt::AlignLeft, kTextIconSize, 0.0, layoutDirection());
}

ShadButtonGroup::ShadButtonGroup(QWidget* parent)
    : ShadButtonGroup(Qt::Horizontal, parent)
{
}

ShadButtonGroup::ShadButtonGroup(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadButtonGroupPrivate())
{
    Q_D(ShadButtonGroup);
    d->q_ptr = this;
    d->_orientation = orientation;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyDefaultText(this, "accessibleName", accessibleName(), tr("按钮组"), [this](const QString& text) { setAccessibleName(text); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadButtonGroup::~ShadButtonGroup()
{
}

void ShadButtonGroup::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadButtonGroup);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    updateGeometry();
    d->relayout();
}

Qt::Orientation ShadButtonGroup::getOrientation() const
{
    Q_D(const ShadButtonGroup);
    return d->_orientation;
}

void ShadButtonGroup::addWidget(QWidget* widget)
{
    insertWidget(-1, widget);
}

void ShadButtonGroup::insertWidget(int index, QWidget* widget)
{
    Q_D(ShadButtonGroup);
    if (widget == nullptr || widget == this)
    {
        return;
    }
    for (const ShadButtonGroupPrivate::Item& item : std::as_const(d->_items))
    {
        if (item.widget.data() == widget)
        {
            return;
        }
    }
    d->insertItem(index, widget, false);
}

void ShadButtonGroup::removeWidget(QWidget* widget)
{
    Q_D(ShadButtonGroup);
    for (int index = 0; index < int(d->_items.size()); ++index)
    {
        if (d->_items.at(index).widget.data() != widget || widget == nullptr)
        {
            continue;
        }
        d->_items.removeAt(index);
        widget->removeEventFilter(this);
        ShadStyleHelper::setGroupEdges(widget, ShadStyleHelper::GroupEdges());
        ShadStyleHelper::clearFocusWithin(widget);
        widget->hide();
        widget->setParent(nullptr);
        updateGeometry();
        d->relayout();
        return;
    }
}

void ShadButtonGroup::addSeparator()
{
    Q_D(ShadButtonGroup);
    d->insertItem(-1, nullptr, true);
}

ShadButtonGroupText* ShadButtonGroup::addText(const QString& text, ShadIconType::IconName icon)
{
    ShadButtonGroupText * textWidget = new ShadButtonGroupText(text, this);
    textWidget->setShadIcon(icon);
    addWidget(textWidget);
    return textWidget;
}

int ShadButtonGroup::count() const
{
    Q_D(const ShadButtonGroup);
    int result = 0;
    for (const ShadButtonGroupPrivate::Item& item : d->_items)
    {
        if (item.isSeparator || !item.widget.isNull())
        {
            ++result;
        }
    }
    return result;
}

QWidget* ShadButtonGroup::widgetAt(int index) const
{
    Q_D(const ShadButtonGroup);
    int current = 0;
    for (const ShadButtonGroupPrivate::Item& item : d->_items)
    {
        if (!item.isSeparator && item.widget.isNull())
        {
            continue;
        }
        if (current == index)
        {
            return item.widget.data();
        }
        ++current;
    }
    return nullptr;
}

QSize ShadButtonGroup::sizeHint() const
{
    Q_D(const ShadButtonGroup);
    return d->contentSize(false);
}

QSize ShadButtonGroup::minimumSizeHint() const
{
    Q_D(const ShadButtonGroup);
    return d->contentSize(true);
}

bool ShadButtonGroup::event(QEvent* event)
{
    Q_D(ShadButtonGroup);
    switch (event->type())
    {
        case QEvent::LayoutRequest:
            {
                // 没有布局时，子组件 updateGeometry 会向父组件发送 LayoutRequest
                updateGeometry();
                d->relayout();
                break;
            }
        case QEvent::ChildRemoved:
            {
                // 子组件被外部删除后重新排列
                updateGeometry();
                d->relayout();
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::event(event);
}

bool ShadButtonGroup::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadButtonGroup);
    QWidget* widget = qobject_cast<QWidget*>(watched);
    if (widget != nullptr && widget->parentWidget() == this)
    {
        switch (event->type())
        {
            case QEvent::FocusIn:
                {
                    // 对应 [&>*]:focus-visible:z-10：获得焦点的组件在最上层，焦点环不被相邻组件遮挡
                    widget->raise();
                    break;
                }
            case QEvent::ShowToParent:
            case QEvent::HideToParent:
                {
                    updateGeometry();
                    d->relayout();
                    break;
                }
            default:
                {
                    break;
                }
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadButtonGroup::changeEvent(QEvent* event)
{
    Q_D(ShadButtonGroup);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        // 横向排列与拼接的起始侧随方向互换
        d->relayout();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        ShadStyleHelper::applyDefaultText(this, "accessibleName", accessibleName(), tr("按钮组"), [this](const QString& text) { setAccessibleName(text); });
    }
    QWidget::changeEvent(event);
}

void ShadButtonGroup::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadButtonGroup);
    QWidget::resizeEvent(event);
    d->relayout();
}

void ShadButtonGroup::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadButtonGroup);
    if (d->_separatorRects.isEmpty())
    {
        return;
    }
    // ButtonGroupSeparator：bg-input
    QPainter painter(this);
    painter.setOpacity(isEnabled() ? 1.0 : ShadStyleHelper::kDisabledOpacity);
    for (const QRect& separatorRect : std::as_const(d->_separatorRects))
    {
        painter.fillRect(separatorRect, shadTheme->getColor(ShadThemeType::Input));
    }
}
