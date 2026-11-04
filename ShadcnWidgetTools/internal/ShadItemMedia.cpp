#include "ShadItemMedia.h"

#include <QEvent>
#include <QMargins>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QtCore/qnamespace.h>

#include "ShadIcon.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // variant="icon"：size-4
    constexpr int kIconSize = 16;

    // variant="image"：size-10 / size-8 / size-6
    int imageSide(ShadItemType::Size size)
    {
        switch (size)
        {
            case ShadItemType::Small:
                return 32;
            case ShadItemType::ExtraSmall:
                return 24;
            case ShadItemType::Medium:
                break;
        }
        return 40;
    }
} // namespace

ShadItemMedia::ShadItemMedia(QWidget* parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadItemMedia::~ShadItemMedia()
{
    if (!_widget.isNull())
    {
        _widget->removeEventFilter(this);
    }
}

void ShadItemMedia::setIcon(ShadIconType::IconName icon)
{
    releaseWidget();
    _pixmap = QPixmap();
    _icon = icon;
    updateGeometry();
    update();
}

ShadIconType::IconName ShadItemMedia::getIcon() const
{
    return _icon;
}

void ShadItemMedia::setPixmap(const QPixmap& pixmap)
{
    releaseWidget();
    _icon = ShadIconType::None;
    _pixmap = pixmap;
    updateGeometry();
    update();
}

QPixmap ShadItemMedia::getPixmap() const
{
    return _pixmap;
}

void ShadItemMedia::setWidget(QWidget* widget)
{
    if (_widget.data() == widget)
    {
        return;
    }
    releaseWidget();
    _icon = ShadIconType::None;
    _pixmap = QPixmap();
    _widget = widget;
    if (widget != nullptr)
    {
        widget->setParent(this);
        // 组件尺寸变化时重新计算媒体区尺寸
        widget->installEventFilter(this);
        widget->show();
        placeWidget();
    }
    updateGeometry();
    update();
}

QWidget* ShadItemMedia::getWidget() const
{
    return _widget.data();
}

void ShadItemMedia::setItemSize(ShadItemType::Size size)
{
    if (_size == size)
    {
        return;
    }
    _size = size;
    updateGeometry();
    update();
}

void ShadItemMedia::setTopOffset(int offset)
{
    if (contentsMargins().top() == offset)
    {
        return;
    }
    setContentsMargins(0, offset, 0, 0);
    updateGeometry();
    placeWidget();
}

bool ShadItemMedia::hasContent() const
{
    return _icon != ShadIconType::None || !_pixmap.isNull() || !_widget.isNull();
}

QSize ShadItemMedia::sizeHint() const
{
    const QMargins margins = contentsMargins();
    const QSize size = contentSize();
    return QSize(size.width() + margins.left() + margins.right(), size.height() + margins.top() + margins.bottom());
}

QSize ShadItemMedia::minimumSizeHint() const
{
    return sizeHint();
}

bool ShadItemMedia::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == _widget.data() && event->type() == QEvent::LayoutRequest)
    {
        updateGeometry();
        placeWidget();
    }
    return QWidget::eventFilter(watched, event);
}

void ShadItemMedia::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    if (!_widget.isNull())
    {
        return;
    }
    const QRectF mediaRect(contentsRect());
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    if (!_pixmap.isNull())
    {
        // object-cover：按物理像素缩放到填满，再从中间裁剪
        const QSize pixelSize = (mediaRect.size() * devicePixelRatioF()).toSize();
        QPixmap scaled = _pixmap.scaled(pixelSize, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
        scaled.setDevicePixelRatio(1.0);
        const QRectF sourceRect((scaled.width() - pixelSize.width()) / 2, (scaled.height() - pixelSize.height()) / 2, pixelSize.width(), pixelSize.height());
        painter.setClipPath(ShadStyleHelper::roundedRectPath(mediaRect, shadTheme->getRadius(ShadThemeType::Small)));
        painter.drawPixmap(mediaRect, scaled, sourceRect);
        return;
    }
    if (_icon != ShadIconType::None)
    {
        ShadIcon::paint(&painter, mediaRect, _icon, shadTheme->getColor(ShadThemeType::Foreground), kIconSize);
    }
}

void ShadItemMedia::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    placeWidget();
}

void ShadItemMedia::releaseWidget()
{
    if (_widget.isNull())
    {
        return;
    }
    QWidget* oldWidget = _widget.data();
    _widget = nullptr;
    oldWidget->removeEventFilter(this);
    ShadStyleHelper::clearFocusWithin(oldWidget);
    oldWidget->hide();
    oldWidget->deleteLater();
}

QSize ShadItemMedia::contentSize() const
{
    if (!_widget.isNull())
    {
        return _widget->sizeHint();
    }
    if (!_pixmap.isNull())
    {
        const int side = imageSide(_size);
        return QSize(side, side);
    }
    if (_icon != ShadIconType::None)
    {
        return QSize(kIconSize, kIconSize);
    }
    return QSize(0, 0);
}

void ShadItemMedia::placeWidget()
{
    if (_widget.isNull())
    {
        return;
    }
    QRect widgetRect(QPoint(0, 0), _widget->sizeHint());
    widgetRect.moveCenter(contentsRect().center());
    _widget->setGeometry(widgetRect);
}
