#include "ShadMapPrivate.h"

#include <QAbstractAnimation>
#include <QColor>
#include <QElapsedTimer>
#include <QFont>
#include <QFontMetricsF>
#include <QHash>
#include <QPainter>
#include <QPainterPath>
#include <QPen>
#include <QPixmap>
#include <QRect>
#include <QSet>
#include <QSharedPointer>
#include <QSize>
#include <QSizeF>
#include <QTimer>
#include <QTransform>
#include <QVarLengthArray>
#include <QVariantAnimation>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtMath>

#include <cmath>
#include <utility>

#include "ShadButton.h"
#include "ShadButtonGroup.h"
#include "ShadDef.h"
#include "ShadMap.h"
#include "ShadMapCompassButton.h"
#include "ShadMapMarker.h"
#include "ShadMapPopup.h"
#include "ShadMapProjection.h"
#include "ShadMapRoute.h"
#include "ShadMapTileLoader.h"
#include "ShadMapVectorStyle.h"
#include "ShadMapVectorTile.h"
#include "ShadSpinner.h"
#include "ShadTheme.h"
#include "ShadTooltip.h"

namespace
{
    constexpr qreal kLabelPaddingX = 6.0;
    constexpr qreal kLabelPaddingY = 2.0;
    constexpr qreal kPillPaddingX = 10.0;
    constexpr qreal kPillPaddingY = 5.0;
    constexpr qreal kSelectedHaloWidth = 4.0;
    constexpr qreal kUserHaloRadius = 16.0;
    constexpr qreal kUserDotRadius = 6.0;
    constexpr qreal kUserRingWidth = 2.0;
    constexpr int kLabelFontSize = 12;
    constexpr int kAttributionFontSize = 10;
    constexpr int kHiddenPopupOffset = 10000;
    constexpr qreal kRotatedTileOverlap = 0.5;
    constexpr int kArcMinimumSteps = 8;

    QPointF anchorAt(const QPointF& center, const QPointF& screen, const QSizeF& size, double zoom)
    {
        const double world = ShadMapProjection::worldSize(zoom);
        return QPointF(center.x() + (screen.x() - size.width() / 2.0) / world, center.y() + (screen.y() - size.height() / 2.0) / world);
    }

    QPointF centerFor(const QPointF& anchor, const QPointF& screen, const QSizeF& size, double zoom)
    {
        const double world = ShadMapProjection::worldSize(zoom);
        return QPointF(anchor.x() - (screen.x() - size.width() / 2.0) / world, anchor.y() - (screen.y() - size.height() / 2.0) / world);
    }

    QFont scaledFont(const QWidget* widget, int pixelSize, QFont::Weight weight)
    {
        QFont font = widget->font();
        font.setPixelSize(pixelSize);
        font.setWeight(weight);
        return font;
    }

    QColor withAlpha(const QColor& color, qreal alpha)
    {
        QColor result = color;
        result.setAlphaF(color.alphaF() * alpha);
        return result;
    }

    bool hasPopupContent(const ShadMapMarker* marker)
    {
        return !marker->getPopupTitle().isEmpty() || !marker->getPopupText().isEmpty();
    }

    int wrapTile(const ShadMapPrivate::TileGrid& grid, int tileX)
    {
        return ((tileX % grid.tileCount) + grid.tileCount) % grid.tileCount;
    }

    QRectF tileTarget(const ShadMapPrivate::TileGrid& grid, int tileX, int tileY)
    {
        const int x0 = qRound((tileX - grid.left) * grid.extent);
        const int x1 = qRound((tileX + 1 - grid.left) * grid.extent);
        const int y0 = qRound((tileY - grid.top) * grid.extent);
        const int y1 = qRound((tileY + 1 - grid.top) * grid.extent);
        return QRectF(x0, y0, x1 - x0, y1 - y0);
    }
} // namespace

ShadMapPrivate::ShadMapPrivate()
    : _vectorRenders(kRenderCacheKilobytes)
{
}

ShadMapPrivate::~ShadMapPrivate()
{
}

double ShadMapPrivate::clampZoom(double zoom) const
{
    return qBound(_minimumZoom, zoom, _maximumZoom);
}

QPointF ShadMapPrivate::clampCenter(const QPointF& center, double zoom) const
{
    Q_Q(const ShadMap);
    if (isGlobe())
    {
        return QPointF(ShadMapProjection::wrap(center.x()), qBound(0.0, center.y(), 1.0));
    }
    const double halfHeight = frameSize().height() / 2.0 / ShadMapProjection::worldSize(zoom);
    const double y = halfHeight >= 0.5 ? 0.5 : qBound(halfHeight, center.y(), 1.0 - halfHeight);
    return QPointF(ShadMapProjection::wrap(center.x()), y);
}

bool ShadMapPrivate::isGlobe() const
{
    return _projection == ShadMapType::Globe;
}

ShadThemeType::ThemeMode ShadMapPrivate::styleMode() const
{
    if (_theme == ShadMapType::Auto)
    {
        return shadTheme->isDark() ? ShadThemeType::Dark : ShadThemeType::Light;
    }
    return _theme == ShadMapType::Dark ? ShadThemeType::Dark : ShadThemeType::Light;
}

QColor ShadMapPrivate::styleColor(ShadThemeType::ThemeColor color) const
{
    return shadTheme->getColor(styleMode(), color);
}

ShadMapProjection::GlobeView ShadMapPrivate::globeView() const
{
    Q_Q(const ShadMap);
    return ShadMapProjection::globeView(_center, _zoom, QPointF(q->width() / 2.0, q->height() / 2.0), _bearing);
}

QPointF ShadMapPrivate::viewCenter() const
{
    Q_Q(const ShadMap);
    return QPointF(q->width() / 2.0, q->height() / 2.0);
}

QPointF ShadMapPrivate::rotated(const QPointF& offset) const
{
    const double cosine = std::cos(qDegreesToRadians(_bearing));
    const double sine = std::sin(qDegreesToRadians(_bearing));
    return QPointF(offset.x() * cosine + offset.y() * sine, -offset.x() * sine + offset.y() * cosine);
}

QPointF ShadMapPrivate::unrotated(const QPointF& offset) const
{
    const double cosine = std::cos(qDegreesToRadians(_bearing));
    const double sine = std::sin(qDegreesToRadians(_bearing));
    return QPointF(offset.x() * cosine - offset.y() * sine, offset.x() * sine + offset.y() * cosine);
}

QPointF ShadMapPrivate::unrotatedScreen(const QPointF& screen) const
{
    const QPointF center = viewCenter();
    return center + unrotated(screen - center);
}

QSizeF ShadMapPrivate::frameSize() const
{
    Q_Q(const ShadMap);
    const double cosine = std::abs(std::cos(qDegreesToRadians(_bearing)));
    const double sine = std::abs(std::sin(qDegreesToRadians(_bearing)));
    return QSizeF(cosine * q->width() + sine * q->height(), sine * q->width() + cosine * q->height());
}

QTransform ShadMapPrivate::frameTransform() const
{
    const QPointF center = viewCenter();
    const QSizeF size = frameSize();
    QTransform transform;
    transform.translate(center.x(), center.y());
    transform.rotate(-_bearing);
    transform.translate(-size.width() / 2.0, -size.height() / 2.0);
    return transform;
}

void ShadMapPrivate::applyBearing(double bearing)
{
    Q_Q(ShadMap);
    const double normalized = bearing - 360.0 * std::round(bearing / 360.0);
    if (qFuzzyCompare(normalized + 360.0, _bearing + 360.0))
    {
        return;
    }
    _bearing = normalized;
    _center = clampCenter(_requestedCenter, _zoom);
    _compassButton->setBearing(_bearing);
    updatePopups();
    q->refresh();
    Q_EMIT q->viewportChanged();
}

QPointF ShadMapPrivate::toScreen(const QPointF& normalized) const
{
    Q_Q(const ShadMap);
    if (isGlobe())
    {
        QPointF screen;
        ShadMapProjection::projectGlobe(globeView(), ShadMapProjection::longitudeOf(normalized.x()), ShadMapProjection::latitudeOf(normalized.y()), screen);
        return screen;
    }
    const double world = ShadMapProjection::worldSize(_zoom);
    double deltaX = normalized.x() - _center.x();
    deltaX -= std::round(deltaX);
    return viewCenter() + rotated(QPointF(deltaX * world, (normalized.y() - _center.y()) * world));
}

QPointF ShadMapPrivate::toNormalized(const QPointF& screen) const
{
    Q_Q(const ShadMap);
    if (isGlobe())
    {
        const ShadMapProjection::GlobeView view = globeView();
        const QPointF offset = screen - view.origin;
        const double distance = std::hypot(offset.x(), offset.y());
        const QPointF inside = distance < view.radius ? screen : view.origin + offset * (view.radius * 0.999 / distance);
        double longitude = 0.0;
        double latitude = 0.0;
        ShadMapProjection::unprojectGlobe(view, inside, longitude, latitude);
        const QPointF normalized = ShadMapProjection::normalizedOf(longitude, latitude);
        return QPointF(ShadMapProjection::wrap(normalized.x()), normalized.y());
    }
    const QPointF anchor = anchorAt(_center, unrotatedScreen(screen), QSizeF(q->size()), _zoom);
    return QPointF(ShadMapProjection::wrap(anchor.x()), qBound(0.0, anchor.y(), 1.0));
}

bool ShadMapPrivate::isVisible(const QPointF& normalized) const
{
    if (!isGlobe())
    {
        return true;
    }
    QPointF screen;
    return ShadMapProjection::projectGlobe(globeView(), ShadMapProjection::longitudeOf(normalized.x()), ShadMapProjection::latitudeOf(normalized.y()), screen, kGlobeVisibleThreshold);
}

bool ShadMapPrivate::isOnMap(const QPointF& screen) const
{
    if (!isGlobe())
    {
        return true;
    }
    const ShadMapProjection::GlobeView view = globeView();
    return std::hypot(screen.x() - view.origin.x(), screen.y() - view.origin.y()) <= view.radius;
}

QRectF ShadMapPrivate::canvasRect() const
{
    Q_Q(const ShadMap);
    return QRectF(q->rect()).adjusted(0.5, 0.5, -0.5, -0.5);
}

void ShadMapPrivate::setView(const QPointF& center, double zoom)
{
    Q_Q(ShadMap);
    const double nextZoom = clampZoom(zoom);
    const QPointF nextCenter = clampCenter(center, nextZoom);
    _requestedCenter = QPointF(ShadMapProjection::wrap(center.x()), qBound(0.0, center.y(), 1.0));
    if (nextCenter == _center && qFuzzyCompare(nextZoom + 1.0, _zoom + 1.0))
    {
        return;
    }
    _center = nextCenter;
    _zoom = nextZoom;
    updateControls();
    updatePopups();
    q->refresh();
    Q_EMIT q->viewportChanged();
}

void ShadMapPrivate::panBy(const QPointF& delta)
{
    const QPointF offset = unrotated(delta);
    if (isGlobe())
    {
        const ShadMapProjection::GlobeView view = globeView();
        const double latitude = std::atan2(view.sinLatitude, view.cosLatitude) + offset.y() / view.radius;
        const double longitude = view.longitude - offset.x() / (view.radius * qMax(kGlobeMinimumCosine, view.cosLatitude));
        setView(ShadMapProjection::normalizedOf(longitude, latitude), _zoom);
        return;
    }
    const double world = ShadMapProjection::worldSize(_zoom);
    setView(_center - offset / world, _zoom);
}

void ShadMapPrivate::zoomAround(const QPointF& screen, double zoom)
{
    Q_Q(ShadMap);
    if (isGlobe())
    {
        setView(_requestedCenter, zoom);
        return;
    }
    const double nextZoom = clampZoom(zoom);
    const QSizeF size(q->size());
    const QPointF point = unrotatedScreen(screen);
    setView(centerFor(anchorAt(_requestedCenter, point, size, _zoom), point, size, nextZoom), nextZoom);
}

void ShadMapPrivate::animateTo(const QPointF& center, double zoom)
{
    animateView(center, zoom, _bearing);
}

void ShadMapPrivate::animateView(const QPointF& center, double zoom, double bearing)
{
    stopAnimation();
    const double bearingDelta = bearing - _bearing;
    _animationFromBearing = _bearing;
    _animationToBearing = _bearing + bearingDelta - 360.0 * std::round(bearingDelta / 360.0);
    const double targetZoom = clampZoom(zoom);
    const QPointF target(ShadMapProjection::wrap(center.x()), qBound(0.0, center.y(), 1.0));
    double deltaX = target.x() - _center.x();
    deltaX -= std::round(deltaX);
    _isAnimationAnchored = false;
    _animationFromCenter = _center;
    _animationToCenter = QPointF(_center.x() + deltaX, target.y());
    _animationFromZoom = _zoom;
    _animationToZoom = targetZoom;
    _animation->start();
}

void ShadMapPrivate::animateZoomAround(const QPointF& screen, double zoom)
{
    Q_Q(ShadMap);
    if (isGlobe())
    {
        animateTo(_requestedCenter, zoom);
        return;
    }
    stopAnimation();
    _isAnimationAnchored = true;
    _animationFromBearing = _bearing;
    _animationToBearing = _bearing;
    _animationAnchorScreen = unrotatedScreen(screen);
    _animationAnchor = anchorAt(_requestedCenter, _animationAnchorScreen, QSizeF(q->size()), _zoom);
    _animationFromZoom = _zoom;
    _animationToZoom = clampZoom(zoom);
    _animation->start();
}

void ShadMapPrivate::stopAnimation()
{
    Q_Q(ShadMap);
    _animation->stop();
    if (_snapTimer->isActive())
    {
        _snapTimer->stop();
        q->refresh();
    }
}

void ShadMapPrivate::applyAnimation(qreal progress)
{
    Q_Q(ShadMap);
    const double zoom = _animationFromZoom + (_animationToZoom - _animationFromZoom) * progress;
    applyBearing(_animationFromBearing + (_animationToBearing - _animationFromBearing) * progress);
    if (_isAnimationAnchored)
    {
        setView(centerFor(_animationAnchor, _animationAnchorScreen, QSizeF(q->size()), zoom), zoom);
        return;
    }
    setView(_animationFromCenter + (_animationToCenter - _animationFromCenter) * progress, zoom);
}

void ShadMapPrivate::scheduleSnap(const QPointF& screen)
{
    _snapPosition = screen;
    _snapTimer->start();
}

void ShadMapPrivate::rememberHome()
{
    _homeCenter = _requestedCenter;
    _homeZoom = _zoom;
}

bool ShadMapPrivate::fitView(const QList<ShadMapCoordinate>& coordinates, int padding, QPointF& center, double& zoom) const
{
    Q_Q(const ShadMap);
    if (coordinates.isEmpty() || q->width() <= 0 || q->height() <= 0)
    {
        return false;
    }
    const QPointF first = ShadMapProjection::project(coordinates.first());
    double minX = first.x();
    double maxX = first.x();
    double minY = first.y();
    double maxY = first.y();
    for (const ShadMapCoordinate& coordinate : coordinates)
    {
        const QPointF point = ShadMapProjection::project(coordinate);
        minX = qMin(minX, point.x());
        maxX = qMax(maxX, point.x());
        minY = qMin(minY, point.y());
        maxY = qMax(maxY, point.y());
    }
    const QRectF area = fitArea(maxX - minX, maxY - minY, padding);
    center = QPointF((minX + maxX) / 2.0, (minY + maxY) / 2.0);
    if (maxX > minX || maxY > minY)
    {
        const double availableWidth = qMax(1.0, area.width() - 2 * padding);
        const double availableHeight = qMax(1.0, area.height() - 2 * padding);
        const double zoomX = maxX > minX ? std::log2(availableWidth / (ShadMapProjection::kTileSize * (maxX - minX))) : kHighestZoom;
        const double zoomY = maxY > minY ? std::log2(availableHeight / (ShadMapProjection::kTileSize * (maxY - minY))) : kHighestZoom;
        zoom = clampZoom(std::floor(qMin(zoomX, zoomY)));
    }
    if (!isGlobe())
    {
        center -= unrotated(area.center() - viewCenter()) / ShadMapProjection::worldSize(zoom);
    }
    return true;
}

QRectF ShadMapPrivate::fitArea(double spanX, double spanY, int padding) const
{
    Q_Q(const ShadMap);
    const QRectF full(q->rect());
    QList<QRectF> blocked;
    for (const QPointer<QWidget>& overlay : std::as_const(_overlays))
    {
        if (!overlay.isNull() && !overlay->isHidden() && blocked.size() < kMaximumFitOverlays)
        {
            blocked.append(QRectF(overlay->geometry()).adjusted(-kControlInset, -kControlInset, kControlInset, kControlInset).intersected(full));
        }
    }
    QRectF best = full;
    double bestScore = -1.0;
    for (int mask = 0; mask < (1 << blocked.size()); ++mask)
    {
        QRectF area = full;
        for (int index = 0; index < blocked.size(); ++index)
        {
            const QRectF& rect = blocked.at(index);
            const bool isHorizontal = (mask & (1 << index)) != 0;
            if (isHorizontal && rect.center().x() < full.center().x())
            {
                area.setLeft(qMax(area.left(), rect.right()));
            }
            else if (isHorizontal)
            {
                area.setRight(qMin(area.right(), rect.left()));
            }
            else if (rect.center().y() < full.center().y())
            {
                area.setTop(qMax(area.top(), rect.bottom()));
            }
            else
            {
                area.setBottom(qMin(area.bottom(), rect.top()));
            }
        }
        const double width = area.width() - 2 * padding;
        const double height = area.height() - 2 * padding;
        if (width <= 0.0 || height <= 0.0)
        {
            continue;
        }
        const double scaleX = spanX > 0.0 ? width / spanX : kUnboundedScale;
        const double scaleY = spanY > 0.0 ? height / spanY : kUnboundedScale;
        const double score = spanX > 0.0 || spanY > 0.0 ? qMin(scaleX, scaleY) : width * height;
        if (score > bestScore)
        {
            best = area;
            bestScore = score;
        }
    }
    return best;
}

void ShadMapPrivate::applyFit()
{
    QPointF center;
    double zoom = _zoom;
    if (!fitView(_fitCoordinates, _fitPadding, center, zoom))
    {
        return;
    }
    stopAnimation();
    setView(center, zoom);
    rememberHome();
}

void ShadMapPrivate::applyTileSource()
{
    Q_Q(ShadMap);
    _vectorRenders.clear();
    _loader->setFormat(_tileFormat);
    if (_tileFormat == ShadMapType::Vector)
    {
        _loader->setUrlTemplate(_vectorUrl);
    }
    else
    {
        const bool isDarkSource = styleMode() == ShadThemeType::Dark && !_darkUrl.isEmpty();
        _loader->setUrlTemplate(isDarkSource ? _darkUrl : _lightUrl);
    }
    q->refresh();
}

void ShadMapPrivate::updateControls()
{
    Q_Q(ShadMap);
    _zoomInButton->setEnabled(_zoom < _maximumZoom);
    _zoomOutButton->setEnabled(_zoom > _minimumZoom);
    _controls->setVisible(_isControlsVisible && _controls->count() > 0);
    const bool isLeft = _controlPosition == ShadMapType::TopLeft || _controlPosition == ShadMapType::BottomLeft;
    const bool isTop = _controlPosition == ShadMapType::TopLeft || _controlPosition == ShadMapType::TopRight;
    const QSize controlSize = _controls->sizeHint();
    const int offset = kControlInset - kControlFocusMargin;
    const int x = isLeft ? offset : q->width() - controlSize.width() - offset;
    const int y = isTop ? offset : q->height() - controlSize.height() - offset;
    _controls->setGeometry(x, y, controlSize.width(), controlSize.height());
    const ShadPlacementType::Side side = isLeft ? ShadPlacementType::Right : ShadPlacementType::Left;
    for (int index = 0; index < _controls->count(); ++index)
    {
        if (QWidget* widget = _controls->widgetAt(index))
        {
            ShadTooltip::setSide(widget, side);
        }
    }
    const QSize spinnerSize = _spinner->sizeHint();
    _spinner->setGeometry((q->width() - spinnerSize.width()) / 2, (q->height() - spinnerSize.height()) / 2, spinnerSize.width(), spinnerSize.height());
}

void ShadMapPrivate::updateMarkerWidgets()
{
    bool hasContent = false;
    for (const ShadMapMarker* marker : std::as_const(_markers))
    {
        hasContent = hasContent || marker->getContentWidget() != nullptr;
    }
    if (!hasContent)
    {
        return;
    }
    QHash<const ShadMapMarker*, QPointF> positions;
    const QList<Item> items = layoutItems();
    for (const Item& item : items)
    {
        if (item.markers.size() == 1)
        {
            positions.insert(item.markers.first(), item.position);
        }
    }
    for (const ShadMapMarker* marker : std::as_const(_markers))
    {
        QWidget* content = marker->getContentWidget();
        if (content == nullptr)
        {
            continue;
        }
        const auto found = positions.constFind(marker);
        if (found == positions.constEnd())
        {
            content->hide();
            continue;
        }
        content->setGeometry(markerRect(marker, found.value()).toAlignedRect());
        content->show();
    }
}

void ShadMapPrivate::updatePopups()
{
    updateMarkerWidgets();
    for (ShadMapPopup* popup : std::as_const(_popups))
    {
        if (popup->isHidden())
        {
            continue;
        }
        popup->adjustSize();
        const QPointF normalized = ShadMapProjection::project(popup->getCoordinate());
        if (!isVisible(normalized))
        {
            popup->move(-kHiddenPopupOffset - popup->width(), -kHiddenPopupOffset - popup->height());
            continue;
        }
        const QPointF anchor = toScreen(normalized);
        const int offset = popup->getAnchorOffset();
        switch (popup->getPlacement())
        {
            case ShadPlacementType::Bottom:
                popup->move(qRound(anchor.x() - popup->width() / 2.0), qRound(anchor.y() + offset));
                break;
            case ShadPlacementType::Left:
                popup->move(qRound(anchor.x() - offset - popup->width()), qRound(anchor.y() - popup->height() / 2.0));
                break;
            case ShadPlacementType::Right:
                popup->move(qRound(anchor.x() + offset), qRound(anchor.y() - popup->height() / 2.0));
                break;
            default:
                popup->move(qRound(anchor.x() - popup->width() / 2.0), qRound(anchor.y() - offset - popup->height()));
                break;
        }
    }
}

void ShadMapPrivate::rebuildControls()
{
    Q_Q(ShadMap);
    ShadButton* const buttons[] = {_zoomInButton, _zoomOutButton, _compassButton, _resetButton, _locateButton};
    const bool visibility[] = {_isZoomControlVisible, _isZoomControlVisible, _isCompassControlVisible, _isResetControlVisible, _isLocateControlVisible};
    for (ShadButton* button : buttons)
    {
        if (button->parentWidget() == _controls)
        {
            _controls->removeWidget(button);
            button->setParent(q);
        }
    }
    int position = 0;
    for (int index = 0; index < kBuiltinControlCount; ++index)
    {
        if (visibility[index])
        {
            _controls->insertWidget(position++, buttons[index]);
            buttons[index]->show();
        }
    }
    updateControls();
    q->refresh();
}

void ShadMapPrivate::retranslate()
{
    _compassButton->setToolTip(ShadMap::tr("转回正北"));
    _compassButton->setAccessibleName(ShadMap::tr("转回正北"));
    _locateButton->setToolTip(ShadMap::tr("定位到当前位置"));
    _locateButton->setAccessibleName(ShadMap::tr("定位到当前位置"));
    _zoomInButton->setToolTip(ShadMap::tr("放大"));
    _zoomInButton->setAccessibleName(ShadMap::tr("放大"));
    _zoomOutButton->setToolTip(ShadMap::tr("缩小"));
    _zoomOutButton->setAccessibleName(ShadMap::tr("缩小"));
    _resetButton->setToolTip(ShadMap::tr("回到初始位置"));
    _resetButton->setAccessibleName(ShadMap::tr("回到初始位置"));
}

void ShadMapPrivate::rebuildClusters() const
{
    _clusterGroups.clear();
    QHash<quint64, int> cells;
    const bool isMerged = _zoom < _maximumZoom;
    const double world = ShadMapProjection::worldSize(_zoom);
    for (ShadMapMarker* marker : std::as_const(_markers))
    {
        if (isMerged)
        {
            const QPointF normalized = ShadMapProjection::project(marker->getCoordinate());
            const quint64 cellX = quint64(qFloor(ShadMapProjection::wrap(normalized.x()) * world / kClusterCellSize));
            const quint64 cellY = quint64(qFloor(qBound(0.0, normalized.y(), 1.0) * world / kClusterCellSize));
            const quint64 cell = (cellX << 32) | cellY;
            const auto found = cells.constFind(cell);
            if (found != cells.constEnd())
            {
                _clusterGroups[found.value()].append(marker);
                continue;
            }
            cells.insert(cell, int(_clusterGroups.size()));
        }
        _clusterGroups.append(QList<ShadMapMarker*>{marker});
    }
    _clusterZoom = _zoom;
    _isClusterValid = true;
}

QList<ShadMapPrivate::Item> ShadMapPrivate::layoutItems() const
{
    QList<Item> items;
    if (!_isClusteringEnabled)
    {
        for (ShadMapMarker* marker : std::as_const(_markers))
        {
            const QPointF normalized = ShadMapProjection::project(marker->getCoordinate());
            if (isVisible(normalized))
            {
                items.append(Item{toScreen(normalized), QList<ShadMapMarker*>{marker}});
            }
        }
        appendWorldCopies(items);
        return items;
    }
    const bool isZooming = _animation->state() == QAbstractAnimation::Running || _snapTimer->isActive();
    if (!_isClusterValid || (!isZooming && !qFuzzyCompare(_clusterZoom + 1.0, _zoom + 1.0)))
    {
        rebuildClusters();
    }
    for (const QList<ShadMapMarker*>& group : std::as_const(_clusterGroups))
    {
        QPointF position;
        int visibleCount = 0;
        for (const ShadMapMarker* marker : group)
        {
            const QPointF normalized = ShadMapProjection::project(marker->getCoordinate());
            if (isVisible(normalized))
            {
                position += toScreen(normalized);
                ++visibleCount;
            }
        }
        if (visibleCount > 0)
        {
            items.append(Item{position / qreal(visibleCount), group});
        }
    }
    appendWorldCopies(items);
    return items;
}

void ShadMapPrivate::appendWorldCopies(QList<Item>& items) const
{
    Q_Q(const ShadMap);
    const int copies = worldCopies();
    const double world = ShadMapProjection::worldSize(_zoom);
    const QRectF viewport = QRectF(q->rect()).adjusted(-kWorldCopyMargin, -kWorldCopyMargin, kWorldCopyMargin, kWorldCopyMargin);
    const qsizetype count = items.size();
    for (qsizetype index = 0; index < count; ++index)
    {
        const Item item = items.at(index);
        if (item.markers.size() == 1 && item.markers.first()->getContentWidget() != nullptr)
        {
            continue;
        }
        for (int copy = -copies; copy <= copies; ++copy)
        {
            const QPointF position = item.position + rotated(QPointF(copy * world, 0.0));
            if (copy != 0 && viewport.contains(position))
            {
                items.append(Item{position, item.markers});
            }
        }
    }
}

int ShadMapPrivate::itemAt(const QList<Item>& items, const QPointF& position) const
{
    for (int index = int(items.size()) - 1; index >= 0; --index)
    {
        const Item& item = items.at(index);
        if (item.markers.size() == 1 && (!item.markers.first()->getText().isEmpty() || item.markers.first()->getContentWidget() != nullptr))
        {
            if (markerRect(item.markers.first(), item.position).contains(position))
            {
                return index;
            }
            continue;
        }
        const qreal radius = item.markers.size() > 1 ? itemRadius(item) : qreal(kMarkerHitRadius);
        if (std::hypot(item.position.x() - position.x(), item.position.y() - position.y()) <= radius)
        {
            return index;
        }
    }
    return -1;
}

qreal ShadMapPrivate::itemRadius(const Item& item) const
{
    if (item.markers.size() <= 1)
    {
        return kMarkerRingRadius;
    }
    if (item.markers.size() < 10)
    {
        return 16.0;
    }
    return item.markers.size() < 100 ? 18.0 : 20.0;
}

QRectF ShadMapPrivate::markerRect(const ShadMapMarker* marker, const QPointF& position) const
{
    Q_Q(const ShadMap);
    if (const QWidget* content = marker->getContentWidget())
    {
        const QSizeF size(content->sizeHint().expandedTo(content->minimumSize()).boundedTo(content->maximumSize()));
        return QRectF(position.x() - size.width() / 2.0, position.y() - size.height() / 2.0, size.width(), size.height());
    }
    const QString text = marker->getText();
    if (text.isEmpty())
    {
        return QRectF(position.x() - kMarkerRingRadius, position.y() - kMarkerRingRadius, 2.0 * kMarkerRingRadius, 2.0 * kMarkerRingRadius);
    }
    const QFontMetricsF metrics(scaledFont(q, kLabelFontSize, QFont::DemiBold));
    const qreal width = metrics.horizontalAdvance(text) + 2.0 * kPillPaddingX;
    const qreal height = metrics.height() + 2.0 * kPillPaddingY;
    return QRectF(position.x() - width / 2.0, position.y() - height / 2.0, width, height);
}

void ShadMapPrivate::togglePopup(ShadMapMarker* marker)
{
    const bool isShown = _popupMarker == marker && !_markerPopup.isNull() && !_markerPopup->isHidden();
    if (isShown)
    {
        closeMarkerPopup();
        return;
    }
    showMarkerPopup(marker);
}

void ShadMapPrivate::showMarkerPopup(ShadMapMarker* marker)
{
    Q_Q(ShadMap);
    if (!hasPopupContent(marker))
    {
        closeMarkerPopup();
        return;
    }
    if (_markerPopup.isNull())
    {
        _markerPopup = q->addPopup(marker->getCoordinate());
        _markerPopup->setCloseButtonVisible(true);
    }
    _popupMarker = marker;
    applyMarkerPopup(marker);
    _markerPopup->show();
    _markerPopup->raise();
    restack();
    updatePopups();
}

void ShadMapPrivate::applyMarkerPopup(const ShadMapMarker* marker)
{
    const QString title = marker->getPopupTitle();
    const QString text = marker->getPopupText();
    _markerPopup->setCoordinate(marker->getCoordinate());
    _markerPopup->setAnchorOffset(qRound(markerRect(marker, QPointF()).height() / 2.0) + kLabelGap);
    _markerPopup->setTitle(title);
    _markerPopup->setDescription(title.isEmpty() ? QString() : text);
    _markerPopup->setText(title.isEmpty() ? text : QString());
}

void ShadMapPrivate::restack()
{
    for (const QPointer<QWidget>& overlay : std::as_const(_overlays))
    {
        if (!overlay.isNull())
        {
            overlay->raise();
        }
    }
    _controls->raise();
    _spinner->raise();
}

bool ShadMapPrivate::updateOverlays()
{
    Q_Q(ShadMap);
    bool isChanged = false;
    for (qsizetype index = _overlays.size() - 1; index >= 0; --index)
    {
        QWidget* overlay = _overlays.at(index).data();
        if (overlay == nullptr)
        {
            _overlays.removeAt(index);
            _overlayPositions.removeAt(index);
            isChanged = true;
            continue;
        }
        const ShadMapType::ControlPosition position = _overlayPositions.at(index);
        const bool isLeft = position == ShadMapType::TopLeft || position == ShadMapType::BottomLeft;
        const bool isTop = position == ShadMapType::TopLeft || position == ShadMapType::TopRight;
        const QSize size = overlay->sizeHint().boundedTo(QSize(q->width() - 2 * kControlInset, q->height() - 2 * kControlInset)).expandedTo(overlay->minimumSize());
        const int x = isLeft ? kControlInset : q->width() - size.width() - kControlInset;
        const int y = isTop ? kControlInset : q->height() - size.height() - kControlInset;
        const QRect geometry(x, y, size.width(), size.height());
        isChanged = isChanged || overlay->geometry() != geometry;
        overlay->setGeometry(geometry);
    }
    return isChanged;
}

void ShadMapPrivate::syncMarkerPopup(ShadMapMarker* marker)
{
    if (_popupMarker != marker || _markerPopup.isNull() || _markerPopup->isHidden())
    {
        return;
    }
    if (!hasPopupContent(marker))
    {
        closeMarkerPopup();
        return;
    }
    applyMarkerPopup(marker);
}

void ShadMapPrivate::closeMarkerPopup()
{
    if (!_markerPopup.isNull())
    {
        _markerPopup->hide();
    }
    _popupMarker = nullptr;
}

void ShadMapPrivate::zoomToItem(const Item& item)
{
    QList<ShadMapCoordinate> coordinates;
    for (ShadMapMarker* marker : item.markers)
    {
        coordinates.append(marker->getCoordinate());
    }
    QPointF center;
    double zoom = _zoom;
    _hasFit = false;
    if (fitView(coordinates, kClusterCellSize, center, zoom) && zoom > _zoom)
    {
        animateTo(center, zoom);
        return;
    }
    animateZoomAround(item.position, std::floor(_zoom) + 1.0);
}

QRectF ShadMapPrivate::attributionRect() const
{
    Q_Q(const ShadMap);
    if (_attribution.isEmpty())
    {
        return QRectF();
    }
    const QFontMetricsF metrics(scaledFont(q, kAttributionFontSize, QFont::Normal));
    const qreal width = qMin<qreal>(q->width() - 2 * kAttributionInset, metrics.horizontalAdvance(_attribution) + 2.0 * kLabelPaddingX);
    const qreal height = metrics.height() + 2.0 * kLabelPaddingY;
    const qreal x = _controlPosition == ShadMapType::BottomRight ? kAttributionInset : q->width() - kAttributionInset - width;
    return QRectF(x, q->height() - kAttributionInset - height, width, height);
}

ShadMapPrivate::TileGrid ShadMapPrivate::tileGrid(int level) const
{
    Q_Q(const ShadMap);
    TileGrid grid;
    grid.level = level;
    grid.tileCount = 1 << level;
    grid.extent = ShadMapProjection::kTileSize * std::pow(2.0, _zoom - level);
    const QSizeF frame = frameSize();
    grid.left = _center.x() * grid.tileCount - frame.width() / 2.0 / grid.extent;
    grid.top = _center.y() * grid.tileCount - frame.height() / 2.0 / grid.extent;
    grid.firstX = qFloor(grid.left);
    grid.lastX = qFloor(grid.left + frame.width() / grid.extent);
    grid.firstY = qMax(0, qFloor(grid.top));
    grid.lastY = qMin(grid.tileCount - 1, qFloor(grid.top + frame.height() / grid.extent));
    return grid;
}

ShadMapPrivate::VectorLayout ShadMapPrivate::vectorLayout() const
{
    VectorLayout layout;
    if (!_loader->isSourceReady())
    {
        return layout;
    }
    layout.baseZoom = qMax(0, qRound(_zoom));
    layout.grid = tileGrid(qMax(0, layout.baseZoom - 1));
    layout.sourceLevel = qMin(layout.grid.level, _loader->sourceMaximumZoom());
    layout.shift = layout.grid.level - layout.sourceLevel;
    layout.logicalSize = ShadMapProjection::kTileSize << (layout.baseZoom - layout.grid.level);
    layout.isValid = layout.sourceLevel >= _loader->sourceMinimumZoom();
    return layout;
}

int ShadMapPrivate::rasterLevel(double zoom) const
{
    const int maximumLevel = qMin(qFloor(qMin(_maximumZoom, kHighestZoom)), _loader->sourceMaximumZoom());
    return qBound(qMin(_loader->sourceMinimumZoom(), maximumLevel), qRound(zoom), maximumLevel);
}

bool ShadMapPrivate::beginVectorPass(VectorLayout& layout, VectorPass& pass)
{
    Q_Q(ShadMap);
    _loader->ensureSource();
    layout = vectorLayout();
    if (!layout.isValid)
    {
        return false;
    }
    pass.devicePixelRatio = q->devicePixelRatioF();
    if (!qFuzzyCompare(pass.devicePixelRatio, _renderDevicePixelRatio))
    {
        _vectorRenders.clear();
        _renderDevicePixelRatio = pass.devicePixelRatio;
    }
    pass.palette = ShadMapVectorStyle::palette(styleMode());
    pass.budget.start();
    return true;
}

const QPixmap* ShadMapPrivate::vectorRender(const VectorLayout& layout, int wrappedX, int tileY, VectorPass& pass)
{
    const int divisions = 1 << layout.shift;
    const int sourceX = wrappedX >> layout.shift;
    const int sourceY = tileY >> layout.shift;
    pass.keys.insert(ShadMapTileLoader::tileKey(layout.sourceLevel, sourceX, sourceY));
    const quint64 renderKey = ShadMapTileLoader::tileKey(layout.baseZoom, wrappedX, tileY);
    if (const QPixmap* rendered = _vectorRenders.object(renderKey))
    {
        return rendered;
    }
    const QSharedPointer<const ShadMapVectorTile> source = _loader->vectorTile(layout.sourceLevel, sourceX, sourceY);
    if (source.isNull())
    {
        return nullptr;
    }
    if (pass.budget.elapsed() >= kRenderBudgetMs)
    {
        pass.isPending = true;
        return nullptr;
    }
    const QRectF sourceRect(double(wrappedX % divisions) / divisions, double(tileY % divisions) / divisions, 1.0 / divisions, 1.0 / divisions);
    QPixmap* pixmap = new QPixmap(ShadMapVectorStyle::renderTile(*source, sourceRect, layout.logicalSize, pass.devicePixelRatio, layout.baseZoom, pass.palette));
    const int cost = qMax(1, int(qint64(pixmap->width()) * pixmap->height() * 4 / 1024));
    _vectorRenders.insert(renderKey, pixmap, cost);
    return _vectorRenders.object(renderKey);
}

bool ShadMapPrivate::vectorFallback(const VectorLayout& layout, int wrappedX, int tileY, const QPixmap*& pixmap, QRectF& fraction) const
{
    for (int level = 1; level <= kMaxFallbackLevels && layout.baseZoom - level >= 1; ++level)
    {
        const QPixmap* parent = _vectorRenders.object(ShadMapTileLoader::tileKey(layout.baseZoom - level, wrappedX >> level, tileY >> level));
        if (parent == nullptr)
        {
            continue;
        }
        const int divisions = 1 << level;
        pixmap = parent;
        fraction = QRectF(double(wrappedX % divisions) / divisions, double(tileY % divisions) / divisions, 1.0 / divisions, 1.0 / divisions);
        return true;
    }
    return false;
}

void ShadMapPrivate::endVectorPass(const VectorPass& pass)
{
    Q_Q(ShadMap);
    _loader->retain(pass.keys);
    if (pass.isPending)
    {
        QTimer::singleShot(0, q, [q]() { q->refresh(); });
    }
}

void ShadMapPrivate::paintTiles(QPainter& painter)
{
    Q_Q(ShadMap);
    _loader->setHighDpi(q->devicePixelRatioF() >= 1.5);
    const int tileZoom = rasterLevel(_zoom);
    const TileGrid grid = tileGrid(tileZoom);
    QSet<quint64> keys;
    const qreal margin = beginFrame(painter);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (int tileY = grid.firstY; tileY <= grid.lastY; ++tileY)
    {
        for (int tileX = grid.firstX; tileX <= grid.lastX; ++tileX)
        {
            const int wrappedX = wrapTile(grid, tileX);
            const QRectF target = tileTarget(grid, tileX, tileY).adjusted(-margin, -margin, margin, margin);
            keys.insert(ShadMapTileLoader::tileKey(tileZoom, wrappedX, tileY));
            const QPixmap pixmap = _loader->tile(tileZoom, wrappedX, tileY);
            if (!pixmap.isNull())
            {
                painter.drawPixmap(target, pixmap, QRectF(pixmap.rect()));
                continue;
            }
            for (int level = 1; level <= kMaxFallbackLevels && level <= tileZoom; ++level)
            {
                const QPixmap parent = _loader->cachedTile(tileZoom - level, wrappedX >> level, tileY >> level);
                if (parent.isNull())
                {
                    continue;
                }
                const int divisions = 1 << level;
                const qreal sourceWidth = qreal(parent.width()) / divisions;
                const qreal sourceHeight = qreal(parent.height()) / divisions;
                painter.drawPixmap(target, parent, QRectF((wrappedX % divisions) * sourceWidth, (tileY % divisions) * sourceHeight, sourceWidth, sourceHeight));
                break;
            }
        }
    }
    painter.restore();
    _loader->retain(keys);
}

qreal ShadMapPrivate::beginFrame(QPainter& painter) const
{
    painter.save();
    if (qFuzzyIsNull(_bearing))
    {
        return 0.0;
    }
    painter.setWorldTransform(frameTransform(), true);
    painter.setRenderHint(QPainter::Antialiasing, false);
    return kRotatedTileOverlap;
}

void ShadMapPrivate::paintVectorTiles(QPainter& painter)
{
    VectorLayout layout;
    VectorPass pass;
    if (!beginVectorPass(layout, pass))
    {
        return;
    }
    const TileGrid& grid = layout.grid;
    const qreal margin = beginFrame(painter);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    for (int tileY = grid.firstY; tileY <= grid.lastY; ++tileY)
    {
        for (int tileX = grid.firstX; tileX <= grid.lastX; ++tileX)
        {
            const int wrappedX = wrapTile(grid, tileX);
            const QPixmap* pixmap = vectorRender(layout, wrappedX, tileY, pass);
            QRectF fraction(0.0, 0.0, 1.0, 1.0);
            if (pixmap == nullptr && !vectorFallback(layout, wrappedX, tileY, pixmap, fraction))
            {
                continue;
            }
            painter.drawPixmap(tileTarget(grid, tileX, tileY).adjusted(-margin, -margin, margin, margin), *pixmap, QRectF(fraction.x() * pixmap->width(), fraction.y() * pixmap->height(), fraction.width() * pixmap->width(), fraction.height() * pixmap->height()));
        }
    }
    painter.restore();
    endVectorPass(pass);
}

void ShadMapPrivate::paintVectorLabels(QPainter& painter) const
{
    Q_Q(const ShadMap);
    const VectorLayout layout = vectorLayout();
    if (_tileFormat != ShadMapType::Vector || !layout.isValid)
    {
        return;
    }
    const TileGrid& grid = layout.grid;
    const int divisions = 1 << layout.shift;
    const double sourceCount = double(1 << layout.sourceLevel);
    const QStringList nameKeys = ShadMapVectorStyle::nameKeys();
    QList<ShadMapVectorStyle::Label> labels;
    const auto collect = [&](int wrappedX, int tileY, const ShadMapVectorStyle::PointMapper& mapper)
    {
        const QSharedPointer<const ShadMapVectorTile> source = _loader->cachedVectorTile(layout.sourceLevel, wrappedX >> layout.shift, tileY >> layout.shift);
        if (!source.isNull())
        {
            const QRectF sourceRect(double(wrappedX % divisions) / divisions, double(tileY % divisions) / divisions, 1.0 / divisions, 1.0 / divisions);
            ShadMapVectorStyle::collectLabels(*source, sourceRect, mapper, layout.baseZoom, nameKeys, labels);
        }
    };
    if (isGlobe())
    {
        const ShadMapProjection::GlobeView view = globeView();
        const QList<QPoint> tiles = globeTiles(view, grid.level);
        for (const QPoint& tile : tiles)
        {
            const int sourceX = tile.x() >> layout.shift;
            const int sourceY = tile.y() >> layout.shift;
            collect(tile.x(), tile.y(), [&view, sourceX, sourceY, sourceCount](const QPointF& point, QPointF& screen)
            {
                return ShadMapProjection::projectGlobe(view, ShadMapProjection::longitudeOf((sourceX + point.x()) / sourceCount), ShadMapProjection::latitudeOf((sourceY + point.y()) / sourceCount), screen, kGlobeVisibleThreshold);
            });
        }
    }
    else
    {
        const QTransform frame = frameTransform();
        for (int tileY = grid.firstY; tileY <= grid.lastY; ++tileY)
        {
            for (int tileX = grid.firstX; tileX <= grid.lastX; ++tileX)
            {
                const int wrappedX = wrapTile(grid, tileX);
                const QRectF target = tileTarget(grid, tileX, tileY);
                const QRectF sourceRect(double(wrappedX % divisions) / divisions, double(tileY % divisions) / divisions, 1.0 / divisions, 1.0 / divisions);
                collect(wrappedX, tileY, [target, sourceRect, &frame](const QPointF& point, QPointF& screen)
                {
                    screen = frame.map(QPointF(target.left() + (point.x() - sourceRect.left()) / sourceRect.width() * target.width(), target.top() + (point.y() - sourceRect.top()) / sourceRect.height() * target.height()));
                    return true;
                });
            }
        }
    }
    ShadMapVectorStyle::paintLabels(painter, labels, q->font(), QRectF(q->rect()), ShadMapVectorStyle::palette(styleMode()));
}

QList<QPoint> ShadMapPrivate::globeTiles(const ShadMapProjection::GlobeView& view, int level) const
{
    Q_Q(const ShadMap);
    QList<QPoint> tiles;
    const int count = 1 << level;
    if (level <= kGlobeScanLevel)
    {
        for (int tileY = 0; tileY < count; ++tileY)
        {
            for (int tileX = 0; tileX < count; ++tileX)
            {
                tiles.append(QPoint(tileX, tileY));
            }
        }
        return tiles;
    }
    bool isCovered = true;
    double minX = _center.x();
    double maxX = _center.x();
    double minY = _center.y();
    double maxY = _center.y();
    for (int row = 0; row <= kGlobeSampleCount && isCovered; ++row)
    {
        for (int column = 0; column <= kGlobeSampleCount; ++column)
        {
            double longitude = 0.0;
            double latitude = 0.0;
            if (!ShadMapProjection::unprojectGlobe(view, QPointF(q->width() * double(column) / kGlobeSampleCount, q->height() * double(row) / kGlobeSampleCount), longitude, latitude))
            {
                isCovered = false;
                break;
            }
            const QPointF normalized = ShadMapProjection::normalizedOf(longitude, latitude);
            const double deltaX = normalized.x() - _center.x();
            const double x = _center.x() + deltaX - std::round(deltaX);
            minX = qMin(minX, x);
            maxX = qMax(maxX, x);
            minY = qMin(minY, normalized.y());
            maxY = qMax(maxY, normalized.y());
        }
    }
    if (isCovered)
    {
        int firstX = qFloor(minX * count) - 1;
        int lastX = qFloor(maxX * count) + 1;
        if (lastX - firstX + 1 >= count)
        {
            firstX = 0;
            lastX = count - 1;
        }
        const int firstY = qMax(0, qFloor(minY * count) - 1);
        const int lastY = qMin(count - 1, qFloor(maxY * count) + 1);
        for (int tileY = firstY; tileY <= lastY; ++tileY)
        {
            for (int tileX = firstX; tileX <= lastX; ++tileX)
            {
                tiles.append(QPoint(((tileX % count) + count) % count, tileY));
            }
        }
        return tiles;
    }
    const QRectF viewport = QRectF(q->rect()).adjusted(-kGlobeTileMargin, -kGlobeTileMargin, kGlobeTileMargin, kGlobeTileMargin);
    for (int tileY = 0; tileY < count; ++tileY)
    {
        for (int tileX = 0; tileX < count; ++tileX)
        {
            bool isFront = false;
            QRectF bounds;
            for (int sample = 0; sample < 9; ++sample)
            {
                QPointF screen;
                if (!ShadMapProjection::projectGlobe(view, ShadMapProjection::longitudeOf((tileX + (sample % 3) / 2.0) / count), ShadMapProjection::latitudeOf((tileY + (sample / 3) / 2.0) / count), screen, kGlobeTileThreshold))
                {
                    continue;
                }
                bounds = isFront ? bounds.united(QRectF(screen, QSizeF(1.0, 1.0))) : QRectF(screen, QSizeF(1.0, 1.0));
                isFront = true;
            }
            if (isFront && bounds.intersects(viewport))
            {
                tiles.append(QPoint(tileX, tileY));
            }
        }
    }
    return tiles;
}

void ShadMapPrivate::drawGlobeTile(QPainter& painter, const ShadMapProjection::GlobeView& view, int level, int tileX, int tileY, const QPixmap& pixmap, const QRectF& fraction) const
{
    Q_Q(const ShadMap);
    const double count = double(1 << level);
    const double northLatitude = ShadMapProjection::latitudeOf(tileY / count);
    const double southLatitude = ShadMapProjection::latitudeOf((tileY + 1) / count);
    const double cellAngle = qMin(kGlobeMaximumCellAngle, std::sqrt(kGlobeCellError / view.radius));
    const double widest = northLatitude * southLatitude <= 0.0 ? 1.0 : std::cos(qMin(std::abs(northLatitude), std::abs(southLatitude)));
    const int columns = qBound(1, qCeil(2.0 * M_PI / count * widest / cellAngle), kGlobeMaximumCells);
    const int rows = qBound(1, qCeil((northLatitude - southLatitude) / cellAngle), kGlobeMaximumCells);
    const int stride = columns + 1;
    QVarLengthArray<QPointF, 289> points(stride * (rows + 1));
    QVarLengthArray<bool, 289> flags(stride * (rows + 1));
    for (int row = 0; row <= rows; ++row)
    {
        const double latitude = ShadMapProjection::latitudeOf((tileY + double(row) / rows) / count);
        for (int column = 0; column <= columns; ++column)
        {
            flags[row * stride + column] = ShadMapProjection::projectGlobe(view, ShadMapProjection::longitudeOf((tileX + double(column) / columns) / count), latitude, points[row * stride + column]);
        }
    }
    const QRectF viewport = QRectF(q->rect()).adjusted(-1.0, -1.0, 1.0, 1.0);
    const qreal sourceX = fraction.x() * pixmap.width();
    const qreal sourceY = fraction.y() * pixmap.height();
    const qreal sourceWidth = fraction.width() * pixmap.width() / columns;
    const qreal sourceHeight = fraction.height() * pixmap.height() / rows;
    for (int row = 0; row < rows; ++row)
    {
        for (int column = 0; column < columns; ++column)
        {
            const int index = row * stride + column;
            if (!flags[index] || !flags[index + 1] || !flags[index + stride] || !flags[index + stride + 1])
            {
                continue;
            }
            const QPointF topLeft = points[index];
            const QPointF topRight = points[index + 1];
            const QPointF bottomLeft = points[index + stride];
            const QPointF bottomRight = points[index + stride + 1];
            const qreal left = qMin(qMin(topLeft.x(), topRight.x()), qMin(bottomLeft.x(), bottomRight.x()));
            const qreal right = qMax(qMax(topLeft.x(), topRight.x()), qMax(bottomLeft.x(), bottomRight.x()));
            const qreal top = qMin(qMin(topLeft.y(), topRight.y()), qMin(bottomLeft.y(), bottomRight.y()));
            const qreal bottom = qMax(qMax(topLeft.y(), topRight.y()), qMax(bottomLeft.y(), bottomRight.y()));
            if (right < viewport.left() || left > viewport.right() || bottom < viewport.top() || top > viewport.bottom())
            {
                continue;
            }
            const QPointF horizontal = ((topRight - topLeft) + (bottomRight - bottomLeft)) / 2.0;
            const QPointF vertical = ((bottomLeft - topLeft) + (bottomRight - topRight)) / 2.0;
            const QPointF origin = (topLeft + topRight + bottomLeft + bottomRight) / 4.0 - (horizontal + vertical) / 2.0;
            const qreal marginX = kGlobeCellOverlap / qMax(1.0, std::hypot(horizontal.x(), horizontal.y()));
            const qreal marginY = kGlobeCellOverlap / qMax(1.0, std::hypot(vertical.x(), vertical.y()));
            const qreal cellLeft = qMax<qreal>(0.0, column - marginX);
            const qreal cellRight = qMin<qreal>(columns, column + 1.0 + marginX);
            const qreal cellTop = qMax<qreal>(0.0, row - marginY);
            const qreal cellBottom = qMin<qreal>(rows, row + 1.0 + marginY);
            painter.setWorldTransform(QTransform(horizontal.x(), horizontal.y(), vertical.x(), vertical.y(), origin.x(), origin.y()));
            painter.drawPixmap(QRectF(cellLeft - column, cellTop - row, cellRight - cellLeft, cellBottom - cellTop), pixmap, QRectF(sourceX + cellLeft * sourceWidth, sourceY + cellTop * sourceHeight, (cellRight - cellLeft) * sourceWidth, (cellBottom - cellTop) * sourceHeight));
        }
    }
    painter.resetTransform();
}

void ShadMapPrivate::paintGlobeTiles(QPainter& painter, const ShadMapProjection::GlobeView& view)
{
    Q_Q(ShadMap);
    if (_tileFormat == ShadMapType::Vector)
    {
        VectorLayout layout;
        VectorPass pass;
        if (!beginVectorPass(layout, pass))
        {
            return;
        }
        const QList<QPoint> tiles = globeTiles(view, layout.grid.level);
        for (const QPoint& tile : tiles)
        {
            const QPixmap* pixmap = vectorRender(layout, tile.x(), tile.y(), pass);
            QRectF fraction(0.0, 0.0, 1.0, 1.0);
            if (pixmap != nullptr || vectorFallback(layout, tile.x(), tile.y(), pixmap, fraction))
            {
                drawGlobeTile(painter, view, layout.grid.level, tile.x(), tile.y(), *pixmap, fraction);
            }
        }
        endVectorPass(pass);
        return;
    }
    _loader->setHighDpi(q->devicePixelRatioF() >= 1.5);
    const int level = rasterLevel(_zoom + std::log2(qMax(kGlobeMinimumCosine, view.cosLatitude)));
    const QList<QPoint> tiles = globeTiles(view, level);
    QSet<quint64> keys;
    for (const QPoint& tile : tiles)
    {
        keys.insert(ShadMapTileLoader::tileKey(level, tile.x(), tile.y()));
        const QPixmap pixmap = _loader->tile(level, tile.x(), tile.y());
        if (!pixmap.isNull())
        {
            drawGlobeTile(painter, view, level, tile.x(), tile.y(), pixmap, QRectF(0.0, 0.0, 1.0, 1.0));
            continue;
        }
        for (int step = 1; step <= kMaxFallbackLevels && step <= level; ++step)
        {
            const QPixmap parent = _loader->cachedTile(level - step, tile.x() >> step, tile.y() >> step);
            if (parent.isNull())
            {
                continue;
            }
            const int divisions = 1 << step;
            drawGlobeTile(painter, view, level, tile.x(), tile.y(), parent, QRectF(double(tile.x() % divisions) / divisions, double(tile.y() % divisions) / divisions, 1.0 / divisions, 1.0 / divisions));
            break;
        }
    }
    _loader->retain(keys);
}

void ShadMapPrivate::paintGlobe(QPainter& painter)
{
    Q_Q(ShadMap);
    const ShadMapProjection::GlobeView view = globeView();
    const QRectF bounds(q->rect());
    const bool isCovering = view.radius >= std::hypot(bounds.width(), bounds.height()) / 2.0 + 1.0;
    painter.fillRect(bounds, styleColor(isCovering ? ShadThemeType::Muted : ShadThemeType::Background));
    QPainterPath sphere;
    if (!isCovering)
    {
        sphere.addEllipse(view.origin, view.radius, view.radius);
        painter.fillPath(sphere, styleColor(ShadThemeType::Muted));
    }
    painter.save();
    if (!isCovering)
    {
        painter.setClipPath(sphere, Qt::IntersectClip);
    }
    painter.setRenderHint(QPainter::Antialiasing, false);
    painter.setRenderHint(QPainter::SmoothPixmapTransform, true);
    paintGlobeTiles(painter, view);
    painter.restore();
    if (!isCovering)
    {
        painter.setPen(QPen(styleColor(ShadThemeType::Border), 1.0));
        painter.setBrush(Qt::NoBrush);
        painter.drawPath(sphere);
    }
}

QPainterPath ShadMapPrivate::linePath(const QList<QPointF>& points) const
{
    QPainterPath path;
    if (points.isEmpty())
    {
        return path;
    }
    const double world = ShadMapProjection::worldSize(_zoom);
    if (!isGlobe())
    {
        const QPointF center = viewCenter();
        QPointF offset = unrotated(toScreen(points.first()) - center);
        path.moveTo(center + rotated(offset));
        for (qsizetype index = 1; index < points.size(); ++index)
        {
            offset += (points.at(index) - points.at(index - 1)) * world;
            path.lineTo(center + rotated(offset));
        }
        return path;
    }
    bool isPenDown = false;
    for (qsizetype index = 0; index < points.size(); ++index)
    {
        const QPointF from = points.at(qMax<qsizetype>(0, index - 1));
        const QPointF delta = points.at(index) - from;
        const int steps = qBound(1, qCeil(std::hypot(delta.x(), delta.y()) * world / kGlobeLineStep), kGlobeLineMaximumSteps);
        for (int step = 1; step <= steps; ++step)
        {
            const QPointF point = from + delta * (double(step) / steps);
            if (!isVisible(point))
            {
                isPenDown = false;
                continue;
            }
            const QPointF screen = toScreen(point);
            if (isPenDown)
            {
                path.lineTo(screen);
            }
            else
            {
                path.moveTo(screen);
            }
            isPenDown = true;
        }
    }
    return path;
}

QPainterPath ShadMapPrivate::greatCirclePath(const ShadMapCoordinate& from, const ShadMapCoordinate& to) const
{
    QPainterPath path;
    const ShadMapProjection::GlobeView view = globeView();
    const double fromLatitude = qDegreesToRadians(from.latitude);
    const double fromLongitude = qDegreesToRadians(from.longitude);
    const double toLatitude = qDegreesToRadians(to.latitude);
    const double toLongitude = qDegreesToRadians(to.longitude);
    const double fromVector[3] = {std::cos(fromLatitude) * std::cos(fromLongitude), std::cos(fromLatitude) * std::sin(fromLongitude), std::sin(fromLatitude)};
    const double toVector[3] = {std::cos(toLatitude) * std::cos(toLongitude), std::cos(toLatitude) * std::sin(toLongitude), std::sin(toLatitude)};
    const double angle = std::acos(qBound(-1.0, fromVector[0] * toVector[0] + fromVector[1] * toVector[1] + fromVector[2] * toVector[2], 1.0));
    const double sine = std::sin(angle);
    if (sine < 1e-9)
    {
        return path;
    }
    const int steps = qBound(kArcMinimumSteps, qCeil(angle * view.radius / kGlobeLineStep), kGlobeLineMaximumSteps);
    bool isPenDown = false;
    for (int step = 0; step <= steps; ++step)
    {
        const double fromWeight = std::sin(angle * (steps - step) / steps) / sine;
        const double toWeight = std::sin(angle * step / steps) / sine;
        const double x = fromVector[0] * fromWeight + toVector[0] * toWeight;
        const double y = fromVector[1] * fromWeight + toVector[1] * toWeight;
        const double z = fromVector[2] * fromWeight + toVector[2] * toWeight;
        QPointF screen;
        if (!ShadMapProjection::projectGlobe(view, std::atan2(y, x), std::asin(qBound(-1.0, z, 1.0)), screen))
        {
            isPenDown = false;
            continue;
        }
        if (isPenDown)
        {
            path.lineTo(screen);
        }
        else
        {
            path.moveTo(screen);
        }
        isPenDown = true;
    }
    return path;
}

void ShadMapPrivate::paintRoutes(QPainter& painter) const
{
    for (const ShadMapRoute* route : std::as_const(_routes))
    {
        const QList<ShadMapCoordinate> coordinates = route->getCoordinates();
        if (coordinates.size() < 2)
        {
            continue;
        }
        QList<QPointF> points;
        points.reserve(coordinates.size());
        QPointF previous = ShadMapProjection::project(coordinates.first());
        points.append(previous);
        for (int index = 1; index < coordinates.size(); ++index)
        {
            const QPointF current = ShadMapProjection::project(coordinates.at(index));
            const double deltaX = current.x() - previous.x();
            previous = QPointF(previous.x() + deltaX - std::round(deltaX), current.y());
            points.append(previous);
        }
        const QPainterPath path = linePath(points);
        const QColor color = route->getColor().isValid() ? route->getColor() : shadTheme->getColor(ShadThemeType::Primary);
        QPen pen(withAlpha(color, route->getOpacity()), route->getWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin);
        const QList<qreal> dashPattern = route->getDashPattern();
        if (!dashPattern.isEmpty())
        {
            pen.setDashPattern(dashPattern);
        }
        painter.setPen(pen);
        painter.setBrush(Qt::NoBrush);
        drawWorldCopies(painter, path);
    }
}

int ShadMapPrivate::worldCopies() const
{
    if (isGlobe())
    {
        return 0;
    }
    return qMin(kMaximumWorldCopies, qCeil(frameSize().width() / ShadMapProjection::worldSize(_zoom) / 2.0) + 1);
}

void ShadMapPrivate::drawWorldCopies(QPainter& painter, const QPainterPath& path) const
{
    Q_Q(const ShadMap);
    painter.drawPath(path);
    const double world = ShadMapProjection::worldSize(_zoom);
    const QRectF viewport = QRectF(q->rect()).adjusted(-kWorldCopyMargin, -kWorldCopyMargin, kWorldCopyMargin, kWorldCopyMargin);
    const QRectF bounds = path.boundingRect();
    const int copies = worldCopies();
    for (int copy = -copies; copy <= copies; ++copy)
    {
        const QPointF offset = rotated(QPointF(copy * world, 0.0));
        if (copy != 0 && bounds.translated(offset).intersects(viewport))
        {
            painter.drawPath(path.translated(offset));
        }
    }
}

void ShadMapPrivate::paintArcs(QPainter& painter) const
{
    const double world = ShadMapProjection::worldSize(_zoom);
    for (const ShadMapArc* arc : std::as_const(_arcs))
    {
        const QColor arcColor = arc->getColor().isValid() ? arc->getColor() : shadTheme->getColor(ShadThemeType::Primary);
        if (isGlobe())
        {
            painter.setPen(QPen(arcColor, arc->getWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
            painter.setBrush(Qt::NoBrush);
            painter.drawPath(greatCirclePath(arc->getFrom(), arc->getTo()));
            continue;
        }
        const QPointF fromNormalized = ShadMapProjection::project(arc->getFrom());
        const QPointF toNormalized = ShadMapProjection::project(arc->getTo());
        const double deltaX = toNormalized.x() - fromNormalized.x();
        const QPointF from = toScreen(fromNormalized);
        const QPointF to = from + rotated(QPointF((deltaX - std::round(deltaX)) * world, (toNormalized.y() - fromNormalized.y()) * world));
        if (from == to)
        {
            continue;
        }
        QPointF normal(to.y() - from.y(), from.x() - to.x());
        if (normal.y() > 0.0)
        {
            normal = -normal;
        }
        QPainterPath path;
        path.moveTo(from);
        path.quadTo((from + to) / 2.0 + normal * arc->getCurvature(), to);
        painter.setPen(QPen(arcColor, arc->getWidth(), Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.setBrush(Qt::NoBrush);
        drawWorldCopies(painter, path);
    }
}

void ShadMapPrivate::paintItems(QPainter& painter) const
{
    Q_Q(const ShadMap);
    const QColor background = shadTheme->getColor(ShadThemeType::Background);
    const QColor foreground = shadTheme->getColor(ShadThemeType::Foreground);
    const QColor border = shadTheme->getColor(ShadThemeType::Border);
    const QColor primary = shadTheme->getColor(ShadThemeType::Primary);
    const QFont labelFont = scaledFont(q, kLabelFontSize, QFont::Medium);
    const QFont pillFont = scaledFont(q, kLabelFontSize, QFont::DemiBold);
    const QFontMetricsF labelMetrics(labelFont);
    painter.setFont(labelFont);
    const QPointF userNormalized = ShadMapProjection::project(_userLocation);
    if (_hasUserLocation && isVisible(userNormalized))
    {
        const QPointF userPosition = toScreen(userNormalized);
        const QColor userColor(0x3b, 0x82, 0xf6);
        painter.setPen(Qt::NoPen);
        painter.setBrush(withAlpha(userColor, 0.2));
        painter.drawEllipse(userPosition, kUserHaloRadius, kUserHaloRadius);
        painter.setPen(QPen(QColor(Qt::white), kUserRingWidth));
        painter.setBrush(userColor);
        painter.drawEllipse(userPosition, kUserDotRadius, kUserDotRadius);
    }
    const QList<Item> items = layoutItems();
    for (const Item& item : items)
    {
        const QPointF position = item.position;
        if (item.markers.size() > 1)
        {
            const qreal radius = itemRadius(item);
            painter.setPen(Qt::NoPen);
            painter.setBrush(withAlpha(primary, 0.25));
            painter.drawEllipse(position, radius + 4.0, radius + 4.0);
            painter.setPen(QPen(background, 2.0));
            painter.setBrush(primary);
            painter.drawEllipse(position, radius, radius);
            painter.setPen(shadTheme->getColor(ShadThemeType::PrimaryForeground));
            painter.drawText(QRectF(position.x() - radius, position.y() - radius, 2.0 * radius, 2.0 * radius), Qt::AlignCenter, QString::number(item.markers.size()));
            continue;
        }
        const ShadMapMarker* marker = item.markers.first();
        const QColor markerColor = marker->getColor().isValid() ? marker->getColor() : primary;
        const QRectF body = markerRect(marker, position);
        const QString text = marker->getText();
        painter.setPen(Qt::NoPen);
        if (marker->getContentWidget() != nullptr)
        {
            painter.setBrush(Qt::NoBrush);
        }
        else if (!text.isEmpty())
        {
            const qreal radius = body.height() / 2.0;
            painter.setBrush(QColor(0, 0, 0, 40));
            painter.drawRoundedRect(body.adjusted(-0.5, 0.5, 0.5, 1.5), radius + 0.5, radius + 0.5);
            painter.setPen(QPen(marker->isSelected() ? primary : border, 1.0));
            painter.setBrush(marker->isSelected() ? primary : (marker->getColor().isValid() ? marker->getColor() : background));
            painter.drawRoundedRect(body, radius, radius);
            painter.setFont(pillFont);
            painter.setPen(marker->isSelected() ? shadTheme->getColor(ShadThemeType::PrimaryForeground) : foreground);
            painter.drawText(body, Qt::AlignCenter, text);
            painter.setFont(labelFont);
        }
        else
        {
            if (marker->isSelected())
            {
                painter.setBrush(withAlpha(markerColor, 0.3));
                painter.drawEllipse(position, kMarkerRingRadius + kSelectedHaloWidth, kMarkerRingRadius + kSelectedHaloWidth);
            }
            painter.setBrush(QColor(0, 0, 0, 48));
            painter.drawEllipse(position + QPointF(0.0, 1.0), kMarkerRingRadius + 1.0, kMarkerRingRadius + 1.0);
            painter.setBrush(marker->getRingColor().isValid() ? marker->getRingColor() : background);
            painter.drawEllipse(position, qreal(kMarkerRingRadius), qreal(kMarkerRingRadius));
            painter.setBrush(markerColor);
            painter.drawEllipse(position, qreal(kMarkerRadius), qreal(kMarkerRadius));
        }
        const QString label = marker->getLabel();
        if (label.isEmpty())
        {
            continue;
        }
        const qreal labelWidth = labelMetrics.horizontalAdvance(label) + 2.0 * kLabelPaddingX;
        const qreal labelHeight = labelMetrics.height() + 2.0 * kLabelPaddingY;
        const qreal labelTop = marker->getLabelPosition() == ShadMapType::Top ? body.top() - kLabelGap - labelHeight : body.bottom() + kLabelGap;
        const QRectF labelRect(position.x() - labelWidth / 2.0, labelTop, labelWidth, labelHeight);
        painter.setPen(QPen(border, 1.0));
        painter.setBrush(background);
        painter.drawRoundedRect(labelRect, labelHeight / 2.0, labelHeight / 2.0);
        painter.setPen(foreground);
        painter.drawText(labelRect, Qt::AlignCenter, label);
    }
}

void ShadMapPrivate::paintAttribution(QPainter& painter) const
{
    Q_Q(const ShadMap);
    const QRectF rect = attributionRect();
    if (rect.isEmpty())
    {
        return;
    }
    const QFont font = scaledFont(q, kAttributionFontSize, QFont::Normal);
    const QFontMetricsF metrics(font);
    painter.setPen(Qt::NoPen);
    painter.setBrush(withAlpha(shadTheme->getColor(ShadThemeType::Background), 0.8));
    painter.drawRoundedRect(rect, rect.height() / 2.0, rect.height() / 2.0);
    painter.setFont(font);
    painter.setPen(shadTheme->getColor(ShadThemeType::MutedForeground));
    painter.drawText(rect, Qt::AlignCenter, metrics.elidedText(_attribution, Qt::ElideRight, rect.width() - 2.0 * kLabelPaddingX));
}
