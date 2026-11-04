#include "ShadDialogOverlay.h"

#include <QColor>
#include <QEvent>
#include <QMetaObject>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadWindowCorner.h"

namespace
{
    constexpr qreal kOverlayAlpha = 0.3;
} // namespace

ShadDialogOverlay::ShadDialogOverlay(QWidget* host)
    : QWidget(host), _host(host)
{
    setAttribute(Qt::WA_TransparentForMouseEvents);
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    setGeometry(host->rect());
    host->installEventFilter(this);
}

ShadDialogOverlay::~ShadDialogOverlay()
{
}

void ShadDialogOverlay::setProgress(qreal progress)
{
    _progress = qBound(0.0, progress, 1.0);
    update();
}

bool ShadDialogOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _host)
    {
        if (event->type() == QEvent::Resize)
        {
            setGeometry(_host->rect());
        }
        else if (event->type() == QEvent::ChildAdded)
        {
            // 新增子组件会叠在遮罩之上，添加过程中不能调整顺序，延后处理
            QMetaObject::invokeMethod(this, [this]() { bringToFront(); }, Qt::QueuedConnection);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadDialogOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (_progress <= 0.0)
    {
        return;
    }
    QPainter painter(this);
    painter.fillRect(rect(), QColor(0, 0, 0, qRound(255 * kOverlayAlpha * _progress)));
}

void ShadDialogOverlay::bringToFront()
{
    raise();
    // 窗口圆角需要盖在遮罩之上
    for (ShadWindowCorner* corner : _host->findChildren<ShadWindowCorner*>(QString(), Qt::FindDirectChildrenOnly))
    {
        corner->raise();
    }
}
