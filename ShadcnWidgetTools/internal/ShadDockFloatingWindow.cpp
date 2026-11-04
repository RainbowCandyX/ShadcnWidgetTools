#include "ShadDockFloatingWindow.h"

#include <QApplication>
#include <QEvent>
#include <QMouseEvent>
#include <QString>
#include <QVBoxLayout>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadDockContainer.h"
#include "ShadTitleBar.h"

#ifdef Q_OS_WIN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace
{
    constexpr int kTitleBarHeight = 32;
} // namespace

ShadDockFloatingWindow::ShadDockFloatingWindow(QWidget* parent)
    : ShadWidget(parent)
{
    setAttribute(Qt::WA_QuitOnClose, false);
    setTitleBarHeight(kTitleBarHeight);
    setButtonFlags(ShadWindowType::MaximizeButton | ShadWindowType::CloseButton);
    // 关闭由停靠管理器处理
    setDefaultCloseEnabled(false);
    QVBoxLayout* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    _container = new ShadDockContainer(this);
    layout->addWidget(_container);
    _titleBar = findChild<ShadTitleBar*>(QString(), Qt::FindDirectChildrenOnly);
    if (_titleBar != nullptr)
    {
        _titleBar->installEventFilter(this);
    }
}

ShadDockFloatingWindow::~ShadDockFloatingWindow()
{
}

ShadDockContainer* ShadDockFloatingWindow::container() const
{
    return _container;
}

bool ShadDockFloatingWindow::event(QEvent* event)
{
    if (event->type() == QEvent::WindowActivate)
    {
        Q_EMIT activated();
    }
    return ShadWidget::event(event);
}

bool ShadDockFloatingWindow::eventFilter(QObject* watched, QEvent* event)
{
    if (watched != _titleBar)
    {
        return ShadWidget::eventFilter(watched, event);
    }
    switch (event->type())
    {
        case QEvent::MouseButtonPress:
            {
                const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                _isTitlePressed = mouseEvent->button() == Qt::LeftButton && !isMaximized() && !isFullScreen() && _titleBar->isCaptionAt(mouseEvent->position().toPoint());
                _titlePressGlobalPos = mouseEvent->globalPosition().toPoint();
                break;
            }
        case QEvent::MouseMove:
            {
                const QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (_isTitlePressed && mouseEvent->buttons().testFlag(Qt::LeftButton) && (mouseEvent->globalPosition().toPoint() - _titlePressGlobalPos).manhattanLength() >= QApplication::startDragDistance())
                {
                    _isTitlePressed = false;
                    // 改由停靠管理器移动窗口
                    Q_EMIT titleBarDragRequested(_titlePressGlobalPos);
                    return true;
                }
                break;
            }
        case QEvent::MouseButtonRelease:
        case QEvent::MouseButtonDblClick:
            _isTitlePressed = false;
            break;
        default:
            break;
    }
    return ShadWidget::eventFilter(watched, event);
}

bool ShadDockFloatingWindow::nativeEvent(const QByteArray& eventType, void* message, qintptr* result)
{
#ifdef Q_OS_WIN
    if (eventType == QByteArrayLiteral("windows_generic_MSG"))
    {
        const MSG* msg = static_cast<const MSG*>(message);
        if (msg->message == WM_MOVING)
        {
            Q_EMIT systemMoving();
        }
        else if (msg->message == WM_EXITSIZEMOVE)
        {
            Q_EMIT systemMoveFinished();
        }
    }
#endif
    return ShadWidget::nativeEvent(eventType, message, result);
}
