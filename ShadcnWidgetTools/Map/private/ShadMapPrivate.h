#ifndef SHADMAPPRIVATE_H
#define SHADMAPPRIVATE_H

#include <QCache>
#include <QColor>
#include <QElapsedTimer>
#include <QList>
#include <QPainterPath>
#include <QPixmap>
#include <QPoint>
#include <QPointF>
#include <QPointer>
#include <QRectF>
#include <QSet>
#include <QSizeF>
#include <QString>
#include <QStringList>
#include <QTransform>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadMapDef.h"
#include "ShadMapProjection.h"
#include "ShadMapVectorStyle.h"
#include "ShadProperty.h"

class QPainter;
class QTimer;
class QVariantAnimation;
class ShadButton;
class ShadButtonGroup;
class ShadMap;
class ShadMapArc;
class ShadMapCompassButton;
class ShadMapMarker;
class ShadMapPopup;
class ShadMapRoute;
class ShadMapTileLoader;
class ShadSpinner;

class ShadMapPrivate
{
    SHAD_D_CREATE(ShadMap)

    public:
        ShadMapPrivate();
        ~ShadMapPrivate();

        static constexpr double kDefaultMinimumZoom = 1.0;
        static constexpr double kDefaultMaximumZoom = 19.0;
        static constexpr double kHighestZoom = 22.0;
        static constexpr int kMaxFallbackLevels = 4;
        static constexpr int kControlInset = 12;
        static constexpr int kControlFocusMargin = 3;
        static constexpr int kAttributionInset = 8;
        static constexpr int kMarkerRadius = 6;
        static constexpr int kMarkerRingRadius = 9;
        static constexpr int kMarkerHitRadius = 12;
        static constexpr int kLabelGap = 4;
        static constexpr int kClusterCellSize = 56;
        static constexpr int kPanStep = 80;
        static constexpr int kAnimationDurationMs = 300;
        static constexpr int kSnapDelayMs = 180;
        static constexpr int kCanvasReleaseMs = 30000;
        static constexpr int kSpinnerSize = 24;
        static constexpr int kRenderCacheKilobytes = 64 * 1024;
        static constexpr qint64 kRenderBudgetMs = 12;
        static constexpr int kBuiltinControlCount = 5;
        static constexpr double kRotateDegreesPerPixel = 0.5;
        static constexpr int kMaximumWorldCopies = 8;
        static constexpr int kMaximumFitOverlays = 4;
        static constexpr double kUnboundedScale = 1e18;
        static constexpr double kWorldCopyMargin = 48.0;
        static constexpr double kLocateZoom = 14.0;
        static constexpr int kGlobeScanLevel = 2;
        static constexpr int kGlobeSampleCount = 12;
        static constexpr int kGlobeMaximumCells = 64;
        static constexpr int kGlobeLineMaximumSteps = 256;
        static constexpr double kGlobeLineStep = 12.0;
        static constexpr double kGlobeCellError = 0.75;
        static constexpr double kGlobeMaximumCellAngle = 0.12;
        static constexpr double kGlobeCellOverlap = 0.6;
        static constexpr double kGlobeTileThreshold = -0.2;
        static constexpr double kGlobeVisibleThreshold = 0.02;
        static constexpr double kGlobeTileMargin = 48.0;
        static constexpr double kGlobeMinimumCosine = 0.1;

        struct Item
        {
            QPointF position;
            QList<ShadMapMarker*> markers;
        };

        struct TileGrid
        {
            int level{0};
            int tileCount{1};
            double extent{0.0};
            double left{0.0};
            double top{0.0};
            int firstX{0};
            int lastX{-1};
            int firstY{0};
            int lastY{-1};
        };

        struct VectorLayout
        {
            TileGrid grid;
            int baseZoom{0};
            int sourceLevel{0};
            int shift{0};
            int logicalSize{0};
            bool isValid{false};
        };

        struct VectorPass
        {
            ShadMapVectorStyle::Palette palette;
            QElapsedTimer budget;
            QSet<quint64> keys;
            qreal devicePixelRatio{1.0};
            bool isPending{false};
        };

        double clampZoom(double zoom) const;
        QPointF clampCenter(const QPointF& center, double zoom) const;
        bool isGlobe() const;
        ShadThemeType::ThemeMode styleMode() const;
        QColor styleColor(ShadThemeType::ThemeColor color) const;
        ShadMapProjection::GlobeView globeView() const;
        QPointF viewCenter() const;
        QPointF rotated(const QPointF& offset) const;
        QPointF unrotated(const QPointF& offset) const;
        QPointF unrotatedScreen(const QPointF& screen) const;
        QSizeF frameSize() const;
        QTransform frameTransform() const;
        qreal beginFrame(QPainter& painter) const;
        void applyBearing(double bearing);
        QPointF toScreen(const QPointF& normalized) const;
        QPointF toNormalized(const QPointF& screen) const;
        bool isVisible(const QPointF& normalized) const;
        bool isOnMap(const QPointF& screen) const;
        QRectF canvasRect() const;

        void setView(const QPointF& center, double zoom);
        void panBy(const QPointF& delta);
        void zoomAround(const QPointF& screen, double zoom);
        void animateTo(const QPointF& center, double zoom);
        void animateView(const QPointF& center, double zoom, double bearing);
        void animateZoomAround(const QPointF& screen, double zoom);
        void stopAnimation();
        void applyAnimation(qreal progress);
        void scheduleSnap(const QPointF& screen);
        void rememberHome();
        bool fitView(const QList<ShadMapCoordinate>& coordinates, int padding, QPointF& center, double& zoom) const;
        QRectF fitArea(double spanX, double spanY, int padding) const;
        void applyFit();

        void applyTileSource();
        void rebuildControls();
        void updateControls();
        void updateMarkerWidgets();
        void updatePopups();
        void retranslate();

        void rebuildClusters() const;
        QList<Item> layoutItems() const;
        int itemAt(const QList<Item>& items, const QPointF& position) const;
        qreal itemRadius(const Item& item) const;
        QRectF markerRect(const ShadMapMarker* marker, const QPointF& position) const;
        void togglePopup(ShadMapMarker* marker);
        void showMarkerPopup(ShadMapMarker* marker);
        void applyMarkerPopup(const ShadMapMarker* marker);
        void restack();
        bool updateOverlays();
        void syncMarkerPopup(ShadMapMarker* marker);
        void closeMarkerPopup();
        void zoomToItem(const Item& item);

        QRectF attributionRect() const;
        TileGrid tileGrid(int level) const;
        VectorLayout vectorLayout() const;
        int rasterLevel(double zoom) const;
        bool beginVectorPass(VectorLayout& layout, VectorPass& pass);
        const QPixmap* vectorRender(const VectorLayout& layout, int wrappedX, int tileY, VectorPass& pass);
        bool vectorFallback(const VectorLayout& layout, int wrappedX, int tileY, const QPixmap*& pixmap, QRectF& fraction) const;
        void endVectorPass(const VectorPass& pass);
        void paintTiles(QPainter& painter);
        void paintVectorTiles(QPainter& painter);
        void paintVectorLabels(QPainter& painter) const;
        QList<QPoint> globeTiles(const ShadMapProjection::GlobeView& view, int level) const;
        void drawGlobeTile(QPainter& painter, const ShadMapProjection::GlobeView& view, int level, int tileX, int tileY, const QPixmap& pixmap, const QRectF& fraction) const;
        void paintGlobe(QPainter& painter);
        void paintGlobeTiles(QPainter& painter, const ShadMapProjection::GlobeView& view);
        QPainterPath linePath(const QList<QPointF>& points) const;
        QPainterPath greatCirclePath(const ShadMapCoordinate& from, const ShadMapCoordinate& to) const;
        int worldCopies() const;
        void drawWorldCopies(QPainter& painter, const QPainterPath& path) const;
        void appendWorldCopies(QList<Item>& items) const;
        void paintRoutes(QPainter& painter) const;
        void paintArcs(QPainter& painter) const;
        void paintItems(QPainter& painter) const;
        void paintAttribution(QPainter& painter) const;

        ShadMapTileLoader* _loader{nullptr};
        ShadButtonGroup* _controls{nullptr};
        ShadButton* _zoomInButton{nullptr};
        ShadButton* _zoomOutButton{nullptr};
        ShadButton* _resetButton{nullptr};
        ShadButton* _locateButton{nullptr};
        ShadMapCompassButton* _compassButton{nullptr};
        ShadSpinner* _spinner{nullptr};
        QVariantAnimation* _animation{nullptr};
        QTimer* _snapTimer{nullptr};

        QString _lightUrl;
        QString _darkUrl;
        QString _vectorUrl;
        QString _attribution;
        ShadMapType::TileFormat _tileFormat{ShadMapType::Raster};
        ShadMapType::Projection _projection{ShadMapType::Mercator};
        ShadMapType::Theme _theme{ShadMapType::Auto};
        QCache<quint64, QPixmap> _vectorRenders;
        QPixmap _canvas;
        QPixmap _tileLayer;
        QPixmap _labelLayer;
        QTimer* _canvasTimer{nullptr};
        bool _isCanvasDirty{true};
        bool _isBaseDirty{true};
        qreal _renderDevicePixelRatio{0.0};

        QPointF _center{0.5, 0.5};
        QPointF _requestedCenter{0.5, 0.5};
        double _zoom{2.0};
        double _minimumZoom{kDefaultMinimumZoom};
        double _maximumZoom{kDefaultMaximumZoom};
        QPointF _homeCenter{0.5, 0.5};
        double _homeZoom{2.0};

        QList<ShadMapCoordinate> _fitCoordinates;
        int _fitPadding{0};
        bool _hasFit{false};

        QPointF _animationFromCenter;
        QPointF _animationToCenter;
        double _animationFromZoom{0.0};
        double _animationToZoom{0.0};
        QPointF _animationAnchor;
        QPointF _animationAnchorScreen;
        bool _isAnimationAnchored{false};
        QPointF _snapPosition;

        QList<ShadMapMarker*> _markers;
        mutable QList<QList<ShadMapMarker*>> _clusterGroups;
        mutable double _clusterZoom{0.0};
        mutable bool _isClusterValid{false};
        QList<ShadMapRoute*> _routes;
        QList<ShadMapArc*> _arcs;
        QList<ShadMapPopup*> _popups;
        QList<QPointer<QWidget>> _overlays;
        QList<ShadMapType::ControlPosition> _overlayPositions;
        QPointer<ShadMapPopup> _markerPopup;
        QPointer<ShadMapMarker> _popupMarker;

        QPointer<ShadMapMarker> _pressedMarker;
        QPointF _pressPosition;
        QPointF _lastPosition;
        bool _isPressed{false};
        bool _isDragging{false};

        ShadMapType::ControlPosition _controlPosition{ShadMapType::TopRight};
        bool _isInteractive{true};
        bool _isLoading{false};
        bool _isControlsVisible{true};
        bool _isClusteringEnabled{false};
        bool _isZoomControlVisible{true};
        bool _isResetControlVisible{true};
        bool _isLocateControlVisible{false};
        bool _isCompassControlVisible{false};
        bool _isRotationEnabled{true};
        bool _isRotating{false};
        double _bearing{0.0};
        double _animationFromBearing{0.0};
        double _animationToBearing{0.0};
        bool _hasUserLocation{false};
        ShadMapCoordinate _userLocation;
        bool _isFocusVisible{false};
};

#endif // SHADMAPPRIVATE_H
