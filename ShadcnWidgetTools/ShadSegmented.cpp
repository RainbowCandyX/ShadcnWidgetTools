#include "ShadSegmented.h"

#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QHelpEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QRect>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>
#include <QtMath>

#include "ShadSegmentedPrivate.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kPadding = 3;
    constexpr int kSmallHeight = 32;
    constexpr int kDefaultHeight = 36;
    constexpr int kLargeHeight = 40;
    constexpr int kSmallPaddingX = 8;
    constexpr int kDefaultPaddingX = 12;
    constexpr int kLargePaddingX = 16;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 6;
    constexpr int kFontPixelSize = 14;
    constexpr int kSlideDurationMs = 200;
    constexpr qreal kDarkActiveAlpha = 0.3;
    constexpr qreal kOutlineWidth = 1.0;
} // namespace

ShadSegmented::ShadSegmented(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSegmentedPrivate())
{
    Q_D(ShadSegmented);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    setProperty(ShadStyleHelper::kCustomToolTipProperty, true);
    d->_slideAnimation = new QVariantAnimation(this);
    d->_slideAnimation->setDuration(kSlideDurationMs);
    d->_slideAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_slideAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_blockRect = value.toRectF();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { layoutItems(); });
}

ShadSegmented::~ShadSegmented()
{
}

int ShadSegmented::addItem(const QString& text)
{
    return insertItem(-1, ShadIconType::None, text);
}

int ShadSegmented::addItem(ShadIconType::IconName icon, const QString& text)
{
    return insertItem(-1, icon, text);
}

int ShadSegmented::insertItem(int index, const QString& text)
{
    return insertItem(index, ShadIconType::None, text);
}

int ShadSegmented::insertItem(int index, ShadIconType::IconName icon, const QString& text)
{
    Q_D(ShadSegmented);
    const int position = index < 0 || index > count() ? count() : index;
    setHoverIndex(-1);
    ShadSegmentedPrivate::Item item;
    item.icon = icon;
    item.text = text;
    item.hoverTransition = new ShadTransition(this);
    item.activeTransition = new ShadTransition(this, kSlideDurationMs);
    d->_items.insert(position, item);
    if (d->_currentIndex >= position)
    {
        ++d->_currentIndex;
    }
    layoutItems();
    if (d->_currentIndex < 0)
    {
        setCurrentIndex(position);
        d->_items.at(position).activeTransition->jumpTo(1.0);
    }
    return position;
}

void ShadSegmented::removeItem(int index)
{
    Q_D(ShadSegmented);
    if (index < 0 || index >= count())
    {
        return;
    }
    setHoverIndex(-1);
    d->_pressedIndex = -1;
    delete d->_items.at(index).hoverTransition;
    delete d->_items.at(index).activeTransition;
    d->_items.removeAt(index);
    const int oldCurrent = d->_currentIndex;
    if (index < oldCurrent)
    {
        --d->_currentIndex;
    }
    else if (index == oldCurrent)
    {
        d->_currentIndex = count() > 0 ? qMin(index, count() - 1) : -1;
        if (d->_currentIndex >= 0)
        {
            d->_items.at(d->_currentIndex).activeTransition->jumpTo(1.0);
        }
    }
    layoutItems();
    if (index == oldCurrent)
    {
        Q_EMIT currentIndexChanged(d->_currentIndex);
    }
}

void ShadSegmented::clear()
{
    Q_D(ShadSegmented);
    setHoverIndex(-1);
    d->_pressedIndex = -1;
    for (const ShadSegmentedPrivate::Item& item : d->_items)
    {
        delete item.hoverTransition;
        delete item.activeTransition;
    }
    d->_items.clear();
    const bool isChanged = d->_currentIndex != -1;
    d->_currentIndex = -1;
    layoutItems();
    if (isChanged)
    {
        Q_EMIT currentIndexChanged(-1);
    }
}

int ShadSegmented::count() const
{
    Q_D(const ShadSegmented);
    return int(d->_items.size());
}

void ShadSegmented::setItemText(int index, const QString& text)
{
    Q_D(ShadSegmented);
    if (index < 0 || index >= count() || d->_items.at(index).text == text)
    {
        return;
    }
    d->_items[index].text = text;
    layoutItems();
}

QString ShadSegmented::getItemText(int index) const
{
    Q_D(const ShadSegmented);
    return index >= 0 && index < count() ? d->_items.at(index).text : QString();
}

void ShadSegmented::setItemIcon(int index, ShadIconType::IconName icon)
{
    Q_D(ShadSegmented);
    if (index < 0 || index >= count() || d->_items.at(index).icon == icon)
    {
        return;
    }
    d->_items[index].icon = icon;
    layoutItems();
}

ShadIconType::IconName ShadSegmented::getItemIcon(int index) const
{
    Q_D(const ShadSegmented);
    return index >= 0 && index < count() ? d->_items.at(index).icon : ShadIconType::None;
}

void ShadSegmented::setItemEnabled(int index, bool enabled)
{
    Q_D(ShadSegmented);
    if (index < 0 || index >= count() || d->_items.at(index).isEnabled == enabled)
    {
        return;
    }
    d->_items[index].isEnabled = enabled;
    if (!enabled && d->_hoverIndex == index)
    {
        setHoverIndex(-1);
    }
    update();
}

bool ShadSegmented::isItemEnabled(int index) const
{
    Q_D(const ShadSegmented);
    return index >= 0 && index < count() && d->_items.at(index).isEnabled;
}

void ShadSegmented::setItemToolTip(int index, const QString& toolTip)
{
    Q_D(ShadSegmented);
    if (index >= 0 && index < count())
    {
        d->_items[index].toolTip = toolTip;
    }
}

QString ShadSegmented::getItemToolTip(int index) const
{
    Q_D(const ShadSegmented);
    return index >= 0 && index < count() ? d->_items.at(index).toolTip : QString();
}

void ShadSegmented::setCurrentIndex(int index)
{
    Q_D(ShadSegmented);
    if (index < -1 || index >= count() || index == d->_currentIndex)
    {
        return;
    }
    if (d->_currentIndex >= 0)
    {
        d->_items.at(d->_currentIndex).activeTransition->animateTo(0.0);
    }
    d->_currentIndex = index;
    if (index >= 0)
    {
        d->_items.at(index).activeTransition->animateTo(1.0);
    }
    syncBlock(d->_isPositioned && isVisible());
    update();
    Q_EMIT currentIndexChanged(index);
}

int ShadSegmented::getCurrentIndex() const
{
    Q_D(const ShadSegmented);
    return d->_currentIndex;
}

void ShadSegmented::setSegmentedSize(ShadSegmentedType::Size size)
{
    Q_D(ShadSegmented);
    if (d->_size == size)
    {
        return;
    }
    d->_size = size;
    layoutItems();
}

ShadSegmentedType::Size ShadSegmented::getSegmentedSize() const
{
    Q_D(const ShadSegmented);
    return d->_size;
}

void ShadSegmented::setBlock(bool block)
{
    Q_D(ShadSegmented);
    if (d->_isBlock == block)
    {
        return;
    }
    d->_isBlock = block;
    setSizePolicy(block ? QSizePolicy::Expanding : QSizePolicy::Fixed, QSizePolicy::Fixed);
    layoutItems();
}

bool ShadSegmented::isBlock() const
{
    Q_D(const ShadSegmented);
    return d->_isBlock;
}

QSize ShadSegmented::sizeHint() const
{
    Q_D(const ShadSegmented);
    int width = 2 * kPadding;
    for (int index = 0; index < count(); ++index)
    {
        width += contentWidth(index);
    }
    int height = kDefaultHeight;
    if (d->_size == ShadSegmentedType::Small)
    {
        height = kSmallHeight;
    }
    else if (d->_size == ShadSegmentedType::Large)
    {
        height = kLargeHeight;
    }
    return QSize(width, height);
}

QSize ShadSegmented::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadSegmented::event(QEvent* event)
{
    Q_D(ShadSegmented);
    if (event->type() == QEvent::ToolTip)
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        const int index = itemAt(helpEvent->pos());
        if (index >= 0 && !d->_items.at(index).toolTip.isEmpty())
        {
            shadTooltip->showText(this, itemRect(index).toAlignedRect(), d->_items.at(index).toolTip);
        }
        return true;
    }
    return QWidget::event(event);
}

void ShadSegmented::mouseMoveEvent(QMouseEvent* event)
{
    setHoverIndex(itemAt(event->position().toPoint()));
    QWidget::mouseMoveEvent(event);
}

void ShadSegmented::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadSegmented);
    const int index = itemAt(event->position().toPoint());
    if (event->button() != Qt::LeftButton || !isSelectable(index))
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->_pressedIndex = index;
    event->accept();
}

void ShadSegmented::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadSegmented);
    if (event->button() != Qt::LeftButton || d->_pressedIndex < 0)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int index = itemAt(event->position().toPoint());
    const int pressed = d->_pressedIndex;
    d->_pressedIndex = -1;
    if (index == pressed && isSelectable(index))
    {
        setCurrentIndex(index);
    }
    event->accept();
}

void ShadSegmented::leaveEvent(QEvent* event)
{
    setHoverIndex(-1);
    QWidget::leaveEvent(event);
}

void ShadSegmented::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadSegmented);
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    int step = 0;
    int target = -1;
    bool isHandled = false;
    switch (event->key())
    {
        case Qt::Key_Left:
            step = isRightToLeft ? 1 : -1;
            isHandled = true;
            break;
        case Qt::Key_Right:
            step = isRightToLeft ? -1 : 1;
            isHandled = true;
            break;
        case Qt::Key_Home:
            target = stepSelectable(-1, 1);
            isHandled = true;
            break;
        case Qt::Key_End:
            target = stepSelectable(count(), -1);
            isHandled = true;
            break;
        default:
            break;
    }
    if (!isHandled)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    if (step != 0)
    {
        target = stepSelectable(d->_currentIndex < 0 ? (step > 0 ? -1 : count()) : d->_currentIndex, step);
    }
    d->_isFocusVisible = true;
    if (target >= 0)
    {
        setCurrentIndex(target);
    }
    update();
    event->accept();
}

void ShadSegmented::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadSegmented);
    d->_isFocusVisible = ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event);
    update();
    QWidget::focusInEvent(event);
}

void ShadSegmented::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadSegmented);
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        d->_isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadSegmented::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        setHoverIndex(-1);
    }
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        layoutItems();
    }
    QWidget::changeEvent(event);
}

void ShadSegmented::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadSegmented);
    syncBlock(false);
    d->_isPositioned = true;
    QWidget::resizeEvent(event);
}

void ShadSegmented::showEvent(QShowEvent* event)
{
    Q_D(ShadSegmented);
    syncBlock(false);
    d->_isPositioned = true;
    QWidget::showEvent(event);
}

void ShadSegmented::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(const ShadSegmented);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isDark = shadTheme->isDark();
    const qreal baseOpacity = isEnabled() ? 1.0 : kDisabledOpacity;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor activeBackground = isDark ? withAlpha(selectionColor(ShadThemeType::Input), kDarkActiveAlpha) : selectionColor(ShadThemeType::Background);
    const QFont font = itemFont();
    const int paddingX = itemPaddingX();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(baseOpacity);
    fillRoundedRect(&painter, QRectF(rect()), shadTheme->getRadius(ShadThemeType::Large), shadTheme->getColor(ShadThemeType::Muted));
    QRectF blockArea;
    if (d->_currentIndex >= 0 && !d->_blockRect.isEmpty())
    {
        blockArea = visualRect(direction, QRectF(rect()), d->_blockRect);
        painter.setOpacity(baseOpacity * (d->_items.at(d->_currentIndex).isEnabled ? 1.0 : kDisabledOpacity));
        drawShadowSm(&painter, blockArea, radius);
        fillRoundedRect(&painter, blockArea, radius, activeBackground);
        if (isDark)
        {
            strokeRoundedRect(&painter, blockArea, radius, selectionColor(ShadThemeType::Input));
        }
    }
    for (int index = 0; index < count(); ++index)
    {
        const ShadSegmentedPrivate::Item& item = d->_items.at(index);
        const QRectF area = itemRect(index);
        const qreal level = qMax(item.hoverTransition->value(), item.activeTransition->value());
        painter.setOpacity(baseOpacity * (item.isEnabled ? 1.0 : kDisabledOpacity));
        const QColor textColor = mix(mutedForeground, foreground, level);
        const QRectF contentRect = area.adjusted(paddingX, 0, -paddingX, 0);
        drawIconText(&painter, contentRect, item.icon, QIcon(), item.text, font, textColor, kIconGap, Qt::AlignHCenter | Qt::AlignVCenter, kIconSize, 0.0, direction);
    }
    if (hasFocus() && d->_isFocusVisible && !blockArea.isEmpty())
    {
        painter.setOpacity(baseOpacity);
        drawFocusRing(&painter, blockArea, radius, focusRingColor(false));
        strokeRoundedRect(&painter, blockArea.adjusted(-kOutlineWidth, -kOutlineWidth, kOutlineWidth, kOutlineWidth), radius + kOutlineWidth, shadTheme->getColor(ShadThemeType::Ring), kOutlineWidth);
        strokeRoundedRect(&painter, blockArea, radius, shadTheme->getColor(ShadThemeType::Ring));
    }
}

QFont ShadSegmented::itemFont() const
{
    QFont font = this->font();
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Medium);
    return font;
}

int ShadSegmented::itemPaddingX() const
{
    Q_D(const ShadSegmented);
    if (d->_size == ShadSegmentedType::Small)
    {
        return kSmallPaddingX;
    }
    return d->_size == ShadSegmentedType::Large ? kLargePaddingX : kDefaultPaddingX;
}

int ShadSegmented::itemHeight() const
{
    return qMax(0, height() - 2 * kPadding);
}

int ShadSegmented::contentWidth(int index) const
{
    Q_D(const ShadSegmented);
    const ShadSegmentedPrivate::Item& item = d->_items.at(index);
    const qreal content = ShadStyleHelper::iconTextWidth(itemFont(), item.text, item.icon != ShadIconType::None, kIconGap, kIconSize);
    return qCeil(content) + 2 * itemPaddingX();
}

QRectF ShadSegmented::logicalItemRect(int index) const
{
    Q_D(const ShadSegmented);
    if (d->_isBlock)
    {
        const qreal each = count() > 0 ? (width() - 2 * kPadding) / qreal(count()) : 0.0;
        return QRectF(kPadding + index * each, kPadding, each, itemHeight());
    }
    qreal left = kPadding;
    for (int position = 0; position < index; ++position)
    {
        left += contentWidth(position);
    }
    return QRectF(left, kPadding, contentWidth(index), itemHeight());
}

QRectF ShadSegmented::itemRect(int index) const
{
    return ShadStyleHelper::visualRect(layoutDirection(), QRectF(rect()), logicalItemRect(index));
}

int ShadSegmented::itemAt(const QPoint& position) const
{
    for (int index = 0; index < count(); ++index)
    {
        if (itemRect(index).contains(QPointF(position)))
        {
            return index;
        }
    }
    return -1;
}

bool ShadSegmented::isSelectable(int index) const
{
    Q_D(const ShadSegmented);
    return index >= 0 && index < count() && d->_items.at(index).isEnabled && isEnabled();
}

int ShadSegmented::stepSelectable(int from, int step) const
{
    for (int index = from + step; index >= 0 && index < count(); index += step)
    {
        if (isSelectable(index))
        {
            return index;
        }
    }
    return -1;
}

void ShadSegmented::setHoverIndex(int index)
{
    Q_D(ShadSegmented);
    const int target = isSelectable(index) ? index : -1;
    if (target == d->_hoverIndex)
    {
        return;
    }
    if (d->_hoverIndex >= 0 && d->_hoverIndex < count())
    {
        d->_items.at(d->_hoverIndex).hoverTransition->animateTo(0.0);
    }
    d->_hoverIndex = target;
    if (target >= 0)
    {
        d->_items.at(target).hoverTransition->animateTo(1.0);
    }
}

void ShadSegmented::syncBlock(bool animate)
{
    Q_D(ShadSegmented);
    const QRectF target = d->_currentIndex >= 0 ? logicalItemRect(d->_currentIndex) : QRectF();
    d->_slideAnimation->stop();
    if (!animate || d->_blockRect.isEmpty() || target.isEmpty())
    {
        d->_blockRect = target;
        update();
        return;
    }
    if (d->_blockRect == target)
    {
        return;
    }
    d->_slideAnimation->setStartValue(d->_blockRect);
    d->_slideAnimation->setEndValue(target);
    d->_slideAnimation->start();
}

void ShadSegmented::layoutItems()
{
    updateGeometry();
    syncBlock(false);
    update();
}
