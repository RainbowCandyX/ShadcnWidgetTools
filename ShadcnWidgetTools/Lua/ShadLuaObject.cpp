#include "ShadLuaObject.h"

#include <QWidget>

#include "ShadLuaEngine.h"
#include "ShadLuaEnginePrivate.h"
#include "ShadLuaInclude.h"

struct ShadLuaOwnership
{
    ~ShadLuaOwnership()
    {
        QObject* target = object.data();
        if (target == nullptr || isReleased || target->parent() != nullptr)
        {
            return;
        }
        // 正在显示的顶层窗口不随句柄回收关闭
        const QWidget* widget = qobject_cast<QWidget*>(target);
        if (widget != nullptr && widget->isVisible())
        {
            return;
        }
        target->deleteLater();
    }

    QPointer<QObject> object;
    bool isReleased{false};
};

ShadLuaObject::ShadLuaObject()
{
}

ShadLuaObject::ShadLuaObject(QObject* object, bool isOwned)
    : _object(object)
{
    if (isOwned && object != nullptr)
    {
        _ownership = std::make_shared<ShadLuaOwnership>();
        _ownership->object = object;
    }
}

ShadLuaObject::~ShadLuaObject()
{
}

QObject* ShadLuaObject::object() const
{
    return _object.data();
}

bool ShadLuaObject::isValid() const
{
    return !_object.isNull();
}

void ShadLuaObject::release()
{
    if (_ownership != nullptr)
    {
        _ownership->isReleased = true;
    }
}

void ShadLuaObject::push(lua_State* L, QObject* object, bool isOwned)
{
    if (object == nullptr)
    {
        lua_pushnil(L);
        return;
    }
    const int top = lua_gettop(L);
    if (!luabridge::push(L, ShadLuaObject(object, isOwned)))
    {
        lua_settop(L, top);
        lua_pushnil(L);
        return;
    }
    // 记到当前脚本名下，卸载脚本时一并清理
    ShadLuaEngine* engine = isOwned ? ShadLuaEngine::fromState(L) : nullptr;
    if (engine != nullptr)
    {
        ShadLuaEnginePrivate::get(engine)->trackObject(object);
    }
}

ShadLuaObject* ShadLuaObject::handle(lua_State* L, int index)
{
    if (!lua_isuserdata(L, index))
    {
        return nullptr;
    }
    const auto result = luabridge::get<ShadLuaObject*>(L, index);
    return result ? *result : nullptr;
}

QObject* ShadLuaObject::get(lua_State* L, int index)
{
    const ShadLuaObject* object = handle(L, index);
    return object != nullptr ? object->object() : nullptr;
}
