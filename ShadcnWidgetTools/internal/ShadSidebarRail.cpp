#include "ShadSidebarRail.h"

#include <QApplication>
#include <QCursor>
#include <QEnterEvent>
#include <QEvent>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // after:w-[2px]
    constexpr int kLineWidth = 2;
} // namespace

ShadSidebarRail::ShadSidebarRail(QWidget* parent)
    : QWidget(parent)
{
    setCursor(Qt::SizeHorCursor);
    setFocusPolicy(Qt::NoFocus);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSidebarRail::~ShadSidebarRail()
{
}

void ShadSidebarRail::setLineAtStart(bool lineAtStart)
{
    _isLineAtStart = lineAtStart;
    update();
}

void ShadSidebarRail::enterEvent(QEnterEvent* event)
{
    _isHovered = true;
    update();
    QWidget::enterEvent(event);
}

void ShadSidebarRail::leaveEvent(QEvent* event)
{
    _isHovered = false;
    update();
    QWidget::leaveEvent(event);
}

void ShadSidebarRail::setDragEnabled(bool enabled)
{
    if (_isDragEnabled == enabled)
    {
        return;
    }
    _isDragEnabled = enabled;
}

void ShadSidebarRail::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton)
    {
        _isPressed = true;
        _isDragging = false;
        _pressGlobalX = qRound(event->globalPosition().x());
        event->accept();
        return;
    }
    QWidget::mousePressEvent(event);
}

void ShadSidebarRail::mouseMoveEvent(QMouseEvent* event)
{
    if (!_isPressed || !_isDragEnabled)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const int deltaX = qRound(event->globalPosition().x()) - _pressGlobalX;
    if (!_isDragging && qAbs(deltaX) >= QApplication::startDragDistance())
    {
        _isDragging = true;
        Q_EMIT dragStarted();
    }
    if (_isDragging)
    {
        Q_EMIT dragMoved(deltaX);
    }
    event->accept();
}

void ShadSidebarRail::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton || !_isPressed)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    _isPressed = false;
    if (_isDragging)
    {
        _isDragging = false;
        // 拖动期间光标可能已离开切换条，松开后按实际位置更新悬停状态
        _isHovered = rect().contains(event->position().toPoint());
        update();
        Q_EMIT dragFinished();
    }
    else if (rect().contains(event->position().toPoint()))
    {
        Q_EMIT clicked();
    }
    event->accept();
}

void ShadSidebarRail::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    // 拖动期间光标可能移出切换条，竖线保持显示
    if (!_isHovered && !_isDragging)
    {
        return;
    }
    QPainter painter(this);
    const QRect logicalLine(_isLineAtStart ? 0 : width() - kLineWidth, 0, kLineWidth, height());
    painter.fillRect(ShadStyleHelper::visualRect(layoutDirection(), rect(), logicalLine), shadTheme->getColor(ShadThemeType::SidebarBorder));
}
