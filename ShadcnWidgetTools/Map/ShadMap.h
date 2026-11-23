#ifndef SHADMAP_H
#define SHADMAP_H

#include <QByteArray>
#include <QList>
#include <QPointF>
#include <QSize>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QWidget>

#include "ShadIconType.h"
#include "ShadMapDef.h"
#include "ShadProperty.h"

class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QPaintEvent;
class QPainter;
class QResizeEvent;
class QWheelEvent;
class ShadButton;
class ShadMapArc;
class ShadMapMarker;
class ShadMapPopup;
class ShadMapPrivate;
class ShadMapRoute;

class SHADCN_MAP_EXPORT ShadMap : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadMap)
    Q_PROPERTY(double zoom READ getZoom WRITE setZoom NOTIFY viewportChanged)
    Q_PROPERTY(double minimumZoom READ getMinimumZoom WRITE setMinimumZoom)
    Q_PROPERTY(double maximumZoom READ getMaximumZoom WRITE setMaximumZoom)
    Q_PROPERTY(QString attribution READ getAttribution WRITE setAttribution)
    Q_PROPERTY(bool interactive READ isInteractive WRITE setInteractive)
    Q_PROPERTY(bool loading READ isLoading WRITE setLoading)
    Q_PROPERTY(bool controlsVisible READ isControlsVisible WRITE setControlsVisible)
    Q_PROPERTY(ShadMapType::ControlPosition controlPosition READ getControlPosition WRITE setControlPosition)
    Q_PROPERTY(bool clusteringEnabled READ isClusteringEnabled WRITE setClusteringEnabled)
    Q_PROPERTY(ShadMapType::TileFormat tileFormat READ getTileFormat WRITE setTileFormat)
    Q_PROPERTY(ShadMapType::Projection projection READ getProjection WRITE setProjection)
    Q_PROPERTY(ShadMapType::Theme theme READ getTheme WRITE setTheme)
    Q_PROPERTY(double bearing READ getBearing WRITE setBearing NOTIFY viewportChanged)
    Q_PROPERTY(bool rotationEnabled READ isRotationEnabled WRITE setRotationEnabled)
    Q_PROPERTY(QString vectorTileUrl READ getVectorTileUrl WRITE setVectorTileUrl)

    public:
        explicit ShadMap(QWidget* parent = nullptr);
        ~ShadMap() override;

        void setTileUrl(const QString& lightUrl, const QString& darkUrl = QString());
        QString getTileUrl() const;
        QString getDarkTileUrl() const;

        void setProjection(ShadMapType::Projection projection);
        ShadMapType::Projection getProjection() const;

        void setBearing(double bearing);
        double getBearing() const;

        void setRotationEnabled(bool enabled);
        bool isRotationEnabled() const;

        void setCompassControlVisible(bool visible);
        bool isCompassControlVisible() const;

        void setTheme(ShadMapType::Theme theme);
        ShadMapType::Theme getTheme() const;

        void setTileFormat(ShadMapType::TileFormat format);
        ShadMapType::TileFormat getTileFormat() const;

        void setVectorTileUrl(const QString& url);
        QString getVectorTileUrl() const;

        void setTileSubdomains(const QStringList& subdomains);
        QStringList getTileSubdomains() const;

        void setAttribution(const QString& attribution);
        QString getAttribution() const;

        void setCenter(const ShadMapCoordinate& center);
        ShadMapCoordinate getCenter() const;

        void setZoom(double zoom);
        double getZoom() const;

        void setMinimumZoom(double zoom);
        double getMinimumZoom() const;

        void setMaximumZoom(double zoom);
        double getMaximumZoom() const;

        void setInteractive(bool interactive);
        bool isInteractive() const;

        void setLoading(bool loading);
        bool isLoading() const;

        void setControlsVisible(bool visible);
        bool isControlsVisible() const;

        void setControlPosition(ShadMapType::ControlPosition position);
        ShadMapType::ControlPosition getControlPosition() const;

        void setClusteringEnabled(bool enabled);
        bool isClusteringEnabled() const;

        void flyTo(const ShadMapCoordinate& center, double zoom);
        void fitBounds(const QList<ShadMapCoordinate>& coordinates, int padding = 48);

        ShadMapCoordinate coordinateAt(const QPointF& position) const;
        QPointF positionOf(const ShadMapCoordinate& coordinate) const;

        ShadMapMarker* addMarker(const ShadMapCoordinate& coordinate);
        void removeMarker(ShadMapMarker* marker);
        QList<ShadMapMarker*> markers() const;
        void clearMarkers();

        ShadMapRoute* addRoute(const QList<ShadMapCoordinate>& coordinates);
        void removeRoute(ShadMapRoute* route);

        ShadMapArc* addArc(const ShadMapCoordinate& from, const ShadMapCoordinate& to);
        void removeArc(ShadMapArc* arc);

        ShadMapPopup* addPopup(const ShadMapCoordinate& coordinate, const QString& text = QString());
        void removePopup(ShadMapPopup* popup);

        void showMarkerPopup(ShadMapMarker* marker);
        void hideMarkerPopup();

        void addOverlay(QWidget* widget, ShadMapType::ControlPosition position = ShadMapType::TopLeft);
        void removeOverlay(QWidget* widget);

        QList<ShadMapArc*> addArcs(const QList<ShadMapArcData>& arcs);
        ShadMapArc* arc(const QString& id) const;
        QList<ShadMapArc*> arcs() const;
        void clearArcs();

        QList<ShadMapMarker*> addGeoJson(const QByteArray& data);
        void loadGeoJson(const QUrl& url);

        void setZoomControlVisible(bool visible);
        bool isZoomControlVisible() const;

        void setResetControlVisible(bool visible);
        bool isResetControlVisible() const;

        void setLocateControlVisible(bool visible);
        bool isLocateControlVisible() const;

        ShadButton* addControlButton(ShadIconType::IconName icon, const QString& toolTip = QString());
        void removeControlButton(ShadButton* button);
        void addControlSeparator();

        void setUserLocation(const ShadMapCoordinate& coordinate);
        ShadMapCoordinate getUserLocation() const;
        bool hasUserLocation() const;
        void clearUserLocation();

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    public Q_SLOTS:
        void zoomIn();
        void zoomOut();
        void resetView();
        void resetNorth();
        void locate();

    Q_SIGNALS:
        void viewportChanged();
        void clicked(const ShadMapCoordinate& coordinate);
        void markerClicked(ShadMapMarker* marker);
        void locateRequested();
        void geoJsonLoaded(const QUrl& url, int count);
        void geoJsonFailed(const QUrl& url, const QString& error);

    protected:
        bool event(QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mouseReleaseEvent(QMouseEvent* event) override;
        void mouseDoubleClickEvent(QMouseEvent* event) override;
        void wheelEvent(QWheelEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void paintBase(QPainter& painter);
        void paintCanvas(QPainter& painter);
        void refresh();
        void refreshOverlay();
};

#endif // SHADMAP_H
