#include "ShadMapRoute.h"

ShadMapRoute::ShadMapRoute(const QList<ShadMapCoordinate>& coordinates, QObject* parent)
    : QObject(parent), _coordinates(coordinates)
{
}

ShadMapRoute::~ShadMapRoute()
{
}

void ShadMapRoute::setCoordinates(const QList<ShadMapCoordinate>& coordinates)
{
    _coordinates = coordinates;
    Q_EMIT changed();
}

QList<ShadMapCoordinate> ShadMapRoute::getCoordinates() const
{
    return _coordinates;
}

void ShadMapRoute::setColor(const QColor& color)
{
    _color = color;
    Q_EMIT changed();
}

QColor ShadMapRoute::getColor() const
{
    return _color;
}

void ShadMapRoute::setWidth(qreal width)
{
    _width = qMax<qreal>(0.5, width);
    Q_EMIT changed();
}

qreal ShadMapRoute::getWidth() const
{
    return _width;
}

void ShadMapRoute::setOpacity(qreal opacity)
{
    _opacity = qBound<qreal>(0.0, opacity, 1.0);
    Q_EMIT changed();
}

qreal ShadMapRoute::getOpacity() const
{
    return _opacity;
}

void ShadMapRoute::setDashPattern(const QList<qreal>& pattern)
{
    _dashPattern = pattern;
    Q_EMIT changed();
}

QList<qreal> ShadMapRoute::getDashPattern() const
{
    return _dashPattern;
}

ShadMapArc::ShadMapArc(const ShadMapCoordinate& from, const ShadMapCoordinate& to, QObject* parent)
    : QObject(parent), _from(from), _to(to)
{
}

ShadMapArc::~ShadMapArc()
{
}

void ShadMapArc::setId(const QString& id)
{
    _id = id;
}

QString ShadMapArc::getId() const
{
    return _id;
}

void ShadMapArc::setEndpoints(const ShadMapCoordinate& from, const ShadMapCoordinate& to)
{
    _from = from;
    _to = to;
    Q_EMIT changed();
}

ShadMapCoordinate ShadMapArc::getFrom() const
{
    return _from;
}

ShadMapCoordinate ShadMapArc::getTo() const
{
    return _to;
}

void ShadMapArc::setColor(const QColor& color)
{
    _color = color;
    Q_EMIT changed();
}

QColor ShadMapArc::getColor() const
{
    return _color;
}

void ShadMapArc::setWidth(qreal width)
{
    _width = qMax<qreal>(0.5, width);
    Q_EMIT changed();
}

qreal ShadMapArc::getWidth() const
{
    return _width;
}

void ShadMapArc::setCurvature(qreal curvature)
{
    _curvature = curvature;
    Q_EMIT changed();
}

qreal ShadMapArc::getCurvature() const
{
    return _curvature;
}
