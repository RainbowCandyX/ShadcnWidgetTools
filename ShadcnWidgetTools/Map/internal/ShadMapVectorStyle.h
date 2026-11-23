#ifndef SHADMAPVECTORSTYLE_H
#define SHADMAPVECTORSTYLE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPixmap>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QtGlobal>

#include <functional>

#include "ShadDef.h"

class QPainter;
class ShadMapVectorTile;

namespace ShadMapVectorStyle
{
    struct Palette
    {
        QColor land;
        QColor water;
        QColor waterLine;
        QColor park;
        QColor wood;
        QColor sand;
        QColor ice;
        QColor landuse;
        QColor building;
        QColor buildingOutline;
        QColor roadMinor;
        QColor roadMajor;
        QColor roadCasing;
        QColor motorway;
        QColor rail;
        QColor boundary;
        QColor labelPrimary;
        QColor labelSecondary;
        QColor labelWater;
        QColor halo;
    };

    enum LabelKind
    {
        Country = 0,
        State,
        City,
        Town,
        Village,
        Suburb,
        Water,
        Road,
        LabelKindCount,
    };

    struct Label
    {
        QString text;
        QPointF position;
        LabelKind kind{City};
        int priority{0};
        qreal angle{0.0};
        qreal length{0.0};
    };

    Palette palette(ShadThemeType::ThemeMode mode);
    QStringList nameKeys();
    QPixmap renderTile(const ShadMapVectorTile& tile, const QRectF& sourceRect, int logicalSize, qreal devicePixelRatio, int zoom, const Palette& palette);
    using PointMapper = std::function<bool(const QPointF& point, QPointF& screen)>;

    void collectLabels(const ShadMapVectorTile& tile, const QRectF& sourceRect, const PointMapper& mapper, int zoom, const QStringList& nameKeys, QList<Label>& labels);
    void paintLabels(QPainter& painter, QList<Label>& labels, const QFont& baseFont, const QRectF& bounds, const Palette& palette);
} // namespace ShadMapVectorStyle

#endif // SHADMAPVECTORSTYLE_H
