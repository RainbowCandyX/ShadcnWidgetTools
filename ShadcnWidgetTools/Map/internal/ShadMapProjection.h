#ifndef SHADMAPPROJECTION_H
#define SHADMAPPROJECTION_H

#include <QPointF>
#include <QtGlobal>
#include <QtMath>

#include <cmath>

#include "ShadMapDef.h"

namespace ShadMapProjection
{
    constexpr int kTileSize = 256;
    constexpr double kMaxLatitude = 85.05112878;

    inline QPointF project(const ShadMapCoordinate& coordinate)
    {
        const double latitude = qBound(-kMaxLatitude, coordinate.latitude, kMaxLatitude);
        const double sine = std::sin(qDegreesToRadians(latitude));
        return QPointF((coordinate.longitude + 180.0) / 360.0, 0.5 - std::log((1.0 + sine) / (1.0 - sine)) / (4.0 * M_PI));
    }

    inline ShadMapCoordinate unproject(const QPointF& point)
    {
        const double latitude = qRadiansToDegrees(std::atan(std::sinh(M_PI - 2.0 * M_PI * point.y())));
        return ShadMapCoordinate(latitude, point.x() * 360.0 - 180.0);
    }

    inline double worldSize(double zoom)
    {
        return kTileSize * std::pow(2.0, zoom);
    }

    inline double wrap(double value)
    {
        const double wrapped = std::fmod(value, 1.0);
        return wrapped < 0.0 ? wrapped + 1.0 : wrapped;
    }

    struct GlobeView
    {
        double longitude{0.0};
        double sinLatitude{0.0};
        double cosLatitude{1.0};
        double radius{1.0};
        double cosBearing{1.0};
        double sinBearing{0.0};
        QPointF origin;
    };

    inline double longitudeOf(double x)
    {
        return (x - 0.5) * 2.0 * M_PI;
    }

    inline double latitudeOf(double y)
    {
        return std::atan(std::sinh(M_PI * (1.0 - 2.0 * y)));
    }

    inline QPointF normalizedOf(double longitude, double latitude)
    {
        const double limit = qDegreesToRadians(kMaxLatitude);
        return QPointF(longitude / (2.0 * M_PI) + 0.5, 0.5 - std::asinh(std::tan(qBound(-limit, latitude, limit))) / (2.0 * M_PI));
    }

    inline GlobeView globeView(const QPointF& center, double zoom, const QPointF& origin, double bearing)
    {
        const double latitude = latitudeOf(center.y());
        GlobeView view;
        view.longitude = longitudeOf(center.x());
        view.sinLatitude = std::sin(latitude);
        view.cosLatitude = std::cos(latitude);
        view.radius = worldSize(zoom) / (2.0 * M_PI);
        view.origin = origin;
        view.cosBearing = std::cos(qDegreesToRadians(bearing));
        view.sinBearing = std::sin(qDegreesToRadians(bearing));
        return view;
    }

    inline bool projectGlobe(const GlobeView& view, double longitude, double latitude, QPointF& screen, double threshold = 0.0)
    {
        const double delta = longitude - view.longitude;
        const double sinLatitude = std::sin(latitude);
        const double cosLatitude = std::cos(latitude);
        const double cosDelta = std::cos(delta);
        const double offsetX = view.radius * cosLatitude * std::sin(delta);
        const double offsetY = -view.radius * (view.cosLatitude * sinLatitude - view.sinLatitude * cosLatitude * cosDelta);
        screen = QPointF(view.origin.x() + offsetX * view.cosBearing + offsetY * view.sinBearing, view.origin.y() - offsetX * view.sinBearing + offsetY * view.cosBearing);
        return view.sinLatitude * sinLatitude + view.cosLatitude * cosLatitude * cosDelta > threshold;
    }

    inline bool unprojectGlobe(const GlobeView& view, const QPointF& screen, double& longitude, double& latitude)
    {
        const double screenX = screen.x() - view.origin.x();
        const double screenY = screen.y() - view.origin.y();
        const double x = (screenX * view.cosBearing - screenY * view.sinBearing) / view.radius;
        const double y = -(screenX * view.sinBearing + screenY * view.cosBearing) / view.radius;
        const double squared = x * x + y * y;
        if (squared > 1.0)
        {
            return false;
        }
        const double depth = std::sqrt(1.0 - squared);
        latitude = std::asin(qBound(-1.0, depth * view.sinLatitude + y * view.cosLatitude, 1.0));
        longitude = view.longitude + std::atan2(x, depth * view.cosLatitude - y * view.sinLatitude);
        return true;
    }
} // namespace ShadMapProjection

#endif // SHADMAPPROJECTION_H
