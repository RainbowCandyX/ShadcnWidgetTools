#include "ShadTracePrivate.h"

#include <QAbstractAnimation>
#include <QDesktopServices>
#include <QFont>
#include <QRect>
#include <QStringList>
#include <QVBoxLayout>
#include <QVariantAnimation>
#include <QWidget>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTrace.h"
#include "ShadTransition.h"

ShadTraceLinePrivate::ShadTraceLinePrivate()
{
}

ShadTraceLinePrivate::~ShadTraceLinePrivate()
{
}

ShadTraceLinePrivate::Layout ShadTraceLinePrivate::arrange(qreal left, qreal centerY, qreal titleWidth) const
{
    Layout layout;
    qreal x = left;
    bool hasPrevious = false;
    // flex gap：只在相邻元素之间计入间距
    const auto place = [&](qreal width, qreal gap, qreal height)
    {
        if (hasPrevious)
        {
            x += gap;
        }
        hasPrevious = true;
        const QRectF rect(x, centerY - height / 2.0, width, height);
        x += width;
        return rect;
    };
    const bool hasTitle = !_title.isEmpty();
    if (_icon != ShadIconType::None)
    {
        layout.icon = place(kIconSize, kGap, kIconSize);
    }
    if (hasTitle)
    {
        layout.title = place(titleWidth, kGap, kLineHeight);
        layout.elidedTitle = ShadStyleHelper::elideText(titleFont(), _title, titleWidth);
    }
    if (!_description.isEmpty())
    {
        layout.description = place(ShadStyleHelper::textWidth(descriptionFont(), _description), kGap, kLineHeight);
    }
    if (_indicatorIcon != ShadIconType::None)
    {
        layout.indicator = place(kIconSize, hasTitle ? kTrailingIconGap : kGap, kIconSize);
    }
    if (_hasChevron)
    {
        layout.chevron = place(kIconSize, kChevronGap, kIconSize);
    }
    layout.right = x;
    return layout;
}

ShadTraceLinePrivate::Layout ShadTraceLinePrivate::computeLayout() const
{
    Q_Q(const ShadTraceLine);
    const qreal margin = ShadStyleHelper::kFocusRingWidth;
    const qreal available = qMax<qreal>(0.0, q->width() - 2.0 * margin);
    // TraceTitle 为 min-w-0 truncate，空间不够时只压缩标题，描述与图标保持完整
    const qreal titleNatural = ShadStyleHelper::textWidth(titleFont(), _title);
    const qreal overflow = qMax<qreal>(0.0, contentWidth(titleNatural) - available);
    const qreal titleWidth = qMax<qreal>(0.0, titleNatural - overflow);
    return arrange(margin, q->height() / 2.0, titleWidth);
}

qreal ShadTraceLinePrivate::contentWidth(qreal titleWidth) const
{
    return arrange(0.0, 0.0, titleWidth).right;
}

QFont ShadTraceLinePrivate::titleFont() const
{
    Q_Q(const ShadTraceLine);
    QFont font = q->font();
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Normal);
    return font;
}

QFont ShadTraceLinePrivate::descriptionFont() const
{
    QFont font = titleFont();
    if (_isDescriptionMonospace)
    {
        // font-mono：ui-monospace, SFMono-Regular, Menlo, Monaco, Consolas, "Liberation Mono", "Courier New", monospace
        font.setFamilies(ShadStyleHelper::monospaceFontFamilies());
        font.setStyleHint(QFont::Monospace);
        font.setFixedPitch(true);
    }
    return font;
}

QColor ShadTraceLinePrivate::lineColor() const
{
    // data-[status=error]:text-destructive，悬停时保持 destructive
    if (_status == ShadTraceType::Error)
    {
        return shadTheme->getColor(ShadThemeType::Destructive);
    }
    // text-muted-foreground，可点击时 hover:text-foreground（transition-colors）
    return ShadStyleHelper::mix(shadTheme->getColor(ShadThemeType::MutedForeground), shadTheme->getColor(ShadThemeType::Foreground), _hoverTransition->value());
}

void ShadTraceLinePrivate::updateAnimations()
{
    Q_Q(ShadTraceLine);
    const bool isShown = q->isVisible();
    const bool shouldSpin = isShown && _icon == ShadIconType::LoaderCircle;
    if (shouldSpin && _spinAnimation->state() != QAbstractAnimation::Running)
    {
        _spinAnimation->start();
    }
    else if (!shouldSpin && _spinAnimation->state() == QAbstractAnimation::Running)
    {
        _spinAnimation->stop();
        _spinAngle = 0.0;
    }
    // data-[status=running]：标题扫光
    const bool shouldShimmer = isShown && _status == ShadTraceType::Running && !_title.isEmpty();
    if (shouldShimmer && _shimmerAnimation->state() != QAbstractAnimation::Running)
    {
        _shimmerAnimation->start();
    }
    else if (!shouldShimmer && _shimmerAnimation->state() == QAbstractAnimation::Running)
    {
        _shimmerAnimation->stop();
        _shimmerProgress = 0.0;
    }
}

void ShadTraceLinePrivate::activate()
{
    Q_Q(ShadTraceLine);
    if (!_url.isEmpty())
    {
        QDesktopServices::openUrl(_url);
    }
    Q_EMIT q->clicked();
}

void ShadTraceLinePrivate::setChevronVisible(bool visible)
{
    Q_Q(ShadTraceLine);
    if (_hasChevron == visible)
    {
        return;
    }
    _hasChevron = visible;
    q->updateGeometry();
    q->update();
}

void ShadTraceLinePrivate::setChevronProgress(qreal progress)
{
    Q_Q(ShadTraceLine);
    _chevronProgress = progress;
    q->update();
}

ShadTraceCollapsiblePrivate::ShadTraceCollapsiblePrivate()
{
}

ShadTraceCollapsiblePrivate::~ShadTraceCollapsiblePrivate()
{
}

void ShadTraceCollapsiblePrivate::createTrigger(ShadIconType::IconName icon, const QString& title)
{
    Q_Q(ShadTraceCollapsible);
    _trigger = new ShadTraceLine(icon, title, q);
    _trigger->setClickable(true);
    _trigger->d_func()->setChevronVisible(true);
}

void ShadTraceCollapsiblePrivate::applyContentMargins()
{
    Q_Q(ShadTraceCollapsible);
    using ShadStyleHelper::kFocusRingWidth;
    // 详情区内的组件四周预留了焦点环边距，扣除后组件的可视边缘与 pl-4 / pr-1 / mt-2 / mb-1 对齐
    // 起始侧：触发行的焦点环边距 + ml + 竖线 + pl-4，再扣除组件自身的焦点环边距
    const int start = kFocusRingWidth + kContentIndent + kBorderWidth + kContentPaddingStart - kFocusRingWidth;
    const int end = kContentPaddingEnd - kFocusRingWidth;
    // 顶部：触发行底部的焦点环边距与组件顶部的焦点环边距都计入 mt-2
    const int top = kContentTop - 2 * kFocusRingWidth;
    const int bottom = kContentBottom - kFocusRingWidth;
    const bool isRightToLeft = q->layoutDirection() == Qt::RightToLeft;
    _contentLayout->setContentsMargins(isRightToLeft ? end : start, top, isRightToLeft ? start : end, bottom);
}

int ShadTraceCollapsiblePrivate::contentHeight(int width) const
{
    // 详情区中有自动换行的文字时，高度随宽度变化
    if (_contentLayout->hasHeightForWidth())
    {
        return _contentLayout->heightForWidth(width);
    }
    return _contentLayout->sizeHint().height();
}

void ShadTraceCollapsiblePrivate::updateLayout()
{
    Q_Q(ShadTraceCollapsible);
    if (_isUpdatingLayout)
    {
        return;
    }
    _isUpdatingLayout = true;
    const int width = q->width();
    // 触发行按内容宽度显示（w-fit max-w-full），位于起始侧
    const QSize triggerHint = _trigger->sizeHint();
    const int triggerWidth = qMin(triggerHint.width(), width);
    const int triggerHeight = triggerHint.height();
    _trigger->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), QRect(0, 0, triggerWidth, triggerHeight)));
    // 详情区始终按完整高度排列，组件高度随展开进度变化，超出部分被裁剪（overflow-hidden）
    const int fullHeight = contentHeight(width);
    _content->setGeometry(0, triggerHeight, width, fullHeight);
    _preferredHeight = triggerHeight + qRound(fullHeight * _progress);
    _isUpdatingLayout = false;
    const QSize hint = q->sizeHint();
    if (hint != _lastSizeHint)
    {
        _lastSizeHint = hint;
        q->updateGeometry();
    }
    q->update();
}

void ShadTraceCollapsiblePrivate::setProgress(qreal progress)
{
    _progress = progress;
    _trigger->d_func()->setChevronProgress(progress);
    updateLayout();
}

ShadTracePrivate::ShadTracePrivate()
{
}

ShadTracePrivate::~ShadTracePrivate()
{
}
