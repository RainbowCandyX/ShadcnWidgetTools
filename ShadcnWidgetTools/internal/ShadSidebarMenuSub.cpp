#include "ShadSidebarMenuSub.h"

#include <QEasingCurve>
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadFrameAnimation.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kLineOffset = 14;
    constexpr int kLineWidth = 1;
    constexpr int kInnerPadding = 10;
    constexpr int kOuterMarginEnd = 14;
    constexpr int kPaddingY = 2;
    constexpr int kItemGap = 4;
    constexpr int kAnimationDurationMs = 200;
} // namespace

ShadSidebarMenuSub::ShadSidebarMenuSub(QWidget* parent)
    : ShadSidebarCollapseArea(parent)
{
    _layout = new QVBoxLayout(contentWidget());
    _layout->setSpacing(kItemGap);
    applyMargins();
    _animation = new ShadFrameAnimation(this);
    _animation->setDuration(kAnimationDurationMs);
    _animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(_animation, &ShadFrameAnimation::valueChanged, this, [this](qreal value) { applyProgress(value); });
    connect(_animation, &ShadFrameAnimation::finished, this, [this]()
    {
        if (!_isExpanded)
        {
            hide();
        }
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
    applyProgress(0.0);
    hide();
}

ShadSidebarMenuSub::~ShadSidebarMenuSub()
{
}

QVBoxLayout* ShadSidebarMenuSub::itemLayout() const
{
    return _layout;
}

void ShadSidebarMenuSub::setExpanded(bool expanded, bool animated)
{
    _isExpanded = expanded;
    _animation->stop();
    if (expanded)
    {
        show();
    }
    if (!animated)
    {
        applyProgress(expanded ? 1.0 : 0.0);
        setVisible(expanded);
        return;
    }
    _animation->setStartValue(_expandProgress);
    _animation->setEndValue(expanded ? 1.0 : 0.0);
    _animation->start();
}

bool ShadSidebarMenuSub::isExpanded() const
{
    return _isExpanded;
}

qreal ShadSidebarMenuSub::contentOpacity() const
{
    return _isExpanded ? qBound(0.0, _expandProgress * 2.0 - 1.0, 1.0) : 0.0;
}

void ShadSidebarMenuSub::setIconProgress(qreal progress)
{
    if (qFuzzyCompare(_iconProgress, progress))
    {
        return;
    }
    _iconProgress = progress;
    setProgress(_expandProgress * _iconProgress);
}

qreal ShadSidebarMenuSub::expandProgress() const
{
    return _expandProgress;
}

bool ShadSidebarMenuSub::event(QEvent* event)
{
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        applyMargins();
    }
    return ShadSidebarCollapseArea::event(event);
}

void ShadSidebarMenuSub::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(this);
    const QRect lineRect(kLineOffset, 0, kLineWidth, height());
    painter.fillRect(ShadStyleHelper::visualRect(layoutDirection(), rect(), lineRect), shadTheme->getColor(ShadThemeType::SidebarBorder));
}

void ShadSidebarMenuSub::applyMargins()
{
    // contentsMargins 是物理方向，RTL 时起始侧在右边
    const int start = kLineOffset + kLineWidth + kInnerPadding;
    const int end = kOuterMarginEnd + kInnerPadding;
    if (layoutDirection() == Qt::RightToLeft)
    {
        _layout->setContentsMargins(end, kPaddingY, start, kPaddingY);
    }
    else
    {
        _layout->setContentsMargins(start, kPaddingY, end, kPaddingY);
    }
}

void ShadSidebarMenuSub::applyProgress(qreal progress)
{
    _expandProgress = progress;
    setProgress(_expandProgress * _iconProgress);
    if (QWidget* content = contentWidget())
    {
        content->update();
    }
    Q_EMIT progressChanged(progress);
}
