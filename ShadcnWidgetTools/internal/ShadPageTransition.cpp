#include "ShadPageTransition.h"

#include <QEasingCurve>
#include <QLayout>
#include <QPaintEvent>
#include <QPainter>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadFrameAnimation.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kSlideDistance = 12;
} // namespace

ShadPageTransition::ShadPageTransition(QWidget* area)
    : QWidget(area)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setFocusPolicy(Qt::NoFocus);
    hide();
    _animation = new ShadFrameAnimation(this);
    _animation->setStartValue(0.0);
    _animation->setEndValue(1.0);
    _animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(_animation, &ShadFrameAnimation::valueChanged, this, [this](qreal value) { applyProgress(value); });
    connect(_animation, &ShadFrameAnimation::finished, this, [this]() { finish(); });
}

ShadPageTransition::~ShadPageTransition()
{
}

void ShadPageTransition::capture(QWidget* page)
{
    finish();
    QWidget* area = parentWidget();
    if (!shadTheme->isPageTransitionEnabled() || shadTheme->getPageTransitionDuration() <= 0 || page == nullptr || area == nullptr || !area->isVisible())
    {
        return;
    }
    _snapshot = ShadStyleHelper::grabContent(page);
    _snapshotRect = page->geometry();
}

void ShadPageTransition::play(QWidget* page, int direction)
{
    QWidget* area = parentWidget();
    if (_snapshot.isNull() || page == nullptr || area == nullptr || !area->isVisible())
    {
        finish();
        return;
    }
    // 新显示的页面先排布
    if (area->layout() != nullptr)
    {
        area->layout()->activate();
    }
    _page = page;
    _pageBase = page->pos();
    _offset = 0;
    if (shadTheme->getPageTransitionStyle() == ShadThemeType::Slide)
    {
        const int sign = (direction >= 0) == (layoutDirection() == Qt::LeftToRight) ? 1 : -1;
        _offset = sign * kSlideDistance;
    }
    setGeometry(area->rect());
    raise();
    show();
    applyProgress(0.0);
    _animation->setDuration(shadTheme->getPageTransitionDuration());
    _animation->start();
}

void ShadPageTransition::finish()
{
    _animation->stop();
    if (!_page.isNull())
    {
        _page->move(_pageBase);
    }
    _page = nullptr;
    _snapshot = QPixmap();
    _progress = 1.0;
    hide();
}

void ShadPageTransition::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (_snapshot.isNull())
    {
        return;
    }
    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setOpacity(1.0 - _progress);
    painter.drawPixmap(_snapshotRect.translated(-qRound(_offset * _progress), 0), _snapshot);
}

void ShadPageTransition::applyProgress(qreal progress)
{
    _progress = progress;
    if (QWidget* area = parentWidget())
    {
        setGeometry(area->rect());
    }
    if (!_page.isNull() && _offset != 0)
    {
        _page->move(_pageBase + QPoint(qRound(_offset * (1.0 - progress)), 0));
    }
    update();
}
