#include "ShadCodeBlockPrivate.h"

#include <QAbstractAnimation>
#include <QBrush>
#include <QClipboard>
#include <QColor>
#include <QGraphicsOpacityEffect>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QImage>
#include <QLinearGradient>
#include <QPaintEvent>
#include <QPainter>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QScrollBar>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QtGlobal>

#include "ShadButton.h"
#include "ShadCodeBlock.h"
#include "ShadCodeBlockView.h"
#include "ShadScrollArea.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"

namespace
{
    // 渐隐宽度 min(12%, 40px)，滚动 96px 的距离内逐渐出现，与 ShadAttachmentGroup 的 scroll-fade 一致
    constexpr qreal kFadeRatio = 0.12;
    constexpr int kFadeMaxSize = 40;
    constexpr int kFadeRevealDistance = 96;
    // ring-1 ring-foreground/10
    constexpr qreal kFloatingRingWidth = 1.0;
    constexpr qreal kFloatingRingAlpha = 0.1;

    // 一侧的渐隐宽度：distance 为这一侧还能滚动的距离
    qreal fadeSize(int extent, int distance)
    {
        if (distance <= 0 || extent <= 0)
        {
            return 0.0;
        }
        const qreal size = qMin<qreal>(extent * kFadeRatio, kFadeMaxSize);
        return size * qBound<qreal>(0.0, distance / qreal(kFadeRevealDistance), 1.0);
    }

    // 从 from 到 to 的遮罩渐变：两端按渐隐宽度从透明过渡到不透明
    QLinearGradient fadeGradient(const QPointF& from, const QPointF& to, qreal extent, qreal startFade, qreal endFade)
    {
        QLinearGradient gradient(from, to);
        const QColor transparent(0, 0, 0, 0);
        const QColor opaque(0, 0, 0, 255);
        gradient.setColorAt(0.0, startFade > 0.0 ? transparent : opaque);
        gradient.setColorAt(qBound<qreal>(0.0, startFade / extent, 0.5), opaque);
        gradient.setColorAt(qBound<qreal>(0.5, 1.0 - endFade / extent, 1.0), opaque);
        gradient.setColorAt(1.0, endFade > 0.0 ? transparent : opaque);
        return gradient;
    }

    // 组件被显式隐藏（调用过 hide 或 setVisible(false)）；尚未显示过的子组件不算隐藏
    bool isExplicitlyHidden(const QWidget* widget)
    {
        return widget->testAttribute(Qt::WA_WState_ExplicitShowHide) && widget->testAttribute(Qt::WA_WState_Hidden);
    }
} // namespace

ShadCodeBlockActions::ShadCodeBlockActions(QWidget* parent)
    : QWidget(parent)
{
}

ShadCodeBlockActions::~ShadCodeBlockActions()
{
}

void ShadCodeBlockActions::setFloating(bool floating)
{
    if (_isFloating == floating)
    {
        return;
    }
    _isFloating = floating;
    update();
}

bool ShadCodeBlockActions::isFloating() const
{
    return _isFloating;
}

void ShadCodeBlockActions::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (!_isFloating)
    {
        return;
    }
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor background = shadTheme->getColor(ShadThemeType::Background);
    const QColor ring = withAlpha(shadTheme->getColor(ShadThemeType::Foreground), kFloatingRingAlpha);
    const QList<QWidget*> children = findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
    for (QWidget* child : children)
    {
        if (!child->isVisible())
        {
            continue;
        }
        // 子组件四周预留了焦点环边距，按钮的可视区域在内侧
        const QRectF buttonRect = insetRect(child->geometry(), kFocusRingWidth);
        fillRoundedRect(&painter, buttonRect, radius, background);
        drawFocusRing(&painter, buttonRect, radius, ring, kFloatingRingWidth);
    }
}

ShadCodeBlockPrivate::ShadCodeBlockPrivate()
{
}

ShadCodeBlockPrivate::~ShadCodeBlockPrivate()
{
}

ShadCodeBlockPrivate::Metrics ShadCodeBlockPrivate::metrics() const
{
    if (_size == ShadCodeBlockType::Small)
    {
        return {12, 12, 6, 8, 16, 12, 14, 12, 8};
    }
    return {16, 16, 8, 8, 20, 14, 16, 14, 12};
}

bool ShadCodeBlockPrivate::hasHeader() const
{
    return !_title.isEmpty() || _titleIcon != ShadIconType::None;
}

bool ShadCodeBlockPrivate::hasFooter() const
{
    return _footerLayout != nullptr && _footerLayout->count() > 0;
}

QList<QWidget*> ShadCodeBlockPrivate::actionWidgets() const
{
    QList<QWidget*> widgets;
    for (const QPointer<QWidget>& widget : _actionWidgets)
    {
        if (!widget.isNull() && widget->parentWidget() == _actions && !isExplicitlyHidden(widget))
        {
            widgets.append(widget.data());
        }
    }
    if (_isCopyButtonVisible)
    {
        widgets.append(_copyButton);
    }
    return widgets;
}

void ShadCodeBlockPrivate::updateLayout()
{
    Q_Q(ShadCodeBlock);
    // 代码区重新排版时会发出 contentSizeChanged，避免在排列过程中重入
    if (_isUpdatingLayout)
    {
        return;
    }
    _isUpdatingLayout = true;
    using ShadStyleHelper::kFocusRingWidth;
    const Metrics values = metrics();
    const int frame = kFocusRingWidth + kBorderWidth;
    const QRect innerRect = q->rect().adjusted(frame, frame, -frame, -frame);
    const int innerWidth = qMax(0, innerRect.width());
    const bool isHeader = hasHeader();
    const bool isFooter = hasFooter();

    // 代码区的内边距随内容滚动：有标题栏时顶部为 pt-2，有底栏时底部为 pb-2
    _view->setPadding(values.paddingX, isHeader ? values.headerGap : values.paddingY, isFooter ? values.footerGap : values.paddingY);
    _view->setViewportWidth(innerWidth);

    int y = frame;
    QRect headerRect;
    if (isHeader)
    {
        headerRect = QRect(innerRect.left() + values.paddingX, frame + values.paddingY, qMax(0, innerWidth - 2 * values.paddingX), values.titleHeight);
        y = headerRect.bottom() + 1;
    }
    // 代码区占满边框内侧的宽度，行背景横跨左右内边距
    const int contentHeight = _view->contentHeight();
    const int visibleHeight = _maximumContentHeight > 0 ? qMin(contentHeight, _maximumContentHeight) : contentHeight;
    _scrollArea->setGeometry(innerRect.left(), y, innerWidth, visibleHeight);
    y += visibleHeight;

    if (isFooter)
    {
        // 底栏中的组件四周预留了焦点环边距，底栏向外扩展同样的距离，使组件的可视边缘与内边距对齐
        const int footerHeight = _footerLayout->sizeHint().height();
        const QRect footerRect(innerRect.left() + values.paddingX - kFocusRingWidth, y - kFocusRingWidth, qMax(0, innerWidth - 2 * values.paddingX + 2 * kFocusRingWidth), footerHeight);
        _footer->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), footerRect));
        _footer->show();
        y = footerRect.bottom() + 1 - kFocusRingWidth + values.paddingY;
    }
    else if (_footer != nullptr)
    {
        _footer->hide();
    }
    _preferredHeight = y + frame;

    layoutActions(values, innerRect, isHeader);
    // 标题占据标题栏中操作区之外的部分
    _titleRect = QRect();
    if (isHeader)
    {
        int titleRight = headerRect.right() + 1;
        if (_actions->isVisibleTo(q))
        {
            const QRect actionsRect = ShadStyleHelper::visualRect(q->layoutDirection(), q->rect(), _actions->geometry());
            titleRight = qMin(titleRight, actionsRect.left() + kFocusRingWidth - kHeaderGap);
        }
        _titleRect = QRect(headerRect.left(), headerRect.top(), qMax(0, titleRight - headerRect.left()), headerRect.height());
    }
    _isUpdatingLayout = false;
    updateFade();
    // 高度或代码的自然宽度变化时通知父布局
    const QSize hint = q->sizeHint();
    if (hint != _lastSizeHint)
    {
        _lastSizeHint = hint;
        q->updateGeometry();
    }
    q->update();
}

void ShadCodeBlockPrivate::layoutActions(const Metrics& values, const QRect& innerRect, bool isHeader)
{
    Q_Q(ShadCodeBlock);
    using ShadStyleHelper::kFocusRingWidth;
    const QList<QWidget*> widgets = actionWidgets();
    if (widgets.isEmpty())
    {
        _actions->hide();
        return;
    }
    // 按可视尺寸（扣除焦点环边距）排列，按钮之间 gap-1
    int visualWidth = 0;
    int widgetHeight = 0;
    for (QWidget* widget : widgets)
    {
        const QSize hint = widget->sizeHint();
        visualWidth += hint.width() - 2 * kFocusRingWidth;
        widgetHeight = qMax(widgetHeight, hint.height());
    }
    visualWidth += kActionGap * (int(widgets.size()) - 1);
    const int visualHeight = widgetHeight - 2 * kFocusRingWidth;

    // 先按 LTR 计算（结束侧为右侧），再按布局方向镜像
    QRect visualArea;
    if (isHeader)
    {
        const int headerTop = innerRect.top() + values.paddingY;
        const int right = innerRect.right() + 1 - values.paddingX + kActionEndOffset;
        visualArea = QRect(right - visualWidth, headerTop + (values.titleHeight - visualHeight) / 2, visualWidth, visualHeight);
    }
    else
    {
        const int right = innerRect.right() + 1 - values.floatingInset;
        visualArea = QRect(right - visualWidth, innerRect.top() + values.floatingInset, visualWidth, visualHeight);
    }
    const QRect containerRect = visualArea.adjusted(-kFocusRingWidth, -kFocusRingWidth, kFocusRingWidth, kFocusRingWidth);
    const Qt::LayoutDirection direction = q->layoutDirection();
    _actions->setGeometry(ShadStyleHelper::visualRect(direction, q->rect(), containerRect));
    const QRect localBounds(QPoint(0, 0), containerRect.size());
    int x = 0;
    for (QWidget* widget : widgets)
    {
        const QSize hint = widget->sizeHint();
        const QRect logicalRect(x, (containerRect.height() - hint.height()) / 2, hint.width(), hint.height());
        widget->setGeometry(ShadStyleHelper::visualRect(direction, localBounds, logicalRect));
        widget->show();
        x += hint.width() - 2 * kFocusRingWidth + kActionGap;
    }
    _actions->setFloating(!isHeader);
    _actions->show();
    _actions->raise();
}

void ShadCodeBlockPrivate::updateFade()
{
    QWidget* viewport = _scrollArea->viewport();
    const int width = viewport->width();
    const int height = viewport->height();
    const QScrollBar* horizontalBar = _scrollArea->horizontalScrollBar();
    const QScrollBar* verticalBar = _scrollArea->verticalScrollBar();
    // 代码区始终从左到右，起始侧固定为左侧
    const qreal leftFade = fadeSize(width, horizontalBar->value());
    const qreal rightFade = fadeSize(width, horizontalBar->maximum() - horizontalBar->value());
    const qreal topFade = fadeSize(height, verticalBar->value());
    const qreal bottomFade = fadeSize(height, verticalBar->maximum() - verticalBar->value());
    const bool isHorizontal = leftFade > 0.0 || rightFade > 0.0;
    const bool isVertical = topFade > 0.0 || bottomFade > 0.0;
    if (width <= 0 || height <= 0 || (!isHorizontal && !isVertical))
    {
        _fadeEffect->setEnabled(false);
        return;
    }
    const QLinearGradient horizontalGradient = fadeGradient(QPointF(0.0, 0.0), QPointF(width, 0.0), width, leftFade, rightFade);
    const QLinearGradient verticalGradient = fadeGradient(QPointF(0.0, 0.0), QPointF(0.0, height), height, topFade, bottomFade);
    if (isHorizontal && isVertical)
    {
        // 两个方向都可滚动时，把两个渐变相乘得到四周的遮罩
        QImage mask(viewport->size(), QImage::Format_ARGB32_Premultiplied);
        mask.fill(Qt::transparent);
        QPainter painter(&mask);
        painter.fillRect(mask.rect(), QBrush(horizontalGradient));
        painter.setCompositionMode(QPainter::CompositionMode_DestinationIn);
        painter.fillRect(mask.rect(), QBrush(verticalGradient));
        painter.end();
        _fadeEffect->setOpacityMask(QBrush(mask));
    }
    else
    {
        _fadeEffect->setOpacityMask(QBrush(isHorizontal ? horizontalGradient : verticalGradient));
    }
    _fadeEffect->setEnabled(true);
}

void ShadCodeBlockPrivate::updateActionsOpacity(bool isAnimated)
{
    // 标题栏中的操作区始终显示
    if (hasHeader())
    {
        _actionsAnimation->stop();
        _actionsEffect->setOpacity(1.0);
        _actionsEffect->setEnabled(false);
        return;
    }
    const qreal target = _isHovered || _isFocusWithin ? 1.0 : 0.0;
    _actionsEffect->setEnabled(true);
    if (!isAnimated)
    {
        _actionsAnimation->stop();
        _actionsEffect->setOpacity(target);
        return;
    }
    if (_actionsAnimation->state() == QAbstractAnimation::Running && qFuzzyCompare(_actionsAnimation->endValue().toReal(), target))
    {
        return;
    }
    _actionsAnimation->stop();
    _actionsAnimation->setStartValue(_actionsEffect->opacity());
    _actionsAnimation->setEndValue(target);
    _actionsAnimation->start();
}

void ShadCodeBlockPrivate::copyAll()
{
    Q_Q(ShadCodeBlock);
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard == nullptr)
    {
        return;
    }
    clipboard->setText(_view->plainText());
    _copyButton->setShadIcon(ShadIconType::Check);
    updateCopyToolTip(ShadCodeBlock::tr("已复制"));
    _copyResetTimer->start();
    Q_EMIT q->copied();
}

void ShadCodeBlockPrivate::resetCopyButton()
{
    _copyButton->setShadIcon(ShadIconType::Copy);
    updateCopyToolTip(ShadCodeBlock::tr("复制"));
}

void ShadCodeBlockPrivate::updateCopyToolTip(const QString& text)
{
    _copyButton->setToolTip(text);
    if (_copyButton->underMouse() && _copyButton->isVisible())
    {
        const int margin = ShadStyleHelper::kFocusRingWidth;
        shadTooltip->showText(_copyButton, _copyButton->rect().adjusted(margin, margin, -margin, -margin), text);
    }
}
