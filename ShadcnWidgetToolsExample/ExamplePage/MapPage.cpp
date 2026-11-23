#include "MapPage.h"

#include <QAbstractButton>
#include <QColor>
#include <QFont>
#include <QHBoxLayout>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QLatin1Char>
#include <QList>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QPen>
#include <QPointF>
#include <QRectF>
#include <QString>
#include <QStringList>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>
#include <QtMath>

#include <cmath>
#include <functional>
#include <initializer_list>
#include <memory>

#include "ExampleI18n.h"
#include "ShadBadge.h"
#include "ShadButton.h"
#include "ShadCard.h"
#include "ShadDef.h"
#include "ShadIcon.h"
#include "ShadIconType.h"
#include "ShadItem.h"
#include "ShadLabel.h"
#include "ShadMap.h"
#include "ShadMapDef.h"
#include "ShadMapMarker.h"
#include "ShadMapPopup.h"
#include "ShadMapRoute.h"
#include "ShadSegmented.h"
#include "ShadTheme.h"

namespace
{
    constexpr int kMapHeight = 360;
    constexpr int kStaticMapWidth = 420;
    constexpr int kStaticMapHeight = 240;
    constexpr int kHomesMapHeight = 520;
    constexpr int kPanelWidth = 260;
    constexpr int kHomesPanelWidth = 280;
    constexpr int kStatSpacing = 2;
    constexpr int kStatGap = 24;
    constexpr int kStatValueFontSize = 16;
    constexpr int kStatCaptionFontSize = 12;
    constexpr int kPriceFontSize = 13;
    constexpr int kFlightMapHeight = 420;
    constexpr double kGlobeZoom = 2.5;
    constexpr double kEarthRadiusKm = 6371.0;
    constexpr int kTrackIntervalMs = 50;
    constexpr double kTrackDurationMs = 20000.0;
    constexpr double kTrackMinutes = 7.0;
    constexpr double kRailDurationMs = 30000.0;
    constexpr double kRailHours = 144.0;
    constexpr double kRailKilometers = 9288.0;
    constexpr int kRailPanelWidth = 300;
    constexpr int kRailPadding = 32;
    constexpr int kLegendLineWidth = 18;
    constexpr int kLegendLineHeight = 12;
    constexpr int kLegendSpacing = 8;
    constexpr qreal kLegendStrokeWidth = 2.0;
    constexpr int kVehicleIconSize = 28;
    constexpr int kVehicleGlyphSize = 16;
    constexpr int kFitPadding = 64;
    constexpr int kFleetPadding = 32;
    constexpr int kPopupOffset = 13;
    constexpr int kClusterPointsPerCity = 24;
    constexpr double kCityZoom = 11.0;
    constexpr double kDistrictZoom = 12.0;
    constexpr double kCountryZoom = 4.0;
    constexpr double kWorldZoom = 2.0;
    constexpr double kClusterSpread = 3.0;
    constexpr double kArcCurvature = 0.3;
    constexpr qreal kDashedRouteWidth = 3.0;

    const ShadMapCoordinate kTaipei(25.0330, 121.5654);
    const ShadMapCoordinate kShanghai(31.2304, 121.4737);
    const ShadMapCoordinate kTaipei101(25.0338, 121.5645);
    const ShadMapCoordinate kMemorialHall(25.0346, 121.5218);
    const ShadMapCoordinate kTaipeiStation(25.0478, 121.5170);
    const ShadMapCoordinate kLongshanTemple(25.0372, 121.4999);
    const ShadMapCoordinate kTaoyuan(25.0130, 121.2150);
    const ShadMapCoordinate kHsinchu(24.8080, 121.0403);
    const ShadMapCoordinate kTaichung(24.1123, 120.6157);
    const ShadMapCoordinate kChiayi(23.4597, 120.3239);
    const ShadMapCoordinate kTainan(22.9249, 120.2857);
    const ShadMapCoordinate kKaohsiung(22.6871, 120.3080);
    const ShadMapCoordinate kYilan(24.7547, 121.7581);
    const ShadMapCoordinate kHualien(23.9930, 121.6011);
    const ShadMapCoordinate kBeijing(39.9042, 116.4074);
    const ShadMapCoordinate kGuangzhou(23.1291, 113.2644);
    const ShadMapCoordinate kChengdu(30.5728, 104.0668);
    const ShadMapCoordinate kWuhan(30.5928, 114.3055);
    const ShadMapCoordinate kXian(34.3416, 108.9398);
    const ShadMapCoordinate kTokyo(35.6762, 139.6503);
    const ShadMapCoordinate kSingapore(1.3521, 103.8198);
    const ShadMapCoordinate kDubai(25.2048, 55.2708);
    const ShadMapCoordinate kLondon(51.5074, -0.1278);
    const ShadMapCoordinate kNewYork(40.7128, -74.0060);
    const ShadMapCoordinate kAsiaCenter(30.0, 70.0);
    const ShadMapCoordinate kChinaCenter(33.0, 113.0);

    const QColor kBlue(0x25, 0x63, 0xeb);
    const QColor kGreen(0x16, 0xa3, 0x4a);
    const QColor kAmber(0xd9, 0x77, 0x06);
    const QColor kRed(0xdc, 0x26, 0x26);
    const QColor kGray(0x9c, 0xa3, 0xaf);

    ShadMap* createMap(QWidget* parent)
    {
        ShadMap* map = new ShadMap(parent);
        const QString cartoKey = qEnvironmentVariable("SHADCN_EXAMPLE_CARTO_KEY").trimmed();
        if (cartoKey.isEmpty())
        {
            map->setTileUrl(QStringLiteral("https://tile.openstreetmap.org/{z}/{x}/{y}.png"));
            map->setAttribution(QStringLiteral("© OpenStreetMap contributors"));
        }
        else
        {
            const QString keyQuery = QStringLiteral("?key=") + QString::fromLatin1(QUrl::toPercentEncoding(cartoKey));
            map->setTileUrl(QStringLiteral("https://{s}.basemaps.cartocdn.com/rastertiles/light_all/{z}/{x}/{y}.png") + keyQuery, QStringLiteral("https://{s}.basemaps.cartocdn.com/rastertiles/dark_all/{z}/{x}/{y}.png") + keyQuery);
            map->setTileSubdomains(QStringList{QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("c"), QStringLiteral("d")});
            map->setAttribution(QStringLiteral("© OpenStreetMap contributors © CARTO"));
        }
        map->setFixedHeight(kMapHeight);
        return map;
    }

    ShadMapMarker* addLabeledMarker(ShadMap* map, const ShadMapCoordinate& coordinate, ExampleI18n::TextFunction label)
    {
        ShadMapMarker* marker = map->addMarker(coordinate);
        ExampleI18n::bind(marker, [marker, label]() { marker->setLabel(label()); });
        return marker;
    }

    QString vectorAttribution()
    {
        return QStringLiteral("© OpenMapTiles © OpenStreetMap contributors");
    }

    ShadMap* createVectorMap(QWidget* parent)
    {
        ShadMap* map = createMap(parent);
        map->setVectorTileUrl(QStringLiteral("https://tiles.openfreemap.org/planet"));
        map->setTileFormat(ShadMapType::Vector);
        map->setAttribution(vectorAttribution());
        return map;
    }

    ShadLabel* createTextLabel(QWidget* parent, int pixelSize, QFont::Weight weight, ShadThemeType::ThemeColor colorRole)
    {
        ShadLabel* label = new ShadLabel(parent);
        QFont font = label->font();
        font.setPixelSize(pixelSize);
        font.setWeight(weight);
        label->setFont(font);
        label->setColorRole(colorRole);
        return label;
    }

    QWidget* createStat(QWidget* parent, ExampleI18n::TextFunction value, ExampleI18n::TextFunction caption)
    {
        QWidget* stat = new QWidget(parent);
        QVBoxLayout* layout = new QVBoxLayout(stat);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kStatSpacing);
        ShadLabel* valueLabel = createTextLabel(stat, kStatValueFontSize, QFont::DemiBold, ShadThemeType::Foreground);
        ShadLabel* captionLabel = createTextLabel(stat, kStatCaptionFontSize, QFont::Normal, ShadThemeType::MutedForeground);
        ExampleI18n::bind(stat, [valueLabel, captionLabel, value, caption]()
        {
            valueLabel->setText(value());
            captionLabel->setText(caption());
        });
        layout->addWidget(valueLabel);
        layout->addWidget(captionLabel);
        return stat;
    }

    ShadCard* createPanel(QWidget* parent, int width, ExampleI18n::TextFunction title, ExampleI18n::TextFunction description)
    {
        ShadCard* panel = new ShadCard(parent);
        panel->setFixedWidth(width);
        ExampleI18n::bind(panel, [panel, title, description]()
        {
            panel->setTitle(title());
            panel->setDescription(description());
        });
        return panel;
    }

    void addStats(ShadCard* panel, const QList<QWidget*>& stats)
    {
        QWidget* row = new QWidget(panel);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kStatGap);
        for (QWidget* stat : stats)
        {
            stat->setParent(row);
            layout->addWidget(stat);
        }
        layout->addStretch(1);
        panel->contentLayout()->addWidget(row);
    }

    class VehicleIcon : public QWidget
    {
        public:
            VehicleIcon(ShadIconType::IconName icon, QWidget* parent)
                : QWidget(parent), _icon(icon)
            {
                setFixedSize(kVehicleIconSize, kVehicleIconSize);
                QObject::connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QPen(shadTheme->getColor(ShadThemeType::Background), 1.0));
                painter.setBrush(shadTheme->getColor(ShadThemeType::Primary));
                painter.drawEllipse(QRectF(rect()).adjusted(1.0, 1.0, -1.0, -1.0));
                ShadIcon::paint(&painter, QRectF(rect()), _icon, shadTheme->getColor(ShadThemeType::PrimaryForeground), kVehicleGlyphSize);
            }

        private:
            ShadIconType::IconName _icon;
    };

    class LegendLine : public QWidget
    {
        public:
            LegendLine(const QColor& color, QWidget* parent)
                : QWidget(parent), _color(color)
            {
                setFixedSize(kLegendLineWidth, kLegendLineHeight);
            }

        protected:
            void paintEvent(QPaintEvent* event) override
            {
                Q_UNUSED(event);
                QPainter painter(this);
                painter.setRenderHint(QPainter::Antialiasing, true);
                painter.setPen(QPen(_color, kLegendStrokeWidth, Qt::SolidLine, Qt::RoundCap));
                painter.drawLine(QPointF(kLegendStrokeWidth, height() / 2.0), QPointF(width() - kLegendStrokeWidth, height() / 2.0));
            }

        private:
            QColor _color;
    };

    QWidget* createLegendRow(QWidget* parent, const QColor& color, const QString& text, const QString& value)
    {
        QWidget* row = new QWidget(parent);
        QHBoxLayout* layout = new QHBoxLayout(row);
        layout->setContentsMargins(0, 0, 0, 0);
        layout->setSpacing(kLegendSpacing);
        ShadLabel* textLabel = createTextLabel(row, kPriceFontSize, QFont::Normal, ShadThemeType::Foreground);
        textLabel->setText(text);
        ShadLabel* valueLabel = createTextLabel(row, kPriceFontSize, QFont::Normal, ShadThemeType::MutedForeground);
        valueLabel->setText(value);
        layout->addWidget(new LegendLine(color, row));
        layout->addWidget(textLabel);
        layout->addStretch(1);
        layout->addWidget(valueLabel);
        return row;
    }

    ShadLabel* createStatusLabel(QWidget* parent)
    {
        ShadLabel* label = new ShadLabel(parent);
        label->setColorRole(ShadThemeType::MutedForeground);
        return label;
    }

    double distanceBetween(const ShadMapCoordinate& from, const ShadMapCoordinate& to)
    {
        const double fromLatitude = qDegreesToRadians(from.latitude);
        const double toLatitude = qDegreesToRadians(to.latitude);
        const double halfLatitude = std::sin((toLatitude - fromLatitude) / 2.0);
        const double halfLongitude = std::sin(qDegreesToRadians(to.longitude - from.longitude) / 2.0);
        const double value = halfLatitude * halfLatitude + std::cos(fromLatitude) * std::cos(toLatitude) * halfLongitude * halfLongitude;
        return 2.0 * kEarthRadiusKm * std::asin(std::sqrt(qBound(0.0, value, 1.0)));
    }

    double routeLength(const QList<ShadMapCoordinate>& route)
    {
        double length = 0.0;
        for (qsizetype index = 1; index < route.size(); ++index)
        {
            length += distanceBetween(route.at(index - 1), route.at(index));
        }
        return length;
    }

    ShadMapCoordinate splitRoute(const QList<ShadMapCoordinate>& route, double progress, QList<ShadMapCoordinate>& passed, QList<ShadMapCoordinate>& remaining)
    {
        double target = routeLength(route) * qBound(0.0, progress, 1.0);
        ShadMapCoordinate position = route.first();
        qsizetype next = route.size();
        passed.append(route.first());
        for (qsizetype index = 1; index < route.size(); ++index)
        {
            const double segment = distanceBetween(route.at(index - 1), route.at(index));
            if (target <= segment)
            {
                const double ratio = segment > 0.0 ? target / segment : 0.0;
                position = ShadMapCoordinate(route.at(index - 1).latitude + (route.at(index).latitude - route.at(index - 1).latitude) * ratio, route.at(index - 1).longitude + (route.at(index).longitude - route.at(index - 1).longitude) * ratio);
                next = index;
                break;
            }
            target -= segment;
            position = route.at(index);
            passed.append(position);
        }
        passed.append(position);
        remaining.append(position);
        for (qsizetype index = next; index < route.size(); ++index)
        {
            remaining.append(route.at(index));
        }
        return position;
    }

    ShadLabel* addLiveStat(QWidget* row, QHBoxLayout* layout, ExampleI18n::TextFunction caption)
    {
        QWidget* stat = new QWidget(row);
        QVBoxLayout* statLayout = new QVBoxLayout(stat);
        statLayout->setContentsMargins(0, 0, 0, 0);
        statLayout->setSpacing(kStatSpacing);
        ShadLabel* valueLabel = createTextLabel(stat, kStatValueFontSize, QFont::DemiBold, ShadThemeType::Foreground);
        ShadLabel* captionLabel = createTextLabel(stat, kStatCaptionFontSize, QFont::Normal, ShadThemeType::MutedForeground);
        ExampleI18n::bind(captionLabel, [captionLabel, caption]() { captionLabel->setText(caption()); });
        statLayout->addWidget(valueLabel);
        statLayout->addWidget(captionLabel);
        layout->addWidget(stat);
        return valueLabel;
    }

    QList<QWidget*> createTrackButtons(QWidget* parent, ShadMap* map, double durationMs, const std::function<void(double)>& apply)
    {
        ShadButton* toggle = new ShadButton(parent);
        toggle->setButtonSize(ShadButtonType::Small);
        ShadButton* reset = new ShadButton(parent);
        reset->setVariant(ShadButtonType::Outline);
        reset->setButtonSize(ShadButtonType::Small);
        ExampleI18n::bind(reset, [reset]() { reset->setText(MapPage::tr("重新开始")); });
        QTimer* timer = new QTimer(map);
        timer->setInterval(kTrackIntervalMs);
        const std::shared_ptr<double> progress = std::make_shared<double>(0.0);
        const auto update = [apply, progress, timer, toggle]()
        {
            apply(*progress);
            toggle->setText(timer->isActive() ? MapPage::tr("暂停") : MapPage::tr("开始"));
        };
        ExampleI18n::bind(toggle, update);
        QObject::connect(timer, &QTimer::timeout, map, [progress, timer, update, durationMs]()
        {
            *progress = qMin(1.0, *progress + kTrackIntervalMs / durationMs);
            if (*progress >= 1.0)
            {
                timer->stop();
            }
            update();
        });
        QObject::connect(toggle, &QAbstractButton::clicked, map, [progress, timer, update]()
        {
            if (timer->isActive())
            {
                timer->stop();
            }
            else
            {
                *progress = *progress >= 1.0 ? 0.0 : *progress;
                timer->start();
            }
            update();
        });
        QObject::connect(reset, &QAbstractButton::clicked, map, [progress, update]()
        {
            *progress = 0.0;
            update();
        });
        return QList<QWidget*>{toggle, reset};
    }

    double nextOffset(quint32& seed)
    {
        seed = seed * 1664525u + 1013904223u;
        return (double((seed >> 8) & 0xffffu) / 65535.0 - 0.5) * kClusterSpread;
    }
} // namespace

MapPage::MapPage(QWidget* parent)
    : ExampleBasePage([] { return tr("地图"); }, [] { return tr("ShadMap：基于栅格瓦片的地图，支持标记、弹出框、路线、弧线与聚合，瓦片随主题在浅色与深色之间切换。"); }, parent)
{
    ShadCard* basicCard = addSection([] { return tr("基础用法"); }, [] { return tr("setTileUrl(浅色地址, 深色地址) 设置瓦片地址，深色主题下自动切换；拖动平移，滚轮或双击缩放，方向键与 + - 键也可以操作"); });
    ShadMap* basicMap = createMap(this);
    basicMap->setCenter(kTaipei);
    basicMap->setZoom(kCityZoom);
    addStretchItem(basicCard, basicMap);

    ShadCard* vectorCard = addSection([] { return tr("矢量瓦片"); }, [] { return tr("setTileFormat(ShadMapType::Vector) 改用矢量瓦片，setVectorTileUrl 可以传瓦片地址模板或 TileJSON 地址；矢量瓦片在本地绘制，颜色跟随主题，地名跟随界面语言；setTheme 让单张地图固定使用浅色或深色"); });
    ShadMap* vectorMap = createMap(this);
    const QString rasterAttribution = vectorMap->getAttribution();
    vectorMap->setVectorTileUrl(QStringLiteral("https://tiles.openfreemap.org/planet"));
    vectorMap->setTileFormat(ShadMapType::Vector);
    vectorMap->setAttribution(vectorAttribution());
    vectorMap->setCenter(kTaipei);
    vectorMap->setZoom(kCityZoom);
    ShadSegmented* formatSwitch = new ShadSegmented(this);
    formatSwitch->addItem(QString());
    formatSwitch->addItem(QString());
    formatSwitch->setCurrentIndex(0);
    ExampleI18n::bind(formatSwitch, [formatSwitch]()
    {
        formatSwitch->setItemText(0, tr("矢量"));
        formatSwitch->setItemText(1, tr("栅格 PNG"));
    });
    connect(formatSwitch, &ShadSegmented::currentIndexChanged, vectorMap, [vectorMap, rasterAttribution](int index)
    {
        const bool isVector = index == 0;
        vectorMap->setTileFormat(isVector ? ShadMapType::Vector : ShadMapType::Raster);
        vectorMap->setAttribution(isVector ? vectorAttribution() : rasterAttribution);
    });
    ShadSegmented* themeSwitch = new ShadSegmented(this);
    themeSwitch->addItem(QString());
    themeSwitch->addItem(QString());
    themeSwitch->addItem(QString());
    themeSwitch->setCurrentIndex(0);
    ExampleI18n::bind(themeSwitch, [themeSwitch]()
    {
        themeSwitch->setItemText(0, tr("跟随主题"));
        themeSwitch->setItemText(1, tr("浅色"));
        themeSwitch->setItemText(2, tr("深色"));
    });
    connect(themeSwitch, &ShadSegmented::currentIndexChanged, vectorMap, [vectorMap](int index) { vectorMap->setTheme(index == 1 ? ShadMapType::Light : (index == 2 ? ShadMapType::Dark : ShadMapType::Auto)); });
    addRow(vectorCard, {formatSwitch, themeSwitch});
    addStretchItem(vectorCard, vectorMap);

    ShadCard* markerCard = addSection([] { return tr("标记"); }, [] { return tr("addMarker 添加标记，可设置颜色、标签、悬停提示与弹出文字；点击带弹出文字的标记显示弹出框；setContentWidget 把任意组件作为标记"); });
    ShadMap* markerMap = createMap(this);
    ShadMapMarker* towerMarker = addLabeledMarker(markerMap, kTaipei101, [] { return tr("台北 101"); });
    ExampleI18n::bind(towerMarker, [towerMarker]() { towerMarker->setPopupText(tr("台北 101：信义区的地标摩天大楼")); });
    ShadMapMarker* hallMarker = addLabeledMarker(markerMap, kMemorialHall, [] { return tr("中正纪念堂"); });
    hallMarker->setColor(kBlue);
    hallMarker->setLabelPosition(ShadMapType::Bottom);
    ExampleI18n::bind(hallMarker, [hallMarker]() { hallMarker->setPopupText(tr("中正纪念堂：中正区的纪念建筑与广场")); });
    ShadMapMarker* stationMarker = addLabeledMarker(markerMap, kTaipeiStation, [] { return tr("台北车站"); });
    stationMarker->setColor(kGreen);
    ExampleI18n::bind(stationMarker, [stationMarker]() { stationMarker->setPopupText(tr("台北车站：铁路、高铁与捷运的交通枢纽")); });
    ShadMapMarker* templeMarker = addLabeledMarker(markerMap, kLongshanTemple, [] { return tr("龙山寺"); });
    templeMarker->setColor(kAmber);
    ExampleI18n::bind(templeMarker, [templeMarker]() { templeMarker->setToolTip(tr("悬停时显示的提示")); });
    ShadMapMarker* badgeMarker = markerMap->addMarker(ShadMapCoordinate(25.0400, 121.5603));
    ShadBadge* badge = new ShadBadge(this);
    badgeMarker->setContentWidget(badge);
    ExampleI18n::bind(badgeMarker, [badgeMarker, badge]()
    {
        badge->setText(tr("展览中"));
        badgeMarker->setPopupText(tr("国父纪念馆：这个标记是一个徽章组件"));
    });
    markerMap->fitBounds(QList<ShadMapCoordinate>{kTaipei101, kMemorialHall, kTaipeiStation, kLongshanTemple}, kFitPadding);
    ShadLabel* markerStatus = createStatusLabel(this);
    ExampleI18n::bind(markerStatus, [markerStatus]() { markerStatus->setText(tr("点击标记查看详情")); });
    connect(markerMap, &ShadMap::markerClicked, markerStatus, [markerStatus](ShadMapMarker* marker) { markerStatus->setText(tr("已点击：%1").arg(marker->getLabel())); });
    addStretchItem(markerCard, markerMap);
    addItem(markerCard, markerStatus);

    ShadCard* dragCard = addSection([] { return tr("可拖动标记"); }, [] { return tr("setDraggable(true) 后标记可以拖动，松开时发出 dragEnded 信号"); });
    ShadMap* dragMap = createMap(this);
    dragMap->setCenter(kTaipei);
    dragMap->setZoom(kDistrictZoom);
    ShadMapMarker* dragMarker = dragMap->addMarker(kTaipei);
    dragMarker->setColor(kRed);
    dragMarker->setDraggable(true);
    ShadLabel* dragStatus = createStatusLabel(this);
    ExampleI18n::bind(dragStatus, [dragStatus]() { dragStatus->setText(tr("拖动标记到新的位置")); });
    connect(dragMarker, &ShadMapMarker::dragEnded, dragStatus, [dragStatus](const ShadMapCoordinate& coordinate) { dragStatus->setText(tr("纬度 %1，经度 %2").arg(QString::number(coordinate.latitude, 'f', 5), QString::number(coordinate.longitude, 'f', 5))); });
    addStretchItem(dragCard, dragMap);
    addItem(dragCard, dragStatus);

    ShadCard* routeCard = addSection([] { return tr("路线"); }, [] { return tr("addRoute 按坐标列表绘制折线，可设置颜色、线宽、透明度与虚线；fitBounds 让视图容纳全部坐标"); });
    ShadMap* routeMap = createMap(this);
    const QList<ShadMapCoordinate> railway{kTaipeiStation, kTaoyuan, kHsinchu, kTaichung, kChiayi, kTainan, kKaohsiung};
    const QList<ShadMapCoordinate> planned{kTaipeiStation, kYilan, kHualien};
    ShadMapRoute* railwayRoute = routeMap->addRoute(railway);
    railwayRoute->setColor(kBlue);
    ShadMapRoute* plannedRoute = routeMap->addRoute(planned);
    plannedRoute->setColor(kAmber);
    plannedRoute->setWidth(kDashedRouteWidth);
    plannedRoute->setDashPattern(QList<qreal>{1.0, 2.5});
    addLabeledMarker(routeMap, kTaipeiStation, [] { return tr("台北"); });
    addLabeledMarker(routeMap, kKaohsiung, [] { return tr("高雄"); })->setColor(kBlue);
    addLabeledMarker(routeMap, kHualien, [] { return tr("花莲"); })->setColor(kAmber);
    routeMap->fitBounds(railway + planned, kFitPadding);
    addStretchItem(routeCard, routeMap);

    ShadCard* arcCard = addSection([] { return tr("弧线"); }, [] { return tr("addArc 在两个坐标之间绘制弧线，setCurvature 调整弯曲程度，适合表示航线或数据流向"); });
    ShadMap* arcMap = createMap(this);
    arcMap->setCenter(kAsiaCenter);
    arcMap->setZoom(kWorldZoom);
    arcMap->addArc(kTaipei, kTokyo)->setCurvature(kArcCurvature);
    arcMap->addArc(kTaipei, kSingapore);
    arcMap->addArc(kTaipei, kDubai)->setColor(kBlue);
    arcMap->addArc(kTaipei, kLondon)->setColor(kBlue);
    addLabeledMarker(arcMap, kTaipei, [] { return tr("台北"); })->setLabelPosition(ShadMapType::Bottom);
    addLabeledMarker(arcMap, kTokyo, [] { return tr("东京"); });
    addLabeledMarker(arcMap, kSingapore, [] { return tr("新加坡"); })->setLabelPosition(ShadMapType::Bottom);
    addLabeledMarker(arcMap, kDubai, [] { return tr("迪拜"); })->setLabelPosition(ShadMapType::Bottom);
    addLabeledMarker(arcMap, kLondon, [] { return tr("伦敦"); });
    addStretchItem(arcCard, arcMap);

    ShadCard* clusterCard = addSection([] { return tr("聚合"); }, [] { return tr("addGeoJson 读取 GeoJSON 中的点并逐个添加为标记，loadGeoJson 从地址下载；setClusteringEnabled(true) 把相邻的标记合并成带数量的圆点，点击圆点放大展开"); });
    ShadMap* clusterMap = createMap(this);
    clusterMap->setClusteringEnabled(true);
    clusterMap->setCenter(kChinaCenter);
    clusterMap->setZoom(kCountryZoom);
    quint32 seed = 20261123u;
    QJsonArray features;
    for (const ShadMapCoordinate& city : {kShanghai, kBeijing, kGuangzhou, kChengdu, kWuhan, kXian})
    {
        for (int index = 0; index < kClusterPointsPerCity; ++index)
        {
            const double latitudeOffset = nextOffset(seed);
            const double longitudeOffset = nextOffset(seed);
            const QJsonObject geometry{{QStringLiteral("type"), QStringLiteral("Point")}, {QStringLiteral("coordinates"), QJsonArray{city.longitude + longitudeOffset, city.latitude + latitudeOffset}}};
            features.append(QJsonObject{{QStringLiteral("type"), QStringLiteral("Feature")}, {QStringLiteral("geometry"), geometry}});
        }
    }
    const QJsonObject collection{{QStringLiteral("type"), QStringLiteral("FeatureCollection")}, {QStringLiteral("features"), features}};
    clusterMap->addGeoJson(QJsonDocument(collection).toJson(QJsonDocument::Compact));
    addStretchItem(clusterCard, clusterMap);

    ShadCard* flyCard = addSection([] { return tr("飞行定位"); }, [] { return tr("flyTo 以动画移动到指定坐标与缩放级别"); });
    ShadMap* flyMap = createMap(this);
    flyMap->setCenter(kTaipei);
    flyMap->setZoom(kCityZoom);
    QList<QWidget*> flyButtons;
    const auto addFlyButton = [this, flyMap, &flyButtons](const ShadMapCoordinate& coordinate, ExampleI18n::TextFunction text)
    {
        ShadButton* button = new ShadButton(this);
        button->setVariant(ShadButtonType::Outline);
        button->setButtonSize(ShadButtonType::Small);
        ExampleI18n::bind(button, [button, text]() { button->setText(text()); });
        connect(button, &QAbstractButton::clicked, flyMap, [flyMap, coordinate]() { flyMap->flyTo(coordinate, kCityZoom); });
        flyButtons.append(button);
    };
    addFlyButton(kTaipei, [] { return tr("台北"); });
    addFlyButton(kTokyo, [] { return tr("东京"); });
    addFlyButton(kLondon, [] { return tr("伦敦"); });
    addFlyButton(kNewYork, [] { return tr("纽约"); });
    addRow(flyCard, flyButtons);
    addStretchItem(flyCard, flyMap);

    ShadCard* popupCard = addSection([] { return tr("弹出框"); }, [] { return tr("addPopup 在坐标上添加常驻的弹出框，setContentWidget 可以放入任意组件"); });
    ShadMap* popupMap = createMap(this);
    popupMap->setCenter(kTaipei);
    popupMap->setZoom(kCityZoom);
    popupMap->addMarker(kTaipei);
    ShadMapPopup* popup = popupMap->addPopup(kTaipei);
    popup->setAnchorOffset(kPopupOffset);
    popup->setCloseButtonVisible(true);
    ExampleI18n::bind(popup, [popup]() { popup->setText(tr("台北：这个弹出框固定在坐标上，随地图移动")); });
    ShadButton* popupButton = new ShadButton(this);
    popupButton->setVariant(ShadButtonType::Outline);
    popupButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(popupButton, [popupButton]() { popupButton->setText(tr("显示弹出框")); });
    connect(popupButton, &QAbstractButton::clicked, popup, [popup]() { popup->show(); });
    addRow(popupCard, {popupButton});
    addStretchItem(popupCard, popupMap);

    ShadCard* plannerCard = addSection([] { return tr("路线规划"); }, [] { return tr("addOverlay 把任意组件固定在地图的一角；标记用 setPopupTitle 与 setPopupText 设置弹出框的标题和说明，showMarkerPopup 直接打开弹出框"); });
    ShadMap* plannerMap = createVectorMap(this);
    const ShadMapCoordinate plannerStop(25.0416, 121.5437);
    const ShadMapCoordinate plannerEnd(25.0411, 121.5660);
    const QList<ShadMapCoordinate> plannerRoute{kTaipeiStation, ShadMapCoordinate(25.0448, 121.5233), ShadMapCoordinate(25.0424, 121.5329), plannerStop, ShadMapCoordinate(25.0415, 121.5512), ShadMapCoordinate(25.0413, 121.5578), plannerEnd};
    plannerMap->addRoute(plannerRoute)->setColor(kBlue);
    addLabeledMarker(plannerMap, kTaipeiStation, [] { return tr("起点"); })->setColor(kGreen);
    addLabeledMarker(plannerMap, plannerEnd, [] { return tr("终点"); });
    ShadMapMarker* stopMarker = plannerMap->addMarker(plannerStop);
    stopMarker->setColor(kBlue);
    ExampleI18n::bind(stopMarker, [stopMarker]()
    {
        stopMarker->setPopupTitle(tr("忠孝复兴站"));
        stopMarker->setPopupText(tr("下一站") + QLatin1Char('\n') + tr("预计 9:19 到达"));
    });
    ShadCard* plannerPanel = createPanel(this, kPanelWidth, [] { return tr("路线规划"); }, [] { return tr("忠孝东路配送"); });
    addStats(plannerPanel, {createStat(this, [] { return tr("7 分钟"); }, [] { return tr("预计到达"); }), createStat(this, [] { return QStringLiteral("3"); }, [] { return tr("停靠点"); }), createStat(this, [] { return QStringLiteral("4.6"); }, [] { return tr("公里"); })});
    plannerMap->addOverlay(plannerPanel, ShadMapType::TopLeft);
    plannerMap->fitBounds(plannerRoute, kFitPadding);
    plannerMap->showMarkerPopup(stopMarker);
    addStretchItem(plannerCard, plannerMap);

    ShadCard* trackCard = addSection([] { return tr("实时位置"); }, [] { return tr("定时调用标记的 setCoordinate 与路线的 setCoordinates：车辆沿路线从起点移动到终点，走过的路线变灰，面板中的剩余时间与距离同步刷新"); });
    ShadMap* trackMap = createVectorMap(this);
    ShadMapRoute* remainingRoute = trackMap->addRoute(plannerRoute);
    remainingRoute->setColor(kBlue);
    ShadMapRoute* passedRoute = trackMap->addRoute(QList<ShadMapCoordinate>{plannerRoute.first(), plannerRoute.first()});
    passedRoute->setColor(kGray);
    addLabeledMarker(trackMap, plannerRoute.first(), [] { return tr("起点"); })->setColor(kGreen);
    addLabeledMarker(trackMap, plannerRoute.last(), [] { return tr("终点"); });
    ShadMapMarker* vehicleMarker = trackMap->addMarker(plannerRoute.first());
    vehicleMarker->setColor(kBlue);
    vehicleMarker->setSelected(true);
    trackMap->fitBounds(plannerRoute, kFitPadding);
    ShadCard* trackPanel = createPanel(this, kPanelWidth, [] { return tr("实时配送"); }, [] { return tr("机车 14 正在前往终点"); });
    QWidget* trackStats = new QWidget(trackPanel);
    QHBoxLayout* trackStatsLayout = new QHBoxLayout(trackStats);
    trackStatsLayout->setContentsMargins(0, 0, 0, 0);
    trackStatsLayout->setSpacing(kStatGap);
    ShadLabel* remainingTimeLabel = addLiveStat(trackStats, trackStatsLayout, [] { return tr("剩余时间"); });
    ShadLabel* remainingDistanceLabel = addLiveStat(trackStats, trackStatsLayout, [] { return tr("剩余距离"); });
    ShadLabel* progressLabel = addLiveStat(trackStats, trackStatsLayout, [] { return tr("进度"); });
    trackStatsLayout->addStretch(1);
    trackPanel->contentLayout()->addWidget(trackStats);
    trackMap->addOverlay(trackPanel, ShadMapType::TopLeft);
    const double trackLength = routeLength(plannerRoute);
    const QList<QWidget*> trackButtons = createTrackButtons(this, trackMap, kTrackDurationMs, [plannerRoute, trackLength, vehicleMarker, passedRoute, remainingRoute, remainingTimeLabel, remainingDistanceLabel, progressLabel](double progress)
    {
        QList<ShadMapCoordinate> passed;
        QList<ShadMapCoordinate> remaining;
        vehicleMarker->setCoordinate(splitRoute(plannerRoute, progress, passed, remaining));
        passedRoute->setCoordinates(passed);
        remainingRoute->setCoordinates(remaining);
        remainingTimeLabel->setText(tr("%1 分钟").arg(qCeil(kTrackMinutes * (1.0 - progress))));
        remainingDistanceLabel->setText(tr("%1 公里").arg(QString::number(trackLength * (1.0 - progress), 'f', 1)));
        progressLabel->setText(QStringLiteral("%1%").arg(qRound(progress * 100.0)));
    });
    addRow(trackCard, trackButtons);
    addStretchItem(trackCard, trackMap);

    ShadCard* railCard = addSection([] { return tr("长途追踪"); }, [] { return tr("路线跨越大陆时做法相同：列车沿西伯利亚铁路从莫斯科开往符拉迪沃斯托克，setContentWidget 把列车显示为图标，经过的站点用 setColor 变灰，面板中的下一站、剩余时间与距离同步刷新；路线是按主要车站连成的简化折线"); });
    ShadMap* railMap = createVectorMap(this);
    struct RailPoint
    {
        ShadMapCoordinate coordinate;
        ExampleI18n::TextFunction name;
    };
    const QList<RailPoint> railPoints{
        RailPoint{ShadMapCoordinate(55.7558, 37.6173), [] { return tr("莫斯科"); }},
        RailPoint{ShadMapCoordinate(56.3269, 44.0059), nullptr},
        RailPoint{ShadMapCoordinate(58.6035, 49.6680), nullptr},
        RailPoint{ShadMapCoordinate(58.0105, 56.2502), nullptr},
        RailPoint{ShadMapCoordinate(56.8389, 60.6057), [] { return tr("叶卡捷琳堡"); }},
        RailPoint{ShadMapCoordinate(57.1530, 65.5343), nullptr},
        RailPoint{ShadMapCoordinate(54.9885, 73.3242), nullptr},
        RailPoint{ShadMapCoordinate(55.0084, 82.9357), [] { return tr("新西伯利亚"); }},
        RailPoint{ShadMapCoordinate(56.0153, 92.8932), nullptr},
        RailPoint{ShadMapCoordinate(52.2870, 104.3050), [] { return tr("伊尔库茨克"); }},
        RailPoint{ShadMapCoordinate(51.6600, 103.7100), nullptr},
        RailPoint{ShadMapCoordinate(51.5200, 104.1500), nullptr},
        RailPoint{ShadMapCoordinate(51.5500, 105.1200), nullptr},
        RailPoint{ShadMapCoordinate(51.7200, 105.8700), nullptr},
        RailPoint{ShadMapCoordinate(51.8335, 107.5841), nullptr},
        RailPoint{ShadMapCoordinate(52.0340, 113.4990), nullptr},
        RailPoint{ShadMapCoordinate(53.7400, 119.7700), nullptr},
        RailPoint{ShadMapCoordinate(53.9800, 123.9400), nullptr},
        RailPoint{ShadMapCoordinate(53.4500, 125.8000), nullptr},
        RailPoint{ShadMapCoordinate(50.9200, 128.4700), nullptr},
        RailPoint{ShadMapCoordinate(48.7900, 132.9200), nullptr},
        RailPoint{ShadMapCoordinate(48.4827, 135.0838), [] { return tr("哈巴罗夫斯克"); }},
        RailPoint{ShadMapCoordinate(45.9300, 133.7300), nullptr},
        RailPoint{ShadMapCoordinate(43.8000, 131.9500), nullptr},
        RailPoint{ShadMapCoordinate(43.1155, 131.8855), [] { return tr("符拉迪沃斯托克"); }},
    };
    QList<ShadMapCoordinate> railRoute;
    QList<double> railStopDistances;
    QList<ExampleI18n::TextFunction> railStopNames;
    QList<ShadMapMarker*> railStopMarkers;
    for (const RailPoint& point : railPoints)
    {
        railRoute.append(point.coordinate);
        if (point.name)
        {
            railStopDistances.append(routeLength(railRoute));
            railStopNames.append(point.name);
            railStopMarkers.append(addLabeledMarker(railMap, point.coordinate, point.name));
        }
    }
    ShadMapRoute* railRemaining = railMap->addRoute(railRoute);
    railRemaining->setColor(kBlue);
    ShadMapRoute* railPassed = railMap->addRoute(QList<ShadMapCoordinate>{railRoute.first(), railRoute.first()});
    railPassed->setColor(kGray);
    ShadMapMarker* trainMarker = railMap->addMarker(railRoute.first());
    trainMarker->setContentWidget(new VehicleIcon(ShadIconType::TrainFront, this));
    ShadCard* railPanel = createPanel(this, kRailPanelWidth, [] { return tr("西伯利亚铁路"); }, [] { return tr("莫斯科") + QStringLiteral(" → ") + tr("符拉迪沃斯托克"); });
    QWidget* railStats = new QWidget(railPanel);
    QHBoxLayout* railStatsLayout = new QHBoxLayout(railStats);
    railStatsLayout->setContentsMargins(0, 0, 0, 0);
    railStatsLayout->setSpacing(kStatGap);
    ShadLabel* railTimeLabel = addLiveStat(railStats, railStatsLayout, [] { return tr("剩余时间"); });
    ShadLabel* railDistanceLabel = addLiveStat(railStats, railStatsLayout, [] { return tr("剩余距离"); });
    ShadLabel* railProgressLabel = addLiveStat(railStats, railStatsLayout, [] { return tr("进度"); });
    railStatsLayout->addStretch(1);
    railPanel->contentLayout()->addWidget(railStats);
    ShadLabel* nextStopLabel = createStatusLabel(railPanel);
    railPanel->contentLayout()->addWidget(nextStopLabel);
    railMap->addOverlay(railPanel, ShadMapType::TopLeft);
    railMap->fitBounds(railRoute, kRailPadding);
    const double railLength = routeLength(railRoute);
    const QList<QWidget*> railButtons = createTrackButtons(this, railMap, kRailDurationMs, [railRoute, railStopDistances, railStopNames, railStopMarkers, railLength, trainMarker, railPassed, railRemaining, railTimeLabel, railDistanceLabel, railProgressLabel, nextStopLabel](double progress)
    {
        QList<ShadMapCoordinate> passed;
        QList<ShadMapCoordinate> remaining;
        trainMarker->setCoordinate(splitRoute(railRoute, progress, passed, remaining));
        railPassed->setCoordinates(passed);
        railRemaining->setCoordinates(remaining);
        const double travelled = railLength * progress;
        for (qsizetype index = 0; index < railStopMarkers.size(); ++index)
        {
            railStopMarkers.at(index)->setColor(railStopDistances.at(index) < travelled ? kGray : (index == 0 ? kGreen : kBlue));
        }
        qsizetype next = 0;
        while (next < railStopDistances.size() && railStopDistances.at(next) <= travelled)
        {
            ++next;
        }
        nextStopLabel->setText(next < railStopNames.size() ? tr("下一站：%1").arg(railStopNames.at(next)()) : tr("已到达终点"));
        railTimeLabel->setText(tr("%1 小时").arg(qCeil(kRailHours * (1.0 - progress))));
        railDistanceLabel->setText(tr("%1 公里").arg(qRound(kRailKilometers * (1.0 - progress))));
        railProgressLabel->setText(QStringLiteral("%1%").arg(qRound(progress * 100.0)));
    });
    addRow(railCard, railButtons);
    addStretchItem(railCard, railMap);

    ShadCard* homesCard = addSection([] { return tr("房源列表"); }, [] { return tr("setText 把标记显示为文字胶囊，setSelected 切换选中态；点击列表项或标记时两边同步选中，并打开对应的弹出框"); });
    ShadMap* homesMap = createVectorMap(this);
    homesMap->setFixedHeight(kHomesMapHeight);
    struct Home
    {
        ShadMapCoordinate coordinate;
        ExampleI18n::TextFunction name;
        ExampleI18n::TextFunction district;
        int bedrooms;
        int bathrooms;
        QString price;
    };
    const QList<Home> homes{
        Home{ShadMapCoordinate(25.0300, 121.5357), [] { return tr("大安森林景观宅"); }, [] { return tr("大安区"); }, 3, 2, QStringLiteral("NT$28.8M")},
        Home{ShadMapCoordinate(25.0360, 121.5680), [] { return tr("信义高楼层公寓"); }, [] { return tr("信义区"); }, 2, 1, QStringLiteral("NT$36.5M")},
        Home{ShadMapCoordinate(25.0636, 121.5265), [] { return tr("中山电梯华厦"); }, [] { return tr("中山区"); }, 2, 2, QStringLiteral("NT$21.5M")},
        Home{ShadMapCoordinate(25.0930, 121.5250), [] { return tr("士林河岸宅"); }, [] { return tr("士林区"); }, 4, 3, QStringLiteral("NT$42.0M")},
        Home{ShadMapCoordinate(25.0340, 121.4990), [] { return tr("万华小套房"); }, [] { return tr("万华区"); }, 1, 1, QStringLiteral("NT$9.8M")},
        Home{ShadMapCoordinate(25.0790, 121.5750), [] { return tr("内湖科技宅"); }, [] { return tr("内湖区"); }, 3, 2, QStringLiteral("NT$24.8M")},
    };
    ShadCard* homesPanel = createPanel(this, kHomesPanelWidth, [] { return tr("待售房源"); }, [] { return tr("台北市 6 套"); });
    QList<ShadMapCoordinate> homeCoordinates;
    QList<ShadMapMarker*> homeMarkers;
    QList<ShadItem*> homeItems;
    for (const Home& home : homes)
    {
        homeCoordinates.append(home.coordinate);
        ShadMapMarker* marker = homesMap->addMarker(home.coordinate);
        marker->setText(home.price);
        ExampleI18n::bind(marker, [marker, home]()
        {
            marker->setPopupTitle(home.name());
            marker->setPopupText(home.district() + QLatin1Char('\n') + tr("%1 房 %2 卫").arg(home.bedrooms).arg(home.bathrooms) + QLatin1Char('\n') + home.price);
        });
        ShadItem* item = new ShadItem(homesPanel);
        item->setSize(ShadItemType::Small);
        item->setClickable(true);
        ExampleI18n::bind(item, [item, home]()
        {
            item->setTitle(home.name());
            item->setDescription(tr("%1 房 %2 卫").arg(home.bedrooms).arg(home.bathrooms));
        });
        ShadLabel* priceLabel = createTextLabel(item, kPriceFontSize, QFont::Medium, ShadThemeType::Foreground);
        priceLabel->setText(home.price);
        item->addActionWidget(priceLabel);
        homesPanel->contentLayout()->addWidget(item);
        homeMarkers.append(marker);
        homeItems.append(item);
    }
    const auto selectHome = [homesMap, homeMarkers, homeItems](int selected)
    {
        for (int index = 0; index < homeMarkers.size(); ++index)
        {
            homeMarkers.at(index)->setSelected(index == selected);
            homeItems.at(index)->setVariant(index == selected ? ShadItemType::Muted : ShadItemType::Default);
        }
        if (selected >= 0)
        {
            homesMap->showMarkerPopup(homeMarkers.at(selected));
        }
        else
        {
            homesMap->hideMarkerPopup();
        }
    };
    for (int index = 0; index < homeItems.size(); ++index)
    {
        connect(homeItems.at(index), &ShadItem::clicked, homesMap, [selectHome, index]() { selectHome(index); });
    }
    connect(homesMap, &ShadMap::markerClicked, homesMap, [selectHome, homeMarkers](ShadMapMarker* marker) { selectHome(int(homeMarkers.indexOf(marker))); });
    connect(homesMap, &ShadMap::clicked, homesMap, [selectHome]() { selectHome(-1); });
    homesMap->addOverlay(homesPanel, ShadMapType::TopLeft);
    homesMap->fitBounds(homeCoordinates, kFitPadding);
    selectHome(1);
    addStretchItem(homesCard, homesMap);

    ShadCard* fleetCard = addSection([] { return tr("车队调度"); }, [] { return tr("多个标记各自带弹出框的标题与说明，配合浮动面板展示整体状态；矢量瓦片在街道级别显示路名"); });
    ShadMap* fleetMap = createVectorMap(this);
    const ShadMapCoordinate bikePosition(25.0420, 121.5320);
    const ShadMapCoordinate idlePosition(25.0520, 121.5440);
    const ShadMapCoordinate truckPosition(25.0300, 121.5200);
    const QList<ShadMapCoordinate> fleetRoute{bikePosition, ShadMapCoordinate(25.0424, 121.5437), ShadMapCoordinate(25.0330, 121.5437), ShadMapCoordinate(25.0330, 121.5530)};
    fleetMap->addRoute(fleetRoute)->setColor(kBlue);
    ShadMapMarker* bikeMarker = fleetMap->addMarker(bikePosition);
    bikeMarker->setColor(kBlue);
    ExampleI18n::bind(bikeMarker, [bikeMarker]()
    {
        bikeMarker->setPopupTitle(tr("机车 14"));
        bikeMarker->setPopupText(tr("骑手：小陈") + QLatin1Char('\n') + tr("配送中 · 6 分钟"));
    });
    ShadMapMarker* idleMarker = fleetMap->addMarker(idlePosition);
    idleMarker->setColor(kGreen);
    ExampleI18n::bind(idleMarker, [idleMarker]()
    {
        idleMarker->setPopupTitle(tr("机车 07"));
        idleMarker->setPopupText(tr("骑手：小林") + QLatin1Char('\n') + tr("空闲 · 等待派单"));
    });
    ShadMapMarker* truckMarker = fleetMap->addMarker(truckPosition);
    truckMarker->setColor(kAmber);
    ExampleI18n::bind(truckMarker, [truckMarker]()
    {
        truckMarker->setPopupTitle(tr("货车 03"));
        truckMarker->setPopupText(tr("司机：老王") + QLatin1Char('\n') + tr("待处理 · 装货中"));
    });
    ShadCard* fleetPanel = createPanel(this, kPanelWidth, [] { return tr("车队调度"); }, [] { return tr("台北市中心区域"); });
    addStats(fleetPanel, {createStat(this, [] { return QStringLiteral("18"); }, [] { return tr("在线"); }), createStat(this, [] { return QStringLiteral("92%"); }, [] { return tr("准时率"); }), createStat(this, [] { return QStringLiteral("4"); }, [] { return tr("待处理"); })});
    fleetMap->addOverlay(fleetPanel, ShadMapType::TopLeft);
    fleetMap->fitBounds(fleetRoute + QList<ShadMapCoordinate>{idlePosition, truckPosition}, kFleetPadding);
    fleetMap->showMarkerPopup(bikeMarker);
    addStretchItem(fleetCard, fleetMap);

    ShadCard* flightCard = addSection([] { return tr("航线图"); }, [] { return tr("setProjection(ShadMapType::Globe) 切换为地球仪视图，拖动旋转地球，弧线沿大圆航线绘制；addArc 为每条航线设置不同的颜色，浮动面板中用图例列出航线与客座率"); });
    ShadMap* flightMap = createVectorMap(this);
    flightMap->setFixedHeight(kFlightMapHeight);
    flightMap->setProjection(ShadMapType::Globe);
    flightMap->setCenter(ShadMapCoordinate(25.0, 75.0));
    flightMap->setZoom(kGlobeZoom);
    const QList<ShadMapArc*> flightArcs = flightMap->addArcs(QList<ShadMapArcData>{ShadMapArcData{QStringLiteral("TPE-HND"), kTaipei, kTokyo}, ShadMapArcData{QStringLiteral("HND-SIN"), kTokyo, kSingapore}, ShadMapArcData{QStringLiteral("TPE-LHR"), kTaipei, kLondon}, ShadMapArcData{QStringLiteral("DXB-SIN"), kDubai, kSingapore}});
    for (ShadMapArc* arc : flightArcs)
    {
        arc->setColor(kBlue);
    }
    flightMap->arc(QStringLiteral("TPE-LHR"))->setColor(kGreen);
    flightMap->arc(QStringLiteral("DXB-SIN"))->setColor(kAmber);
    flightMap->addMarker(kTaipei)->setLabel(QStringLiteral("TPE"));
    flightMap->addMarker(kTokyo)->setLabel(QStringLiteral("HND"));
    flightMap->addMarker(kSingapore)->setLabel(QStringLiteral("SIN"));
    flightMap->addMarker(kDubai)->setLabel(QStringLiteral("DXB"));
    flightMap->addMarker(kLondon)->setLabel(QStringLiteral("LHR"));
    for (ShadMapMarker* marker : flightMap->markers())
    {
        marker->setColor(kBlue);
    }
    ShadCard* flightPanel = createPanel(this, kPanelWidth, [] { return tr("航班运营"); }, [] { return tr("长途运力调度"); });
    addStats(flightPanel, {createStat(this, [] { return QStringLiteral("6"); }, [] { return tr("航线"); }), createStat(this, [] { return QStringLiteral("1.1k"); }, [] { return tr("座位"); }), createStat(this, [] { return QStringLiteral("94%"); }, [] { return tr("峰值"); })});
    flightPanel->contentLayout()->addWidget(createLegendRow(flightPanel, kBlue, QStringLiteral("TPE → HND"), QStringLiteral("82%")));
    flightPanel->contentLayout()->addWidget(createLegendRow(flightPanel, kBlue, QStringLiteral("HND → SIN"), QStringLiteral("76%")));
    flightPanel->contentLayout()->addWidget(createLegendRow(flightPanel, kGreen, QStringLiteral("TPE → LHR"), QStringLiteral("91%")));
    flightPanel->contentLayout()->addWidget(createLegendRow(flightPanel, kAmber, QStringLiteral("DXB → SIN"), QStringLiteral("68%")));
    flightMap->addOverlay(flightPanel, ShadMapType::BottomLeft);
    ShadSegmented* projectionSwitch = new ShadSegmented(this);
    projectionSwitch->addItem(QString());
    projectionSwitch->addItem(QString());
    projectionSwitch->setCurrentIndex(0);
    ExampleI18n::bind(projectionSwitch, [projectionSwitch]()
    {
        projectionSwitch->setItemText(0, tr("地球仪"));
        projectionSwitch->setItemText(1, tr("平面"));
    });
    connect(projectionSwitch, &ShadSegmented::currentIndexChanged, flightMap, [flightMap](int index) { flightMap->setProjection(index == 0 ? ShadMapType::Globe : ShadMapType::Mercator); });
    addRow(flightCard, {projectionSwitch});
    addStretchItem(flightCard, flightMap);

    ShadCard* controlCard = addSection([] { return tr("控件"); }, [] { return tr("右键拖动或按住 Ctrl 拖动可以旋转地图，setCompassControlVisible 显示指北针，点击转回正北；setLocateControlVisible 显示定位按钮，addControlButton 添加自定义按钮，addControlSeparator 添加分隔线；组件不获取系统定位，点击定位按钮时由程序用 setUserLocation 提供坐标"); });
    ShadMap* controlMap = createVectorMap(this);
    controlMap->setCenter(kTaipei);
    controlMap->setZoom(kDistrictZoom);
    controlMap->setCompassControlVisible(true);
    controlMap->setLocateControlVisible(true);
    controlMap->addControlSeparator();
    ShadButton* globeButton = controlMap->addControlButton(ShadIconType::Globe);
    ExampleI18n::bind(globeButton, [globeButton]() { globeButton->setToolTip(tr("切换地球仪视图")); });
    connect(globeButton, &QAbstractButton::clicked, controlMap, [controlMap]() { controlMap->setProjection(controlMap->getProjection() == ShadMapType::Globe ? ShadMapType::Mercator : ShadMapType::Globe); });
    connect(controlMap, &ShadMap::locateRequested, controlMap, [controlMap]() { controlMap->setUserLocation(kTaipei101); });
    ShadLabel* controlStatus = createStatusLabel(this);
    ExampleI18n::bind(controlStatus, [controlStatus]() { controlStatus->setText(tr("点击定位按钮，示例用台北 101 的坐标模拟当前位置")); });
    addStretchItem(controlCard, controlMap);
    addItem(controlCard, controlStatus);

    ShadCard* staticCard = addSection([] { return tr("静态与加载"); }, [] { return tr("setInteractive(false) 关闭拖动与缩放，setControlsVisible(false) 隐藏缩放按钮，适合做位置预览；setLoading 显示加载状态"); });
    ShadMap* staticMap = createMap(this);
    staticMap->setFixedSize(kStaticMapWidth, kStaticMapHeight);
    staticMap->setInteractive(false);
    staticMap->setControlsVisible(false);
    staticMap->setCenter(kTokyo);
    staticMap->setZoom(kCityZoom);
    staticMap->addMarker(kTokyo);
    ShadButton* loadingButton = new ShadButton(this);
    loadingButton->setVariant(ShadButtonType::Outline);
    loadingButton->setButtonSize(ShadButtonType::Small);
    ExampleI18n::bind(loadingButton, [loadingButton]() { loadingButton->setText(tr("切换加载状态")); });
    connect(loadingButton, &QAbstractButton::clicked, staticMap, [staticMap]() { staticMap->setLoading(!staticMap->isLoading()); });
    addRow(staticCard, {loadingButton});
    addItem(staticCard, staticMap);
}

MapPage::~MapPage()
{
}
