#ifndef SHADLUABINDER_H
#define SHADLUABINDER_H

#include <QByteArray>
#include <QColor>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QString>
#include <QStringList>
#include <QVariant>
#include <QWidget>
#include <QtGlobal>

#include <cstddef>
#include <functional>
#include <tuple>
#include <type_traits>
#include <utility>

#include "ShadLuaDef.h"
#include "ShadLuaObject.h"
#include "ShadLuaStack.h"

struct lua_State;

// 返回压入栈的返回值个数，出错时压入错误信息并返回 -1
using ShadLuaMethod = std::function<int(lua_State*, QObject*)>;
using ShadLuaConstructor = std::function<int(lua_State*)>;

// 全部 Lua 状态共用的绑定表，注册只能在 GUI 线程进行
class SHADCN_LUA_EXPORT ShadLuaRegistry
{
    public:
        ShadLuaRegistry() = delete;

        static void addMethod(const QMetaObject* metaObject, const char* name, ShadLuaMethod method);
        static void addConstructor(const char* luaName, ShadLuaConstructor constructor);
        static void addFunction(const char* luaName, const char* functionName, ShadLuaConstructor function);
        static const ShadLuaMethod* findMethod(const QMetaObject* metaObject, const QByteArray& name);
        // method 必须来自 findMethod
        static void pushMethod(lua_State* L, const ShadLuaMethod* method);
        static void installConstructors(lua_State* L);
};

namespace ShadLuaBind
{
    // T 为不支持的类型时编译失败
    template <class T>
    bool get(lua_State* L, int index, T& value)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            value = ShadLuaStack::getVariant(L, index).toBool();
            return true;
        }
        else if constexpr (std::is_enum_v<T> || std::is_integral_v<T>)
        {
            qint64 number = 0;
            if (!ShadLuaStack::getInteger(L, index, number))
            {
                return false;
            }
            value = static_cast<T>(number);
            return true;
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            double number = 0.0;
            if (!ShadLuaStack::getNumber(L, index, number))
            {
                return false;
            }
            value = static_cast<T>(number);
            return true;
        }
        else if constexpr (std::is_same_v<T, QString>)
        {
            return ShadLuaStack::getString(L, index, value);
        }
        else if constexpr (std::is_same_v<T, QStringList>)
        {
            return ShadLuaStack::getStringList(L, index, value);
        }
        else if constexpr (std::is_same_v<T, QList<int>>)
        {
            return ShadLuaStack::getIntList(L, index, value);
        }
        else if constexpr (std::is_same_v<T, QColor>)
        {
            return ShadLuaStack::getColor(L, index, value);
        }
        else if constexpr (std::is_same_v<T, QVariant>)
        {
            value = ShadLuaStack::getVariant(L, index);
            return true;
        }
        else if constexpr (std::is_pointer_v<T> && std::is_base_of_v<QObject, std::remove_pointer_t<T>>)
        {
            value = qobject_cast<T>(ShadLuaObject::get(L, index));
            return value != nullptr || ShadLuaObject::handle(L, index) == nullptr;
        }
        else
        {
            static_assert(sizeof(T) == 0, "ShadLuaBind 不支持该参数类型，请改用 rawMethod 自行读取参数");
            return false;
        }
    }

    // T 为不支持的类型时编译失败
    template <class T>
    void push(lua_State* L, const T& value)
    {
        if constexpr (std::is_same_v<T, bool>)
        {
            ShadLuaStack::pushVariant(L, QVariant(value));
        }
        else if constexpr (std::is_enum_v<T> || std::is_integral_v<T>)
        {
            ShadLuaStack::pushVariant(L, QVariant(static_cast<qlonglong>(value)));
        }
        else if constexpr (std::is_floating_point_v<T>)
        {
            ShadLuaStack::pushVariant(L, QVariant(static_cast<double>(value)));
        }
        else if constexpr (std::is_same_v<T, QString>)
        {
            ShadLuaStack::pushString(L, value);
        }
        else if constexpr (std::is_same_v<T, QStringList>)
        {
            ShadLuaStack::pushStringList(L, value);
        }
        else if constexpr (std::is_same_v<T, QList<int>>)
        {
            ShadLuaStack::pushIntList(L, value);
        }
        else if constexpr (std::is_same_v<T, QColor>)
        {
            ShadLuaStack::pushColor(L, value);
        }
        else if constexpr (std::is_same_v<T, QVariant>)
        {
            ShadLuaStack::pushVariant(L, value);
        }
        else if constexpr (std::is_pointer_v<T> && std::is_base_of_v<QObject, std::remove_pointer_t<T>>)
        {
            ShadLuaObject::push(L, value);
        }
        else
        {
            static_assert(sizeof(T) == 0, "ShadLuaBind 不支持该返回类型，请改用 rawMethod 自行压入返回值");
        }
    }

    template <class Callable, class... Args, std::size_t... Indices>
    int invoke(lua_State* L, int firstIndex, Callable&& callable, std::tuple<Args...>& arguments, std::index_sequence<Indices...>)
    {
        int failedArgument = 0;
        const bool isRead = ((ShadLuaBind::get(L, firstIndex + int(Indices), std::get<Indices>(arguments)) ? true : (failedArgument = int(Indices) + 1, false)) && ...);
        if (!isRead)
        {
            return ShadLuaStack::argumentError(L, failedArgument);
        }
        if constexpr (std::is_void_v<std::invoke_result_t<Callable, Args&...>>)
        {
            callable(std::get<Indices>(arguments)...);
            return 0;
        }
        else
        {
            ShadLuaBind::push(L, callable(std::get<Indices>(arguments)...));
            return 1;
        }
    }

    template <class T>
    T* newWithParent(lua_State* L)
    {
        return new T(ShadLuaObject::get<QWidget>(L, 1));
    }

    template <class T>
    T* newWithText(lua_State* L)
    {
        QString text;
        if (ShadLuaStack::getString(L, 1, text))
        {
            return new T(text, ShadLuaObject::get<QWidget>(L, 2));
        }
        return new T(ShadLuaObject::get<QWidget>(L, 1));
    }

    template <class T>
    T* newWithTitle(lua_State* L)
    {
        QString title;
        if (ShadLuaStack::getString(L, 1, title))
        {
            QString description;
            ShadLuaStack::getString(L, 2, description);
            return new T(title, description, ShadLuaObject::get<QWidget>(L, 3));
        }
        return new T(ShadLuaObject::get<QWidget>(L, 1));
    }

    template <class T, class E>
    T* newWithEnum(lua_State* L)
    {
        qint64 value = 0;
        if (ShadLuaStack::getInteger(L, 1, value))
        {
            return new T(static_cast<E>(value), ShadLuaObject::get<QWidget>(L, 2));
        }
        return new T(ShadLuaObject::get<QWidget>(L, 1));
    }
} // namespace ShadLuaBind

// 为 QObject 子类注册构造函数与方法，属性、信号与槽由通用层自动提供
template <class T>
class ShadLuaClass
{
    public:
        // luaName 为空时不提供构造函数
        explicit ShadLuaClass(const char* luaName = nullptr)
            : _luaName(luaName)
        {
            static_assert(std::is_base_of_v<QObject, T>, "ShadLuaClass 只能用于 QObject 的子类");
        }

        // factory 返回 nullptr 表示参数不正确
        ShadLuaClass& constructor(std::function<T*(lua_State*)> factory)
        {
            Q_ASSERT(_luaName != nullptr);
            ShadLuaRegistry::addConstructor(_luaName, [factory](lua_State* L) -> int
            {
                T* object = factory(L);
                if (object == nullptr)
                {
                    return ShadLuaStack::error(L, QStringLiteral("构造函数的参数不正确"));
                }
                ShadLuaObject::push(L, object, true);
                return 1;
            });
            return *this;
        }

        ShadLuaClass& staticMethod(const char* name, ShadLuaConstructor function)
        {
            Q_ASSERT(_luaName != nullptr);
            ShadLuaRegistry::addFunction(_luaName, name, std::move(function));
            return *this;
        }

        // 重载的成员函数需要用 static_cast 指明签名
        template <class C, class R, class... Args>
        ShadLuaClass& method(const char* name, R (C::*function)(Args...))
        {
            static_assert(std::is_base_of_v<C, T>, "成员函数必须属于 T 或其基类");
            return rawMethod(name, [function](lua_State* L, T* self) -> int
            {
                std::tuple<std::decay_t<Args>...> arguments;
                return ShadLuaBind::invoke(L, 2, [self, function](std::decay_t<Args>&... values) -> R { return (static_cast<C*>(self)->*function)(values...); }, arguments, std::index_sequence_for<Args...>{});
            });
        }

        template <class C, class R, class... Args>
        ShadLuaClass& method(const char* name, R (C::*function)(Args...) const)
        {
            static_assert(std::is_base_of_v<C, T>, "成员函数必须属于 T 或其基类");
            return rawMethod(name, [function](lua_State* L, T* self) -> int
            {
                std::tuple<std::decay_t<Args>...> arguments;
                return ShadLuaBind::invoke(L, 2, [self, function](std::decay_t<Args>&... values) -> R { return (static_cast<const C*>(self)->*function)(values...); }, arguments, std::index_sequence_for<Args...>{});
            });
        }

        template <class R, class... Args>
        ShadLuaClass& method(const char* name, R (*function)(T*, Args...))
        {
            return rawMethod(name, [function](lua_State* L, T* self) -> int
            {
                std::tuple<std::decay_t<Args>...> arguments;
                return ShadLuaBind::invoke(L, 2, [self, function](std::decay_t<Args>&... values) -> R { return function(self, values...); }, arguments, std::index_sequence_for<Args...>{});
            });
        }

        ShadLuaClass& rawMethod(const char* name, std::function<int(lua_State*, T*)> function)
        {
            ShadLuaRegistry::addMethod(&T::staticMetaObject, name, [function](lua_State* L, QObject* object) -> int
            {
                T* self = qobject_cast<T*>(object);
                if (self == nullptr)
                {
                    return ShadLuaStack::error(L, QStringLiteral("对象的类型不正确"));
                }
                return function(L, self);
            });
            return *this;
        }

    private:
        const char* _luaName{nullptr};
};

#endif // SHADLUABINDER_H
