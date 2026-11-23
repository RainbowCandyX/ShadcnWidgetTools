#include "ShadMapCompassButton.h"

#include <QPaintEvent>
#include <QPainter>
#include <QPointF>
#include <QPolygonF>
#include <QRectF>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadTheme.h"

namespace
{
    constexpr qreal kNeedleLength = 7.0;
    constexpr qreal kNeedleHalfWidth = 3.0;
} // namespace

ShadMapCompassButton::ShadMapCompassButton(QWidget* parent)
    : ShadButton(parent)
{
}

ShadMapCompassButton::~ShadMapCompassButton()
{
}

void ShadMapCompassButton::setBearing(double bearing)
{
    if (qFuzzyCompare(bearing + 360.0, _bearing + 360.0))
    {
        return;
    }
    _bearing = bearing;
    update();
}

double ShadMapCompassButton::getBearing() const
{
    return _bearing;
}

void ShadMapCompassButton::paintEvent(QPaintEvent* event)
{
    ShadButton::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.translate(QRectF(rect()).center());
    painter.rotate(-_bearing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(shadTheme->getColor(ShadThemeType::Destructive));
    painter.drawPolygon(QPolygonF{QPointF(0.0, -kNeedleLength), QPointF(-kNeedleHalfWidth, 0.0), QPointF(kNeedleHalfWidth, 0.0)});
    painter.setBrush(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter.drawPolygon(QPolygonF{QPointF(0.0, kNeedleLength), QPointF(-kNeedleHalfWidth, 0.0), QPointF(kNeedleHalfWidth, 0.0)});
}
