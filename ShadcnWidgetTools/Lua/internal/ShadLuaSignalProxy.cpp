#include "ShadLuaSignalProxy.h"

#include <QMetaType>
#include <QVariant>
#include <QVariantList>
#include <QtCore/qnamespace.h>

#include "ShadLuaEngine.h"
#include "ShadLuaEnginePrivate.h"
#include "ShadLuaInclude.h"

ShadLuaSignalProxy::ShadLuaSignalProxy(ShadLuaEngine* engine)
    : QObject(nullptr), _engine(engine)
{
}

ShadLuaSignalProxy::~ShadLuaSignalProxy()
{
}

bool ShadLuaSignalProxy::connectSignal(QObject* sender, const QByteArray& signalName, lua_State* L, int functionIndex, QString* errorMessage)
{
    const QMetaObject* metaObject = sender->metaObject();
    QMetaMethod signal;
    for (int index = 0; index < metaObject->methodCount(); ++index)
    {
        const QMetaMethod method = metaObject->method(index);
        if (method.methodType() == QMetaMethod::Signal && method.name() == signalName && (!signal.isValid() || method.parameterCount() > signal.parameterCount()))
        {
            signal = method;
        }
    }
    if (!signal.isValid())
    {
        *errorMessage = QStringLiteral("%1 没有信号 %2").arg(QString::fromUtf8(metaObject->className()), QString::fromUtf8(signalName));
        return false;
    }
    Connection connection;
    connection.sender = sender;
    connection.signal = signal;
    connection.scriptId = ShadLuaEnginePrivate::get(_engine)->_currentScript;
    lua_pushvalue(L, functionIndex);
    connection.functionReference = luaL_ref(L, LUA_REGISTRYINDEX);
    const int slotIndex = QObject::staticMetaObject.methodCount() + int(_connections.size());
    connection.handle = QMetaObject::connect(sender, signal.methodIndex(), this, slotIndex, Qt::DirectConnection, nullptr);
    _connections.append(connection);
    return true;
}

void ShadLuaSignalProxy::disconnectScript(lua_State* L, int scriptId)
{
    for (Connection& connection : _connections)
    {
        if (!connection.isActive || connection.scriptId != scriptId)
        {
            continue;
        }
        connection.isActive = false;
        QObject::disconnect(connection.handle);
        luaL_unref(L, LUA_REGISTRYINDEX, connection.functionReference);
    }
}

int ShadLuaSignalProxy::qt_metacall(QMetaObject::Call call, int id, void** arguments)
{
    id = QObject::qt_metacall(call, id, arguments);
    if (id < 0 || call != QMetaObject::InvokeMetaMethod)
    {
        return id;
    }
    if (id >= _connections.size() || !_connections.at(id).isActive)
    {
        return -1;
    }
    // 回调中新增连接会使列表重新分配，先复制一份
    const Connection connection = _connections.at(id);
    QVariantList values;
    for (int index = 0; index < connection.signal.parameterCount(); ++index)
    {
        const QMetaType type = connection.signal.parameterMetaType(index);
        const void* data = arguments[index + 1];
        if (type.id() == QMetaType::QVariant)
        {
            values.append(*static_cast<const QVariant*>(data));
        }
        else
        {
            values.append(type.isValid() ? QVariant(type, data) : QVariant());
        }
    }
    // 回调期间的对象与信号记在连接所属的脚本名下
    ShadLuaEnginePrivate* engine = ShadLuaEnginePrivate::get(_engine);
    const int previousScript = engine->_currentScript;
    engine->_currentScript = connection.scriptId;
    engine->callReference(connection.functionReference, values);
    engine->_currentScript = previousScript;
    return -1;
}
