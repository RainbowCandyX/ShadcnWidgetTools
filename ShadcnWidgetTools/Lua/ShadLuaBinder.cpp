#include "ShadLuaBinder.h"

#include <QHash>
#include <QList>

#include <utility>

#include "ShadLuaInclude.h"
#include "ShadLuaMeta.h"

namespace
{
    struct Function
    {
        QByteArray tableName;
        QByteArray name;
        ShadLuaConstructor* function{nullptr};
    };

    // 堆上分配且不释放，地址作为闭包上值长期有效
    struct Registry
    {
        QHash<const QMetaObject*, QHash<QByteArray, ShadLuaMethod*>> methods;
        QList<Function> functions;
    };

    Registry& registry()
    {
        static Registry instance;
        return instance;
    }

    int callMethod(lua_State* L)
    {
        const ShadLuaMethod* method = static_cast<const ShadLuaMethod*>(lua_touserdata(L, lua_upvalueindex(1)));
        QObject* object = ShadLuaObject::get(L, 1);
        if (object == nullptr)
        {
            return ShadLuaStack::error(L, QStringLiteral("对象无效或已销毁；方法需要用冒号调用，例如 object:method()"));
        }
        return (*method)(L, object);
    }

    // C++ 局部对象已在 callMethod 中析构，可安全抛出 Lua 错误
    int methodEntry(lua_State* L)
    {
        const int count = callMethod(L);
        return count < 0 ? ShadLuaMeta::raise(L) : count;
    }

    int callConstructor(lua_State* L)
    {
        const ShadLuaConstructor* constructor = static_cast<const ShadLuaConstructor*>(lua_touserdata(L, lua_upvalueindex(1)));
        return (*constructor)(L);
    }

    int constructorEntry(lua_State* L)
    {
        const int count = callConstructor(L);
        return count < 0 ? ShadLuaMeta::raise(L) : count;
    }
} // namespace

void ShadLuaRegistry::addMethod(const QMetaObject* metaObject, const char* name, ShadLuaMethod method)
{
    QHash<QByteArray, ShadLuaMethod*>& methods = registry().methods[metaObject];
    const QByteArray key(name);
    // 重复注册时覆盖原有实现
    const auto it = methods.constFind(key);
    if (it != methods.constEnd())
    {
        *it.value() = std::move(method);
        return;
    }
    methods.insert(key, new ShadLuaMethod(std::move(method)));
}

void ShadLuaRegistry::addConstructor(const char* luaName, ShadLuaConstructor constructor)
{
    addFunction(luaName, "new", std::move(constructor));
}

void ShadLuaRegistry::addFunction(const char* luaName, const char* functionName, ShadLuaConstructor function)
{
    const QByteArray tableName(luaName);
    const QByteArray name(functionName);
    // 重复注册时覆盖原有实现
    for (const Function& entry : std::as_const(registry().functions))
    {
        if (entry.tableName == tableName && entry.name == name)
        {
            *entry.function = std::move(function);
            return;
        }
    }
    registry().functions.append(Function{tableName, name, new ShadLuaConstructor(std::move(function))});
}

const ShadLuaMethod* ShadLuaRegistry::findMethod(const QMetaObject* metaObject, const QByteArray& name)
{
    const QHash<const QMetaObject*, QHash<QByteArray, ShadLuaMethod*>>& methods = registry().methods;
    for (const QMetaObject* current = metaObject; current != nullptr; current = current->superClass())
    {
        const auto classIt = methods.constFind(current);
        if (classIt == methods.constEnd())
        {
            continue;
        }
        const auto methodIt = classIt->constFind(name);
        if (methodIt != classIt->constEnd())
        {
            return methodIt.value();
        }
    }
    return nullptr;
}

void ShadLuaRegistry::pushMethod(lua_State* L, const ShadLuaMethod* method)
{
    lua_pushlightuserdata(L, const_cast<ShadLuaMethod*>(method));
    lua_pushcclosure(L, methodEntry, 1);
}

void ShadLuaRegistry::installConstructors(lua_State* L)
{
    // shad 带有 __newindex 元方法，用 rawset 直接写入
    for (const Function& entry : std::as_const(registry().functions))
    {
        lua_pushstring(L, entry.tableName.constData());
        lua_rawget(L, -2);
        if (lua_type(L, -1) != LUA_TTABLE)
        {
            lua_pop(L, 1);
            lua_createtable(L, 0, 1);
            lua_pushstring(L, entry.tableName.constData());
            lua_pushvalue(L, -2);
            lua_rawset(L, -4);
        }
        lua_pushlightuserdata(L, entry.function);
        lua_pushcclosure(L, constructorEntry, 1);
        lua_setfield(L, -2, entry.name.constData());
        lua_pop(L, 1);
    }
}
