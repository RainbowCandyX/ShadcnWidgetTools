#ifndef SHADLUASTACK_H
#define SHADLUASTACK_H

#include <QColor>
#include <QList>
#include <QMetaType>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QtGlobal>

#include "ShadLuaDef.h"

struct lua_State;

// Lua 栈与 Qt 类型之间的转换，get 系列类型不匹配时返回 false
namespace ShadLuaStack
{
    SHADCN_LUA_EXPORT bool getInteger(lua_State* L, int index, qint64& value);
    SHADCN_LUA_EXPORT bool getNumber(lua_State* L, int index, double& value);

    SHADCN_LUA_EXPORT bool getString(lua_State* L, int index, QString& value);
    SHADCN_LUA_EXPORT void pushString(lua_State* L, const QString& value);

    SHADCN_LUA_EXPORT bool getStringList(lua_State* L, int index, QStringList& value);
    SHADCN_LUA_EXPORT void pushStringList(lua_State* L, const QStringList& value);

    SHADCN_LUA_EXPORT bool getIntList(lua_State* L, int index, QList<int>& value);
    SHADCN_LUA_EXPORT void pushIntList(lua_State* L, const QList<int>& value);

    SHADCN_LUA_EXPORT bool getColor(lua_State* L, int index, QColor& value);
    SHADCN_LUA_EXPORT void pushColor(lua_State* L, const QColor& value);

    SHADCN_LUA_EXPORT QVariant getVariant(lua_State* L, int index);
    SHADCN_LUA_EXPORT QVariant getVariant(lua_State* L, int index, QMetaType targetType);
    SHADCN_LUA_EXPORT void pushVariant(lua_State* L, const QVariant& value);

    SHADCN_LUA_EXPORT int tableLength(lua_State* L, int index);

    // 压入错误信息并返回 -1
    SHADCN_LUA_EXPORT int error(lua_State* L, const QString& message);
    SHADCN_LUA_EXPORT int argumentError(lua_State* L, int argument);
} // namespace ShadLuaStack

#endif // SHADLUASTACK_H
