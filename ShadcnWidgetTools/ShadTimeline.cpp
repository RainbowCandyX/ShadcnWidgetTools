#include "ShadTimeline.h"

#include <QBrush>
#include <QColor>
#include <QEvent>
#include <QFont>
#include <QHideEvent>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QResizeEvent>
#include <QShowEvent>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QVariant>
#include <QVariantAnimation>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadIcon.h"
#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTimelinePrivate.h"

namespace
{
    ShadLabel* createLabel(QWidget* parent, int pixelSize, QFont::Weight weight, ShadThemeType::ThemeColor colorRole)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setTextFormat(Qt::PlainText);
        label->setWordWrap(true);
        label->setColorRole(colorRole);
        ShadStyleHelper::applyFont(label, pixelSize, weight);
        label->hide();
        return label;
    }
} // namespace

ShadTimelineItem::ShadTimelineItem(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTimelineItemPrivate())
{
    Q_D(ShadTimelineItem);
    d->q_ptr = this;
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    d->_timeLabel = createLabel(this, ShadTimelineItemPrivate::kTimeFontSize, QFont::Normal, ShadThemeType::MutedForeground);
    d->_titleLabel = createLabel(this, ShadTimelineItemPrivate::kTitleFontSize, QFont::Medium, ShadThemeType::Foreground);
    d->_descriptionLabel = createLabel(this, ShadTimelineItemPrivate::kTextFontSize, QFont::Normal, ShadThemeType::MutedForeground);
    d->_contentWidget = new QWidget(this);
    ShadStyleHelper::applyFont(d->_contentWidget, ShadTimelineItemPrivate::kTextFontSize, QFont::Normal);
    d->_contentLayout = new QVBoxLayout(d->_contentWidget);
    d->_contentLayout->setContentsMargins(0, 0, 0, 0);
    d->_contentLayout->setSpacing(ShadTimelineItemPrivate::kContentGap);

    d->_spinAnimation = new QVariantAnimation(this);
    d->_spinAnimation->setStartValue(0.0);
    d->_spinAnimation->setEndValue(360.0);
    d->_spinAnimation->setDuration(ShadStyleHelper::kSpinDurationMs);
    d->_spinAnimation->setLoopCount(-1);
    connect(d->_spinAnimation, &QVariantAnimation::valueChanged, this, [this, d](const QVariant& value)
    {
        d->_spinAngle = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    d->updateContent();
}

ShadTimelineItem::ShadTimelineItem(const QString& title, const QString& time, const QString& description, QWidget* parent)
    : ShadTimelineItem(parent)
{
    Q_D(ShadTimelineItem);
    d->_titleLabel->setText(title);
    d->_timeLabel->setText(time);
    d->_descriptionLabel->setText(description);
    d->updateContent();
}

ShadTimelineItem::~ShadTimelineItem()
{
    Q_D(ShadTimelineItem);
    d->_spinAnimation->stop();
    QObject::disconnect(d->_spinAnimation, nullptr, this, nullptr);
}

void ShadTimelineItem::setTitle(const QString& title)
{
    Q_D(ShadTimelineItem);
    d->_titleLabel->setText(title);
    d->updateContent();
}

QString ShadTimelineItem::getTitle() const
{
    Q_D(const ShadTimelineItem);
    return d->_titleLabel->text();
}

void ShadTimelineItem::setTime(const QString& time)
{
    Q_D(ShadTimelineItem);
    d->_timeLabel->setText(time);
    d->updateContent();
}

QString ShadTimelineItem::getTime() const
{
    Q_D(const ShadTimelineItem);
    return d->_timeLabel->text();
}

void ShadTimelineItem::setDescription(const QString& description)
{
    Q_D(ShadTimelineItem);
    d->_descriptionLabel->setText(description);
    d->updateContent();
}

QString ShadTimelineItem::getDescription() const
{
    Q_D(const ShadTimelineItem);
    return d->_descriptionLabel->text();
}

void ShadTimelineItem::setIcon(ShadIconType::IconName icon)
{
    Q_D(ShadTimelineItem);
    if (d->_icon == icon)
    {
        return;
    }
    d->_icon = icon;
    d->updateSpin();
    d->updateContent();
}

ShadIconType::IconName ShadTimelineItem::getIcon() const
{
    Q_D(const ShadTimelineItem);
    return d->_icon;
}

void ShadTimelineItem::setColor(ShadThemeType::ThemeColor color)
{
    Q_D(ShadTimelineItem);
    if (d->_color == color)
    {
        return;
    }
    d->_color = color;
    update();
}

ShadThemeType::ThemeColor ShadTimelineItem::getColor() const
{
    Q_D(const ShadTimelineItem);
    return d->_color;
}

QVBoxLayout* ShadTimelineItem::contentLayout() const
{
    Q_D(const ShadTimelineItem);
    return d->_contentLayout;
}

bool ShadTimelineItem::hasHeightForWidth() const
{
    return true;
}

int ShadTimelineItem::heightForWidth(int width) const
{
    Q_D(const ShadTimelineItem);
    return d->computeLayout(width).totalHeight;
}

QSize ShadTimelineItem::sizeHint() const
{
    Q_D(const ShadTimelineItem);
    const int sideWidth = ShadTimelineItemPrivate::kAxisGap + d->naturalTextWidth();
    const int width = ShadTimelineItemPrivate::kAxisWidth + (d->_mode == ShadTimelineType::Alternate ? 2 * sideWidth : sideWidth);
    return QSize(width, heightForWidth(width));
}

QSize ShadTimelineItem::minimumSizeHint() const
{
    Q_D(const ShadTimelineItem);
    int textWidth = ShadTimelineItemPrivate::kMinTextWidth;
    if (d->hasContent())
    {
        textWidth = qMax(textWidth, d->_contentWidget->minimumSizeHint().width());
    }
    const int sideWidth = ShadTimelineItemPrivate::kAxisGap + textWidth;
    const int width = ShadTimelineItemPrivate::kAxisWidth + (d->_mode == ShadTimelineType::Alternate ? 2 * sideWidth : sideWidth);
    return QSize(width, ShadTimelineItemPrivate::kTitleLineHeight);
}

bool ShadTimelineItem::event(QEvent* event)
{
    Q_D(ShadTimelineItem);
    if (event->type() == QEvent::LayoutRequest)
    {
        d->layoutChildren();
        updateGeometry();
    }
    return QWidget::event(event);
}

void ShadTimelineItem::changeEvent(QEvent* event)
{
    Q_D(ShadTimelineItem);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutChildren();
    }
    else if (event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadTimelineItem::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadTimelineItem);
    QWidget::resizeEvent(event);
    d->layoutChildren();
}

void ShadTimelineItem::showEvent(QShowEvent* event)
{
    Q_D(ShadTimelineItem);
    QWidget::showEvent(event);
    d->layoutChildren();
    d->updateSpin();
}

void ShadTimelineItem::hideEvent(QHideEvent* event)
{
    Q_D(ShadTimelineItem);
    QWidget::hideEvent(event);
    d->updateSpin();
}

void ShadTimelineItem::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadTimelineItem);
    using namespace ShadStyleHelper;
    const ShadTimelineItemPrivate::LayoutResult layout = d->computeLayout(width());
    const qreal centerX = layoutDirection() == Qt::RightToLeft ? width() - layout.axisCenterX : layout.axisCenterX;
    const qreal markerRadius = d->markerRadius();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(isEnabled() ? 1.0 : kDisabledOpacity);

    QPen linePen(shadTheme->getColor(ShadThemeType::Border), ShadTimelineItemPrivate::kLineWidth);
    linePen.setCapStyle(Qt::FlatCap);
    const qreal markerTop = layout.dotCenterY - markerRadius - ShadTimelineItemPrivate::kLineGap;
    const qreal markerBottom = layout.dotCenterY + markerRadius + ShadTimelineItemPrivate::kLineGap;
    if (d->_hasLineAbove && markerTop > 0.0)
    {
        QPen pen = linePen;
        if (d->_isLineAboveDashed)
        {
            pen.setDashPattern(QList<qreal>{2.0, 2.0});
        }
        painter.setPen(pen);
        painter.drawLine(QPointF(centerX, 0.0), QPointF(centerX, markerTop));
    }
    if (d->_hasLineBelow && markerBottom < height())
    {
        QPen pen = linePen;
        if (d->_isLineBelowDashed)
        {
            pen.setDashPattern(QList<qreal>{2.0, 2.0});
        }
        painter.setPen(pen);
        painter.drawLine(QPointF(centerX, markerBottom), QPointF(centerX, height()));
    }

    const QColor markerColor = shadTheme->getColor(d->_color);
    const QPointF center(centerX, layout.dotCenterY);
    if (d->_icon == ShadIconType::None)
    {
        painter.setPen(Qt::NoPen);
        painter.setBrush(markerColor);
        painter.drawEllipse(center, markerRadius, markerRadius);
        return;
    }
    painter.translate(center);
    painter.rotate(d->_spinAngle);
    const qreal iconHalf = ShadTimelineItemPrivate::kIconSize / 2.0;
    ShadIcon::paint(&painter, QRectF(-iconHalf, -iconHalf, ShadTimelineItemPrivate::kIconSize, ShadTimelineItemPrivate::kIconSize), d->_icon, markerColor, ShadTimelineItemPrivate::kIconSize);
}

ShadTimeline::ShadTimeline(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTimelinePrivate())
{
    Q_D(ShadTimeline);
    d->q_ptr = this;
    d->_layout = new QVBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
    d->rebuild();
}

ShadTimeline::~ShadTimeline()
{
}

ShadTimelineItem* ShadTimeline::addItem(const QString& title, const QString& time, const QString& description)
{
    Q_D(ShadTimeline);
    ShadTimelineItem* item = new ShadTimelineItem(title, time, description, this);
    d->_items.append(item);
    d->rebuild();
    return item;
}

ShadTimelineItem* ShadTimeline::itemAt(int index) const
{
    Q_D(const ShadTimeline);
    if (index < 0 || index >= d->_items.size())
    {
        return nullptr;
    }
    return d->_items.at(index);
}

int ShadTimeline::count() const
{
    Q_D(const ShadTimeline);
    return int(d->_items.size());
}

void ShadTimeline::clear()
{
    Q_D(ShadTimeline);
    const QList<ShadTimelineItem*> items = d->_items;
    d->_items.clear();
    d->rebuild();
    for (ShadTimelineItem* item : items)
    {
        ShadStyleHelper::clearFocusWithin(item);
        delete item;
    }
}

void ShadTimeline::setMode(ShadTimelineType::Mode mode)
{
    Q_D(ShadTimeline);
    if (d->_mode == mode)
    {
        return;
    }
    d->_mode = mode;
    d->rebuild();
}

ShadTimelineType::Mode ShadTimeline::getMode() const
{
    Q_D(const ShadTimeline);
    return d->_mode;
}

void ShadTimeline::setReverse(bool reverse)
{
    Q_D(ShadTimeline);
    if (d->_isReverse == reverse)
    {
        return;
    }
    d->_isReverse = reverse;
    d->rebuild();
}

bool ShadTimeline::isReverse() const
{
    Q_D(const ShadTimeline);
    return d->_isReverse;
}

void ShadTimeline::setPending(const QString& text)
{
    Q_D(ShadTimeline);
    if (d->_pendingText == text && (d->_pendingItem != nullptr) == !text.isEmpty())
    {
        return;
    }
    d->_pendingText = text;
    if (text.isEmpty())
    {
        ShadTimelineItem* pendingItem = d->_pendingItem;
        d->_pendingItem = nullptr;
        d->rebuild();
        if (pendingItem != nullptr)
        {
            ShadStyleHelper::clearFocusWithin(pendingItem);
            delete pendingItem;
        }
        return;
    }
    if (d->_pendingItem == nullptr)
    {
        d->_pendingItem = new ShadTimelineItem(this);
        d->_pendingItem->setIcon(ShadIconType::LoaderCircle);
        d->_pendingItem->setColor(ShadThemeType::MutedForeground);
        d->_pendingItem->d_func()->setPending(true);
    }
    d->_pendingItem->setTitle(text);
    d->rebuild();
}

QString ShadTimeline::getPending() const
{
    Q_D(const ShadTimeline);
    return d->_pendingText;
}

ShadTimelineItem* ShadTimeline::pendingItem() const
{
    Q_D(const ShadTimeline);
    return d->_pendingItem;
}
