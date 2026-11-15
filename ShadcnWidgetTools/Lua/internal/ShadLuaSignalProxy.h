#ifndef SHADLUASIGNALPROXY_H
#define SHADLUASIGNALPROXY_H

#include <QByteArray>
#include <QList>
#include <QMetaMethod>
#include <QMetaObject>
#include <QObject>
#include <QPointer>
#include <QString>

struct lua_State;
class ShadLuaEngine;

// 把 Qt 信号转发给 Lua 函数，每个 ShadLuaEngine 一个，不对外导出
// 不使用 Q_OBJECT，每个连接对应一个动态槽，在 qt_metacall 中分发
class ShadLuaSignalProxy : public QObject
{
    public:
        explicit ShadLuaSignalProxy(ShadLuaEngine* engine);
        ~ShadLuaSignalProxy() override;

        // 信号不存在时返回 false 并写入 errorMessage，重载信号选择参数最多的一个
        bool connectSignal(QObject* sender, const QByteArray& signalName, lua_State* L, int functionIndex, QString* errorMessage);
        void disconnectScript(lua_State* L, int scriptId);

        int qt_metacall(QMetaObject::Call call, int id, void** arguments) override;

    private:
        struct Connection
        {
            QPointer<QObject> sender;
            QMetaMethod signal;
            QMetaObject::Connection handle;
            int functionReference{0};
            int scriptId{0};
            // 断开后保留位置，保证动态槽编号不变
            bool isActive{true};
        };

        ShadLuaEngine* _engine{nullptr};
        QList<Connection> _connections;
};

#endif // SHADLUASIGNALPROXY_H
