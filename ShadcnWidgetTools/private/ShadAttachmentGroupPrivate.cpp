#include "ShadAttachmentGroupPrivate.h"

#include <QApplication>
#include <QBrush>
#include <QColor>
#include <QGraphicsOpacityEffect>
#include <QHBoxLayout>
#include <QLinearGradient>
#include <QPoint>
#include <QRect>
#include <QScrollArea>
#include <QScrollBar>
#include <QSize>
#include <QTimer>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QWidget>
#include <QtGlobal>

#include <cstdlib>

#include "ShadAttachment.h"
#include "ShadAttachmentGroup.h"
#include "ShadStyleHelper.h"

namespace
{
    // 渐隐宽度 min(12%, spacing * 10)，滚动 spacing * 24 的距离内逐渐出现
    constexpr qreal kFadeRatio = 0.12;
    constexpr int kFadeMaxSize = 40;
    constexpr int kFadeRevealDistance = 96;
    // 滚轮一格的角度增量
    constexpr int kWheelAngleStep = 120;
} // namespace

ShadAttachmentGroupPrivate::ShadAttachmentGroupPrivate()
{
}

ShadAttachmentGroupPrivate::~ShadAttachmentGroupPrivate()
{
}

void ShadAttachmentGroupPrivate::pruneAttachments()
{
    _attachments.removeIf([](const QPointer<ShadAttachment>& attachment) { return attachment.isNull(); });
}

void ShadAttachmentGroupPrivate::updateContentGeometry()
{
    Q_Q(ShadAttachmentGroup);
    // 宽高都取布局的理想尺寸：滚动区域首次显示前视口还没按分组尺寸布局，视口高度是 QScrollArea 的初始值，不能作为内容高度
    const QSize size = _layout->sizeHint();
    if (_content->size() != size)
    {
        _content->resize(size);
    }
    q->updateGeometry();
    updateFade();
}

int ShadAttachmentGroupPrivate::logicalLeft(const QWidget* widget) const
{
    const QRect geometry = widget->geometry();
    if (_content->layoutDirection() == Qt::RightToLeft)
    {
        return _content->width() - (geometry.x() + geometry.width());
    }
    return geometry.x();
}

void ShadAttachmentGroupPrivate::snapToNearest()
{
    QScrollBar* bar = _scrollArea->horizontalScrollBar();
    const int maximum = bar->maximum();
    if (maximum <= 0)
    {
        return;
    }
    const int current = bar->value();
    // 吸附点：每个附件卡片的起始边（扣除焦点环边距后再留出 scroll-px-1），以及两端
    int target = 0;
    int bestDistance = std::abs(current);
    const auto consider = [&](int candidate)
    {
        const int clamped = qBound(0, candidate, maximum);
        const int distance = std::abs(clamped - current);
        if (distance < bestDistance)
        {
            bestDistance = distance;
            target = clamped;
        }
    };
    consider(maximum);
    for (const QPointer<ShadAttachment>& attachment : _attachments)
    {
        if (!attachment.isNull() && !attachment->isHidden())
        {
            consider(logicalLeft(attachment) + ShadStyleHelper::kFocusRingWidth - kScrollPadding);
        }
    }
    if (target != current)
    {
        animateTo(target);
    }
}

void ShadAttachmentGroupPrivate::animateTo(int value)
{
    QScrollBar* bar = _scrollArea->horizontalScrollBar();
    _snapAnimation->stop();
    _snapAnimation->setStartValue(bar->value());
    _snapAnimation->setEndValue(value);
    _snapAnimation->start();
}

void ShadAttachmentGroupPrivate::updateFade()
{
    QScrollBar* bar = _scrollArea->horizontalScrollBar();
    const int width = _scrollArea->viewport()->width();
    const int maximum = bar->maximum();
    if (maximum <= 0 || width <= 0)
    {
        _fadeEffect->setEnabled(false);
        return;
    }
    // 起始侧随已滚动距离出现，结束侧随剩余距离消失
    const qreal size = qMin<qreal>(width * kFadeRatio, kFadeMaxSize);
    const qreal startFade = size * qBound<qreal>(0.0, bar->value() / qreal(kFadeRevealDistance), 1.0);
    const qreal endFade = size * qBound<qreal>(0.0, (maximum - bar->value()) / qreal(kFadeRevealDistance), 1.0);
    if (startFade <= 0.0 && endFade <= 0.0)
    {
        _fadeEffect->setEnabled(false);
        return;
    }
    // 遮罩为视口坐标系下的物理方向，RTL 时起始侧在右
    const bool isRightToLeft = _content->layoutDirection() == Qt::RightToLeft;
    const qreal leftFade = isRightToLeft ? endFade : startFade;
    const qreal rightFade = isRightToLeft ? startFade : endFade;
    QLinearGradient gradient(0.0, 0.0, width, 0.0);
    const QColor transparent(0, 0, 0, 0);
    const QColor opaque(0, 0, 0, 255);
    gradient.setColorAt(0.0, leftFade > 0.0 ? transparent : opaque);
    gradient.setColorAt(qBound<qreal>(0.0, leftFade / width, 0.5), opaque);
    gradient.setColorAt(qBound<qreal>(0.5, 1.0 - rightFade / width, 1.0), opaque);
    gradient.setColorAt(1.0, rightFade > 0.0 ? transparent : opaque);
    _fadeEffect->setOpacityMask(QBrush(gradient));
    _fadeEffect->setEnabled(true);
}

bool ShadAttachmentGroupPrivate::handleWheel(QWheelEvent* event)
{
    // 横向滚动（触控板左右滑动、Shift + 滚轮）交给 QScrollArea 默认处理
    const QPoint angleDelta = event->angleDelta();
    const QPoint pixelDelta = event->pixelDelta();
    const int deltaX = pixelDelta.isNull() ? angleDelta.x() : pixelDelta.x();
    const int deltaY = pixelDelta.isNull() ? angleDelta.y() : pixelDelta.y();
    if (deltaY == 0 || std::abs(deltaX) >= std::abs(deltaY))
    {
        _snapAnimation->stop();
        return false;
    }
    QScrollBar* bar = _scrollArea->horizontalScrollBar();
    if (bar->maximum() <= 0)
    {
        return false;
    }
    // 向上滚动朝起始侧移动；已经到头时交给父组件，页面可以继续竖向滚动
    qreal step = deltaY;
    if (pixelDelta.isNull())
    {
        step = qreal(deltaY) / kWheelAngleStep * bar->singleStep() * QApplication::wheelScrollLines();
    }
    const bool isTowardStart = step > 0;
    if ((isTowardStart && bar->value() <= 0) || (!isTowardStart && bar->value() >= bar->maximum()))
    {
        return false;
    }
    _snapAnimation->stop();
    bar->setValue(bar->value() - qRound(step));
    event->accept();
    return true;
}
