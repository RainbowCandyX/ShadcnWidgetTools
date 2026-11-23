#include "ShadMapVectorStyle.h"

#include <QFontMetricsF>
#include <QHash>
#include <QLatin1Char>
#include <QLatin1String>
#include <QLocale>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPolygonF>
#include <QTransform>
#include <QVariant>
#include <QtCore/qnamespace.h>
#include <QtMath>

#include <algorithm>
#include <cmath>
#include <functional>
#include <utility>

#include "ShadDef.h"
#include "ShadMapVectorTile.h"
#include "ShadTheme.h"

namespace
{
    using Layer = ShadMapVectorTile::Layer;
    using Feature = ShadMapVectorTile::Feature;

    constexpr int kMaxLabels = 250;
    constexpr int kNoMaximumZoom = 99;
    constexpr int kDefaultRank = 50;
    constexpr int kBuildingZoom = 14;
    constexpr int kBuildingOutlineZoom = 16;
    constexpr int kCasingZoom = 12;
    constexpr int kRailZoom = 11;
    constexpr qreal kClipMargin = 0.1;
    constexpr qreal kHaloWidth = 3.0;
    constexpr qreal kLabelPaddingX = 4.0;
    constexpr qreal kLabelPaddingY = 2.0;

    struct RoadStyle
    {
        const char* name;
        int minimumZoom;
        qreal width;
        bool isMajor;
        bool isDashed;
    };

    constexpr RoadStyle kRoadStyles[] = {
        {"path", 16, 1.0, false, true},
        {"track", 15, 1.2, false, true},
        {"service", 15, 1.4, false, false},
        {"minor", 13, 2.4, false, false},
        {"tertiary", 11, 3.2, false, false},
        {"secondary", 9, 3.8, true, false},
        {"primary", 8, 4.5, true, false},
        {"trunk", 6, 5.0, true, false},
        {"motorway", 5, 6.0, true, false},
    };

    constexpr int kRoadStyleCount = int(sizeof(kRoadStyles) / sizeof(kRoadStyles[0]));

    struct PlaceRule
    {
        const char* name;
        ShadMapVectorStyle::LabelKind kind;
        int minimumZoom;
        int maximumZoom;
    };

    constexpr PlaceRule kPlaceRules[] = {
        {"country", ShadMapVectorStyle::Country, 2, 7},
        {"state", ShadMapVectorStyle::State, 5, 9},
        {"province", ShadMapVectorStyle::State, 5, 9},
        {"city", ShadMapVectorStyle::City, 4, kNoMaximumZoom},
        {"town", ShadMapVectorStyle::Town, 8, kNoMaximumZoom},
        {"village", ShadMapVectorStyle::Village, 11, kNoMaximumZoom},
        {"hamlet", ShadMapVectorStyle::Village, 13, kNoMaximumZoom},
        {"suburb", ShadMapVectorStyle::Suburb, 12, kNoMaximumZoom},
        {"quarter", ShadMapVectorStyle::Suburb, 13, kNoMaximumZoom},
        {"neighbourhood", ShadMapVectorStyle::Suburb, 14, kNoMaximumZoom},
    };

    struct LabelStyle
    {
        int pixelSize;
        QFont::Weight weight;
        bool isItalic;
        int colorIndex;
    };

    constexpr LabelStyle kLabelStyles[ShadMapVectorStyle::LabelKindCount] = {
        {14, QFont::DemiBold, false, 0},
        {12, QFont::Medium, false, 1},
        {13, QFont::Medium, false, 0},
        {12, QFont::Medium, false, 0},
        {11, QFont::Normal, false, 0},
        {11, QFont::Normal, false, 1},
        {12, QFont::Normal, true, 2},
        {11, QFont::Normal, false, 1},
    };

    struct RoadLabelRule
    {
        const char* name;
        int minimumZoom;
    };

    constexpr RoadLabelRule kRoadLabelRules[] = {
        {"motorway", 12},
        {"trunk", 12},
        {"primary", 12},
        {"secondary", 13},
        {"tertiary", 14},
        {"minor", 15},
    };

    constexpr int kRoadLabelRuleCount = int(sizeof(kRoadLabelRules) / sizeof(kRoadLabelRules[0]));
    constexpr qreal kRoadMinimumLength = 48.0;
    constexpr qreal kRoadProbeDivisor = 128.0;
    constexpr qreal kRoadAngleWindow = 40.0;
    constexpr qreal kRoadStraightness = 0.9;
    constexpr qreal kRoadLabelMargin = 16.0;
    constexpr qreal kRoadRepeatDistance = 320.0;

    struct Context
    {
        const ShadMapVectorTile& tile;
        QRectF sourceRect;
        int logicalSize;
        int zoom;
    };

    QColor mix(const QColor& from, const QColor& to, qreal amount)
    {
        const qreal keep = 1.0 - amount;
        return QColor::fromRgbF(float(from.redF() * keep + to.redF() * amount), float(from.greenF() * keep + to.greenF() * amount), float(from.blueF() * keep + to.blueF() * amount), float(from.alphaF() * keep + to.alphaF() * amount));
    }

    qreal scaledWidth(qreal width, int zoom, qreal minimum)
    {
        return qBound(minimum, width * std::pow(2.0, (zoom - 14) * 0.6), width * 4.0);
    }

    QTransform layerTransform(const Context& context, double extent)
    {
        const double scale = context.logicalSize / (context.sourceRect.width() * extent);
        QTransform transform;
        transform.scale(scale, scale);
        transform.translate(-context.sourceRect.x() * extent, -context.sourceRect.y() * extent);
        return transform;
    }

    QRectF clipRect(const Context& context, double extent)
    {
        const QRectF rect(context.sourceRect.x() * extent, context.sourceRect.y() * extent, context.sourceRect.width() * extent, context.sourceRect.height() * extent);
        const qreal margin = rect.width() * kClipMargin;
        return rect.adjusted(-margin, -margin, margin, margin);
    }

    bool overlaps(const QRectF& bounds, const QRectF& clip)
    {
        return bounds.left() <= clip.right() && bounds.right() >= clip.left() && bounds.top() <= clip.bottom() && bounds.bottom() >= clip.top();
    }

    bool buildPath(const Feature& feature, const QRectF& clip, QPainterPath& path)
    {
        bool hasPart = false;
        for (const QPolygonF& part : feature.parts)
        {
            if (part.size() < 2 || !overlaps(part.boundingRect(), clip))
            {
                continue;
            }
            path.addPolygon(part);
            if (feature.type == ShadMapVectorTile::Polygon)
            {
                path.closeSubpath();
            }
            hasPart = true;
        }
        return hasPart;
    }

    QString stringProperty(const Layer& layer, const Feature& feature, const QString& key)
    {
        return layer.property(feature, key).toString();
    }

    void fillLayer(QPainter& painter, const Context& context, const QString& name, int minimumZoom, const std::function<QColor(const Layer&, const Feature&)>& colorFor)
    {
        const Layer* layer = context.tile.layer(name);
        if (layer == nullptr || context.zoom < minimumZoom)
        {
            return;
        }
        const QTransform transform = layerTransform(context, layer->extent);
        const QRectF clip = clipRect(context, layer->extent);
        for (const Feature& feature : layer->features)
        {
            if (feature.type != ShadMapVectorTile::Polygon)
            {
                continue;
            }
            const QColor color = colorFor(*layer, feature);
            if (!color.isValid())
            {
                continue;
            }
            QPainterPath path;
            path.setFillRule(Qt::OddEvenFill);
            if (buildPath(feature, clip, path))
            {
                painter.fillPath(transform.map(path), color);
            }
        }
    }

    void paintWaterways(QPainter& painter, const Context& context, const ShadMapVectorStyle::Palette& palette)
    {
        const Layer* layer = context.tile.layer(QStringLiteral("waterway"));
        if (layer == nullptr)
        {
            return;
        }
        const QTransform transform = layerTransform(context, layer->extent);
        const QRectF clip = clipRect(context, layer->extent);
        for (const Feature& feature : layer->features)
        {
            if (feature.type != ShadMapVectorTile::LineString)
            {
                continue;
            }
            const QString type = stringProperty(*layer, feature, QStringLiteral("class"));
            qreal width = 0.0;
            if (type == QStringLiteral("river") && context.zoom >= 9)
            {
                width = 1.4;
            }
            else if (type == QStringLiteral("canal") && context.zoom >= 12)
            {
                width = 1.2;
            }
            else if (context.zoom >= 14)
            {
                width = 0.8;
            }
            QPainterPath path;
            if (width > 0.0 && buildPath(feature, clip, path))
            {
                painter.strokePath(transform.map(path), QPen(palette.waterLine, scaledWidth(width, context.zoom, 0.5), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            }
        }
    }

    void paintBuildings(QPainter& painter, const Context& context, const ShadMapVectorStyle::Palette& palette)
    {
        const Layer* layer = context.tile.layer(QStringLiteral("building"));
        if (layer == nullptr || context.zoom < kBuildingZoom)
        {
            return;
        }
        const QTransform transform = layerTransform(context, layer->extent);
        const QRectF clip = clipRect(context, layer->extent);
        const bool hasOutline = context.zoom >= kBuildingOutlineZoom;
        painter.setPen(hasOutline ? QPen(palette.buildingOutline, 0.5) : QPen(Qt::NoPen));
        painter.setBrush(palette.building);
        for (const Feature& feature : layer->features)
        {
            if (feature.type != ShadMapVectorTile::Polygon)
            {
                continue;
            }
            QPainterPath path;
            path.setFillRule(Qt::OddEvenFill);
            if (buildPath(feature, clip, path))
            {
                painter.drawPath(transform.map(path));
            }
        }
        painter.setBrush(Qt::NoBrush);
        painter.setPen(Qt::NoPen);
    }

    void paintRoads(QPainter& painter, const Context& context, const ShadMapVectorStyle::Palette& palette)
    {
        const Layer* layer = context.tile.layer(QStringLiteral("transportation"));
        if (layer == nullptr)
        {
            return;
        }
        const QTransform transform = layerTransform(context, layer->extent);
        const QRectF clip = clipRect(context, layer->extent);
        QList<QPainterPath> roads(kRoadStyleCount);
        QPainterPath rails;
        for (const Feature& feature : layer->features)
        {
            if (feature.type != ShadMapVectorTile::LineString)
            {
                continue;
            }
            const QString type = stringProperty(*layer, feature, QStringLiteral("class"));
            if (type == QStringLiteral("rail") || type == QStringLiteral("transit"))
            {
                if (context.zoom >= kRailZoom)
                {
                    buildPath(feature, clip, rails);
                }
                continue;
            }
            for (int index = 0; index < kRoadStyleCount; ++index)
            {
                if (type == QLatin1String(kRoadStyles[index].name))
                {
                    if (context.zoom >= kRoadStyles[index].minimumZoom)
                    {
                        buildPath(feature, clip, roads[index]);
                    }
                    break;
                }
            }
        }
        if (!rails.isEmpty())
        {
            QPen railPen(palette.rail, scaledWidth(1.0, context.zoom, 0.6), Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
            painter.strokePath(transform.map(rails), railPen);
        }
        QList<QPainterPath> mapped(kRoadStyleCount);
        for (int index = 0; index < kRoadStyleCount; ++index)
        {
            if (!roads.at(index).isEmpty())
            {
                mapped[index] = transform.map(roads.at(index));
            }
        }
        if (context.zoom >= kCasingZoom)
        {
            for (int index = 0; index < kRoadStyleCount; ++index)
            {
                const RoadStyle& style = kRoadStyles[index];
                if (mapped.at(index).isEmpty() || style.isDashed)
                {
                    continue;
                }
                painter.strokePath(mapped.at(index), QPen(palette.roadCasing, scaledWidth(style.width, context.zoom, 0.6) + 2.0, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            }
        }
        for (int index = 0; index < kRoadStyleCount; ++index)
        {
            const RoadStyle& style = kRoadStyles[index];
            if (mapped.at(index).isEmpty())
            {
                continue;
            }
            const bool isHighway = index >= kRoadStyleCount - 2;
            const QColor color = isHighway ? palette.motorway : (style.isMajor ? palette.roadMajor : palette.roadMinor);
            QPen pen(color, scaledWidth(style.width, context.zoom, 0.6), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
            if (style.isDashed)
            {
                pen.setColor(palette.rail);
                pen.setCapStyle(Qt::FlatCap);
                pen.setDashPattern(QList<qreal>{2.0, 2.0});
            }
            painter.strokePath(mapped.at(index), pen);
        }
    }

    void paintBoundaries(QPainter& painter, const Context& context, const ShadMapVectorStyle::Palette& palette)
    {
        const Layer* layer = context.tile.layer(QStringLiteral("boundary"));
        if (layer == nullptr)
        {
            return;
        }
        const QTransform transform = layerTransform(context, layer->extent);
        const QRectF clip = clipRect(context, layer->extent);
        QPainterPath countries;
        QPainterPath states;
        for (const Feature& feature : layer->features)
        {
            if (feature.type != ShadMapVectorTile::LineString || layer->property(feature, QStringLiteral("maritime")).toInt() == 1)
            {
                continue;
            }
            const int level = layer->property(feature, QStringLiteral("admin_level")).toInt();
            if (level == 2)
            {
                buildPath(feature, clip, countries);
            }
            else if ((level == 3 || level == 4) && context.zoom >= 5)
            {
                buildPath(feature, clip, states);
            }
        }
        if (!states.isEmpty())
        {
            QPen pen(palette.boundary, 0.8, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
            pen.setDashPattern(QList<qreal>{3.0, 3.0});
            painter.strokePath(transform.map(states), pen);
        }
        if (!countries.isEmpty())
        {
            QPen pen(palette.boundary, 1.2, Qt::SolidLine, Qt::FlatCap, Qt::RoundJoin);
            pen.setDashPattern(QList<qreal>{4.0, 2.0});
            painter.strokePath(transform.map(countries), pen);
        }
    }

    QString chineseVariant(const QString& name)
    {
        QString normalized = name;
        normalized.replace(QLatin1Char(';'), QLatin1Char('/'));
        QStringList variants;
        const QStringList parts = normalized.split(QLatin1Char('/'), Qt::SkipEmptyParts);
        for (const QString& part : parts)
        {
            const QString variant = part.trimmed();
            if (!variant.isEmpty())
            {
                variants.append(variant);
            }
        }
        if (variants.size() < 2)
        {
            return name;
        }
        return variants.at(QLocale().script() == QLocale::TraditionalHanScript ? 1 : 0);
    }

    QString featureName(const Layer& layer, const Feature& feature, const QStringList& nameKeys)
    {
        for (const QString& key : nameKeys)
        {
            const QString name = stringProperty(layer, feature, key).trimmed();
            if (!name.isEmpty())
            {
                return key == QLatin1String("name:zh") ? chineseVariant(name) : name;
            }
        }
        return QString();
    }

    void appendLabel(const Layer& layer, const Feature& feature, ShadMapVectorStyle::LabelKind kind, const QRectF& sourceRect, const ShadMapVectorStyle::PointMapper& mapper, const QStringList& nameKeys, QList<ShadMapVectorStyle::Label>& labels)
    {
        if (feature.type != ShadMapVectorTile::Point || feature.parts.isEmpty() || feature.parts.first().isEmpty())
        {
            return;
        }
        const QPointF point = feature.parts.first().first();
        const qreal x = point.x() / layer.extent;
        const qreal y = point.y() / layer.extent;
        if (x < sourceRect.left() || x >= sourceRect.right() || y < sourceRect.top() || y >= sourceRect.bottom())
        {
            return;
        }
        QPointF position;
        if (!mapper(QPointF(x, y), position))
        {
            return;
        }
        const QString text = featureName(layer, feature, nameKeys);
        if (text.isEmpty())
        {
            return;
        }
        const QVariant rankValue = layer.property(feature, QStringLiteral("rank"));
        const int rank = rankValue.isValid() ? qBound(0, rankValue.toInt(), 999) : kDefaultRank;
        labels.append(ShadMapVectorStyle::Label{text, position, kind, int(kind) * 1000 + rank});
    }

    qreal polylineLength(const QPolygonF& line)
    {
        qreal length = 0.0;
        for (qsizetype index = 1; index < line.size(); ++index)
        {
            length += std::hypot(line.at(index).x() - line.at(index - 1).x(), line.at(index).y() - line.at(index - 1).y());
        }
        return length;
    }

    QPointF pointAlong(const QPolygonF& line, qreal distance)
    {
        for (qsizetype index = 1; index < line.size(); ++index)
        {
            const QPointF from = line.at(index - 1);
            const QPointF to = line.at(index);
            const qreal segment = std::hypot(to.x() - from.x(), to.y() - from.y());
            if (distance <= segment && segment > 0.0)
            {
                return from + (to - from) * (distance / segment);
            }
            distance -= segment;
        }
        return line.last();
    }

    void appendRoadLabel(const Layer& layer, const Feature& feature, int rank, const QRectF& sourceRect, const ShadMapVectorStyle::PointMapper& mapper, const QStringList& nameKeys, QList<ShadMapVectorStyle::Label>& labels)
    {
        const QPolygonF* longest = nullptr;
        qreal longestLength = 0.0;
        for (const QPolygonF& part : feature.parts)
        {
            const qreal length = part.size() < 2 ? 0.0 : polylineLength(part);
            if (length > longestLength)
            {
                longest = &part;
                longestLength = length;
            }
        }
        if (longest == nullptr)
        {
            return;
        }
        const QPointF middle = pointAlong(*longest, longestLength / 2.0);
        const qreal x = middle.x() / layer.extent;
        const qreal y = middle.y() / layer.extent;
        if (x < sourceRect.left() || x >= sourceRect.right() || y < sourceRect.top() || y >= sourceRect.bottom())
        {
            return;
        }
        const qreal probe = qMin(longestLength / 2.0, layer.extent / kRoadProbeDivisor);
        const QPointF probeBefore = pointAlong(*longest, longestLength / 2.0 - probe);
        const QPointF probeAfter = pointAlong(*longest, longestLength / 2.0 + probe);
        const qreal probeChord = std::hypot(probeAfter.x() - probeBefore.x(), probeAfter.y() - probeBefore.y());
        QPointF position;
        QPointF probeFrom;
        QPointF probeTo;
        if (probeChord <= 0.0 || !mapper(QPointF(x, y), position) || !mapper(probeBefore / layer.extent, probeFrom) || !mapper(probeAfter / layer.extent, probeTo))
        {
            return;
        }
        const qreal scale = std::hypot(probeTo.x() - probeFrom.x(), probeTo.y() - probeFrom.y()) / probeChord;
        const qreal screenLength = longestLength * scale;
        if (scale <= 0.0 || screenLength < kRoadMinimumLength)
        {
            return;
        }
        const qreal window = qMin(longestLength / 2.0, kRoadAngleWindow / scale);
        const QPointF before = pointAlong(*longest, longestLength / 2.0 - window);
        const QPointF after = pointAlong(*longest, longestLength / 2.0 + window);
        const qreal chord = std::hypot(after.x() - before.x(), after.y() - before.y());
        QPointF screenBefore;
        QPointF screenAfter;
        if (chord < 2.0 * window * kRoadStraightness || !mapper(before / layer.extent, screenBefore) || !mapper(after / layer.extent, screenAfter))
        {
            return;
        }
        const QString text = featureName(layer, feature, nameKeys);
        if (text.isEmpty())
        {
            return;
        }
        qreal angle = qRadiansToDegrees(std::atan2(screenAfter.y() - screenBefore.y(), screenAfter.x() - screenBefore.x()));
        if (angle > 90.0)
        {
            angle -= 180.0;
        }
        else if (angle < -90.0)
        {
            angle += 180.0;
        }
        const int priority = int(ShadMapVectorStyle::Road) * 1000 + rank * 100 + 99 - qMin(99, int(screenLength / 20.0));
        labels.append(ShadMapVectorStyle::Label{text, position, ShadMapVectorStyle::Road, priority, angle, screenLength});
    }
} // namespace

namespace ShadMapVectorStyle
{
    Palette palette(ShadThemeType::ThemeMode mode)
    {
        const bool isDark = mode == ShadThemeType::Dark;
        const QColor background = shadTheme->getColor(mode, ShadThemeType::Background);
        const QColor foreground = shadTheme->getColor(mode, ShadThemeType::Foreground);
        const QColor muted = shadTheme->getColor(mode, ShadThemeType::Muted);
        const QColor blue(0x3b, 0x82, 0xf6);
        const QColor green(0x22, 0xc5, 0x5e);
        const QColor amber(0xf5, 0x9e, 0x0b);
        Palette result;
        result.land = muted;
        result.water = mix(muted, blue, isDark ? 0.28 : 0.22);
        result.waterLine = mix(muted, blue, isDark ? 0.4 : 0.35);
        result.park = mix(muted, green, 0.14);
        result.wood = mix(muted, green, 0.2);
        result.sand = mix(muted, amber, 0.12);
        result.ice = mix(muted, background, 0.6);
        result.landuse = mix(muted, foreground, 0.03);
        result.building = mix(muted, foreground, isDark ? 0.1 : 0.07);
        result.buildingOutline = mix(muted, foreground, isDark ? 0.16 : 0.13);
        result.roadMinor = isDark ? mix(muted, foreground, 0.14) : background;
        result.roadMajor = isDark ? mix(muted, foreground, 0.24) : background;
        result.roadCasing = isDark ? mix(muted, background, 0.6) : shadTheme->getColor(mode, ShadThemeType::Border);
        result.motorway = isDark ? mix(muted, amber, 0.3) : mix(background, amber, 0.25);
        result.rail = mix(muted, foreground, 0.25);
        result.boundary = mix(muted, foreground, 0.35);
        result.labelPrimary = foreground;
        result.labelSecondary = shadTheme->getColor(mode, ShadThemeType::MutedForeground);
        result.labelWater = mix(foreground, blue, 0.55);
        result.halo = muted;
        return result;
    }

    QStringList nameKeys()
    {
        const QLocale locale;
        QStringList keys;
        if (locale.language() == QLocale::Chinese)
        {
            keys.append(locale.script() == QLocale::TraditionalHanScript ? QStringLiteral("name:zh-Hant") : QStringLiteral("name:zh-Hans"));
            keys.append(QStringLiteral("name:zh"));
        }
        else if (locale.language() != QLocale::C && locale.language() != QLocale::AnyLanguage)
        {
            const QString code = QLocale::languageToCode(locale.language());
            if (!code.isEmpty())
            {
                keys.append(QStringLiteral("name:") + code);
            }
            if (code == QStringLiteral("en"))
            {
                keys.append(QStringLiteral("name_en"));
            }
        }
        keys.append(QStringLiteral("name"));
        return keys;
    }

    QPixmap renderTile(const ShadMapVectorTile& tile, const QRectF& sourceRect, int logicalSize, qreal devicePixelRatio, int zoom, const Palette& palette)
    {
        const int pixelSize = qMax(1, qCeil(logicalSize * devicePixelRatio));
        QPixmap pixmap(pixelSize, pixelSize);
        pixmap.setDevicePixelRatio(devicePixelRatio);
        pixmap.fill(palette.land);
        const Context context{tile, sourceRect, logicalSize, zoom};
        QPainter painter(&pixmap);
        painter.setRenderHint(QPainter::Antialiasing, true);
        painter.setPen(Qt::NoPen);
        fillLayer(painter, context, QStringLiteral("landcover"), 0, [&palette](const Layer& layer, const Feature& feature)
        {
            const QString type = stringProperty(layer, feature, QStringLiteral("class"));
            if (type == QStringLiteral("wood") || type == QStringLiteral("forest"))
            {
                return palette.wood;
            }
            if (type == QStringLiteral("grass") || type == QStringLiteral("wetland"))
            {
                return palette.park;
            }
            if (type == QStringLiteral("sand"))
            {
                return palette.sand;
            }
            return type == QStringLiteral("ice") ? palette.ice : QColor();
        });
        fillLayer(painter, context, QStringLiteral("park"), 6, [&palette](const Layer&, const Feature&) { return palette.park; });
        fillLayer(painter, context, QStringLiteral("landuse"), 10, [&palette](const Layer& layer, const Feature& feature)
        {
            static const QStringList greenTypes{QStringLiteral("cemetery"), QStringLiteral("pitch"), QStringLiteral("stadium"), QStringLiteral("playground"), QStringLiteral("garden")};
            static const QStringList builtTypes{QStringLiteral("commercial"), QStringLiteral("retail"), QStringLiteral("industrial"), QStringLiteral("railway"), QStringLiteral("hospital"), QStringLiteral("school"), QStringLiteral("university"), QStringLiteral("college"), QStringLiteral("kindergarten")};
            const QString type = stringProperty(layer, feature, QStringLiteral("class"));
            if (greenTypes.contains(type))
            {
                return palette.park;
            }
            return builtTypes.contains(type) ? palette.landuse : QColor();
        });
        fillLayer(painter, context, QStringLiteral("water"), 0, [&palette](const Layer&, const Feature&) { return palette.water; });
        paintWaterways(painter, context, palette);
        paintBuildings(painter, context, palette);
        paintRoads(painter, context, palette);
        paintBoundaries(painter, context, palette);
        return pixmap;
    }

    void collectLabels(const ShadMapVectorTile& tile, const QRectF& sourceRect, const PointMapper& mapper, int zoom, const QStringList& nameKeys, QList<Label>& labels)
    {
        if (const Layer* places = tile.layer(QStringLiteral("place")))
        {
            for (const Feature& feature : places->features)
            {
                const QString type = stringProperty(*places, feature, QStringLiteral("class"));
                for (const PlaceRule& rule : kPlaceRules)
                {
                    if (type == QLatin1String(rule.name))
                    {
                        if (zoom >= rule.minimumZoom && zoom <= rule.maximumZoom)
                        {
                            appendLabel(*places, feature, rule.kind, sourceRect, mapper, nameKeys, labels);
                        }
                        break;
                    }
                }
            }
        }
        if (const Layer* waterNames = tile.layer(QStringLiteral("water_name")))
        {
            for (const Feature& feature : waterNames->features)
            {
                const QString type = stringProperty(*waterNames, feature, QStringLiteral("class"));
                const bool isOpenWater = type == QStringLiteral("ocean") || type == QStringLiteral("sea");
                const bool isVisible = isOpenWater ? zoom >= 3 && zoom <= 10 : zoom >= 11;
                if (isVisible)
                {
                    appendLabel(*waterNames, feature, Water, sourceRect, mapper, nameKeys, labels);
                }
            }
        }
        if (const Layer* roadNames = tile.layer(QStringLiteral("transportation_name")))
        {
            for (const Feature& feature : roadNames->features)
            {
                if (feature.type != ShadMapVectorTile::LineString)
                {
                    continue;
                }
                const QString type = stringProperty(*roadNames, feature, QStringLiteral("class"));
                for (int index = 0; index < kRoadLabelRuleCount; ++index)
                {
                    if (type == QLatin1String(kRoadLabelRules[index].name))
                    {
                        if (zoom >= kRoadLabelRules[index].minimumZoom)
                        {
                            appendRoadLabel(*roadNames, feature, index, sourceRect, mapper, nameKeys, labels);
                        }
                        break;
                    }
                }
            }
        }
    }

    void paintLabels(QPainter& painter, QList<Label>& labels, const QFont& baseFont, const QRectF& bounds, const Palette& palette)
    {
        if (labels.isEmpty())
        {
            return;
        }
        std::stable_sort(labels.begin(), labels.end(), [](const Label& left, const Label& right) { return left.priority < right.priority; });
        QList<QFont> fonts;
        QList<QFontMetricsF> metrics;
        for (const LabelStyle& style : kLabelStyles)
        {
            QFont font = baseFont;
            font.setPixelSize(style.pixelSize);
            font.setWeight(style.weight);
            font.setItalic(style.isItalic);
            fonts.append(font);
            metrics.append(QFontMetricsF(font));
        }
        const QColor colors[3] = {palette.labelPrimary, palette.labelSecondary, palette.labelWater};
        const QPen haloPen(palette.halo, kHaloWidth, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        QList<QRectF> placed;
        QHash<QString, QList<QPointF>> placedRoads;
        painter.save();
        painter.setRenderHint(QPainter::Antialiasing, true);
        for (const Label& label : std::as_const(labels))
        {
            const int kind = int(label.kind);
            const bool isRoad = label.kind == Road;
            const QFontMetricsF& fontMetrics = metrics.at(kind);
            const qreal width = fontMetrics.horizontalAdvance(label.text);
            const qreal height = fontMetrics.height();
            qreal halfWidth = width / 2.0;
            qreal halfHeight = height / 2.0;
            if (isRoad)
            {
                if (label.length < width + kRoadLabelMargin)
                {
                    continue;
                }
                const qreal cosine = std::abs(std::cos(qDegreesToRadians(label.angle)));
                const qreal sine = std::abs(std::sin(qDegreesToRadians(label.angle)));
                halfWidth = (cosine * width + sine * height) / 2.0;
                halfHeight = (sine * width + cosine * height) / 2.0;
            }
            const QRectF rect(label.position.x() - halfWidth, label.position.y() - halfHeight, 2.0 * halfWidth, 2.0 * halfHeight);
            if (!bounds.contains(rect))
            {
                continue;
            }
            const QRectF padded = rect.adjusted(-kLabelPaddingX, -kLabelPaddingY, kLabelPaddingX, kLabelPaddingY);
            const bool isBlocked = std::any_of(placed.cbegin(), placed.cend(), [&padded](const QRectF& other) { return other.intersects(padded); });
            if (isBlocked)
            {
                continue;
            }
            if (isRoad)
            {
                QList<QPointF>& sameName = placedRoads[label.text];
                const QPointF position = label.position;
                const bool isRepeated = std::any_of(sameName.cbegin(), sameName.cend(), [&position](const QPointF& other) { return std::hypot(other.x() - position.x(), other.y() - position.y()) < kRoadRepeatDistance; });
                if (isRepeated)
                {
                    continue;
                }
                sameName.append(position);
            }
            placed.append(padded);
            QPainterPath textPath;
            textPath.addText(QPointF(-width / 2.0, fontMetrics.ascent() - height / 2.0), fonts.at(kind), label.text);
            QTransform transform;
            transform.translate(label.position.x(), label.position.y());
            transform.rotate(label.angle);
            const QPainterPath path = transform.map(textPath);
            painter.strokePath(path, haloPen);
            painter.fillPath(path, colors[kLabelStyles[kind].colorIndex]);
            if (placed.size() >= kMaxLabels)
            {
                break;
            }
        }
        painter.restore();
    }
} // namespace ShadMapVectorStyle
