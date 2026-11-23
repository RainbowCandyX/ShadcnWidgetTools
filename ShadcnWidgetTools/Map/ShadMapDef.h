#ifndef SHADMAPDEF_H
#define SHADMAPDEF_H

#include <QMetaType>
#include <QObject>
#include <QString>

#define SHADCN_MAP_EXPORT

struct ShadMapCoordinate
{
    double latitude{0.0};
    double longitude{0.0};

    ShadMapCoordinate() = default;

    ShadMapCoordinate(double latitudeValue, double longitudeValue)
        : latitude(latitudeValue), longitude(longitudeValue)
    {
    }

    bool operator==(const ShadMapCoordinate& other) const
    {
        return latitude == other.latitude && longitude == other.longitude;
    }

    bool operator!=(const ShadMapCoordinate& other) const
    {
        return !(*this == other);
    }
};

Q_DECLARE_METATYPE(ShadMapCoordinate)

struct ShadMapArcData
{
    QString id;
    ShadMapCoordinate from;
    ShadMapCoordinate to;
};

namespace ShadMapType
{
    Q_NAMESPACE

    enum ControlPosition
    {
        TopLeft = 0,
        TopRight,
        BottomLeft,
        BottomRight,
    };

    Q_ENUM_NS(ControlPosition)

    enum LabelPosition
    {
        Top = 0,
        Bottom,
    };

    Q_ENUM_NS(LabelPosition)

    enum TileFormat
    {
        Raster = 0,
        Vector,
    };

    Q_ENUM_NS(TileFormat)

    enum Projection
    {
        Mercator = 0,
        Globe,
    };

    Q_ENUM_NS(Projection)

    enum Theme
    {
        Auto = 0,
        Light,
        Dark,
    };

    Q_ENUM_NS(Theme)
} // namespace ShadMapType

#endif // SHADMAPDEF_H
