#include "ShadScrollAreaPrivate.h"

#include <QObject>
#include <QRect>
#include <QScrollBar>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QWidget>

#include "ShadScrollArea.h"
#include "ShadScrollBar.h"

ShadScrollAreaPrivate::ShadScrollAreaPrivate()
{
}

ShadScrollAreaPrivate::~ShadScrollAreaPrivate()
{
}

void ShadScrollAreaPrivate::bindScrollBar(QScrollBar* source, ShadScrollBar* overlay)
{
    syncScrollBar(source, overlay);
    // QScrollArea 先 setRange 再 setPageStep，rangeChanged 时 pageStep 可能还是旧值，排队到本轮更新结束后再同步
    QObject::connect(source, &QScrollBar::rangeChanged, overlay, [this, source, overlay]()
    {
        syncScrollBar(source, overlay);
        updateOverlayBars();
    }, Qt::QueuedConnection);
    QObject::connect(source, &QScrollBar::valueChanged, overlay, [overlay](int value)
    {
        const QSignalBlocker blocker(overlay);
        overlay->setValue(value);
        overlay->update();
    });
    QObject::connect(overlay, &QScrollBar::valueChanged, source, &QScrollBar::setValue);
}

void ShadScrollAreaPrivate::syncScrollBar(QScrollBar* source, ShadScrollBar* overlay)
{
    const QSignalBlocker blocker(overlay);
    overlay->setRange(source->minimum(), source->maximum());
    overlay->setPageStep(source->pageStep());
    overlay->setSingleStep(source->singleStep());
    overlay->setValue(source->value());
    overlay->update();
}

void ShadScrollAreaPrivate::updateOverlayBars()
{
    Q_Q(ShadScrollArea);
    const QRect viewportRect = q->viewport()->geometry();
    const bool isShowing = _isHovered || _hideTimer->isActive();
    const bool showVertical = isShowing && _verticalBar->maximum() > _verticalBar->minimum();
    const bool showHorizontal = isShowing && _horizontalBar->maximum() > _horizontalBar->minimum();
    const int verticalThickness = _verticalBar->sizeHint().width();
    const int horizontalThickness = _horizontalBar->sizeHint().height();

    // 两个方向同时显示时，垂直滚动条给水平滚动条让出右下角
    const QRect verticalRect(viewportRect.right() - verticalThickness + 1, viewportRect.top(), verticalThickness, viewportRect.height() - (showHorizontal ? horizontalThickness : 0));
    const QRect horizontalRect(viewportRect.left(), viewportRect.bottom() - horizontalThickness + 1, viewportRect.width() - (showVertical ? verticalThickness : 0), horizontalThickness);
    // 从右到左布局时垂直滚动条位于左侧
    _verticalBar->setGeometry(QStyle::visualRect(q->layoutDirection(), viewportRect, verticalRect));
    _horizontalBar->setGeometry(QStyle::visualRect(q->layoutDirection(), viewportRect, horizontalRect));
    _verticalBar->setVisible(showVertical);
    _horizontalBar->setVisible(showHorizontal);
    _verticalBar->raise();
    _horizontalBar->raise();
}
