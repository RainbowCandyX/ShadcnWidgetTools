#ifndef SHADLUAOBJECT_H
#define SHADLUAOBJECT_H

#include <QObject>
#include <QPointer>

#include <memory>

#include "ShadLuaDef.h"

struct lua_State;
struct ShadLuaOwnership;

// Lua 中表示 QObject 的句柄，只保存 QPointer，对象销毁后句柄失效
// 由 Lua 创建的无父对象且未显示的对象，在全部句柄回收时删除
class SHADCN_LUA_EXPORT ShadLuaObject
{
    public:
        ShadLuaObject();
        explicit ShadLuaObject(QObject* object, bool isOwned = false);
        ~ShadLuaObject();

        QObject* object() const;
        bool isValid() const;
        void release();

        static void push(lua_State* L, QObject* object, bool isOwned = false);
        static ShadLuaObject* handle(lua_State* L, int index);
        static QObject* get(lua_State* L, int index);

        template <class T>
        static T* get(lua_State* L, int index)
        {
            return qobject_cast<T*>(get(L, index));
        }

    private:
        QPointer<QObject> _object;
        // 句柄共用一份所有权，最后一个销毁时处理对象
        std::shared_ptr<ShadLuaOwnership> _ownership;
};

#endif // SHADLUAOBJECT_H
