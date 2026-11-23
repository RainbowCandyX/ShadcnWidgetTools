#include "ShadMap.h"

#include <QAbstractAnimation>
#include <QApplication>
#include <QColor>
#include <QCursor>
#include <QEasingCurve>
#include <QEvent>
#include <QFocusEvent>
#include <QHelpEvent>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QJsonValue>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QNativeGestureEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSize>
#include <QTimer>
#include <QVariant>
#include <QVariantAnimation>
#include <QWheelEvent>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include <cmath>
#include <initializer_list>
#include <utility>

#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadDef.h"
#include "ShadI18n.h"
#include "ShadIconType.h"
#include "ShadMapCompassButton.h"
#include "ShadMapMarker.h"
#include "ShadMapPopup.h"
#include "ShadMapPrivate.h"
#include "ShadMapProjection.h"
#include "ShadMapRoute.h"
#include "ShadMapTileLoader.h"
#include "ShadSpinner.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"

namespace
{
    struct GeoPoint
    {
        ShadMapCoordinate coordinate;
        QString title;
        QString description;
    };

    void appendGeoPoint(const QJsonArray& position, const QJsonObject& properties, QList<GeoPoint>& points)
    {
        if (position.size() < 2 || !position.at(0).isDouble() || !position.at(1).isDouble())
        {
            return;
        }
        const double longitude = position.at(0).toDouble();
        const double latitude = position.at(1).toDouble();
        if (latitude < -90.0 || latitude > 90.0 || longitude < -180.0 || longitude > 180.0)
        {
            return;
        }
        const QString title = properties.value(QStringLiteral("title")).toString();
        points.append(GeoPoint{ShadMapCoordinate(latitude, longitude), title.isEmpty() ? properties.value(QStringLiteral("name")).toString() : title, properties.value(QStringLiteral("description")).toString()});
    }

    void collectGeoPoints(const QJsonObject& object, const QJsonObject& properties, QList<GeoPoint>& points)
    {
        const QString type = object.value(QStringLiteral("type")).toString();
        if (type == QStringLiteral("FeatureCollection"))
        {
            const QJsonArray features = object.value(QStringLiteral("features")).toArray();
            for (const QJsonValue& feature : features)
            {
                collectGeoPoints(feature.toObject(), QJsonObject(), points);
            }
        }
        else if (type == QStringLiteral("Feature"))
        {
            collectGeoPoints(object.value(QStringLiteral("geometry")).toObject(), object.value(QStringLiteral("properties")).toObject(), points);
        }
        else if (type == QStringLiteral("GeometryCollection"))
        {
            const QJsonArray geometries = object.value(QStringLiteral("geometries")).toArray();
            for (const QJsonValue& geometry : geometries)
            {
                collectGeoPoints(geometry.toObject(), properties, points);
            }
        }
        else if (type == QStringLiteral("Point"))
        {
            appendGeoPoint(object.value(QStringLiteral("coordinates")).toArray(), properties, points);
        }
        else if (type == QStringLiteral("MultiPoint"))
        {
            const QJsonArray positions = object.value(QStringLiteral("coordinates")).toArray();
            for (const QJsonValue& position : positions)
            {
                appendGeoPoint(position.toArray(), properties, points);
            }
        }
    }

    bool parseGeoJson(const QByteArray& data, QList<GeoPoint>& points, QString& error)
    {
        QJsonParseError parseError;
        const QJsonDocument document = QJsonDocument::fromJson(data, &parseError);
        if (parseError.error != QJsonParseError::NoError || !document.isObject())
        {
            error = parseError.error != QJsonParseError::NoError ? parseError.errorString() : ShadMap::tr("GeoJSON 的顶层不是对象");
            return false;
        }
        collectGeoPoints(document.object(), QJsonObject(), points);
        return true;
    }

    QList<ShadMapMarker*> addGeoPoints(ShadMap* map, const QList<GeoPoint>& points)
    {
        QList<ShadMapMarker*> markers;
        markers.reserve(points.size());
        for (const GeoPoint& point : points)
        {
            ShadMapMarker* marker = map->addMarker(point.coordinate);
            marker->setPopupTitle(point.title);
            marker->setPopupText(point.description);
            markers.append(marker);
        }
        return markers;
    }
} // namespace

ShadMap::ShadMap(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadMapPrivate())
{
    Q_D(ShadMap);
    d->q_ptr = this;
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);
    d->_loader = new ShadMapTileLoader(this);
    d->_animation = new QVariantAnimation(this);
    d->_animation->setDuration(ShadMapPrivate::kAnimationDurationMs);
    d->_animation->setEasingCurve(QEasingCurve::OutCubic);
    d->_animation->setStartValue(0.0);
    d->_animation->setEndValue(1.0);
    d->_snapTimer = new QTimer(this);
    d->_snapTimer->setSingleShot(true);
    d->_snapTimer->setInterval(ShadMapPrivate::kSnapDelayMs);
    d->_canvasTimer = new QTimer(this);
    d->_canvasTimer->setSingleShot(true);
    d->_canvasTimer->setInterval(ShadMapPrivate::kCanvasReleaseMs);
    connect(d->_canvasTimer, &QTimer::timeout, this, [d]()
    {
        d->_canvas = QPixmap();
        d->_tileLayer = QPixmap();
        d->_labelLayer = QPixmap();
    });
    d->_controls = new ShadButtonGroup(Qt::Vertical, this);
    d->_zoomInButton = new ShadButton(this);
    d->_zoomOutButton = new ShadButton(this);
    d->_resetButton = new ShadButton(this);
    d->_locateButton = new ShadButton(this);
    d->_compassButton = new ShadMapCompassButton(this);
    d->_zoomInButton->setShadIcon(ShadIconType::Plus);
    d->_zoomOutButton->setShadIcon(ShadIconType::Minus);
    d->_resetButton->setShadIcon(ShadIconType::RotateCcw);
    d->_locateButton->setShadIcon(ShadIconType::LocateFixed);
    for (ShadButton* button : {d->_zoomInButton, d->_zoomOutButton, d->_resetButton, d->_locateButton, static_cast<ShadButton*>(d->_compassButton)})
    {
        button->setVariant(ShadButtonType::Outline);
        button->setButtonSize(ShadButtonType::IconSmall);
        button->hide();
    }
    d->_spinner = new ShadSpinner(this);
    d->_spinner->setSpinnerSize(ShadMapPrivate::kSpinnerSize);
    d->_spinner->setAttribute(Qt::WA_TransparentForMouseEvents, true);
    d->_spinner->hide();
    connect(d->_zoomInButton, &ShadButton::clicked, this, &ShadMap::zoomIn);
    connect(d->_zoomOutButton, &ShadButton::clicked, this, &ShadMap::zoomOut);
    connect(d->_resetButton, &ShadButton::clicked, this, &ShadMap::resetView);
    connect(d->_locateButton, &ShadButton::clicked, this, &ShadMap::locate);
    connect(d->_compassButton, &ShadButton::clicked, this, &ShadMap::resetNorth);
    connect(d->_loader, &ShadMapTileLoader::fetched, this, [this](const QUrl& url, const QByteArray& data, const QString& error)
    {
        QList<GeoPoint> points;
        QString message = error;
        if (!message.isEmpty() || !parseGeoJson(data, points, message))
        {
            Q_EMIT geoJsonFailed(url, message);
            return;
        }
        addGeoPoints(this, points);
        Q_EMIT geoJsonLoaded(url, int(points.size()));
    });
    connect(d->_animation, &QVariantAnimation::valueChanged, this, [d](const QVariant& value) { d->applyAnimation(value.toReal()); });
    connect(d->_snapTimer, &QTimer::timeout, this, [this, d]()
    {
        const double target = std::round(d->_zoom);
        if (!qFuzzyCompare(target + 1.0, d->_zoom + 1.0))
        {
            d->animateZoomAround(d->_snapPosition, target);
        }
        else
        {
            d->updateMarkerWidgets();
            refresh();
        }
    });
    connect(d->_animation, &QAbstractAnimation::stateChanged, this, [this, d](QAbstractAnimation::State state)
    {
        if (state == QAbstractAnimation::Stopped)
        {
            d->updateMarkerWidgets();
            refresh();
        }
    });
    connect(d->_loader, &ShadMapTileLoader::tileLoaded, this, [this]() { refresh(); });
    connect(d->_loader, &ShadMapTileLoader::sourceChanged, this, [this, d]()
    {
        d->_vectorRenders.clear();
        refresh();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [d]() { d->applyTileSource(); });
#ifdef SHADCN_MAP_TRANSLATIONS
    shadI18n->addTranslations(QStringLiteral(":/ShadcnWidgetToolsMap/i18n"), QStringLiteral("ShadcnWidgetToolsMap"));
#endif
    d->retranslate();
    d->rebuildControls();
}

ShadMap::~ShadMap()
{
    Q_D(ShadMap);
    d->_animation->stop();
    d->_snapTimer->stop();
    d->_canvasTimer->stop();
    QObject::disconnect(d->_animation, nullptr, this, nullptr);
    QObject::disconnect(d->_snapTimer, nullptr, this, nullptr);
    QObject::disconnect(d->_canvasTimer, nullptr, this, nullptr);
    QObject::disconnect(d->_loader, nullptr, this, nullptr);
    for (ShadMapMarker* marker : std::as_const(d->_markers))
    {
        QObject::disconnect(marker, nullptr, this, nullptr);
    }
    for (ShadMapRoute* route : std::as_const(d->_routes))
    {
        QObject::disconnect(route, nullptr, this, nullptr);
    }
    for (ShadMapArc* arc : std::as_const(d->_arcs))
    {
        QObject::disconnect(arc, nullptr, this, nullptr);
    }
    for (ShadMapPopup* popup : std::as_const(d->_popups))
    {
        QObject::disconnect(popup, nullptr, this, nullptr);
    }
}

void ShadMap::setTileUrl(const QString& lightUrl, const QString& darkUrl)
{
    Q_D(ShadMap);
    d->_lightUrl = lightUrl;
    d->_darkUrl = darkUrl;
    d->applyTileSource();
}

QString ShadMap::getTileUrl() const
{
    Q_D(const ShadMap);
    return d->_lightUrl;
}

QString ShadMap::getDarkTileUrl() const
{
    Q_D(const ShadMap);
    return d->_darkUrl;
}

void ShadMap::setProjection(ShadMapType::Projection projection)
{
    Q_D(ShadMap);
    if (d->_projection == projection)
    {
        return;
    }
    d->stopAnimation();
    d->_projection = projection;
    d->_center = d->clampCenter(d->_requestedCenter, d->_zoom);
    d->updatePopups();
    refresh();
    Q_EMIT viewportChanged();
}

ShadMapType::Projection ShadMap::getProjection() const
{
    Q_D(const ShadMap);
    return d->_projection;
}

void ShadMap::setBearing(double bearing)
{
    Q_D(ShadMap);
    d->stopAnimation();
    d->applyBearing(bearing);
}

double ShadMap::getBearing() const
{
    Q_D(const ShadMap);
    return d->_bearing;
}

void ShadMap::setRotationEnabled(bool enabled)
{
    Q_D(ShadMap);
    d->_isRotationEnabled = enabled;
    d->_isRotating = false;
}

bool ShadMap::isRotationEnabled() const
{
    Q_D(const ShadMap);
    return d->_isRotationEnabled;
}

void ShadMap::setCompassControlVisible(bool visible)
{
    Q_D(ShadMap);
    if (d->_isCompassControlVisible != visible)
    {
        d->_isCompassControlVisible = visible;
        d->rebuildControls();
    }
}

bool ShadMap::isCompassControlVisible() const
{
    Q_D(const ShadMap);
    return d->_isCompassControlVisible;
}

void ShadMap::setTheme(ShadMapType::Theme theme)
{
    Q_D(ShadMap);
    if (d->_theme == theme)
    {
        return;
    }
    d->_theme = theme;
    d->applyTileSource();
}

ShadMapType::Theme ShadMap::getTheme() const
{
    Q_D(const ShadMap);
    return d->_theme;
}

void ShadMap::setTileFormat(ShadMapType::TileFormat format)
{
    Q_D(ShadMap);
    if (d->_tileFormat == format)
    {
        return;
    }
    d->_tileFormat = format;
    d->applyTileSource();
}

ShadMapType::TileFormat ShadMap::getTileFormat() const
{
    Q_D(const ShadMap);
    return d->_tileFormat;
}

void ShadMap::setVectorTileUrl(const QString& url)
{
    Q_D(ShadMap);
    if (d->_vectorUrl == url)
    {
        return;
    }
    d->_vectorUrl = url;
    d->applyTileSource();
}

QString ShadMap::getVectorTileUrl() const
{
    Q_D(const ShadMap);
    return d->_vectorUrl;
}

void ShadMap::setTileSubdomains(const QStringList& subdomains)
{
    Q_D(ShadMap);
    d->_loader->setSubdomains(subdomains);
    refresh();
}

QStringList ShadMap::getTileSubdomains() const
{
    Q_D(const ShadMap);
    return d->_loader->subdomains();
}

void ShadMap::setAttribution(const QString& attribution)
{
    Q_D(ShadMap);
    if (d->_attribution == attribution)
    {
        return;
    }
    d->_attribution = attribution;
    refresh();
}

QString ShadMap::getAttribution() const
{
    Q_D(const ShadMap);
    return d->_attribution;
}

void ShadMap::setCenter(const ShadMapCoordinate& center)
{
    Q_D(ShadMap);
    d->_hasFit = false;
    d->stopAnimation();
    d->setView(ShadMapProjection::project(center), d->_zoom);
    d->rememberHome();
}

ShadMapCoordinate ShadMap::getCenter() const
{
    Q_D(const ShadMap);
    return ShadMapProjection::unproject(d->_center);
}

void ShadMap::setZoom(double zoom)
{
    Q_D(ShadMap);
    d->_hasFit = false;
    d->stopAnimation();
    d->setView(d->_requestedCenter, zoom);
    d->rememberHome();
}

double ShadMap::getZoom() const
{
    Q_D(const ShadMap);
    return d->_zoom;
}

void ShadMap::setMinimumZoom(double zoom)
{
    Q_D(ShadMap);
    d->_minimumZoom = qBound(0.0, zoom, d->_maximumZoom);
    d->setView(d->_requestedCenter, d->_zoom);
    d->updateControls();
}

double ShadMap::getMinimumZoom() const
{
    Q_D(const ShadMap);
    return d->_minimumZoom;
}

void ShadMap::setMaximumZoom(double zoom)
{
    Q_D(ShadMap);
    d->_maximumZoom = qBound(d->_minimumZoom, zoom, ShadMapPrivate::kHighestZoom);
    d->_isClusterValid = false;
    d->setView(d->_requestedCenter, d->_zoom);
    d->updateControls();
    d->updateMarkerWidgets();
    refresh();
}

double ShadMap::getMaximumZoom() const
{
    Q_D(const ShadMap);
    return d->_maximumZoom;
}

void ShadMap::setInteractive(bool interactive)
{
    Q_D(ShadMap);
    if (d->_isInteractive == interactive)
    {
        return;
    }
    d->_isInteractive = interactive;
    d->_isPressed = false;
    d->_isDragging = false;
    d->_pressedMarker = nullptr;
    unsetCursor();
}

bool ShadMap::isInteractive() const
{
    Q_D(const ShadMap);
    return d->_isInteractive;
}

void ShadMap::setLoading(bool loading)
{
    Q_D(ShadMap);
    if (d->_isLoading == loading)
    {
        return;
    }
    d->_isLoading = loading;
    d->_spinner->setVisible(loading);
    d->_spinner->raise();
    refresh();
}

bool ShadMap::isLoading() const
{
    Q_D(const ShadMap);
    return d->_isLoading;
}

void ShadMap::setControlsVisible(bool visible)
{
    Q_D(ShadMap);
    if (d->_isControlsVisible == visible)
    {
        return;
    }
    d->_isControlsVisible = visible;
    d->updateControls();
    refresh();
}

bool ShadMap::isControlsVisible() const
{
    Q_D(const ShadMap);
    return d->_isControlsVisible;
}

void ShadMap::setControlPosition(ShadMapType::ControlPosition position)
{
    Q_D(ShadMap);
    if (d->_controlPosition == position)
    {
        return;
    }
    d->_controlPosition = position;
    d->updateControls();
    refresh();
}

ShadMapType::ControlPosition ShadMap::getControlPosition() const
{
    Q_D(const ShadMap);
    return d->_controlPosition;
}

void ShadMap::setClusteringEnabled(bool enabled)
{
    Q_D(ShadMap);
    if (d->_isClusteringEnabled == enabled)
    {
        return;
    }
    d->_isClusteringEnabled = enabled;
    d->_isClusterValid = false;
    d->updateMarkerWidgets();
    refreshOverlay();
}

bool ShadMap::isClusteringEnabled() const
{
    Q_D(const ShadMap);
    return d->_isClusteringEnabled;
}

void ShadMap::flyTo(const ShadMapCoordinate& center, double zoom)
{
    Q_D(ShadMap);
    d->_hasFit = false;
    d->animateTo(ShadMapProjection::project(center), zoom);
    d->_homeCenter = ShadMapProjection::project(center);
    d->_homeZoom = d->clampZoom(zoom);
}

void ShadMap::fitBounds(const QList<ShadMapCoordinate>& coordinates, int padding)
{
    Q_D(ShadMap);
    d->_fitCoordinates = coordinates;
    d->_fitPadding = qMax(0, padding);
    d->_hasFit = !coordinates.isEmpty();
    d->applyFit();
}

ShadMapCoordinate ShadMap::coordinateAt(const QPointF& position) const
{
    Q_D(const ShadMap);
    return ShadMapProjection::unproject(d->toNormalized(position));
}

QPointF ShadMap::positionOf(const ShadMapCoordinate& coordinate) const
{
    Q_D(const ShadMap);
    return d->toScreen(ShadMapProjection::project(coordinate));
}

ShadMapMarker* ShadMap::addMarker(const ShadMapCoordinate& coordinate)
{
    Q_D(ShadMap);
    ShadMapMarker* marker = new ShadMapMarker(coordinate, this);
    d->_markers.append(marker);
    d->_isClusterValid = false;
    connect(marker, &ShadMapMarker::changed, this, [this, d, marker]()
    {
        d->_isClusterValid = false;
        d->syncMarkerPopup(marker);
        d->updateMarkerWidgets();
        refreshOverlay();
    });
    connect(marker, &QObject::destroyed, this, [this, d, marker]()
    {
        d->_markers.removeOne(marker);
        d->_isClusterValid = false;
        refreshOverlay();
    });
    refreshOverlay();
    return marker;
}

void ShadMap::removeMarker(ShadMapMarker* marker)
{
    Q_D(ShadMap);
    if (marker == nullptr || !d->_markers.removeOne(marker))
    {
        return;
    }
    d->_isClusterValid = false;
    if (QWidget* content = marker->getContentWidget())
    {
        content->hide();
    }
    if (d->_popupMarker == marker)
    {
        d->closeMarkerPopup();
    }
    if (d->_pressedMarker == marker)
    {
        d->_pressedMarker = nullptr;
    }
    QObject::disconnect(marker, nullptr, this, nullptr);
    marker->deleteLater();
    refreshOverlay();
}

QList<ShadMapMarker*> ShadMap::markers() const
{
    Q_D(const ShadMap);
    return d->_markers;
}

void ShadMap::clearMarkers()
{
    Q_D(ShadMap);
    const QList<ShadMapMarker*> markers = d->_markers;
    for (ShadMapMarker* marker : markers)
    {
        removeMarker(marker);
    }
}

ShadMapRoute* ShadMap::addRoute(const QList<ShadMapCoordinate>& coordinates)
{
    Q_D(ShadMap);
    ShadMapRoute* route = new ShadMapRoute(coordinates, this);
    d->_routes.append(route);
    connect(route, &ShadMapRoute::changed, this, [this]() { refreshOverlay(); });
    connect(route, &QObject::destroyed, this, [this, d, route]()
    {
        d->_routes.removeOne(route);
        refreshOverlay();
    });
    refreshOverlay();
    return route;
}

void ShadMap::removeRoute(ShadMapRoute* route)
{
    Q_D(ShadMap);
    if (route == nullptr || !d->_routes.removeOne(route))
    {
        return;
    }
    QObject::disconnect(route, nullptr, this, nullptr);
    route->deleteLater();
    refreshOverlay();
}

ShadMapArc* ShadMap::addArc(const ShadMapCoordinate& from, const ShadMapCoordinate& to)
{
    Q_D(ShadMap);
    ShadMapArc* arc = new ShadMapArc(from, to, this);
    d->_arcs.append(arc);
    connect(arc, &ShadMapArc::changed, this, [this]() { refreshOverlay(); });
    connect(arc, &QObject::destroyed, this, [this, d, arc]()
    {
        d->_arcs.removeOne(arc);
        refreshOverlay();
    });
    refreshOverlay();
    return arc;
}

void ShadMap::removeArc(ShadMapArc* arc)
{
    Q_D(ShadMap);
    if (arc == nullptr || !d->_arcs.removeOne(arc))
    {
        return;
    }
    QObject::disconnect(arc, nullptr, this, nullptr);
    arc->deleteLater();
    refreshOverlay();
}

QList<ShadMapArc*> ShadMap::addArcs(const QList<ShadMapArcData>& arcs)
{
    QList<ShadMapArc*> result;
    result.reserve(arcs.size());
    for (const ShadMapArcData& data : arcs)
    {
        ShadMapArc* arc = addArc(data.from, data.to);
        arc->setId(data.id);
        result.append(arc);
    }
    return result;
}

ShadMapArc* ShadMap::arc(const QString& id) const
{
    Q_D(const ShadMap);
    for (ShadMapArc* arc : std::as_const(d->_arcs))
    {
        if (arc->getId() == id)
        {
            return arc;
        }
    }
    return nullptr;
}

QList<ShadMapArc*> ShadMap::arcs() const
{
    Q_D(const ShadMap);
    return d->_arcs;
}

void ShadMap::clearArcs()
{
    Q_D(ShadMap);
    const QList<ShadMapArc*> arcs = d->_arcs;
    for (ShadMapArc* arc : arcs)
    {
        removeArc(arc);
    }
}

QList<ShadMapMarker*> ShadMap::addGeoJson(const QByteArray& data)
{
    QList<GeoPoint> points;
    QString error;
    if (!parseGeoJson(data, points, error))
    {
        qWarning("GeoJSON 解析失败：%s", qUtf8Printable(error));
        return QList<ShadMapMarker*>();
    }
    return addGeoPoints(this, points);
}

void ShadMap::loadGeoJson(const QUrl& url)
{
    Q_D(ShadMap);
    d->_loader->fetch(url);
}

void ShadMap::setZoomControlVisible(bool visible)
{
    Q_D(ShadMap);
    if (d->_isZoomControlVisible != visible)
    {
        d->_isZoomControlVisible = visible;
        d->rebuildControls();
    }
}

bool ShadMap::isZoomControlVisible() const
{
    Q_D(const ShadMap);
    return d->_isZoomControlVisible;
}

void ShadMap::setResetControlVisible(bool visible)
{
    Q_D(ShadMap);
    if (d->_isResetControlVisible != visible)
    {
        d->_isResetControlVisible = visible;
        d->rebuildControls();
    }
}

bool ShadMap::isResetControlVisible() const
{
    Q_D(const ShadMap);
    return d->_isResetControlVisible;
}

void ShadMap::setLocateControlVisible(bool visible)
{
    Q_D(ShadMap);
    if (d->_isLocateControlVisible != visible)
    {
        d->_isLocateControlVisible = visible;
        d->rebuildControls();
    }
}

bool ShadMap::isLocateControlVisible() const
{
    Q_D(const ShadMap);
    return d->_isLocateControlVisible;
}

ShadButton* ShadMap::addControlButton(ShadIconType::IconName icon, const QString& toolTip)
{
    Q_D(ShadMap);
    ShadButton* button = new ShadButton(this);
    button->setVariant(ShadButtonType::Outline);
    button->setButtonSize(ShadButtonType::IconSmall);
    button->setShadIcon(icon);
    button->setToolTip(toolTip);
    button->setAccessibleName(toolTip);
    d->_controls->addWidget(button);
    button->show();
    d->updateControls();
    refresh();
    return button;
}

void ShadMap::removeControlButton(ShadButton* button)
{
    Q_D(ShadMap);
    const bool isBuiltin = button == d->_zoomInButton || button == d->_zoomOutButton || button == d->_resetButton || button == d->_locateButton || button == d->_compassButton;
    if (button == nullptr || isBuiltin || button->parentWidget() != d->_controls)
    {
        return;
    }
    d->_controls->removeWidget(button);
    button->deleteLater();
    d->updateControls();
    refresh();
}

void ShadMap::addControlSeparator()
{
    Q_D(ShadMap);
    d->_controls->addSeparator();
    d->updateControls();
    refresh();
}

void ShadMap::setUserLocation(const ShadMapCoordinate& coordinate)
{
    Q_D(ShadMap);
    d->_userLocation = coordinate;
    d->_hasUserLocation = true;
    refreshOverlay();
}

ShadMapCoordinate ShadMap::getUserLocation() const
{
    Q_D(const ShadMap);
    return d->_userLocation;
}

bool ShadMap::hasUserLocation() const
{
    Q_D(const ShadMap);
    return d->_hasUserLocation;
}

void ShadMap::clearUserLocation()
{
    Q_D(ShadMap);
    d->_hasUserLocation = false;
    refreshOverlay();
}

ShadMapPopup* ShadMap::addPopup(const ShadMapCoordinate& coordinate, const QString& text)
{
    Q_D(ShadMap);
    ShadMapPopup* popup = new ShadMapPopup(this);
    popup->setCoordinate(coordinate);
    popup->setText(text);
    d->_popups.append(popup);
    connect(popup, &ShadMapPopup::changed, this, [d]() { d->updatePopups(); });
    connect(popup, &QObject::destroyed, this, [d, popup]() { d->_popups.removeOne(popup); });
    popup->show();
    d->restack();
    d->updatePopups();
    return popup;
}

void ShadMap::removePopup(ShadMapPopup* popup)
{
    Q_D(ShadMap);
    if (popup == nullptr || !d->_popups.removeOne(popup))
    {
        return;
    }
    QObject::disconnect(popup, nullptr, this, nullptr);
    popup->hide();
    popup->deleteLater();
}

void ShadMap::showMarkerPopup(ShadMapMarker* marker)
{
    Q_D(ShadMap);
    if (marker == nullptr || !d->_markers.contains(marker))
    {
        return;
    }
    d->showMarkerPopup(marker);
}

void ShadMap::hideMarkerPopup()
{
    Q_D(ShadMap);
    d->closeMarkerPopup();
}

void ShadMap::addOverlay(QWidget* widget, ShadMapType::ControlPosition position)
{
    Q_D(ShadMap);
    if (widget == nullptr)
    {
        return;
    }
    const qsizetype index = d->_overlays.indexOf(widget);
    if (index >= 0)
    {
        d->_overlayPositions[index] = position;
    }
    else
    {
        d->_overlays.append(widget);
        d->_overlayPositions.append(position);
        widget->setParent(this);
        widget->setAttribute(Qt::WA_NoMousePropagation, true);
        widget->show();
    }
    d->restack();
    d->updateOverlays();
    if (d->_hasFit)
    {
        d->applyFit();
    }
}

void ShadMap::removeOverlay(QWidget* widget)
{
    Q_D(ShadMap);
    const qsizetype index = d->_overlays.indexOf(widget);
    if (index < 0)
    {
        return;
    }
    d->_overlays.removeAt(index);
    d->_overlayPositions.removeAt(index);
    widget->hide();
    widget->setParent(nullptr);
    if (d->_hasFit)
    {
        d->applyFit();
    }
}

QSize ShadMap::sizeHint() const
{
    return QSize(480, 320);
}

QSize ShadMap::minimumSizeHint() const
{
    return QSize(160, 120);
}

void ShadMap::zoomIn()
{
    Q_D(ShadMap);
    d->_hasFit = false;
    d->animateZoomAround(QPointF(rect().center()), std::floor(d->_zoom) + 1.0);
}

void ShadMap::zoomOut()
{
    Q_D(ShadMap);
    d->_hasFit = false;
    d->animateZoomAround(QPointF(rect().center()), std::ceil(d->_zoom) - 1.0);
}

void ShadMap::resetView()
{
    Q_D(ShadMap);
    d->_hasFit = false;
    d->animateView(d->_homeCenter, d->_homeZoom, 0.0);
}

void ShadMap::resetNorth()
{
    Q_D(ShadMap);
    d->animateView(d->_requestedCenter, d->_zoom, 0.0);
}

void ShadMap::locate()
{
    Q_D(ShadMap);
    Q_EMIT locateRequested();
    if (d->_hasUserLocation)
    {
        d->_hasFit = false;
        d->animateTo(ShadMapProjection::project(d->_userLocation), qMax(d->_zoom, ShadMapPrivate::kLocateZoom));
    }
}

bool ShadMap::event(QEvent* event)
{
    Q_D(ShadMap);
    if (event->type() == QEvent::ToolTip)
    {
        const QHelpEvent* helpEvent = static_cast<QHelpEvent*>(event);
        const QList<ShadMapPrivate::Item> items = d->layoutItems();
        const int index = d->itemAt(items, QPointF(helpEvent->pos()));
        const QString text = index >= 0 && items.at(index).markers.size() == 1 ? items.at(index).markers.first()->getToolTip() : QString();
        if (text.isEmpty())
        {
            shadTooltip->hideText();
            event->ignore();
            return true;
        }
        shadTooltip->showText(this, d->markerRect(items.at(index).markers.first(), items.at(index).position).toAlignedRect(), text);
        return true;
    }
    if (event->type() == QEvent::LayoutRequest)
    {
        if (d->updateOverlays() && d->_hasFit)
        {
            d->applyFit();
        }
        d->updatePopups();
    }
#ifndef QT_NO_GESTURES
    if (event->type() == QEvent::NativeGesture && d->_isInteractive)
    {
        const QNativeGestureEvent* gesture = static_cast<QNativeGestureEvent*>(event);
        if (gesture->gestureType() == Qt::ZoomNativeGesture)
        {
            d->_hasFit = false;
            d->stopAnimation();
            d->zoomAround(gesture->position(), d->_zoom + std::log2(qMax(0.1, 1.0 + gesture->value())));
            d->scheduleSnap(gesture->position());
            return true;
        }
        if (gesture->gestureType() == Qt::RotateNativeGesture && d->_isRotationEnabled)
        {
            d->_hasFit = false;
            d->stopAnimation();
            d->applyBearing(d->_bearing - gesture->value());
            return true;
        }
    }
#endif
    return QWidget::event(event);
}

void ShadMap::changeEvent(QEvent* event)
{
    Q_D(ShadMap);
    if (event->type() == QEvent::LanguageChange)
    {
        d->retranslate();
        refresh();
    }
    else if (event->type() == QEvent::FontChange)
    {
        refresh();
    }
    QWidget::changeEvent(event);
}

void ShadMap::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadMap);
    QWidget::resizeEvent(event);
    d->updateOverlays();
    if (d->_hasFit)
    {
        d->applyFit();
    }
    else
    {
        d->_center = d->clampCenter(d->_requestedCenter, d->_zoom);
    }
    d->updateControls();
    d->updatePopups();
}

void ShadMap::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadMap);
    d->_isFocusVisible = event->reason() == Qt::TabFocusReason || event->reason() == Qt::BacktabFocusReason || event->reason() == Qt::ShortcutFocusReason;
    refresh();
    QWidget::focusInEvent(event);
}

void ShadMap::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadMap);
    d->_isFocusVisible = false;
    refresh();
    QWidget::focusOutEvent(event);
}

void ShadMap::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadMap);
    if (!d->_isInteractive)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    const double step = ShadMapPrivate::kPanStep;
    QPointF offset;
    switch (event->key())
    {
        case Qt::Key_Left:
            offset = QPointF(step, 0.0);
            break;
        case Qt::Key_Right:
            offset = QPointF(-step, 0.0);
            break;
        case Qt::Key_Up:
            offset = QPointF(0.0, step);
            break;
        case Qt::Key_Down:
            offset = QPointF(0.0, -step);
            break;
        case Qt::Key_Plus:
        case Qt::Key_Equal:
            zoomIn();
            break;
        case Qt::Key_Minus:
            zoomOut();
            break;
        case Qt::Key_Home:
        case Qt::Key_0:
            resetView();
            break;
        default:
            QWidget::keyPressEvent(event);
            return;
    }
    if (!offset.isNull())
    {
        d->_hasFit = false;
        d->stopAnimation();
        d->panBy(offset);
    }
    event->accept();
}

void ShadMap::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadMap);
    const bool isRotateButton = event->button() == Qt::RightButton || (event->button() == Qt::LeftButton && event->modifiers().testFlag(Qt::ControlModifier));
    if (d->_isInteractive && d->_isRotationEnabled && isRotateButton)
    {
        d->stopAnimation();
        d->_isRotating = true;
        d->_lastPosition = event->position();
        setCursor(Qt::ClosedHandCursor);
        event->accept();
        return;
    }
    if (!d->_isInteractive || event->button() != Qt::LeftButton)
    {
        QWidget::mousePressEvent(event);
        return;
    }
    d->stopAnimation();
    d->_isPressed = true;
    d->_isDragging = false;
    d->_pressPosition = event->position();
    d->_lastPosition = event->position();
    const QList<ShadMapPrivate::Item> items = d->layoutItems();
    const int index = d->itemAt(items, event->position());
    d->_pressedMarker = index >= 0 && items.at(index).markers.size() == 1 ? items.at(index).markers.first() : nullptr;
    event->accept();
}

void ShadMap::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadMap);
    if (!d->_isInteractive)
    {
        QWidget::mouseMoveEvent(event);
        return;
    }
    const QPointF position = event->position();
    if (d->_isRotating)
    {
        d->_hasFit = false;
        d->applyBearing(d->_bearing + (position.x() - d->_lastPosition.x()) * ShadMapPrivate::kRotateDegreesPerPixel);
        d->_lastPosition = position;
        return;
    }
    if (!d->_isPressed)
    {
        setCursor(d->itemAt(d->layoutItems(), position) >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
        return;
    }
    const bool isMarkerDrag = !d->_pressedMarker.isNull() && d->_pressedMarker->isDraggable();
    if (!d->_isDragging)
    {
        if ((position - d->_pressPosition).manhattanLength() < QApplication::startDragDistance())
        {
            return;
        }
        d->_isDragging = true;
        setCursor(Qt::ClosedHandCursor);
    }
    if (isMarkerDrag)
    {
        d->_pressedMarker->setCoordinate(coordinateAt(position));
    }
    else
    {
        d->_hasFit = false;
        d->panBy(position - d->_lastPosition);
    }
    d->_lastPosition = position;
}

void ShadMap::mouseReleaseEvent(QMouseEvent* event)
{
    Q_D(ShadMap);
    if (d->_isRotating)
    {
        d->_isRotating = false;
        setCursor(Qt::ArrowCursor);
        event->accept();
        return;
    }
    if (!d->_isPressed || event->button() != Qt::LeftButton)
    {
        QWidget::mouseReleaseEvent(event);
        return;
    }
    ShadMapMarker* pressedMarker = d->_pressedMarker.data();
    const bool wasDragging = d->_isDragging;
    d->_isPressed = false;
    d->_isDragging = false;
    d->_pressedMarker = nullptr;
    const QList<ShadMapPrivate::Item> items = d->layoutItems();
    const int index = d->itemAt(items, event->position());
    setCursor(index >= 0 ? Qt::PointingHandCursor : Qt::ArrowCursor);
    event->accept();
    if (wasDragging)
    {
        if (pressedMarker != nullptr && pressedMarker->isDraggable())
        {
            Q_EMIT pressedMarker->dragEnded(pressedMarker->getCoordinate());
        }
        return;
    }
    if (index < 0)
    {
        d->closeMarkerPopup();
        if (d->isOnMap(event->position()))
        {
            Q_EMIT clicked(coordinateAt(event->position()));
        }
        return;
    }
    if (items.at(index).markers.size() > 1)
    {
        d->zoomToItem(items.at(index));
        return;
    }
    ShadMapMarker* hitMarker = items.at(index).markers.first();
    d->togglePopup(hitMarker);
    Q_EMIT hitMarker->clicked();
    Q_EMIT markerClicked(hitMarker);
}

void ShadMap::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_D(ShadMap);
    if (!d->_isInteractive || event->button() != Qt::LeftButton)
    {
        QWidget::mouseDoubleClickEvent(event);
        return;
    }
    if (d->itemAt(d->layoutItems(), event->position()) < 0)
    {
        d->_hasFit = false;
        d->animateZoomAround(event->position(), std::floor(d->_zoom) + 1.0);
    }
    event->accept();
}

void ShadMap::wheelEvent(QWheelEvent* event)
{
    Q_D(ShadMap);
    if (!d->_isInteractive)
    {
        QWidget::wheelEvent(event);
        return;
    }
    const QPointF position = event->position();
    d->_hasFit = false;
    if (!event->pixelDelta().isNull() || event->phase() != Qt::NoScrollPhase)
    {
        const double delta = event->pixelDelta().isNull() ? event->angleDelta().y() / 240.0 : event->pixelDelta().y() / 200.0;
        d->stopAnimation();
        d->zoomAround(position, d->_zoom + delta);
        d->scheduleSnap(position);
    }
    else if (event->angleDelta().y() != 0)
    {
        const double base = d->_animation->state() == QAbstractAnimation::Running ? d->_animationToZoom : d->_zoom;
        d->animateZoomAround(position, std::round(base) + (event->angleDelta().y() > 0 ? 1.0 : -1.0));
    }
    event->accept();
}

void ShadMap::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadMap);
    const qreal ratio = devicePixelRatioF();
    const QSize pixelSize(qCeil(width() * ratio), qCeil(height() * ratio));
    const bool isResized = d->_canvas.size() != pixelSize || !qFuzzyCompare(d->_canvas.devicePixelRatio(), ratio);
    if (d->_isCanvasDirty || isResized)
    {
        const bool isBaseDirty = d->_isBaseDirty || isResized;
        if (isResized)
        {
            d->_canvas = QPixmap(pixelSize);
            d->_canvas.setDevicePixelRatio(ratio);
        }
        d->_canvas.fill(Qt::transparent);
        d->_isCanvasDirty = false;
        d->_isBaseDirty = false;
        if (isBaseDirty)
        {
            d->_tileLayer = QPixmap();
            d->_labelLayer = QPixmap();
        }
        else if (d->_tileLayer.isNull())
        {
            d->_tileLayer = QPixmap(pixelSize);
            d->_tileLayer.setDevicePixelRatio(ratio);
            d->_tileLayer.fill(Qt::transparent);
            QPainter tilePainter(&d->_tileLayer);
            tilePainter.setRenderHint(QPainter::Antialiasing, true);
            paintBase(tilePainter);
            d->_labelLayer = QPixmap(pixelSize);
            d->_labelLayer.setDevicePixelRatio(ratio);
            d->_labelLayer.fill(Qt::transparent);
            QPainter labelPainter(&d->_labelLayer);
            labelPainter.setRenderHint(QPainter::Antialiasing, true);
            d->paintVectorLabels(labelPainter);
        }
        QPainter canvasPainter(&d->_canvas);
        paintCanvas(canvasPainter);
    }
    QPainter painter(this);
    painter.drawPixmap(0, 0, d->_canvas);
    d->_canvasTimer->start();
}

void ShadMap::refresh()
{
    Q_D(ShadMap);
    d->_isCanvasDirty = true;
    d->_isBaseDirty = true;
    QWidget::update();
}

void ShadMap::refreshOverlay()
{
    Q_D(ShadMap);
    d->_isCanvasDirty = true;
    QWidget::update();
}

void ShadMap::paintBase(QPainter& painter)
{
    Q_D(ShadMap);
    painter.fillRect(rect(), d->styleColor(ShadThemeType::Muted));
    if (d->isGlobe())
    {
        d->paintGlobe(painter);
    }
    else if (d->_tileFormat == ShadMapType::Vector)
    {
        d->paintVectorTiles(painter);
    }
    else
    {
        d->paintTiles(painter);
    }
}

void ShadMap::paintCanvas(QPainter& painter)
{
    Q_D(ShadMap);
    const QColor background = shadTheme->getColor(ShadThemeType::Background);
    const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
    const QRectF canvas = d->canvasRect();
    QPainterPath clipPath;
    clipPath.addRoundedRect(canvas, radius, radius);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.save();
    painter.setClipPath(clipPath);
    if (d->_tileLayer.isNull())
    {
        paintBase(painter);
    }
    else
    {
        painter.drawPixmap(0, 0, d->_tileLayer);
    }
    d->paintRoutes(painter);
    d->paintArcs(painter);
    if (d->_tileLayer.isNull())
    {
        d->paintVectorLabels(painter);
    }
    else
    {
        painter.drawPixmap(0, 0, d->_labelLayer);
    }
    d->paintItems(painter);
    d->paintAttribution(painter);
    if (!d->_controls->isHidden())
    {
        const int margin = ShadMapPrivate::kControlFocusMargin;
        const qreal controlRadius = shadTheme->getRadius(ShadThemeType::Medium);
        painter.setPen(Qt::NoPen);
        painter.setBrush(background);
        painter.drawRoundedRect(QRectF(d->_controls->geometry()).adjusted(margin, margin, -margin, -margin), controlRadius, controlRadius);
    }
    if (d->_isLoading)
    {
        QColor veil = background;
        veil.setAlphaF(0.6);
        painter.fillRect(rect(), veil);
    }
    painter.restore();
    painter.setPen(QPen(shadTheme->getColor(d->_isFocusVisible ? ShadThemeType::Ring : ShadThemeType::Border), d->_isFocusVisible ? 2.0 : 1.0));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(d->_isFocusVisible ? canvas.adjusted(0.5, 0.5, -0.5, -0.5) : canvas, radius, radius);
}
