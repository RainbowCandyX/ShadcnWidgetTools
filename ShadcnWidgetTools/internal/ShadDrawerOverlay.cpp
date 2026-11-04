#include "ShadDrawerOverlay.h"

#include <QColor>
#include <QEvent>
#include <QImage>
#include <QList>
#include <QMetaObject>
#include <QMouseEvent>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPixmap>
#include <QPoint>
#include <QRegion>
#include <QSize>
#include <QString>
#include <QWheelEvent>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDrawer.h"
#include "ShadTheme.h"
#include "ShadWindowCorner.h"

namespace
{
    constexpr qreal kOverlayAlpha = 0.1;
    constexpr qreal kBlurRadius = 4.0;
} // namespace

ShadDrawerOverlay::ShadDrawerOverlay(QWidget* host)
    : QWidget(host), _host(host)
{
    setAttribute(Qt::WA_NoSystemBackground);
    setFocusPolicy(Qt::NoFocus);
    setGeometry(host->rect());
    host->installEventFilter(this);
    // 等其他组件刷新后再截图
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { QMetaObject::invokeMethod(this, [this]() { refreshBackdrop(); }, Qt::QueuedConnection); });
}

ShadDrawerOverlay::~ShadDrawerOverlay()
{
}

void ShadDrawerOverlay::setLevel(qreal level)
{
    const qreal bounded = qBound(0.0, level, 1.0);
    if (qFuzzyCompare(bounded + 1.0, _level + 1.0))
    {
        return;
    }
    _level = bounded;
    update();
}

void ShadDrawerOverlay::refreshBackdrop()
{
    if (_host.isNull() || parentWidget() != _host)
    {
        return;
    }
    const QSize size = _host->size();
    if (size.isEmpty())
    {
        _backdrop = QPixmap();
        return;
    }
    const qreal ratio = _host->devicePixelRatioF();
    QPixmap pixmap(size * ratio);
    pixmap.setDevicePixelRatio(ratio);
    pixmap.fill(Qt::transparent);
    {
        QPainter painter(&pixmap);
        _host->render(&painter, QPoint(), QRegion(), QWidget::RenderFlags());
        const QList<QWidget*> children = _host->findChildren<QWidget*>(QString(), Qt::FindDirectChildrenOnly);
        for (QWidget* child : children)
        {
            if (child == this || child->isWindow() || !child->isVisible() || qobject_cast<ShadDrawer*>(child) != nullptr || qobject_cast<ShadWindowCorner*>(child) != nullptr)
            {
                continue;
            }
            child->render(&painter, child->pos(), QRegion(), QWidget::DrawChildren);
        }
    }
    // 缩小再放大近似模糊
    const int factor = qMax(1, qRound(kBlurRadius * ratio));
    QImage image = pixmap.toImage();
    const QSize fullSize = image.size();
    image = image.scaled((fullSize / factor).expandedTo(QSize(1, 1)), Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    image = image.scaled(fullSize, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    _backdrop = QPixmap::fromImage(image);
    _backdrop.setDevicePixelRatio(ratio);
    update();
}

bool ShadDrawerOverlay::eventFilter(QObject* watched, QEvent* event)
{
    if (!_host.isNull() && watched == _host && parentWidget() == _host && event->type() == QEvent::Resize)
    {
        setGeometry(_host->rect());
        refreshBackdrop();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadDrawerOverlay::mousePressEvent(QMouseEvent* event)
{
    event->accept();
    Q_EMIT pressed();
}

void ShadDrawerOverlay::mouseReleaseEvent(QMouseEvent* event)
{
    event->accept();
}

void ShadDrawerOverlay::mouseDoubleClickEvent(QMouseEvent* event)
{
    event->accept();
}

void ShadDrawerOverlay::wheelEvent(QWheelEvent* event)
{
    event->accept();
}

void ShadDrawerOverlay::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (_level <= 0.0)
    {
        return;
    }
    QPainter painter(this);
    painter.setOpacity(_level);
    if (!_backdrop.isNull())
    {
        painter.drawPixmap(0, 0, _backdrop);
    }
    painter.fillRect(rect(), QColor(0, 0, 0, qRound(255 * kOverlayAlpha)));
}
