#include "ShadColorSwatches.h"

#include <QEvent>
#include <QFocusEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPoint>
#include <QPointF>
#include <QRectF>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 每格边长与色块直径
    constexpr int kCellSize = 32;
    constexpr int kSwatchDiameter = 24;
    // 悬停与选中圆环：与色块间隔 1px，宽 2px
    constexpr qreal kOutlineGap = 1.0;
    constexpr qreal kOutlineWidth = 2.0;
} // namespace

ShadColorSwatches::ShadColorSwatches(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadColorSwatches::~ShadColorSwatches()
{
}

void ShadColorSwatches::setColors(const QList<QColor>& colors)
{
    _colors = colors;
    if (_selectedIndex >= _colors.size())
    {
        setSelectedIndex(-1);
    }
    _cursorIndex = qBound(0, _cursorIndex, qMax(0, _colors.size() - 1));
    _hoveredIndex = -1;
    updateGeometry();
    update();
}

QList<QColor> ShadColorSwatches::getColors() const
{
    return _colors;
}

void ShadColorSwatches::setColorAt(int index, const QColor& color)
{
    if (index < 0 || index >= _colors.size())
    {
        return;
    }
    _colors[index] = color;
    update();
}

QColor ShadColorSwatches::colorAt(int index) const
{
    return _colors.value(index);
}

int ShadColorSwatches::count() const
{
    return _colors.size();
}

void ShadColorSwatches::setColumns(int columns)
{
    _columns = qMax(1, columns);
    updateGeometry();
    update();
}

int ShadColorSwatches::getColumns() const
{
    return _columns;
}

void ShadColorSwatches::setSelectable(bool selectable)
{
    _isSelectable = selectable;
    if (!selectable)
    {
        setSelectedIndex(-1);
    }
    update();
}

bool ShadColorSwatches::isSelectable() const
{
    return _isSelectable;
}

void ShadColorSwatches::setSelectedIndex(int index)
{
    const int boundedIndex = (index >= 0 && index < _colors.size()) ? index : -1;
    if (_selectedIndex == boundedIndex)
    {
        return;
    }
    _selectedIndex = boundedIndex;
    update();
    Q_EMIT selectedIndexChanged(_selectedIndex);
}

int ShadColorSwatches::getSelectedIndex() const
{
    return _selectedIndex;
}

QSize ShadColorSwatches::sizeHint() const
{
    const int columns = qMin(_columns, qMax(1, _colors.size()));
    const int rows = (_colors.size() + _columns - 1) / _columns;
    return QSize(columns * kCellSize, qMax(1, rows) * kCellSize);
}

QSize ShadColorSwatches::minimumSizeHint() const
{
    return sizeHint();
}

void ShadColorSwatches::changeEvent(QEvent* event)
{
    QWidget::changeEvent(event);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        update();
    }
}

void ShadColorSwatches::focusInEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusInEvent(event);
    update();
}

void ShadColorSwatches::focusOutEvent(QFocusEvent* event)
{
    _isFocusVisible = ShadStyleHelper::resolveFocusVisible(_isFocusVisible, event);
    QWidget::focusOutEvent(event);
    update();
}

void ShadColorSwatches::keyPressEvent(QKeyEvent* event)
{
    if (_colors.isEmpty())
    {
        QWidget::keyPressEvent(event);
        return;
    }
    // RTL 下色块从右向左排列，左右方向键对调
    const int forward = layoutDirection() == Qt::RightToLeft ? -1 : 1;
    switch (event->key())
    {
        case Qt::Key_Left:
            setCursorIndex(_cursorIndex - forward);
            break;
        case Qt::Key_Right:
            setCursorIndex(_cursorIndex + forward);
            break;
        case Qt::Key_Up:
            setCursorIndex(_cursorIndex - _columns);
            break;
        case Qt::Key_Down:
            setCursorIndex(_cursorIndex + _columns);
            break;
        case Qt::Key_Home:
            setCursorIndex(0);
            break;
        case Qt::Key_End:
            setCursorIndex(_colors.size() - 1);
            break;
        case Qt::Key_Return:
        case Qt::Key_Enter:
        case Qt::Key_Space:
            event->accept();
            activate(_cursorIndex);
            return;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    // 方向键操作视为键盘导航，显示焦点环
    _isFocusVisible = true;
    event->accept();
    update();
}

void ShadColorSwatches::leaveEvent(QEvent* event)
{
    _hoveredIndex = -1;
    update();
    QWidget::leaveEvent(event);
}

void ShadColorSwatches::mouseMoveEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (index != _hoveredIndex)
    {
        _hoveredIndex = index;
        setCursor(index >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ShadColorSwatches::mousePressEvent(QMouseEvent* event)
{
    const int index = indexAt(event->position().toPoint());
    if (event->button() != Qt::LeftButton || index < 0)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    event->accept();
    _cursorIndex = index;
    activate(index);
}

void ShadColorSwatches::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    const QColor border = shadTheme->getColor(ShadThemeType::Border);
    const qreal radius = kSwatchDiameter / 2.0;
    for (int index = 0; index < _colors.size(); ++index)
    {
        const QPointF center = QRectF(cellRect(index)).center();
        const QRectF swatchRect(center.x() - radius, center.y() - radius, kSwatchDiameter, kSwatchDiameter);
        const QColor color = _colors.at(index);
        if (_isFocusVisible && hasFocus() && index == _cursorIndex)
        {
            drawFocusRing(&painter, swatchRect, radius, focusRingColor(false));
        }
        if (color.isValid())
        {
            painter.setPen(QPen(border, 1.0));
            painter.setBrush(color);
            painter.drawEllipse(swatchRect.adjusted(0.5, 0.5, -0.5, -0.5));
        }
        else
        {
            // 空槽：虚线圆
            QPen dashedPen(withAlpha(shadTheme->getColor(ShadThemeType::MutedForeground), 0.5), 1.0);
            dashedPen.setStyle(Qt::DashLine);
            painter.setPen(dashedPen);
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(swatchRect.adjusted(0.5, 0.5, -0.5, -0.5));
        }
        const bool isSelected = _isSelectable && index == _selectedIndex;
        if (isSelected || index == _hoveredIndex)
        {
            const qreal outlineRadius = radius + kOutlineGap + kOutlineWidth / 2.0;
            painter.setPen(QPen(isSelected ? shadTheme->getColor(ShadThemeType::Primary) : border, kOutlineWidth));
            painter.setBrush(Qt::NoBrush);
            painter.drawEllipse(center, outlineRadius, outlineRadius);
        }
    }
}

QRect ShadColorSwatches::cellRect(int index) const
{
    const QRect logicalRect((index % _columns) * kCellSize, (index / _columns) * kCellSize, kCellSize, kCellSize);
    return ShadStyleHelper::visualRect(layoutDirection(), rect(), logicalRect);
}

int ShadColorSwatches::indexAt(const QPoint& position) const
{
    const qreal radius = kSwatchDiameter / 2.0 + kOutlineGap + kOutlineWidth;
    for (int index = 0; index < _colors.size(); ++index)
    {
        const QPointF offset = QPointF(position) - QRectF(cellRect(index)).center();
        if (offset.x() * offset.x() + offset.y() * offset.y() <= radius * radius)
        {
            return index;
        }
    }
    return -1;
}

void ShadColorSwatches::activate(int index)
{
    if (index < 0 || index >= _colors.size())
    {
        return;
    }
    if (_isSelectable)
    {
        setSelectedIndex(index);
    }
    update();
    Q_EMIT swatchActivated(index);
}

void ShadColorSwatches::setCursorIndex(int index)
{
    _cursorIndex = qBound(0, index, _colors.size() - 1);
}
