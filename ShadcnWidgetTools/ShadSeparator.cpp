#include "ShadSeparator.h"

#include <QColor>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QSizePolicy>

#include "ShadSeparatorPrivate.h"
#include "ShadTheme.h"

ShadSeparator::ShadSeparator(QWidget* parent)
    : ShadSeparator(Qt::Horizontal, parent)
{
}

ShadSeparator::ShadSeparator(Qt::Orientation orientation, QWidget* parent)
    : QWidget(parent), d_ptr(new ShadSeparatorPrivate())
{
    Q_D(ShadSeparator);
    d->q_ptr = this;
    d->_orientation = orientation;
    if (orientation == Qt::Horizontal)
    {
        setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
    }
    else
    {
        setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Expanding);
    }
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadSeparator::~ShadSeparator()
{
}

void ShadSeparator::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadSeparator);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    setSizePolicy(sizePolicy().transposed());
    updateGeometry();
    update();
}

Qt::Orientation ShadSeparator::getOrientation() const
{
    Q_D(const ShadSeparator);
    return d->_orientation;
}

QSize ShadSeparator::sizeHint() const
{
    return QSize(1, 1);
}

QSize ShadSeparator::minimumSizeHint() const
{
    return QSize(1, 1);
}

void ShadSeparator::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadSeparator);
    QPainter painter(this);
    const QColor color = shadTheme->getColor(ShadThemeType::Border);
    if (d->_orientation == Qt::Horizontal)
    {
        painter.fillRect(QRect(0, (height() - 1) / 2, width(), 1), color);
    }
    else
    {
        painter.fillRect(QRect((width() - 1) / 2, 0, 1, height()), color);
    }
}
