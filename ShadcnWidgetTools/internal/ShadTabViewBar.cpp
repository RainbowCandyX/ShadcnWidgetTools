#include "ShadTabViewBar.h"

#include <QApplication>
#include <QColor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QHelpEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QList>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSize>
#include <QSizeF>
#include <QSizePolicy>
#include <QVariant>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QtGlobal>
#include <QtMath>

#include <cmath>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"
#include "ShadTransition.h"

namespace
{
    constexpr int kBarPadding = 4;
    constexpr int kTabHeight = 32;
    constexpr int kBarHeight = kTabHeight + 2 * kBarPadding;
    constexpr int kTabSpacing = 4;
    constexpr int kMinTabWidth = 56;
    constexpr int kMaxTabWidth = 240;
    // 窄于该宽度时只有当前标签显示关闭按钮
    constexpr int kCloseVisibleWidth = 96;
    constexpr int kTabPaddingX = 8;
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 8;
    constexpr int kCloseSize = 20;
    constexpr int kCloseIconSize = 14;
    constexpr int kCloseInset = 6;
    constexpr int kCloseGap = 4;
    constexpr int kAddSize = 28;
    constexpr int kAddIconSize = 16;
    constexpr int kAddGap = 4;
    constexpr int kFontPixelSize = 13;
    constexpr qreal kSeparatorHeight = 16.0;
    constexpr qreal kHoverAlpha = 0.5;
    constexpr qreal kCloseHoverAlpha = 0.1;
    constexpr qreal kClosePressedAlpha = 0.15;
    constexpr qreal kWheelStep = 48.0;
    constexpr int kLayoutDurationMs = 150;
    constexpr int kDetachDistance = 32;
    constexpr qreal kDropLineWidth = 3.0;
    constexpr qreal kDropHeadDiameter = 10.0;
    constexpr qreal kDropHaloWidth = 2.0;
    constexpr int kPreviewWidth = 320;
    constexpr qreal kPreviewMaxHeight = 200.0;
} // namespace

ShadTabViewBar::ShadTabViewBar(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    // 点击不抢走页面内的焦点
    setFocusPolicy(Qt::TabFocus);
    setMouseTracking(true);
    setProperty(ShadStyleHelper::kCustomToolTipProperty, true);
    _addHoverTransition = new ShadTransition(this);
    _layoutAnimation = new QVariantAnimation(this);
    _layoutAnimation->setStartValue(0.0);
    _layoutAnimation->setEndValue(1.0);
    _layoutAnimation->setDuration(kLayoutDurationMs);
    _layoutAnimation->setEasingCurve(QEasingCurve::OutCubic);
    connect(_layoutAnimation, &QVariantAnimation::valueChanged, this, [this](const QVariant& value)
    {
        _layoutProgress = value.toReal();
        update();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadTabViewBar::~ShadTabViewBar()
{
}

int ShadTabViewBar::insertTab(int index, ShadIconType::IconName icon, const QString& text)
{
    const int position = index < 0 || index > count() ? count() : index;
    endDrag();
    _pressedIndex = -1;
    _pressedPart = Part::None;
    _pressedButton = Qt::NoButton;
    snapshotLayout();
    setHover(-1, Part::None);
    _hiddenIndex = -1;
    _dropSlot = -1;
    Tab tab;
    tab.icon = icon;
    tab.text = text;
    tab.hoverTransition = new ShadTransition(this);
    tab.activeTransition = new ShadTransition(this);
    _tabs.insert(position, tab);
    if (_currentIndex >= position)
    {
        ++_currentIndex;
    }
    if (_focusIndex >= position)
    {
        ++_focusIndex;
    }
    _tabs[position].startX = slotX(slotOf(position));
    _tabs[position].startWidth = 0.0;
    updateGeometry();
    setScrollOffset(_scrollOffset);
    startLayoutAnimation();
    if (_currentIndex < 0)
    {
        setCurrentIndex(position);
    }
    return position;
}

void ShadTabViewBar::removeTab(int index)
{
    if (index < 0 || index >= count())
    {
        return;
    }
    endDrag();
    _pressedIndex = -1;
    _pressedPart = Part::None;
    _pressedButton = Qt::NoButton;
    snapshotLayout();
    setHover(-1, Part::None);
    _hiddenIndex = -1;
    _dropSlot = -1;
    delete _tabs.at(index).hoverTransition;
    delete _tabs.at(index).activeTransition;
    _tabs.removeAt(index);
    if (_focusIndex >= index)
    {
        _focusIndex = qMin(_focusIndex > index ? _focusIndex - 1 : _focusIndex, count() - 1);
    }
    updateGeometry();
    setScrollOffset(_scrollOffset);
    startLayoutAnimation();
    if (index < _currentIndex)
    {
        --_currentIndex;
        return;
    }
    if (index == _currentIndex)
    {
        _currentIndex = -1;
        setCurrentIndex(qMin(index, count() - 1));
        if (_currentIndex < 0)
        {
            Q_EMIT currentChanged(-1);
        }
    }
}

void ShadTabViewBar::moveTab(int from, int to)
{
    if (from < 0 || from >= count() || to < 0 || to >= count() || from == to)
    {
        return;
    }
    snapshotLayout();
    setHover(-1, Part::None);
    _tabs.move(from, to);
    const auto remap = [from, to](int index)
    {
        if (index == from)
        {
            return to;
        }
        if (from < to && index > from && index <= to)
        {
            return index - 1;
        }
        if (to < from && index >= to && index < from)
        {
            return index + 1;
        }
        return index;
    };
    _currentIndex = remap(_currentIndex);
    _focusIndex = remap(_focusIndex);
    _pressedIndex = remap(_pressedIndex);
    _hiddenIndex = remap(_hiddenIndex);
    startLayoutAnimation();
    Q_EMIT tabMoved(from, to);
}

int ShadTabViewBar::count() const
{
    return int(_tabs.size());
}

void ShadTabViewBar::setTabText(int index, const QString& text)
{
    if (index < 0 || index >= count() || _tabs.at(index).text == text)
    {
        return;
    }
    _tabs[index].text = text;
    update();
}

QString ShadTabViewBar::tabText(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).text : QString();
}

void ShadTabViewBar::setTabIcon(int index, ShadIconType::IconName icon)
{
    if (index < 0 || index >= count() || _tabs.at(index).icon == icon)
    {
        return;
    }
    _tabs[index].icon = icon;
    update();
}

ShadIconType::IconName ShadTabViewBar::tabIcon(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).icon : ShadIconType::None;
}

void ShadTabViewBar::setTabFlags(int index, ShadTabViewType::TabFlags flags)
{
    if (index < 0 || index >= count() || _tabs.at(index).flags == flags)
    {
        return;
    }
    _tabs[index].flags = flags;
    if (flags.testFlag(ShadTabViewType::TabDisabled) && _hoverIndex == index)
    {
        setHover(-1, Part::None);
    }
    update();
}

ShadTabViewType::TabFlags ShadTabViewBar::tabFlags(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).flags : ShadTabViewType::TabFlags(ShadTabViewType::NoTabFlag);
}

void ShadTabViewBar::setTabToolTip(int index, const QString& toolTip)
{
    if (index >= 0 && index < count())
    {
        _tabs[index].toolTip = toolTip;
    }
}

QString ShadTabViewBar::tabToolTip(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).toolTip : QString();
}

void ShadTabViewBar::setTabData(int index, const QVariant& data)
{
    if (index >= 0 && index < count())
    {
        _tabs[index].data = data;
    }
}

QVariant ShadTabViewBar::tabData(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).data : QVariant();
}

void ShadTabViewBar::setCurrentIndex(int index)
{
    if (index < -1 || index >= count() || index == _currentIndex)
    {
        return;
    }
    _currentIndex = index;
    if (!hasFocus())
    {
        _focusIndex = index;
    }
    updateActiveTransitions();
    ensureVisible(index);
    update();
    Q_EMIT currentChanged(index);
}

int ShadTabViewBar::currentIndex() const
{
    return _currentIndex;
}

void ShadTabViewBar::selectAdjacent(int step)
{
    const int target = nextSelectable(_currentIndex, step);
    if (target >= 0)
    {
        setCurrentIndex(target);
    }
}

void ShadTabViewBar::setTabsClosable(bool closable)
{
    if (_isTabsClosable == closable)
    {
        return;
    }
    _isTabsClosable = closable;
    update();
}

bool ShadTabViewBar::isTabsClosable() const
{
    return _isTabsClosable;
}

void ShadTabViewBar::setAddButtonVisible(bool visible)
{
    if (_isAddButtonVisible == visible)
    {
        return;
    }
    _isAddButtonVisible = visible;
    setHover(-1, Part::None);
    updateGeometry();
    setScrollOffset(_scrollOffset);
    update();
}

bool ShadTabViewBar::isAddButtonVisible() const
{
    return _isAddButtonVisible;
}

bool ShadTabViewBar::isBlankAt(const QPoint& pos) const
{
    Part part = Part::None;
    return rect().contains(pos) && hitTest(pos, &part) < 0 && part == Part::None;
}

void ShadTabViewBar::setDetachEnabled(bool enabled)
{
    _isDetachEnabled = enabled;
}

void ShadTabViewBar::setHiddenIndex(int index)
{
    const int target = index >= 0 && index < count() ? index : -1;
    if (target == _hiddenIndex)
    {
        return;
    }
    snapshotLayout();
    _hiddenIndex = target;
    setHover(-1, Part::None);
    setScrollOffset(_scrollOffset);
    startLayoutAnimation();
}

void ShadTabViewBar::setDropIndicator(int slot)
{
    const int target = slot >= 0 && slot <= slotCount() ? slot : -1;
    if (target == _dropSlot)
    {
        return;
    }
    _dropSlot = target;
    update();
}

int ShadTabViewBar::dropIndicatorSlot() const
{
    return _dropSlot;
}

int ShadTabViewBar::insertionSlotAt(const QPoint& pos) const
{
    const qreal step = qMax(1.0, tabWidth() + kTabSpacing);
    const qreal x = logicalX(pos.x()) + _scrollOffset - kBarPadding;
    int first = 0;
    int last = 0;
    dropSlotRange(&first, &last);
    return qBound(first, qRound(x / step), last);
}

QRect ShadTabViewBar::restingTabRect(int slot) const
{
    const QRectF logicalRect(kBarPadding + slot * (tabWidth() + kTabSpacing), (height() - kTabHeight) / 2.0, tabWidth(), kTabHeight);
    return toVisual(logicalRect).toAlignedRect();
}

QPixmap ShadTabViewBar::dragPixmap(int index, const QPixmap& pageShot) const
{
    using namespace ShadStyleHelper;
    const qreal dpr = devicePixelRatioF();
    const QSizeF pageSize = pageShot.deviceIndependentSize();
    const qreal scale = pageSize.width() > 0.0 ? kPreviewWidth / pageSize.width() : 0.0;
    const qreal previewHeight = qMin(kPreviewMaxHeight, pageSize.height() * scale);
    const QSize size(kPreviewWidth, kTabHeight + qCeil(previewHeight));
    QPixmap pixmap(QSize(qCeil(size.width() * dpr), qCeil(size.height() * dpr)));
    pixmap.setDevicePixelRatio(dpr);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing | QPainter::SmoothPixmapTransform);
    const QRectF frame(QPointF(0.0, 0.0), QSizeF(size));
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    fillRoundedRect(&painter, frame, radius, shadTheme->getColor(ShadThemeType::Background));
    painter.save();
    painter.setClipPath(roundedRectPath(frame, radius));
    const QRectF header(0.0, 0.0, kPreviewWidth, kTabHeight);
    painter.fillRect(header, shadTheme->getColor(ShadThemeType::Accent));
    if (index >= 0 && index < count())
    {
        drawIconText(&painter, header.adjusted(kTabPaddingX, 0.0, -kTabPaddingX, 0.0), _tabs.at(index).icon, QIcon(), _tabs.at(index).text, tabFont(), shadTheme->getColor(ShadThemeType::Foreground), kIconGap, Qt::AlignLeft | Qt::AlignVCenter, kIconSize, 0.0, layoutDirection());
    }
    if (previewHeight > 0.0)
    {
        const QRectF source(0.0, 0.0, pageShot.width(), previewHeight / scale * pageShot.devicePixelRatio());
        painter.drawPixmap(QRectF(0.0, kTabHeight, kPreviewWidth, previewHeight), pageShot, source);
    }
    painter.restore();
    strokeRoundedRect(&painter, frame, radius, shadTheme->getColor(ShadThemeType::Border));
    return pixmap;
}

QSize ShadTabViewBar::sizeHint() const
{
    const int tabCount = count();
    const int content = tabCount > 0 ? tabCount * kMaxTabWidth + (tabCount - 1) * kTabSpacing : 0;
    const int addButton = _isAddButtonVisible ? kAddGap + kAddSize : 0;
    return QSize(2 * kBarPadding + content + addButton, kBarHeight);
}

QSize ShadTabViewBar::minimumSizeHint() const
{
    const int content = count() > 0 ? kMinTabWidth : 0;
    const int addButton = _isAddButtonVisible ? kAddGap + kAddSize : 0;
    return QSize(2 * kBarPadding + content + addButton, kBarHeight);
}

bool ShadTabViewBar::event(QEvent* event)
{
    if (event->type() == QEvent::ToolTip)
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        Part part = Part::None;
        const int index = hitTest(helpEvent->pos(), &part);
        if (!_isDragging && part == Part::Add)
        {
            shadTooltip->showText(this, toVisual(logicalAddRect()).toAlignedRect(), tr("新建标签页"));
            return true;
        }
        if (!_isDragging && part == Part::Close)
        {
            shadTooltip->showText(this, toVisual(logicalCloseRect(index)).toAlignedRect(), tr("关闭标签页"));
            return true;
        }
        const QString tabTip = part == Part::Tab ? (_tabs.at(index).toolTip.isEmpty() ? _tabs.at(index).text : _tabs.at(index).toolTip) : QString();
        if (!_isDragging && !tabTip.isEmpty())
        {
            shadTooltip->showText(this, toVisual(logicalTabRect(index)).toAlignedRect(), tabTip);
            return true;
        }
        event->ignore();
        return false;
    }
    return QWidget::event(event);
}

void ShadTabViewBar::mousePressEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    Part part = Part::None;
    const int index = hitTest(pos, &part);
    // 空白处交给标题栏拖动窗口
    if (part == Part::None || !isEnabled())
    {
        event->ignore();
        return;
    }
    if (index >= 0 && _tabs.at(index).flags.testFlag(ShadTabViewType::TabDisabled))
    {
        event->accept();
        return;
    }
    if (event->button() == Qt::LeftButton)
    {
        _pressedIndex = index;
        _pressedPart = part;
        _pressedButton = Qt::LeftButton;
        if (part == Part::Tab)
        {
            _focusIndex = index;
            setCurrentIndex(index);
            _pressPos = pos;
            _dragGrabOffset = logicalX(event->position().x()) - logicalTabRect(index).x();
        }
        update();
        event->accept();
        return;
    }
    if (event->button() == Qt::MiddleButton && part != Part::Add && _isTabsClosable && !_tabs.at(index).flags.testFlag(ShadTabViewType::TabNotClosable))
    {
        _pressedIndex = index;
        _pressedPart = Part::Tab;
        _pressedButton = Qt::MiddleButton;
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadTabViewBar::mouseMoveEvent(QMouseEvent* event)
{
    const QPoint pos = event->position().toPoint();
    if (_pressedButton == Qt::LeftButton && _pressedPart == Part::Tab && _pressedIndex >= 0 && !_tabs.at(_pressedIndex).flags.testFlag(ShadTabViewType::TabNotMovable))
    {
        if (_isDetachEnabled && (pos.y() < -kDetachDistance || pos.y() > height() + kDetachDistance))
        {
            const int index = _pressedIndex;
            const QRectF tabArea = toVisual(logicalTabRect(index));
            const QPoint grabOffset(qBound(0, qRound(pos.x() - tabArea.x()), qMax(0, qRound(tabArea.width()) - 1)), qBound(0, _pressPos.y() - qRound(tabArea.y()), kTabHeight - 1));
            endDrag();
            _pressedIndex = -1;
            _pressedPart = Part::None;
            _pressedButton = Qt::NoButton;
            setHover(-1, Part::None);
            event->accept();
            Q_EMIT tabDragOutRequested(index, grabOffset);
            return;
        }
        if (!_isDragging && (pos - _pressPos).manhattanLength() >= QApplication::startDragDistance())
        {
            _isDragging = true;
        }
        if (_isDragging)
        {
            int first = 0;
            int last = 0;
            movableRange(_pressedIndex, &first, &last);
            const qreal x = logicalX(event->position().x()) + _scrollOffset - _dragGrabOffset;
            _dragX = qBound(slotX(first), x, slotX(last));
            const qreal step = tabWidth() + kTabSpacing;
            const int target = qBound(first, qRound((_dragX - slotX(0)) / step), last);
            if (target != _pressedIndex)
            {
                moveTab(_pressedIndex, target);
            }
            update();
            event->accept();
            return;
        }
    }
    Part part = Part::None;
    const int index = hitTest(pos, &part);
    setHover(index, part);
    QWidget::mouseMoveEvent(event);
}

void ShadTabViewBar::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != _pressedButton || _pressedButton == Qt::NoButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const QPoint pos = event->position().toPoint();
    const int pressedIndex = _pressedIndex;
    const Part pressedPart = _pressedPart;
    const Qt::MouseButton pressedButton = _pressedButton;
    const bool wasDragging = _isDragging;
    endDrag();
    _pressedIndex = -1;
    _pressedPart = Part::None;
    _pressedButton = Qt::NoButton;
    Part part = Part::None;
    const int index = hitTest(pos, &part);
    setHover(index, part);
    update();
    event->accept();
    if (wasDragging)
    {
        return;
    }
    // 接收方可以删除本组件，发出信号后不再访问成员
    if (pressedButton == Qt::LeftButton && pressedPart == Part::Add && part == Part::Add)
    {
        Q_EMIT newTabRequested();
        return;
    }
    const bool isCloseClick = pressedButton == Qt::LeftButton && pressedPart == Part::Close && part == Part::Close;
    const bool isMiddleClick = pressedButton == Qt::MiddleButton && part != Part::Add && part != Part::None;
    if ((isCloseClick || isMiddleClick) && index == pressedIndex)
    {
        // 保持宽度以便连续关闭
        _frozenTabWidth = tabWidth();
        Q_EMIT tabCloseRequested(index);
    }
}

void ShadTabViewBar::wheelEvent(QWheelEvent* event)
{
    if (maxScroll() <= 0.0)
    {
        event->ignore();
        return;
    }
    const QPoint pixel = event->pixelDelta();
    const QPoint angle = event->angleDelta();
    qreal delta = 0.0;
    if (!pixel.isNull())
    {
        delta = qAbs(pixel.x()) > qAbs(pixel.y()) ? pixel.x() : pixel.y();
    }
    else
    {
        delta = (qAbs(angle.x()) > qAbs(angle.y()) ? angle.x() : angle.y()) / 120.0 * kWheelStep;
    }
    setScrollOffset(_scrollOffset - delta);
    Part part = Part::None;
    const int index = hitTest(event->position().toPoint(), &part);
    setHover(index, part);
    event->accept();
}

void ShadTabViewBar::leaveEvent(QEvent* event)
{
    setHover(-1, Part::None);
    releaseFrozenWidth();
    QWidget::leaveEvent(event);
}

void ShadTabViewBar::keyPressEvent(QKeyEvent* event)
{
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    int target = -1;
    switch (event->key())
    {
        case Qt::Key_Left:
            target = nextSelectable(_focusIndex, isRightToLeft ? 1 : -1);
            break;
        case Qt::Key_Right:
            target = nextSelectable(_focusIndex, isRightToLeft ? -1 : 1);
            break;
        case Qt::Key_Home:
            target = nextSelectable(-1, 1);
            break;
        case Qt::Key_End:
            target = nextSelectable(-1, -1);
            break;
        case Qt::Key_Space:
        case Qt::Key_Return:
        case Qt::Key_Enter:
            if (isSelectable(_focusIndex))
            {
                setCurrentIndex(_focusIndex);
                event->accept();
                return;
            }
            break;
        default:
            break;
    }
    if (target >= 0)
    {
        moveFocusTo(target);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadTabViewBar::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    _focusIndex = isSelectable(_currentIndex) ? _currentIndex : nextSelectable(-1, 1);
    ensureVisible(_focusIndex);
    update();
    QWidget::focusInEvent(event);
}

void ShadTabViewBar::focusOutEvent(QFocusEvent* event)
{
    // 弹出层或切换窗口时保留焦点环
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        _isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadTabViewBar::resizeEvent(QResizeEvent* event)
{
    setScrollOffset(_scrollOffset);
    ensureVisible(_currentIndex);
    QWidget::resizeEvent(event);
}

void ShadTabViewBar::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::EnabledChange)
    {
        setHover(-1, Part::None);
    }
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadTabViewBar::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const qreal baseOpacity = isEnabled() ? 1.0 : kDisabledOpacity;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const qreal closeRadius = shadTheme->getRadius(ShadThemeType::Small);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor mutedForeground = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor accent = shadTheme->getColor(ShadThemeType::Accent);
    const QColor border = shadTheme->getColor(ShadThemeType::Border);
    const QColor ring = shadTheme->getColor(ShadThemeType::Ring);
    const QFont font = tabFont();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);

    painter.save();
    if (maxScroll() > 0.0)
    {
        painter.setClipRect(toVisual(QRectF(kBarPadding, 0.0, availableWidth(), height())));
    }
    QList<int> visibleTabs;
    visibleTabs.reserve(count());
    for (int index = 0; index < count(); ++index)
    {
        if (index != _hiddenIndex && !(_isDragging && index == _pressedIndex))
        {
            visibleTabs.append(index);
        }
    }
    for (int position = 0; position + 1 < visibleTabs.size(); ++position)
    {
        const Tab& left = _tabs.at(visibleTabs.at(position));
        const Tab& right = _tabs.at(visibleTabs.at(position + 1));
        const qreal leftEmphasis = qMax(left.hoverTransition->value(), left.activeTransition->value());
        const qreal rightEmphasis = qMax(right.hoverTransition->value(), right.activeTransition->value());
        const qreal emphasis = qMax(leftEmphasis, rightEmphasis);
        const QRectF leftRect = logicalTabRect(visibleTabs.at(position));
        const QRectF rightRect = logicalTabRect(visibleTabs.at(position + 1));
        if (emphasis >= 1.0 || rightRect.left() - leftRect.right() > kTabSpacing + 1.0)
        {
            continue;
        }
        const qreal x = std::round(leftRect.right() + kTabSpacing / 2.0 - 0.5);
        const QRectF line(x, (height() - kSeparatorHeight) / 2.0, 1.0, kSeparatorHeight);
        painter.setOpacity(baseOpacity * (1.0 - emphasis));
        painter.fillRect(toVisual(line), border);
    }
    const auto paintTab = [&](int index)
    {
        const Tab& tab = _tabs.at(index);
        const QRectF logicalRect = logicalTabRect(index);
        if (index == _hiddenIndex || logicalRect.width() < 1.0)
        {
            return;
        }
        const QRectF tabArea = toVisual(logicalRect);
        const qreal hover = tab.hoverTransition->value();
        const qreal active = tab.activeTransition->value();
        const qreal opacity = baseOpacity * (tab.flags.testFlag(ShadTabViewType::TabDisabled) ? kDisabledOpacity : 1.0);
        if (_isDragging && index == _pressedIndex)
        {
            painter.setOpacity(opacity);
            drawShadowSm(&painter, tabArea, radius);
        }
        if (hover > 0.0 && active < 1.0)
        {
            painter.setOpacity(opacity * hover * (1.0 - active));
            fillRoundedRect(&painter, tabArea, radius, withAlpha(accent, kHoverAlpha));
        }
        if (active > 0.0)
        {
            painter.setOpacity(opacity * active);
            fillRoundedRect(&painter, tabArea, radius, accent);
        }
        painter.setOpacity(opacity);
        const QColor textColor = mix(mutedForeground, foreground, qMax(hover, active));
        const bool hasClose = isCloseVisible(index);
        QRectF content = logicalRect.adjusted(kTabPaddingX, 0.0, -kTabPaddingX, 0.0);
        if (hasClose)
        {
            content.setRight(logicalCloseRect(index).left() - kCloseGap);
        }
        if (content.width() >= kIconSize)
        {
            drawIconText(&painter, toVisual(content), tab.icon, QIcon(), tab.text, font, textColor, kIconGap, Qt::AlignLeft | Qt::AlignVCenter, kIconSize, 0.0, direction);
        }
        if (hasClose)
        {
            const QRectF closeArea = toVisual(logicalCloseRect(index));
            const bool isCloseHover = _hoverIndex == index && _hoverPart == Part::Close;
            const bool isClosePressed = isCloseHover && _pressedIndex == index && _pressedPart == Part::Close;
            if (isCloseHover)
            {
                fillRoundedRect(&painter, closeArea, closeRadius, withAlpha(foreground, isClosePressed ? kClosePressedAlpha : kCloseHoverAlpha));
            }
            drawIconText(&painter, closeArea, ShadIconType::X, QIcon(), QString(), font, isCloseHover ? foreground : textColor, 0.0, Qt::AlignCenter, kCloseIconSize, 0.0, direction);
        }
        if (index == _focusIndex && hasFocus() && _isFocusVisible)
        {
            drawFocusRing(&painter, tabArea, radius, focusRingColor(false));
            strokeRoundedRect(&painter, tabArea, radius, ring);
        }
    };
    for (int index = 0; index < count(); ++index)
    {
        if (!(_isDragging && index == _pressedIndex))
        {
            paintTab(index);
        }
    }
    if (_isDragging && _pressedIndex >= 0 && _pressedIndex < count())
    {
        paintTab(_pressedIndex);
    }
    painter.restore();

    if (_dropSlot >= 0)
    {
        const qreal headExtent = kDropHeadDiameter / 2.0 + kDropHaloWidth;
        const qreal center = qBound(headExtent, slotX(_dropSlot) - kTabSpacing / 2.0 - _scrollOffset, kBarPadding + availableWidth() + kTabSpacing / 2.0);
        const qreal headCenterY = headExtent;
        const QColor indicatorColor = shadTheme->getColor(ShadThemeType::Primary);
        const QRectF line(center - kDropLineWidth / 2.0, headCenterY, kDropLineWidth, height() - headCenterY - kDropLineWidth / 2.0);
        const QRectF halo(center - headExtent, headCenterY - headExtent, 2.0 * headExtent, 2.0 * headExtent);
        const QRectF head = halo.adjusted(kDropHaloWidth, kDropHaloWidth, -kDropHaloWidth, -kDropHaloWidth);
        painter.save();
        painter.setOpacity(baseOpacity);
        painter.setPen(Qt::NoPen);
        fillRoundedRect(&painter, toVisual(line), kDropLineWidth / 2.0, indicatorColor);
        painter.setBrush(shadTheme->getColor(shadTheme->getThemeMode(), ShadThemeType::Background));
        painter.drawEllipse(toVisual(halo));
        painter.setBrush(indicatorColor);
        painter.drawEllipse(toVisual(head));
        painter.restore();
    }

    if (_isAddButtonVisible)
    {
        const QRectF addArea = toVisual(logicalAddRect());
        const qreal hover = _addHoverTransition->value();
        if (hover > 0.0)
        {
            painter.setOpacity(baseOpacity * hover);
            fillRoundedRect(&painter, addArea, radius, accent);
        }
        painter.setOpacity(baseOpacity);
        drawIconText(&painter, addArea, ShadIconType::Plus, QIcon(), QString(), font, mix(mutedForeground, foreground, hover), 0.0, Qt::AlignCenter, kAddIconSize, 0.0, direction);
    }
}

QFont ShadTabViewBar::tabFont() const
{
    QFont font = this->font();
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Medium);
    return font;
}

qreal ShadTabViewBar::availableWidth() const
{
    const int addButton = _isAddButtonVisible ? kAddGap + kAddSize : 0;
    return qMax(0.0, qreal(width() - 2 * kBarPadding - addButton));
}

int ShadTabViewBar::slotCount() const
{
    return count() - (_hiddenIndex >= 0 ? 1 : 0);
}

int ShadTabViewBar::slotOf(int index) const
{
    return _hiddenIndex >= 0 && index > _hiddenIndex ? index - 1 : index;
}

qreal ShadTabViewBar::tabWidth() const
{
    const int slotTotal = slotCount();
    if (slotTotal <= 0)
    {
        return 0.0;
    }
    const qreal fit = std::floor((availableWidth() - (slotTotal - 1) * kTabSpacing) / slotTotal);
    const qreal width = qBound(qreal(kMinTabWidth), fit, qreal(kMaxTabWidth));
    return _frozenTabWidth >= 0.0 ? qMin(_frozenTabWidth, width) : width;
}

qreal ShadTabViewBar::contentWidth() const
{
    const int slotTotal = slotCount();
    return slotTotal > 0 ? slotTotal * tabWidth() + (slotTotal - 1) * kTabSpacing : 0.0;
}

qreal ShadTabViewBar::maxScroll() const
{
    return qMax(0.0, contentWidth() - availableWidth());
}

qreal ShadTabViewBar::slotX(int slot) const
{
    return kBarPadding + slot * (tabWidth() + kTabSpacing);
}

QRectF ShadTabViewBar::logicalTabRect(int index) const
{
    qreal x = slotX(slotOf(index));
    qreal width = index == _hiddenIndex ? 0.0 : tabWidth();
    if (_isDragging && index == _pressedIndex)
    {
        x = _dragX;
    }
    else if (_layoutProgress < 1.0)
    {
        const Tab& tab = _tabs.at(index);
        x = tab.startX + (x - tab.startX) * _layoutProgress;
        width = tab.startWidth + (width - tab.startWidth) * _layoutProgress;
    }
    return QRectF(x - _scrollOffset, (height() - kTabHeight) / 2.0, width, kTabHeight);
}

QRectF ShadTabViewBar::logicalCloseRect(int index) const
{
    const QRectF tabRect = logicalTabRect(index);
    return QRectF(tabRect.right() - kCloseInset - kCloseSize, tabRect.center().y() - kCloseSize / 2.0, kCloseSize, kCloseSize);
}

QRectF ShadTabViewBar::logicalAddRect() const
{
    qreal x = kBarPadding;
    if (slotCount() > 0)
    {
        qreal right = kBarPadding;
        for (int index = 0; index < count(); ++index)
        {
            const qreal tabRight = _isDragging && index == _pressedIndex ? slotX(slotOf(index)) + tabWidth() - _scrollOffset : logicalTabRect(index).right();
            if (index != _hiddenIndex)
            {
                right = qMax(right, tabRight);
            }
        }
        x = qMin(right, kBarPadding + availableWidth()) + kAddGap;
    }
    return QRectF(x, (height() - kAddSize) / 2.0, kAddSize, kAddSize);
}

QRectF ShadTabViewBar::toVisual(const QRectF& logicalRect) const
{
    return ShadStyleHelper::visualRect(layoutDirection(), QRectF(rect()), logicalRect);
}

bool ShadTabViewBar::isCloseVisible(int index) const
{
    const Tab& tab = _tabs.at(index);
    return _isTabsClosable && !tab.flags.testFlag(ShadTabViewType::TabNotClosable) && !tab.flags.testFlag(ShadTabViewType::TabDisabled) && (index == _currentIndex || tabWidth() >= kCloseVisibleWidth);
}

bool ShadTabViewBar::isSelectable(int index) const
{
    return index >= 0 && index < count() && !_tabs.at(index).flags.testFlag(ShadTabViewType::TabDisabled) && isEnabled();
}

int ShadTabViewBar::nextSelectable(int from, int step) const
{
    const int tabCount = count();
    if (tabCount == 0 || step == 0)
    {
        return -1;
    }
    int index = from < 0 || from >= tabCount ? (step > 0 ? -1 : tabCount) : from;
    for (int attempt = 0; attempt < tabCount; ++attempt)
    {
        index = (index + step + tabCount) % tabCount;
        if (isSelectable(index))
        {
            return index;
        }
    }
    return -1;
}

void ShadTabViewBar::movableRange(int index, int* first, int* last) const
{
    *first = index;
    *last = index;
    while (*first > 0 && !_tabs.at(*first - 1).flags.testFlag(ShadTabViewType::TabNotMovable))
    {
        --*first;
    }
    while (*last + 1 < count() && !_tabs.at(*last + 1).flags.testFlag(ShadTabViewType::TabNotMovable))
    {
        ++*last;
    }
}

void ShadTabViewBar::dropSlotRange(int* first, int* last) const
{
    if (_hiddenIndex >= 0)
    {
        // 拖回自己的标签栏时只能回到原区间
        movableRange(_hiddenIndex, first, last);
        return;
    }
    const int tabCount = count();
    int leading = 0;
    while (leading < tabCount && _tabs.at(leading).flags.testFlag(ShadTabViewType::TabNotMovable))
    {
        ++leading;
    }
    int trailing = 0;
    while (trailing < tabCount - leading && _tabs.at(tabCount - 1 - trailing).flags.testFlag(ShadTabViewType::TabNotMovable))
    {
        ++trailing;
    }
    *first = leading;
    *last = tabCount - trailing;
}

int ShadTabViewBar::hitTest(const QPoint& pos, Part* part) const
{
    *part = Part::None;
    if (_isAddButtonVisible && toVisual(logicalAddRect()).contains(pos))
    {
        *part = Part::Add;
        return -1;
    }
    const qreal x = logicalX(pos.x());
    if (maxScroll() > 0.0 && (x < kBarPadding || x > kBarPadding + availableWidth()))
    {
        return -1;
    }
    // 拖动中的标签优先命中
    QList<int> order;
    order.reserve(count());
    if (_isDragging && _pressedIndex >= 0)
    {
        order.append(_pressedIndex);
    }
    for (int index = 0; index < count(); ++index)
    {
        if (!(_isDragging && index == _pressedIndex))
        {
            order.append(index);
        }
    }
    for (const int index : order)
    {
        if (index == _hiddenIndex || !toVisual(logicalTabRect(index)).contains(pos))
        {
            continue;
        }
        *part = isCloseVisible(index) && toVisual(logicalCloseRect(index)).contains(pos) ? Part::Close : Part::Tab;
        return index;
    }
    return -1;
}

qreal ShadTabViewBar::logicalX(qreal x) const
{
    return layoutDirection() == Qt::RightToLeft ? width() - x : x;
}

void ShadTabViewBar::setHover(int index, Part part)
{
    if (index >= 0 && !isSelectable(index))
    {
        index = -1;
        part = Part::None;
    }
    if (index == _hoverIndex && part == _hoverPart)
    {
        return;
    }
    if (index != _hoverIndex)
    {
        if (_hoverIndex >= 0 && _hoverIndex < count())
        {
            _tabs.at(_hoverIndex).hoverTransition->animateTo(0.0);
        }
        if (index >= 0 && index < count())
        {
            _tabs.at(index).hoverTransition->animateTo(1.0);
        }
    }
    _addHoverTransition->animateTo(part == Part::Add ? 1.0 : 0.0);
    _hoverIndex = index;
    _hoverPart = part;
    update();
}

void ShadTabViewBar::moveFocusTo(int index)
{
    _focusIndex = index;
    _isFocusVisible = true;
    ensureVisible(index);
    update();
}

void ShadTabViewBar::updateActiveTransitions()
{
    for (int index = 0; index < count(); ++index)
    {
        _tabs.at(index).activeTransition->animateTo(index == _currentIndex ? 1.0 : 0.0);
    }
}

void ShadTabViewBar::setScrollOffset(qreal offset)
{
    const qreal bounded = qBound(0.0, offset, maxScroll());
    if (bounded == _scrollOffset)
    {
        return;
    }
    _scrollOffset = bounded;
    update();
}

void ShadTabViewBar::ensureVisible(int index)
{
    if (index < 0 || index >= count())
    {
        return;
    }
    const qreal start = slotX(slotOf(index)) - kBarPadding;
    const qreal end = start + tabWidth();
    qreal offset = _scrollOffset;
    if (start < offset)
    {
        offset = start;
    }
    else if (end > offset + availableWidth())
    {
        offset = end - availableWidth();
    }
    setScrollOffset(offset);
}

void ShadTabViewBar::snapshotLayout()
{
    for (int index = 0; index < count(); ++index)
    {
        const QRectF tabRect = logicalTabRect(index);
        _tabs[index].startX = tabRect.x() + _scrollOffset;
        _tabs[index].startWidth = tabRect.width();
    }
}

void ShadTabViewBar::startLayoutAnimation()
{
    _layoutAnimation->stop();
    _layoutProgress = isVisible() ? 0.0 : 1.0;
    if (isVisible())
    {
        _layoutAnimation->start();
    }
    update();
}

void ShadTabViewBar::releaseFrozenWidth()
{
    if (_frozenTabWidth < 0.0)
    {
        return;
    }
    snapshotLayout();
    _frozenTabWidth = -1.0;
    setScrollOffset(_scrollOffset);
    startLayoutAnimation();
}

void ShadTabViewBar::endDrag()
{
    if (!_isDragging)
    {
        return;
    }
    snapshotLayout();
    _isDragging = false;
    startLayoutAnimation();
}
