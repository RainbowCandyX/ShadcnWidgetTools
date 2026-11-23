#include <QByteArray>
#include <QList>
#include <QMetaType>
#include <QPointF>
#include <QString>
#include <QStringList>
#include <QUrl>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QWidget>
#include <QtGlobal>

#include <cstddef>
#include <functional>
#include <utility>

#include "ShadButton.h"
#include "ShadIconType.h"
#include "ShadLuaBinder.h"
#include "ShadLuaBindings.h"
#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"
#include "ShadMap.h"
#include "ShadMapDef.h"
#include "ShadMapMarker.h"
#include "ShadMapPopup.h"
#include "ShadMapRoute.h"

namespace
{
    int optionalInt(lua_State* L, int index, int defaultValue)
    {
        qint64 value = 0;
        return ShadLuaStack::getInteger(L, index, value) ? int(value) : defaultValue;
    }

    QString optionalString(lua_State* L, int index)
    {
        QString value;
        ShadLuaStack::getString(L, index, value);
        return value;
    }

    bool numberFromVariant(const QVariant& value, double& number)
    {
        const int type = value.metaType().id();
        if (type != QMetaType::LongLong && type != QMetaType::Double)
        {
            return false;
        }
        number = value.toDouble();
        return true;
    }

    bool coordinateFromVariant(const QVariant& value, ShadMapCoordinate& coordinate)
    {
        if (value.metaType().id() == QMetaType::QVariantList)
        {
            const QVariantList list = value.toList();
            return list.size() == 2 && numberFromVariant(list.at(0), coordinate.latitude) && numberFromVariant(list.at(1), coordinate.longitude);
        }
        if (value.metaType().id() == QMetaType::QVariantMap)
        {
            const QVariantMap fields = value.toMap();
            return numberFromVariant(fields.value(QStringLiteral("latitude")), coordinate.latitude) && numberFromVariant(fields.value(QStringLiteral("longitude")), coordinate.longitude);
        }
        return false;
    }

    QVariantMap coordinateMap(const ShadMapCoordinate& coordinate)
    {
        return QVariantMap{{QStringLiteral("latitude"), coordinate.latitude}, {QStringLiteral("longitude"), coordinate.longitude}};
    }

    bool readCoordinate(lua_State* L, int index, ShadMapCoordinate& coordinate)
    {
        return coordinateFromVariant(ShadLuaStack::getVariant(L, index), coordinate);
    }

    bool readList(lua_State* L, int index, QVariantList& list)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        if (ShadLuaStack::tableLength(L, index) > 0)
        {
            list = ShadLuaStack::getVariant(L, index).toList();
            return true;
        }
        lua_pushnil(L);
        if (lua_next(L, index) == 0)
        {
            return true;
        }
        lua_pop(L, 2);
        return false;
    }

    bool readCoordinates(lua_State* L, int index, QList<ShadMapCoordinate>& coordinates)
    {
        QVariantList list;
        if (!readList(L, index, list))
        {
            return false;
        }
        for (const QVariant& item : std::as_const(list))
        {
            ShadMapCoordinate coordinate;
            if (!coordinateFromVariant(item, coordinate))
            {
                return false;
            }
            coordinates.append(coordinate);
        }
        return true;
    }

    int pushCoordinate(lua_State* L, const ShadMapCoordinate& coordinate)
    {
        ShadLuaStack::pushVariant(L, QVariant(coordinateMap(coordinate)));
        return 1;
    }

    template <class T>
    int pushObjects(lua_State* L, const QList<T*>& objects)
    {
        lua_createtable(L, int(objects.size()), 0);
        for (qsizetype position = 0; position < objects.size(); ++position)
        {
            ShadLuaObject::push(L, objects.at(position));
            lua_rawseti(L, -2, int(position) + 1);
        }
        return 1;
    }

    template <class T>
    std::function<int(lua_State*, T*)> coordinateSetter(void (T::*setter)(const ShadMapCoordinate&))
    {
        return [setter](lua_State* L, T* self) -> int
        {
            ShadMapCoordinate coordinate;
            if (!readCoordinate(L, 2, coordinate))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            (self->*setter)(coordinate);
            return 0;
        };
    }

    template <class T>
    std::function<int(lua_State*, T*)> coordinateGetter(ShadMapCoordinate (T::*getter)() const)
    {
        return [getter](lua_State* L, T* self) -> int { return pushCoordinate(L, (self->*getter)()); };
    }

    void registerMap()
    {
        ShadLuaClass<ShadMap> map("Map");
        map.constructor(&ShadLuaBind::newWithParent<ShadMap>);
        map.rawMethod("setTileUrl", [](lua_State* L, ShadMap* self) -> int
        {
            QString lightUrl;
            if (!ShadLuaStack::getString(L, 2, lightUrl))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setTileUrl(lightUrl, optionalString(L, 3));
            return 0;
        });
        map.method("getTileUrl", &ShadMap::getTileUrl);
        map.method("getDarkTileUrl", &ShadMap::getDarkTileUrl);
        map.method("setTileSubdomains", &ShadMap::setTileSubdomains);
        map.method("getTileSubdomains", &ShadMap::getTileSubdomains);

        map.rawMethod("setCenter", coordinateSetter(&ShadMap::setCenter));
        map.rawMethod("getCenter", coordinateGetter(&ShadMap::getCenter));
        map.rawMethod("flyTo", [](lua_State* L, ShadMap* self) -> int
        {
            ShadMapCoordinate center;
            double zoom = 0.0;
            if (!readCoordinate(L, 2, center))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (!ShadLuaStack::getNumber(L, 3, zoom))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            self->flyTo(center, zoom);
            return 0;
        });
        map.rawMethod("fitBounds", [](lua_State* L, ShadMap* self) -> int
        {
            QList<ShadMapCoordinate> coordinates;
            if (!readCoordinates(L, 2, coordinates))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            qint64 padding = 0;
            if (ShadLuaStack::getInteger(L, 3, padding))
            {
                self->fitBounds(coordinates, int(padding));
            }
            else
            {
                self->fitBounds(coordinates);
            }
            return 0;
        });
        map.rawMethod("coordinateAt", [](lua_State* L, ShadMap* self) -> int
        {
            double x = 0.0;
            double y = 0.0;
            if (!ShadLuaStack::getNumber(L, 2, x))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (!ShadLuaStack::getNumber(L, 3, y))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            return pushCoordinate(L, self->coordinateAt(QPointF(x, y)));
        });
        map.rawMethod("positionOf", [](lua_State* L, ShadMap* self) -> int
        {
            ShadMapCoordinate coordinate;
            if (!readCoordinate(L, 2, coordinate))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadLuaStack::pushVariant(L, QVariant(self->positionOf(coordinate)));
            return 1;
        });

        map.rawMethod("addMarker", [](lua_State* L, ShadMap* self) -> int
        {
            ShadMapCoordinate coordinate;
            if (!readCoordinate(L, 2, coordinate))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadLuaObject::push(L, self->addMarker(coordinate));
            return 1;
        });
        map.method("removeMarker", &ShadMap::removeMarker);
        map.rawMethod("markers", [](lua_State* L, ShadMap* self) -> int { return pushObjects(L, self->markers()); });
        map.method("clearMarkers", &ShadMap::clearMarkers);
        map.method("showMarkerPopup", &ShadMap::showMarkerPopup);
        map.method("hideMarkerPopup", &ShadMap::hideMarkerPopup);
        map.rawMethod("addGeoJson", [](lua_State* L, ShadMap* self) -> int
        {
            if (lua_type(L, 2) != LUA_TSTRING)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            size_t length = 0;
            const char* text = lua_tolstring(L, 2, &length);
            return pushObjects(L, self->addGeoJson(QByteArray(text, qsizetype(length))));
        });
        map.method("loadGeoJson", +[](ShadMap* self, QString url) { self->loadGeoJson(QUrl(url)); });

        map.rawMethod("addRoute", [](lua_State* L, ShadMap* self) -> int
        {
            QList<ShadMapCoordinate> coordinates;
            if (!readCoordinates(L, 2, coordinates))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadLuaObject::push(L, self->addRoute(coordinates));
            return 1;
        });
        map.method("removeRoute", &ShadMap::removeRoute);

        map.rawMethod("addArc", [](lua_State* L, ShadMap* self) -> int
        {
            ShadMapCoordinate from;
            ShadMapCoordinate to;
            if (!readCoordinate(L, 2, from))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (!readCoordinate(L, 3, to))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            ShadLuaObject::push(L, self->addArc(from, to));
            return 1;
        });
        map.rawMethod("addArcs", [](lua_State* L, ShadMap* self) -> int
        {
            QVariantList items;
            if (!readList(L, 2, items))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            QList<ShadMapArcData> arcs;
            for (const QVariant& item : std::as_const(items))
            {
                const QVariantMap fields = item.toMap();
                ShadMapArcData data;
                data.id = fields.value(QStringLiteral("id")).toString();
                if (!coordinateFromVariant(fields.value(QStringLiteral("from")), data.from) || !coordinateFromVariant(fields.value(QStringLiteral("to")), data.to))
                {
                    return ShadLuaStack::argumentError(L, 1);
                }
                arcs.append(data);
            }
            return pushObjects(L, self->addArcs(arcs));
        });
        map.method("removeArc", &ShadMap::removeArc);
        map.method("arc", &ShadMap::arc);
        map.rawMethod("arcs", [](lua_State* L, ShadMap* self) -> int { return pushObjects(L, self->arcs()); });
        map.method("clearArcs", &ShadMap::clearArcs);

        map.rawMethod("addPopup", [](lua_State* L, ShadMap* self) -> int
        {
            ShadMapCoordinate coordinate;
            if (!readCoordinate(L, 2, coordinate))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadLuaObject::push(L, self->addPopup(coordinate, optionalString(L, 3)));
            return 1;
        });
        map.method("removePopup", &ShadMap::removePopup);

        map.rawMethod("addOverlay", [](lua_State* L, ShadMap* self) -> int
        {
            QWidget* widget = ShadLuaObject::get<QWidget>(L, 2);
            if (widget == nullptr)
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->addOverlay(widget, static_cast<ShadMapType::ControlPosition>(optionalInt(L, 3, ShadMapType::TopLeft)));
            return 0;
        });
        map.method("removeOverlay", &ShadMap::removeOverlay);

        map.method("setZoomControlVisible", &ShadMap::setZoomControlVisible);
        map.method("isZoomControlVisible", &ShadMap::isZoomControlVisible);
        map.method("setResetControlVisible", &ShadMap::setResetControlVisible);
        map.method("isResetControlVisible", &ShadMap::isResetControlVisible);
        map.method("setLocateControlVisible", &ShadMap::setLocateControlVisible);
        map.method("isLocateControlVisible", &ShadMap::isLocateControlVisible);
        map.method("setCompassControlVisible", &ShadMap::setCompassControlVisible);
        map.method("isCompassControlVisible", &ShadMap::isCompassControlVisible);
        map.rawMethod("addControlButton", [](lua_State* L, ShadMap* self) -> int
        {
            qint64 icon = 0;
            if (!ShadLuaStack::getInteger(L, 2, icon))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            ShadLuaObject::push(L, self->addControlButton(static_cast<ShadIconType::IconName>(icon), optionalString(L, 3)));
            return 1;
        });
        map.method("removeControlButton", &ShadMap::removeControlButton);
        map.method("addControlSeparator", &ShadMap::addControlSeparator);

        map.rawMethod("setUserLocation", coordinateSetter(&ShadMap::setUserLocation));
        map.rawMethod("getUserLocation", coordinateGetter(&ShadMap::getUserLocation));
        map.method("hasUserLocation", &ShadMap::hasUserLocation);
        map.method("clearUserLocation", &ShadMap::clearUserLocation);
    }

    void registerLayers()
    {
        ShadLuaClass<ShadMapMarker> marker;
        marker.rawMethod("setCoordinate", coordinateSetter(&ShadMapMarker::setCoordinate));
        marker.rawMethod("getCoordinate", coordinateGetter(&ShadMapMarker::getCoordinate));
        marker.method("setContentWidget", &ShadMapMarker::setContentWidget);
        marker.method("getContentWidget", &ShadMapMarker::getContentWidget);

        ShadLuaClass<ShadMapRoute> route;
        route.rawMethod("setCoordinates", [](lua_State* L, ShadMapRoute* self) -> int
        {
            QList<ShadMapCoordinate> coordinates;
            if (!readCoordinates(L, 2, coordinates))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            self->setCoordinates(coordinates);
            return 0;
        });
        route.rawMethod("getCoordinates", [](lua_State* L, ShadMapRoute* self) -> int
        {
            const QList<ShadMapCoordinate> coordinates = self->getCoordinates();
            QVariantList list;
            for (const ShadMapCoordinate& coordinate : coordinates)
            {
                list.append(QVariant(coordinateMap(coordinate)));
            }
            ShadLuaStack::pushVariant(L, QVariant(list));
            return 1;
        });
        route.rawMethod("setDashPattern", [](lua_State* L, ShadMapRoute* self) -> int
        {
            QVariantList values;
            if (!readList(L, 2, values))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            QList<qreal> pattern;
            for (const QVariant& value : std::as_const(values))
            {
                double number = 0.0;
                if (!numberFromVariant(value, number))
                {
                    return ShadLuaStack::argumentError(L, 1);
                }
                pattern.append(number);
            }
            self->setDashPattern(pattern);
            return 0;
        });
        route.rawMethod("getDashPattern", [](lua_State* L, ShadMapRoute* self) -> int
        {
            const QList<qreal> pattern = self->getDashPattern();
            QVariantList list;
            for (const qreal value : pattern)
            {
                list.append(QVariant(double(value)));
            }
            ShadLuaStack::pushVariant(L, QVariant(list));
            return 1;
        });

        ShadLuaClass<ShadMapArc> arc;
        arc.rawMethod("setEndpoints", [](lua_State* L, ShadMapArc* self) -> int
        {
            ShadMapCoordinate from;
            ShadMapCoordinate to;
            if (!readCoordinate(L, 2, from))
            {
                return ShadLuaStack::argumentError(L, 1);
            }
            if (!readCoordinate(L, 3, to))
            {
                return ShadLuaStack::argumentError(L, 2);
            }
            self->setEndpoints(from, to);
            return 0;
        });
        arc.rawMethod("getFrom", coordinateGetter(&ShadMapArc::getFrom));
        arc.rawMethod("getTo", coordinateGetter(&ShadMapArc::getTo));

        ShadLuaClass<ShadMapPopup> popup;
        popup.rawMethod("setCoordinate", coordinateSetter(&ShadMapPopup::setCoordinate));
        popup.rawMethod("getCoordinate", coordinateGetter(&ShadMapPopup::getCoordinate));
        popup.method("setContentWidget", &ShadMapPopup::setContentWidget);
        popup.method("getContentWidget", &ShadMapPopup::getContentWidget);
        popup.method("setAnchorOffset", &ShadMapPopup::setAnchorOffset);
        popup.method("getAnchorOffset", &ShadMapPopup::getAnchorOffset);
    }
} // namespace

namespace ShadLuaBindings
{
    void registerMapComponents()
    {
        qRegisterMetaType<ShadMapMarker*>();
        QMetaType::registerConverter<ShadMapCoordinate, QVariantMap>(&coordinateMap);
        registerMap();
        registerLayers();
    }
} // namespace ShadLuaBindings
