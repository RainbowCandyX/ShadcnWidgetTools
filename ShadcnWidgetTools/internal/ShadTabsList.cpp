#include "ShadTabsList.h"

#include <QColor>
#include <QEvent>
#include <QFocusEvent>
#include <QIcon>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QSizePolicy>
#include <QtGlobal>
#include <QtMath>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTransition.h"

namespace
{
    // p-[3px]，横向 h-9
    constexpr int kListPadding = 3;
    constexpr int kListHeight = 36;
    // 横向标签 h-[calc(100%-1px)]；纵向标签为 py-1 + text-sm 行高 20 + 上下 1px 边框
    constexpr int kHorizontalTabHeight = kListHeight - 2 * kListPadding - 1;
    constexpr int kVerticalTabHeight = 30;
    // px-2 与 1px 边框
    constexpr int kTabPaddingX = 8;
    constexpr int kBorderWidth = 1;
    // 图标 size-4，与文字之间 gap-1.5
    constexpr int kIconSize = 16;
    constexpr int kIconGap = 6;
    // Line 变体标签之间 gap-1
    constexpr int kLineSpacing = 4;
    constexpr int kFontPixelSize = 14;
    // 指示线 h-0.5 / w-0.5：横向位于标签下方 bottom-[-5px]，纵向位于结束侧 -right-1
    constexpr int kIndicatorThickness = 2;
    constexpr int kHorizontalIndicatorOffset = 5;
    constexpr int kVerticalIndicatorOffset = 4;
    // 未选中文字 text-foreground/60（暗色 text-muted-foreground），暗色选中项 bg-input/30
    constexpr qreal kInactiveTextAlpha = 0.6;
    constexpr qreal kDarkActiveAlpha = 0.3;
    // focus-visible:outline-1
    constexpr qreal kOutlineWidth = 1.0;
} // namespace

ShadTabsList::ShadTabsList(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadTabsList::~ShadTabsList()
{
}

int ShadTabsList::insertTab(int index, ShadIconType::IconName icon, const QString& text)
{
    const int position = index < 0 || index > count() ? count() : index;
    // 插入后原悬停位置对应的标签会变化，先清除悬停
    setHoverIndex(-1);
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
    updateGeometry();
    update();
    // 第一个标签自动成为当前标签
    if (_currentIndex < 0)
    {
        setCurrentIndex(position);
    }
    return position;
}

void ShadTabsList::removeTab(int index)
{
    if (index < 0 || index >= count())
    {
        return;
    }
    setHoverIndex(-1);
    delete _tabs.at(index).hoverTransition;
    delete _tabs.at(index).activeTransition;
    _tabs.removeAt(index);
    _pressedIndex = -1;
    if (_focusIndex >= index)
    {
        _focusIndex = qMin(_focusIndex > index ? _focusIndex - 1 : _focusIndex, count() - 1);
    }
    updateGeometry();
    update();
    if (index < _currentIndex)
    {
        --_currentIndex;
        return;
    }
    if (index == _currentIndex)
    {
        // 删除当前标签时选中同一位置的标签（删除的是最后一个时选中前一个）
        _currentIndex = -1;
        setCurrentIndex(qMin(index, count() - 1));
        if (_currentIndex < 0)
        {
            Q_EMIT currentChanged(-1);
        }
    }
}

int ShadTabsList::count() const
{
    return int(_tabs.size());
}

void ShadTabsList::setTabText(int index, const QString& text)
{
    if (index < 0 || index >= count() || _tabs.at(index).text == text)
    {
        return;
    }
    _tabs[index].text = text;
    updateGeometry();
    update();
}

QString ShadTabsList::tabText(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).text : QString();
}

void ShadTabsList::setTabIcon(int index, ShadIconType::IconName icon)
{
    if (index < 0 || index >= count() || _tabs.at(index).icon == icon)
    {
        return;
    }
    _tabs[index].icon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadTabsList::tabIcon(int index) const
{
    return index >= 0 && index < count() ? _tabs.at(index).icon : ShadIconType::None;
}

void ShadTabsList::setTabEnabled(int index, bool enabled)
{
    if (index < 0 || index >= count() || _tabs.at(index).isEnabled == enabled)
    {
        return;
    }
    _tabs[index].isEnabled = enabled;
    if (!enabled && _hoverIndex == index)
    {
        setHoverIndex(-1);
    }
    update();
}

bool ShadTabsList::isTabEnabled(int index) const
{
    return index >= 0 && index < count() && _tabs.at(index).isEnabled;
}

void ShadTabsList::setCurrentIndex(int index)
{
    if (index < -1 || index >= count() || index == _currentIndex)
    {
        return;
    }
    _currentIndex = index;
    // 没有键盘焦点时，焦点位置跟随当前标签，下次 Tab 进入时落在当前标签上
    if (!hasFocus())
    {
        _focusIndex = index;
    }
    updateActiveTransitions();
    update();
    Q_EMIT currentChanged(index);
}

int ShadTabsList::currentIndex() const
{
    return _currentIndex;
}

void ShadTabsList::setVariant(ShadTabsType::Variant variant)
{
    if (_variant == variant)
    {
        return;
    }
    _variant = variant;
    updateGeometry();
    update();
}

ShadTabsType::Variant ShadTabsList::getVariant() const
{
    return _variant;
}

void ShadTabsList::setOrientation(Qt::Orientation orientation)
{
    if (_orientation == orientation)
    {
        return;
    }
    _orientation = orientation;
    updateGeometry();
    update();
}

Qt::Orientation ShadTabsList::getOrientation() const
{
    return _orientation;
}

void ShadTabsList::setActivateOnFocus(bool activateOnFocus)
{
    _isActivateOnFocus = activateOnFocus;
}

bool ShadTabsList::isActivateOnFocus() const
{
    return _isActivateOnFocus;
}

int ShadTabsList::indicatorOverflow() const
{
    if (_variant != ShadTabsType::Line)
    {
        return 0;
    }
    if (_orientation == Qt::Horizontal)
    {
        return qMax(0, kListPadding + kHorizontalTabHeight + kHorizontalIndicatorOffset - kListHeight);
    }
    return qMax(0, kVerticalIndicatorOffset - kListPadding);
}

QSize ShadTabsList::sizeHint() const
{
    // 指示线超出的部分也计入组件尺寸，绘制时不会被裁剪
    const QSize size = listRect().size();
    if (_orientation == Qt::Horizontal)
    {
        return QSize(size.width(), size.height() + indicatorOverflow());
    }
    return QSize(size.width() + indicatorOverflow(), size.height());
}

QSize ShadTabsList::minimumSizeHint() const
{
    return sizeHint();
}

void ShadTabsList::mouseMoveEvent(QMouseEvent* event)
{
    setHoverIndex(tabAt(event->position().toPoint()));
    QWidget::mouseMoveEvent(event);
}

void ShadTabsList::mousePressEvent(QMouseEvent* event)
{
    const int index = tabAt(event->position().toPoint());
    if (event->button() != Qt::LeftButton || !isSelectable(index))
    {
        QWidget::mousePressEvent(event);
        return;
    }
    _pressedIndex = index;
    _focusIndex = index;
    update();
    event->accept();
}

void ShadTabsList::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || _pressedIndex < 0)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    const int index = tabAt(event->position().toPoint());
    if (index == _pressedIndex && isSelectable(index))
    {
        setCurrentIndex(index);
    }
    _pressedIndex = -1;
    event->accept();
}

void ShadTabsList::leaveEvent(QEvent* event)
{
    setHoverIndex(-1);
    QWidget::leaveEvent(event);
}

void ShadTabsList::keyPressEvent(QKeyEvent* event)
{
    const bool isHorizontal = _orientation == Qt::Horizontal;
    const bool isRightToLeft = layoutDirection() == Qt::RightToLeft;
    int step = 0;
    int target = -1;
    switch (event->key())
    {
        case Qt::Key_Left:
            // 横向时 RTL 下左右方向对调
            step = isHorizontal ? (isRightToLeft ? 1 : -1) : 0;
            break;
        case Qt::Key_Right:
            step = isHorizontal ? (isRightToLeft ? -1 : 1) : 0;
            break;
        case Qt::Key_Up:
            step = isHorizontal ? 0 : -1;
            break;
        case Qt::Key_Down:
            step = isHorizontal ? 0 : 1;
            break;
        case Qt::Key_Home:
            target = edgeSelectable(true);
            break;
        case Qt::Key_End:
            target = edgeSelectable(false);
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
    if (step != 0)
    {
        target = nextSelectable(_focusIndex, step);
    }
    if (target >= 0)
    {
        moveFocusTo(target);
        event->accept();
        return;
    }
    QWidget::keyPressEvent(event);
}

void ShadTabsList::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    // 键盘进入标签栏时焦点落在当前标签上；当前标签不可用时落在第一个可用的标签上（鼠标按下时已设置）
    if (event->reason() != Qt::MouseFocusReason)
    {
        _focusIndex = isSelectable(_currentIndex) ? _currentIndex : edgeSelectable(true);
    }
    update();
    QWidget::focusInEvent(event);
}

void ShadTabsList::focusOutEvent(QFocusEvent* event)
{
    // 弹出层或窗口切换导致失去焦点时保持状态，焦点回来后继续显示焦点环
    if (event->reason() != Qt::PopupFocusReason && event->reason() != Qt::ActiveWindowFocusReason)
    {
        _isFocusVisible = false;
    }
    update();
    QWidget::focusOutEvent(event);
}

void ShadTabsList::changeEvent(QEvent* event)
{
    if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
    }
    if (event->type() == QEvent::EnabledChange)
    {
        setHoverIndex(-1);
    }
    if (event->type() == QEvent::FontChange || event->type() == QEvent::LayoutDirectionChange || event->type() == QEvent::EnabledChange)
    {
        update();
    }
    QWidget::changeEvent(event);
}

void ShadTabsList::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    const Qt::LayoutDirection direction = layoutDirection();
    const bool isDark = shadTheme->isDark();
    const bool isHorizontal = _orientation == Qt::Horizontal;
    const qreal baseOpacity = isEnabled() ? 1.0 : kDisabledOpacity;
    const qreal radius = shadTheme->getRadius(ShadThemeType::Medium);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor inactiveText = isDark ? shadTheme->getColor(ShadThemeType::MutedForeground) : withAlpha(foreground, kInactiveTextAlpha);
    const QColor activeBackground = isDark ? withAlpha(selectionColor(ShadThemeType::Input), kDarkActiveAlpha) : selectionColor(ShadThemeType::Background);
    const QColor ring = shadTheme->getColor(ShadThemeType::Ring);
    const QFont font = tabFont();
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    painter.setOpacity(baseOpacity);
    // Default：bg-muted rounded-lg；Line：背景透明
    if (_variant == ShadTabsType::Default)
    {
        fillRoundedRect(&painter, QRectF(visualRect(direction, rect(), listRect())), shadTheme->getRadius(ShadThemeType::Large), shadTheme->getColor(ShadThemeType::Muted));
    }
    for (int index = 0; index < count(); ++index)
    {
        const Tab& tab = _tabs.at(index);
        const QRect logicalRect = logicalTabRect(index);
        const QRectF tabArea(visualRect(direction, rect(), logicalRect));
        const qreal hover = tab.hoverTransition->value();
        const qreal active = tab.activeTransition->value();
        const qreal opacity = baseOpacity * (tab.isEnabled ? 1.0 : kDisabledOpacity);
        if (active > 0.0 && _variant == ShadTabsType::Default)
        {
            // data-active:bg-background shadow-sm，暗色 border-input bg-input/30
            painter.setOpacity(opacity * active);
            drawShadowSm(&painter, tabArea, radius);
            fillRoundedRect(&painter, tabArea, radius, activeBackground);
            if (isDark)
            {
                strokeRoundedRect(&painter, tabArea, radius, selectionColor(ShadThemeType::Input));
            }
        }
        else if (active > 0.0)
        {
            // Line：选中项的指示线，透明度过渡（after:transition-opacity）
            QRect indicator(logicalRect.left(), logicalRect.bottom() + 1 + kHorizontalIndicatorOffset - kIndicatorThickness, logicalRect.width(), kIndicatorThickness);
            if (!isHorizontal)
            {
                indicator = QRect(logicalRect.right() + 1 + kVerticalIndicatorOffset - kIndicatorThickness, logicalRect.top(), kIndicatorThickness, logicalRect.height());
            }
            painter.setOpacity(opacity * active);
            painter.fillRect(visualRect(direction, rect(), indicator), foreground);
        }
        painter.setOpacity(opacity);
        // 悬停或选中时文字过渡到 foreground
        const QColor textColor = mix(inactiveText, foreground, qMax(hover, active));
        const QRectF contentRect = tabArea.adjusted(kTabPaddingX + kBorderWidth, 0, -(kTabPaddingX + kBorderWidth), 0);
        const Qt::Alignment alignment = (isHorizontal ? Qt::AlignHCenter : Qt::AlignLeft) | Qt::AlignVCenter;
        drawIconText(&painter, contentRect, tab.icon, QIcon(), tab.text, font, textColor, kIconGap, alignment, kIconSize, 0.0, direction);
        // focus-visible：border-ring、outline-1 outline-ring 与 ring-[3px] ring-ring/50
        if (index == _focusIndex && hasFocus() && _isFocusVisible)
        {
            painter.setOpacity(baseOpacity);
            drawFocusRing(&painter, tabArea, radius, focusRingColor(false));
            strokeRoundedRect(&painter, tabArea.adjusted(-kOutlineWidth, -kOutlineWidth, kOutlineWidth, kOutlineWidth), radius + kOutlineWidth, ring, kOutlineWidth);
            strokeRoundedRect(&painter, tabArea, radius, ring);
        }
    }
}

QFont ShadTabsList::tabFont() const
{
    QFont font = this->font();
    font.setPixelSize(kFontPixelSize);
    font.setWeight(QFont::Medium);
    return font;
}

int ShadTabsList::tabWidth() const
{
    // flex-1：横向时各标签等宽；纵向时 w-full，宽度都取最宽的标签
    const QFont font = tabFont();
    int width = 0;
    for (const Tab& tab : _tabs)
    {
        const qreal content = ShadStyleHelper::iconTextWidth(font, tab.text, tab.icon != ShadIconType::None, kIconGap, kIconSize);
        width = qMax(width, qCeil(content) + 2 * (kTabPaddingX + kBorderWidth));
    }
    return width;
}

int ShadTabsList::tabHeight() const
{
    return _orientation == Qt::Horizontal ? kHorizontalTabHeight : kVerticalTabHeight;
}

int ShadTabsList::tabSpacing() const
{
    return _variant == ShadTabsType::Line ? kLineSpacing : 0;
}

QRect ShadTabsList::listRect() const
{
    const int tabCount = count();
    const int width = tabWidth();
    const int spacing = tabSpacing();
    if (_orientation == Qt::Horizontal)
    {
        const int contentWidth = tabCount > 0 ? tabCount * width + (tabCount - 1) * spacing : 0;
        return QRect(0, 0, 2 * kListPadding + contentWidth, kListHeight);
    }
    // 纵向时高度按内容（h-fit）
    const int contentHeight = tabCount > 0 ? tabCount * kVerticalTabHeight + (tabCount - 1) * spacing : 0;
    return QRect(0, 0, 2 * kListPadding + width, 2 * kListPadding + contentHeight);
}

QRect ShadTabsList::logicalTabRect(int index) const
{
    const int width = tabWidth();
    const int height = tabHeight();
    const int spacing = tabSpacing();
    if (_orientation == Qt::Horizontal)
    {
        return QRect(kListPadding + index * (width + spacing), kListPadding, width, height);
    }
    return QRect(kListPadding, kListPadding + index * (height + spacing), width, height);
}

QRect ShadTabsList::tabRect(int index) const
{
    return ShadStyleHelper::visualRect(layoutDirection(), rect(), logicalTabRect(index));
}

int ShadTabsList::tabAt(const QPoint& position) const
{
    for (int index = 0; index < count(); ++index)
    {
        if (tabRect(index).contains(position))
        {
            return index;
        }
    }
    return -1;
}

bool ShadTabsList::isSelectable(int index) const
{
    return index >= 0 && index < count() && _tabs.at(index).isEnabled && isEnabled();
}

int ShadTabsList::nextSelectable(int from, int step) const
{
    const int tabCount = count();
    if (tabCount == 0 || step == 0)
    {
        return -1;
    }
    // 没有焦点位置时，向后从第一个开始，向前从最后一个开始
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

int ShadTabsList::edgeSelectable(bool isFirst) const
{
    return nextSelectable(-1, isFirst ? 1 : -1);
}

void ShadTabsList::setHoverIndex(int index)
{
    const int target = isSelectable(index) ? index : -1;
    if (target == _hoverIndex)
    {
        return;
    }
    if (_hoverIndex >= 0 && _hoverIndex < count())
    {
        _tabs.at(_hoverIndex).hoverTransition->animateTo(0.0);
    }
    _hoverIndex = target;
    if (_hoverIndex >= 0)
    {
        _tabs.at(_hoverIndex).hoverTransition->animateTo(1.0);
    }
}

void ShadTabsList::moveFocusTo(int index)
{
    _focusIndex = index;
    // 方向键移动焦点属于键盘导航，显示焦点环
    _isFocusVisible = true;
    if (_isActivateOnFocus)
    {
        setCurrentIndex(index);
    }
    update();
}

void ShadTabsList::updateActiveTransitions()
{
    for (int index = 0; index < count(); ++index)
    {
        _tabs.at(index).activeTransition->animateTo(index == _currentIndex ? 1.0 : 0.0);
    }
}
