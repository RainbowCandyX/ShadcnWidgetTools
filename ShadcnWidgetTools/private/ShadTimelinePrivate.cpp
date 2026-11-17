#include "ShadTimelinePrivate.h"

#include <QAbstractAnimation>
#include <QFont>
#include <QLayoutItem>
#include <QSize>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtMath>

#include <algorithm>

#include "ShadLabel.h"
#include "ShadStyleHelper.h"
#include "ShadTimeline.h"

ShadTimelineItemPrivate::ShadTimelineItemPrivate()
{
}

ShadTimelineItemPrivate::~ShadTimelineItemPrivate()
{
}

bool ShadTimelineItemPrivate::hasTime() const
{
    return !_timeLabel->text().isEmpty();
}

bool ShadTimelineItemPrivate::hasTitle() const
{
    return !_titleLabel->text().isEmpty();
}

bool ShadTimelineItemPrivate::hasDescription() const
{
    return !_descriptionLabel->text().isEmpty();
}

bool ShadTimelineItemPrivate::hasContent() const
{
    return !_contentLayout->isEmpty();
}

bool ShadTimelineItemPrivate::isTimeOpposite() const
{
    return _mode == ShadTimelineType::Alternate;
}

qreal ShadTimelineItemPrivate::markerRadius() const
{
    return (_icon == ShadIconType::None ? kDotSize : kIconSize) / 2.0;
}

int ShadTimelineItemPrivate::labelHeight(const ShadLabel* label, int width) const
{
    const int height = label->heightForWidth(width);
    return height >= 0 ? height : label->sizeHint().height();
}

int ShadTimelineItemPrivate::contentHeightForWidth(int width) const
{
    const int height = _contentWidget->heightForWidth(width);
    return height >= 0 ? height : _contentWidget->sizeHint().height();
}

int ShadTimelineItemPrivate::naturalTextWidth() const
{
    int textWidth = 0;
    if (hasTitle())
    {
        textWidth = qMax(textWidth, ShadStyleHelper::textWidth(_titleLabel->font(), _titleLabel->text()));
    }
    if (hasTime())
    {
        textWidth = qMax(textWidth, ShadStyleHelper::textWidth(_timeLabel->font(), _timeLabel->text()));
    }
    if (hasDescription())
    {
        textWidth = qMax(textWidth, ShadStyleHelper::textWidth(_descriptionLabel->font(), _descriptionLabel->text()));
    }
    textWidth = qMin(textWidth, kMaxNaturalTextWidth);
    if (hasContent())
    {
        textWidth = qMax(textWidth, _contentWidget->sizeHint().width());
    }
    return textWidth;
}

ShadTimelineItemPrivate::LayoutResult ShadTimelineItemPrivate::computeLayout(int width) const
{
    LayoutResult result;
    int axisLeft = 0;
    int columnLeft = 0;
    int columnWidth = 1;
    int oppositeLeft = 0;
    int oppositeWidth = 1;
    switch (_mode)
    {
        case ShadTimelineType::Start:
            axisLeft = 0;
            columnLeft = kAxisWidth + kAxisGap;
            columnWidth = qMax(1, width - columnLeft);
            break;
        case ShadTimelineType::End:
            axisLeft = width - kAxisWidth;
            columnLeft = 0;
            columnWidth = qMax(1, axisLeft - kAxisGap);
            break;
        case ShadTimelineType::Alternate:
            {
                axisLeft = (width - kAxisWidth) / 2;
                const int startWidth = qMax(1, axisLeft - kAxisGap);
                const int endLeft = axisLeft + kAxisWidth + kAxisGap;
                const int endWidth = qMax(1, width - endLeft);
                columnLeft = _isContentAtStart ? 0 : endLeft;
                columnWidth = _isContentAtStart ? startWidth : endWidth;
                oppositeLeft = _isContentAtStart ? endLeft : 0;
                oppositeWidth = _isContentAtStart ? endWidth : startWidth;
                break;
            }
    }
    result.axisCenterX = axisLeft + kAxisWidth / 2.0;

    const bool isTimeInColumn = hasTime() && !isTimeOpposite();
    int top = 0;
    bool hasItemAbove = false;
    if (isTimeInColumn)
    {
        const int height = labelHeight(_timeLabel, columnWidth);
        result.timeRect = QRect(columnLeft, top, columnWidth, height);
        top += height;
        hasItemAbove = true;
    }
    const int titleTop = hasItemAbove ? top + kTextGap : top;
    if (hasTitle())
    {
        top = titleTop;
        const int height = labelHeight(_titleLabel, columnWidth);
        result.titleRect = QRect(columnLeft, top, columnWidth, height);
        top += height;
        hasItemAbove = true;
    }
    if (hasDescription())
    {
        if (hasItemAbove)
        {
            top += kTextGap;
        }
        const int height = labelHeight(_descriptionLabel, columnWidth);
        result.descriptionRect = QRect(columnLeft, top, columnWidth, height);
        top += height;
        hasItemAbove = true;
    }
    if (hasContent())
    {
        if (hasItemAbove)
        {
            top += kContentGap;
        }
        const int height = contentHeightForWidth(columnWidth);
        result.contentRect = QRect(columnLeft, top, columnWidth, height);
        top += height;
    }
    result.dotCenterY = titleTop + kTitleLineHeight / 2.0;
    int bottom = qMax(top, qCeil(result.dotCenterY + markerRadius()));
    if (hasTime() && isTimeOpposite())
    {
        const int height = labelHeight(_timeLabel, oppositeWidth);
        result.timeRect = QRect(oppositeLeft, titleTop + (kTitleLineHeight - kTimeLineHeight) / 2, oppositeWidth, height);
        bottom = qMax(bottom, result.timeRect.bottom() + 1);
    }
    result.totalHeight = bottom + (_hasLineBelow ? kItemGap : 0);
    return result;
}

void ShadTimelineItemPrivate::updateContent()
{
    Q_Q(ShadTimelineItem);
    _timeLabel->setVisible(hasTime());
    _titleLabel->setVisible(hasTitle());
    _descriptionLabel->setVisible(hasDescription());
    const Qt::Alignment columnAlignment = (_isContentAtStart ? Qt::AlignRight : Qt::AlignLeft) | Qt::AlignTop;
    const Qt::Alignment timeAlignment = isTimeOpposite() ? ((_isContentAtStart ? Qt::AlignLeft : Qt::AlignRight) | Qt::AlignTop) : columnAlignment;
    _titleLabel->setAlignment(columnAlignment);
    _descriptionLabel->setAlignment(columnAlignment);
    _timeLabel->setAlignment(timeAlignment);
    layoutChildren();
    q->updateGeometry();
    q->update();
}

void ShadTimelineItemPrivate::layoutChildren()
{
    Q_Q(ShadTimelineItem);
    const QRect bounds = q->rect();
    const Qt::LayoutDirection direction = q->layoutDirection();
    const LayoutResult layout = computeLayout(q->width());
    _timeLabel->setGeometry(ShadStyleHelper::visualRect(direction, bounds, layout.timeRect));
    _titleLabel->setGeometry(ShadStyleHelper::visualRect(direction, bounds, layout.titleRect));
    _descriptionLabel->setGeometry(ShadStyleHelper::visualRect(direction, bounds, layout.descriptionRect));
    _contentWidget->setGeometry(ShadStyleHelper::visualRect(direction, bounds, layout.contentRect));
    q->update();
}

void ShadTimelineItemPrivate::setPlacement(ShadTimelineType::Mode mode, bool isContentAtStart, bool hasLineAbove, bool hasLineBelow, bool isLineAboveDashed, bool isLineBelowDashed)
{
    _mode = mode;
    _isContentAtStart = isContentAtStart;
    _hasLineAbove = hasLineAbove;
    _hasLineBelow = hasLineBelow;
    _isLineAboveDashed = isLineAboveDashed;
    _isLineBelowDashed = isLineBelowDashed;
    updateContent();
}

void ShadTimelineItemPrivate::setPending(bool pending)
{
    _isPending = pending;
    _titleLabel->setColorRole(pending ? ShadThemeType::MutedForeground : ShadThemeType::Foreground);
    ShadStyleHelper::applyFont(_titleLabel, kTitleFontSize, pending ? QFont::Normal : QFont::Medium);
    updateContent();
}

void ShadTimelineItemPrivate::updateSpin()
{
    Q_Q(ShadTimelineItem);
    const bool shouldSpin = _icon == ShadIconType::LoaderCircle && q->isVisible();
    if (shouldSpin && _spinAnimation->state() != QAbstractAnimation::Running)
    {
        _spinAnimation->start();
    }
    else if (!shouldSpin)
    {
        _spinAnimation->stop();
        _spinAngle = 0.0;
    }
}

ShadTimelinePrivate::ShadTimelinePrivate()
{
}

ShadTimelinePrivate::~ShadTimelinePrivate()
{
}

void ShadTimelinePrivate::rebuild()
{
    QList<ShadTimelineItem*> order = _items;
    if (_isReverse)
    {
        std::reverse(order.begin(), order.end());
    }
    if (_pendingItem != nullptr)
    {
        if (_isReverse)
        {
            order.prepend(_pendingItem);
        }
        else
        {
            order.append(_pendingItem);
        }
    }
    while (_layout->count() > 0)
    {
        delete _layout->takeAt(0);
    }
    const int itemCount = int(order.size());
    const int pendingIndex = _pendingItem == nullptr ? -1 : (_isReverse ? 0 : itemCount - 1);
    for (int index = 0; index < itemCount; ++index)
    {
        ShadTimelineItem* item = order.at(index);
        _layout->addWidget(item);
        const bool isContentAtStart = _mode == ShadTimelineType::End || (_mode == ShadTimelineType::Alternate && index % 2 == 1);
        const bool hasLineAbove = index > 0;
        const bool hasLineBelow = index < itemCount - 1;
        const bool isLineAboveDashed = hasLineAbove && pendingIndex >= 0 && (index == pendingIndex || index - 1 == pendingIndex);
        const bool isLineBelowDashed = hasLineBelow && pendingIndex >= 0 && (index == pendingIndex || index + 1 == pendingIndex);
        item->d_func()->setPlacement(_mode, isContentAtStart, hasLineAbove, hasLineBelow, isLineAboveDashed, isLineBelowDashed);
    }
    _layout->addStretch(1);
}
