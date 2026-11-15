#include "ShadLuaStack.h"

#include <QByteArray>
#include <QDate>
#include <QDateTime>
#include <QKeySequence>
#include <QObject>
#include <QPoint>
#include <QPointF>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QSizeF>
#include <QTime>
#include <QVariantList>
#include <QVariantMap>
#include <QtCore/qnamespace.h>

#include <cmath>

#include "ShadLuaInclude.h"
#include "ShadLuaObject.h"

namespace
{
    // 最大嵌套层数，避免自引用的表导致无限递归
    constexpr int kMaxTableDepth = 128;
    // 每层递归占用的栈位置，递归前先确保栈空间
    constexpr int kStackSlotsPerLevel = 4;

    int absoluteIndex(lua_State* L, int index)
    {
        return index > 0 || index <= LUA_REGISTRYINDEX ? index : lua_gettop(L) + index + 1;
    }

    bool numberAt(lua_State* L, int tableIndex, int position, double& value)
    {
        lua_rawgeti(L, tableIndex, position);
        const bool isNumber = lua_type(L, -1) == LUA_TNUMBER;
        if (isNumber)
        {
            value = lua_tonumber(L, -1);
        }
        lua_pop(L, 1);
        return isNumber;
    }

    QVariant variantAt(lua_State* L, int index, int depth)
    {
        switch (lua_type(L, index))
        {
            case LUA_TBOOLEAN:
                return QVariant(lua_toboolean(L, index) != 0);
            case LUA_TNUMBER:
            {
                // Lua 5.1 与 LuaJIT 没有整数类型，无小数部分的数字按整数处理
                const double number = lua_tonumber(L, index);
                if (std::floor(number) == number && std::fabs(number) < 9.0e15)
                {
                    return QVariant(qlonglong(number));
                }
                return QVariant(number);
            }
            case LUA_TSTRING:
            {
                QString text;
                ShadLuaStack::getString(L, index, text);
                return QVariant(text);
            }
            case LUA_TUSERDATA:
            {
                QObject* object = ShadLuaObject::get(L, index);
                return object != nullptr ? QVariant::fromValue(object) : QVariant();
            }
            case LUA_TTABLE:
                break;
            default:
                return QVariant();
        }
        if (depth >= kMaxTableDepth || !lua_checkstack(L, kStackSlotsPerLevel))
        {
            return QVariant();
        }
        const int tableIndex = absoluteIndex(L, index);
        const int length = ShadLuaStack::tableLength(L, tableIndex);
        if (length > 0)
        {
            QVariantList list;
            list.reserve(length);
            for (int position = 1; position <= length; ++position)
            {
                lua_rawgeti(L, tableIndex, position);
                list.append(variantAt(L, -1, depth + 1));
                lua_pop(L, 1);
            }
            return QVariant(list);
        }
        QVariantMap map;
        lua_pushnil(L);
        while (lua_next(L, tableIndex) != 0)
        {
            // 不对键调用 lua_tolstring，避免数字键被原地转换而打断遍历
            if (lua_type(L, -2) == LUA_TSTRING)
            {
                QString key;
                ShadLuaStack::getString(L, -2, key);
                map.insert(key, variantAt(L, -1, depth + 1));
            }
            lua_pop(L, 1);
        }
        return QVariant(map);
    }

    void pushVariantAt(lua_State* L, const QVariant& value, int depth)
    {
        const QMetaType type = value.metaType();
        // 本函数必须恰好压入一个值
        lua_checkstack(L, kStackSlotsPerLevel);
        if (!value.isValid() || depth >= kMaxTableDepth)
        {
            lua_pushnil(L);
            return;
        }
        if (type.flags().testFlag(QMetaType::PointerToQObject))
        {
            ShadLuaObject::push(L, *static_cast<QObject* const*>(value.constData()));
            return;
        }
        switch (type.id())
        {
            case QMetaType::Bool:
                lua_pushboolean(L, value.toBool() ? 1 : 0);
                return;
            case QMetaType::Int:
            case QMetaType::UInt:
            case QMetaType::LongLong:
            case QMetaType::ULongLong:
            case QMetaType::Long:
            case QMetaType::ULong:
            case QMetaType::Short:
            case QMetaType::UShort:
            case QMetaType::Char:
            case QMetaType::SChar:
            case QMetaType::UChar:
                lua_pushinteger(L, lua_Integer(value.toLongLong()));
                return;
            case QMetaType::Double:
            case QMetaType::Float:
                lua_pushnumber(L, lua_Number(value.toDouble()));
                return;
            case QMetaType::QString:
            case QMetaType::QChar:
            case QMetaType::QUrl:
                ShadLuaStack::pushString(L, value.toString());
                return;
            case QMetaType::QByteArray:
            {
                const QByteArray bytes = value.toByteArray();
                lua_pushlstring(L, bytes.constData(), size_t(bytes.size()));
                return;
            }
            case QMetaType::QStringList:
                ShadLuaStack::pushStringList(L, value.toStringList());
                return;
            case QMetaType::QColor:
                ShadLuaStack::pushColor(L, value.value<QColor>());
                return;
            case QMetaType::QDate:
                ShadLuaStack::pushString(L, value.toDate().toString(Qt::ISODate));
                return;
            case QMetaType::QTime:
                ShadLuaStack::pushString(L, value.toTime().toString(Qt::ISODate));
                return;
            case QMetaType::QDateTime:
                ShadLuaStack::pushString(L, value.toDateTime().toString(Qt::ISODate));
                return;
            case QMetaType::QKeySequence:
                ShadLuaStack::pushString(L, value.value<QKeySequence>().toString(QKeySequence::PortableText));
                return;
            case QMetaType::QSize:
            case QMetaType::QSizeF:
            {
                const QSizeF size = value.toSizeF();
                lua_createtable(L, 0, 2);
                lua_pushnumber(L, size.width());
                lua_setfield(L, -2, "width");
                lua_pushnumber(L, size.height());
                lua_setfield(L, -2, "height");
                return;
            }
            case QMetaType::QPoint:
            case QMetaType::QPointF:
            {
                const QPointF point = value.toPointF();
                lua_createtable(L, 0, 2);
                lua_pushnumber(L, point.x());
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, point.y());
                lua_setfield(L, -2, "y");
                return;
            }
            case QMetaType::QRect:
            case QMetaType::QRectF:
            {
                const QRectF rect = value.toRectF();
                lua_createtable(L, 0, 4);
                lua_pushnumber(L, rect.x());
                lua_setfield(L, -2, "x");
                lua_pushnumber(L, rect.y());
                lua_setfield(L, -2, "y");
                lua_pushnumber(L, rect.width());
                lua_setfield(L, -2, "width");
                lua_pushnumber(L, rect.height());
                lua_setfield(L, -2, "height");
                return;
            }
            case QMetaType::QVariantList:
            {
                const QVariantList list = value.toList();
                lua_createtable(L, int(list.size()), 0);
                for (int position = 0; position < list.size(); ++position)
                {
                    pushVariantAt(L, list.at(position), depth + 1);
                    lua_rawseti(L, -2, position + 1);
                }
                return;
            }
            case QMetaType::QVariantMap:
            {
                const QVariantMap map = value.toMap();
                lua_createtable(L, 0, int(map.size()));
                for (auto it = map.cbegin(); it != map.cend(); ++it)
                {
                    const QByteArray key = it.key().toUtf8();
                    pushVariantAt(L, it.value(), depth + 1);
                    lua_setfield(L, -2, key.constData());
                }
                return;
            }
            default:
                break;
        }
        if (type.flags().testFlag(QMetaType::IsEnumeration))
        {
            lua_pushinteger(L, lua_Integer(value.toLongLong()));
            return;
        }
        lua_pushnil(L);
    }
} // namespace

namespace ShadLuaStack
{
    bool getInteger(lua_State* L, int index, qint64& value)
    {
        if (lua_type(L, index) != LUA_TNUMBER)
        {
            return false;
        }
        value = qint64(lua_tonumber(L, index));
        return true;
    }

    bool getNumber(lua_State* L, int index, double& value)
    {
        if (lua_type(L, index) != LUA_TNUMBER)
        {
            return false;
        }
        value = lua_tonumber(L, index);
        return true;
    }

    bool getString(lua_State* L, int index, QString& value)
    {
        const int type = lua_type(L, index);
        if (type != LUA_TSTRING && type != LUA_TNUMBER)
        {
            return false;
        }
        // lua_tolstring 会原地转换数字，先复制再转换
        lua_pushvalue(L, index);
        size_t length = 0;
        const char* text = lua_tolstring(L, -1, &length);
        value = QString::fromUtf8(text, qsizetype(length));
        lua_pop(L, 1);
        return true;
    }

    void pushString(lua_State* L, const QString& value)
    {
        const QByteArray utf8 = value.toUtf8();
        lua_pushlstring(L, utf8.constData(), size_t(utf8.size()));
    }

    bool getStringList(lua_State* L, int index, QStringList& value)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int tableIndex = absoluteIndex(L, index);
        const int length = tableLength(L, tableIndex);
        value.clear();
        for (int position = 1; position <= length; ++position)
        {
            QString item;
            lua_rawgeti(L, tableIndex, position);
            const bool isString = getString(L, -1, item);
            lua_pop(L, 1);
            if (!isString)
            {
                return false;
            }
            value.append(item);
        }
        return true;
    }

    void pushStringList(lua_State* L, const QStringList& value)
    {
        lua_createtable(L, int(value.size()), 0);
        for (int position = 0; position < value.size(); ++position)
        {
            pushString(L, value.at(position));
            lua_rawseti(L, -2, position + 1);
        }
    }

    bool getColor(lua_State* L, int index, QColor& value)
    {
        QString name;
        if (lua_type(L, index) != LUA_TSTRING || !getString(L, index, name))
        {
            return false;
        }
        const QColor color(name);
        if (!color.isValid())
        {
            return false;
        }
        value = color;
        return true;
    }

    bool getIntList(lua_State* L, int index, QList<int>& value)
    {
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return false;
        }
        const int tableIndex = absoluteIndex(L, index);
        const int length = tableLength(L, tableIndex);
        value.clear();
        value.reserve(length);
        for (int position = 1; position <= length; ++position)
        {
            qint64 item = 0;
            lua_rawgeti(L, tableIndex, position);
            const bool isInteger = getInteger(L, -1, item);
            lua_pop(L, 1);
            if (!isInteger)
            {
                return false;
            }
            value.append(int(item));
        }
        return true;
    }

    void pushIntList(lua_State* L, const QList<int>& value)
    {
        lua_createtable(L, int(value.size()), 0);
        for (int position = 0; position < value.size(); ++position)
        {
            lua_pushinteger(L, value.at(position));
            lua_rawseti(L, -2, position + 1);
        }
    }

    void pushColor(lua_State* L, const QColor& value)
    {
        if (!value.isValid())
        {
            lua_pushnil(L);
            return;
        }
        pushString(L, value.name(value.alpha() < 255 ? QColor::HexArgb : QColor::HexRgb));
    }

    QVariant getVariant(lua_State* L, int index)
    {
        return variantAt(L, index, 0);
    }

    QVariant getVariant(lua_State* L, int index, QMetaType targetType)
    {
        if (targetType.id() == QMetaType::QKeySequence && lua_type(L, index) == LUA_TSTRING)
        {
            QString text;
            getString(L, index, text);
            return QVariant::fromValue(QKeySequence(text, QKeySequence::PortableText));
        }
        if (lua_type(L, index) != LUA_TTABLE)
        {
            return variantAt(L, index, 0);
        }
        const int tableIndex = absoluteIndex(L, index);
        double first = 0.0;
        double second = 0.0;
        double third = 0.0;
        double fourth = 0.0;
        const bool hasPair = numberAt(L, tableIndex, 1, first) && numberAt(L, tableIndex, 2, second);
        switch (targetType.id())
        {
            case QMetaType::QSize:
                return hasPair ? QVariant(QSize(qRound(first), qRound(second))) : QVariant();
            case QMetaType::QSizeF:
                return hasPair ? QVariant(QSizeF(first, second)) : QVariant();
            case QMetaType::QPoint:
                return hasPair ? QVariant(QPoint(qRound(first), qRound(second))) : QVariant();
            case QMetaType::QPointF:
                return hasPair ? QVariant(QPointF(first, second)) : QVariant();
            case QMetaType::QRect:
            case QMetaType::QRectF:
            {
                if (!hasPair || !numberAt(L, tableIndex, 3, third) || !numberAt(L, tableIndex, 4, fourth))
                {
                    return QVariant();
                }
                const QRectF rect(first, second, third, fourth);
                return targetType.id() == QMetaType::QRect ? QVariant(rect.toRect()) : QVariant(rect);
            }
            default:
                break;
        }
        return variantAt(L, index, 0);
    }

    void pushVariant(lua_State* L, const QVariant& value)
    {
        pushVariantAt(L, value, 0);
    }

    int tableLength(lua_State* L, int index)
    {
#if LUA_VERSION_NUM >= 502
        return int(lua_rawlen(L, index));
#else
        return int(lua_objlen(L, index));
#endif
    }

    int error(lua_State* L, const QString& message)
    {
        pushString(L, message);
        return -1;
    }

    int argumentError(lua_State* L, int argument)
    {
        return error(L, QStringLiteral("第 %1 个参数的类型不正确").arg(argument));
    }
} // namespace ShadLuaStack
