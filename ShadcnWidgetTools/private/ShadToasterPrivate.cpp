#include "ShadToasterPrivate.h"

#include <QPoint>
#include <QRegion>
#include <QVariant>
#include <QtGlobal>

#include <limits>

#include "ShadStyleHelper.h"
#include "ShadToastItem.h"
#include "ShadToaster.h"

ShadToasterPrivate::ShadToasterPrivate()
{
}

ShadToasterPrivate::~ShadToasterPrivate()
{
}

bool ShadToasterPrivate::isBottom() const
{
    return _position == ShadToastType::BottomLeft || _position == ShadToastType::BottomCenter || _position == ShadToastType::BottomRight;
}

int ShadToasterPrivate::cardWidth() const
{
    if (_host.isNull())
    {
        return 0;
    }
    return qBound(0, _host->width() - 2 * kViewportOffset, kMaxCardWidth);
}

int ShadToasterPrivate::titleBarInset() const
{
    if (_host.isNull())
    {
        return 0;
    }
    // 按 titleBarHeight 属性读取，不依赖具体类型
    const QVariant value = _host->property("titleBarHeight");
    return value.isValid() ? qMax(0, value.toInt()) : 0;
}

QRect ShadToasterPrivate::toasterGeometry() const
{
    if (_host.isNull())
    {
        return QRect();
    }
    const int hostWidth = _host->width();
    const int width = cardWidth() + 2 * kShadowMargin;
    // Left / Right 为起始侧与结束侧，x 按 LTR 计算，RTL 时在宿主内镜像
    int x = 0;
    switch (_position)
    {
        case ShadToastType::TopLeft:
        case ShadToastType::BottomLeft:
            x = kViewportOffset - kShadowMargin;
            break;
        case ShadToastType::TopCenter:
        case ShadToastType::BottomCenter:
            x = (hostWidth - width) / 2;
            break;
        case ShadToastType::TopRight:
        case ShadToastType::BottomRight:
            x = hostWidth - kViewportOffset - cardWidth() - kShadowMargin;
            break;
    }
    const int top = qMin(titleBarInset(), _host->height());
    return ShadStyleHelper::visualRect(_host->layoutDirection(), _host->rect(), QRect(x, top, width, _host->height() - top));
}

QRect ShadToasterPrivate::offscreenRect(const QRect& rect) const
{
    const int distance = qRound(rect.height() * 1.5);
    return rect.translated(0, isBottom() ? distance : -distance);
}

QRect ShadToasterPrivate::exitRect(const QRect& rect) const
{
    Q_Q(const ShadToaster);
    const int height = rect.height();
    const int distance = qRound(height * 1.5);
    if (isBottom())
    {
        // 锚点处卡片下边缘距窗口底部 16px，下移 1.5 倍高度后上边缘已在窗口外
        const int anchor = q->height() - kViewportOffset;
        return QRect(rect.x(), anchor - height + distance, rect.width(), height);
    }
    return QRect(rect.x(), kViewportOffset - distance, rect.width(), height);
}

ShadToastItem* ShadToasterPrivate::findItem(int id) const
{
    for (ShadToastItem* item : _items)
    {
        if (item->toastId() == id)
        {
            return item;
        }
    }
    return nullptr;
}

int ShadToasterPrivate::effectiveLimit() const
{
    return _limit > 0 ? _limit : std::numeric_limits<int>::max();
}

void ShadToasterPrivate::relayout(bool animate)
{
    Q_Q(ShadToaster);
    if (_host.isNull())
    {
        return;
    }
    const QRect newGeometry = toasterGeometry();
    const QRect oldGeometry = q->geometry();
    if (oldGeometry != newGeometry)
    {
        q->setGeometry(newGeometry);
        // 切换位置时先保持 toast 在窗口中的视觉位置不变，再从原处动画移动到新位置
        const QPoint delta = oldGeometry.topLeft() - newGeometry.topLeft();
        if (animate && !delta.isNull())
        {
            for (ShadToastItem* item : _items)
            {
                if (item->isPlaced())
                {
                    item->moveTo(item->geometry().translated(delta), false);
                }
            }
        }
    }

    const int width = cardWidth();
    const int left = kShadowMargin;
    const bool isBottomPosition = isBottom();
    // 底部时以卡片下边缘对齐，顶部时以卡片上边缘对齐
    const int anchor = isBottomPosition ? newGeometry.height() - kViewportOffset : kViewportOffset;
    QList<int> heights;
    for (ShadToastItem* item : _items)
    {
        heights.append(item->naturalHeight(width));
    }
    const int frontHeight = heights.isEmpty() ? 0 : heights.first();
    int stackedOffset = 0;
    for (int i = 0; i < _items.size(); ++i)
    {
        ShadToastItem* item = _items.at(i);
        const bool isContentVisible = _isExpanded || i == 0;
        QRect target;
        if (isContentVisible)
        {
            // 展开或最前面的 toast：完整高度，按顺序依次排开
            const int height = heights.at(i);
            const int top = isBottomPosition ? anchor - stackedOffset - height : anchor + stackedOffset;
            target = QRect(left, top, width, height);
            stackedOffset += height + kGap;
        }
        else
        {
            // 收起时后面的 toast：与最前面同高后按层级缩小，并向外露出 peek；超出可见层数的与最后一层重叠
            const int layer = qMin(i, kVisibleStackCount - 1);
            const qreal scale = qMax<qreal>(0.0, 1.0 - kScaleStep * layer);
            const int scaledWidth = qRound(width * scale);
            const int scaledHeight = qRound(frontHeight * scale);
            const int x = left + (width - scaledWidth) / 2;
            const int top = isBottomPosition ? anchor - frontHeight - kPeek * layer : anchor + frontHeight + kPeek * layer - scaledHeight;
            target = QRect(x, top, scaledWidth, scaledHeight);
        }
        item->setContentVisible(isContentVisible);
        if (!item->isPlaced())
        {
            // 新 toast 从堆叠方向的窗口外滑入
            item->moveTo(animate ? offscreenRect(target) : target, false);
            item->show();
        }
        item->moveTo(target, animate);
        // 收起时隐藏超出可见层数的 toast，展开后恢复
        item->setVisible(_isExpanded || i < kVisibleStackCount);
    }
    // 越靠前的 toast 层级越高，正在离场的保持在最上层
    for (int i = static_cast<int>(_items.size()) - 1; i >= 0; --i)
    {
        _items.at(i)->raise();
    }
    for (ShadToastItem* item : _exitingItems)
    {
        item->raise();
    }
    updateMask();
    q->update();
}

void ShadToasterPrivate::updateMask()
{
    Q_Q(ShadToaster);
    const bool hasToast = !_items.isEmpty() || !_exitingItems.isEmpty();
    if (!hasToast)
    {
        // 空遮罩等同于不限制鼠标区域，没有 toast 时直接隐藏，避免挡住宿主窗口
        q->hide();
        return;
    }
    QRegion region;
    for (ShadToastItem* item : _items)
    {
        if (!item->isHidden())
        {
            region += item->geometry().adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin);
        }
    }
    for (ShadToastItem* item : _exitingItems)
    {
        if (!item->isHidden())
        {
            region += item->geometry().adjusted(-kShadowMargin, -kShadowMargin, kShadowMargin, kShadowMargin);
        }
    }
    region &= QRegion(q->rect());
    if (region.isEmpty())
    {
        q->hide();
        return;
    }
    q->setMask(region);
    q->show();
}

void ShadToasterPrivate::setExpanded(bool expanded)
{
    if (_isExpanded == expanded)
    {
        return;
    }
    _isExpanded = expanded;
    relayout(true);
}

void ShadToasterPrivate::beginDismiss(ShadToastItem* item)
{
    Q_Q(ShadToaster);
    if (item == nullptr || !_items.removeOne(item))
    {
        return;
    }
    const int id = item->toastId();
    _exitingItems.append(item);
    item->startExit(exitRect(item->geometry()));
    Q_EMIT q->toastClosed(id);
    relayout(true);
}

void ShadToasterPrivate::onExitFinished(ShadToastItem* item)
{
    Q_Q(ShadToaster);
    if (!_exitingItems.removeOne(item))
    {
        return;
    }
    item->hide();
    item->deleteLater();
    if (_items.isEmpty() && _exitingItems.isEmpty())
    {
        // 全部关闭后隐藏，鼠标不会再触发离开事件，这里直接复位展开状态
        _isExpanded = false;
    }
    updateMask();
    q->update();
}
