#include "ShadLuaEngine.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QDebug>
#include <QFile>
#include <QFileInfo>
#include <QIODevice>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QStringList>
#include <QVariantList>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadLuaBindings.h"
#include "ShadLuaEnginePrivate.h"
#include "ShadLuaHttp.h"
#include "ShadLuaInclude.h"
#include "ShadLuaMeta.h"
#include "ShadLuaModules.h"
#include "ShadLuaObject.h"
#include "ShadLuaSignalProxy.h"
#include "ShadLuaStack.h"

namespace
{
    // 取变量的地址作为注册表键
    const char kEngineKey = 0;
    constexpr int kTrackedObjectsPruneInterval = 256;

    struct LibraryEntry
    {
        ShadLuaType::Library library;
        const char* name;
        lua_CFunction open;
    };

    int traceback(lua_State* L)
    {
        const char* message = lua_tostring(L, 1);
        luaL_traceback(L, L, message != nullptr ? message : "未知错误", 1);
        return 1;
    }

    void openLibrary(lua_State* L, const LibraryEntry& entry)
    {
        lua_pushcfunction(L, entry.open);
        lua_pushstring(L, entry.name);
        lua_call(L, 1, 1);
        lua_setglobal(L, entry.name);
    }

    QByteArray chunkNameOf(const QString& name)
    {
        return "=" + (name.isEmpty() ? QByteArrayLiteral("script") : name.toUtf8());
    }

    ShadLuaEngine* engineFor(ShadLuaType::Scope scope)
    {
        return scope == ShadLuaType::Shared ? ShadLuaEngine::shared() : new ShadLuaEngine();
    }

    // 决定独立引擎的归属：随组件或 parent 销毁，失败或无 parent 时立即删除
    QWidget* adoptWidget(ShadLuaEngine* engine, QWidget* widget, QWidget* parent, ShadLuaType::Scope scope, QString* errorMessage)
    {
        const bool isFailed = widget == nullptr && !engine->getLastError().isEmpty();
        if (errorMessage != nullptr)
        {
            *errorMessage = isFailed ? engine->getLastError() : QString();
        }
        if (scope == ShadLuaType::Shared)
        {
            return widget;
        }
        if (widget != nullptr)
        {
            engine->setParent(widget);
        }
        else if (!isFailed && parent != nullptr)
        {
            engine->setParent(parent);
        }
        else
        {
            delete engine;
        }
        return widget;
    }

    int emitPrinted(lua_State* L, int first)
    {
        QStringList parts;
        for (int index = first; index <= lua_gettop(L); ++index)
        {
            QString text;
            ShadLuaStack::getString(L, index, text);
            parts.append(text);
        }
        ShadLuaEngine* engine = ShadLuaEngine::fromState(L);
        if (engine != nullptr)
        {
            ShadLuaEnginePrivate::get(engine)->printMessage(parts.join(u'\t'));
        }
        return 0;
    }

    // tostring 可能抛出 Lua 错误，调用期间不能有需要析构的 C++ 对象
    int printEntry(lua_State* L)
    {
        const int count = lua_gettop(L);
        lua_getglobal(L, "tostring");
        for (int index = 1; index <= count; ++index)
        {
            lua_pushvalue(L, count + 1);
            lua_pushvalue(L, index);
            lua_call(L, 1, 1);
        }
        return emitPrinted(L, count + 2);
    }
} // namespace

ShadLuaEnginePrivate::ShadLuaEnginePrivate()
{
}

ShadLuaEnginePrivate::~ShadLuaEnginePrivate()
{
    // 先清理脚本对象再删除信号转发对象，状态关闭后不再有回调
    const QList<int> scriptIds = _scripts.keys();
    for (const int scriptId : scriptIds)
    {
        releaseScript(scriptId);
    }
    delete _http;
    delete _signalProxy;
    if (_state != nullptr)
    {
        lua_close(_state);
    }
}

ShadLuaEnginePrivate* ShadLuaEnginePrivate::get(ShadLuaEngine* engine)
{
    return engine->d_func();
}

void ShadLuaEnginePrivate::createState(ShadLuaType::Libraries libraries)
{
    Q_Q(ShadLuaEngine);
    ShadLuaBindings::registerAll();
    _signalProxy = new ShadLuaSignalProxy(q);
    _state = luaL_newstate();
    const LibraryEntry entries[] = {
        {ShadLuaType::BaseLibrary, "_G", luaopen_base},
        {ShadLuaType::TableLibrary, LUA_TABLIBNAME, luaopen_table},
        {ShadLuaType::StringLibrary, LUA_STRLIBNAME, luaopen_string},
        {ShadLuaType::MathLibrary, LUA_MATHLIBNAME, luaopen_math},
        {ShadLuaType::IoLibrary, LUA_IOLIBNAME, luaopen_io},
        {ShadLuaType::OsLibrary, LUA_OSLIBNAME, luaopen_os},
        {ShadLuaType::PackageLibrary, LUA_LOADLIBNAME, luaopen_package},
        {ShadLuaType::DebugLibrary, LUA_DBLIBNAME, luaopen_debug},
#if defined(SHADCN_LUA_JIT)
        {ShadLuaType::BitLibrary, LUA_BITLIBNAME, luaopen_bit},
        {ShadLuaType::JitLibrary, LUA_JITLIBNAME, luaopen_jit},
        {ShadLuaType::FfiLibrary, LUA_FFILIBNAME, luaopen_ffi},
#else
        {ShadLuaType::CoroutineLibrary, LUA_COLIBNAME, luaopen_coroutine},
        {ShadLuaType::Utf8Library, LUA_UTF8LIBNAME, luaopen_utf8},
#endif
    };
    for (const LibraryEntry& entry : entries)
    {
        if (libraries.testFlag(entry.library))
        {
            openLibrary(_state, entry);
        }
    }
    // print 改由 messagePrinted 信号输出，图形程序没有控制台
    if (libraries.testFlag(ShadLuaType::BaseLibrary))
    {
        lua_pushcfunction(_state, printEntry);
        lua_setglobal(_state, "print");
    }
    lua_pushlightuserdata(_state, const_cast<char*>(&kEngineKey));
    lua_pushlightuserdata(_state, q);
    lua_rawset(_state, LUA_REGISTRYINDEX);
    ShadLuaMeta::install(_state);
    ShadLuaModules::install(_state, libraries.testFlag(ShadLuaType::HttpLibrary));
}

bool ShadLuaEnginePrivate::execute(const QByteArray& code, const QByteArray& chunkName, int resultCount)
{
    // 字节码以 ESC 开头
    if (code.startsWith('\x1b'))
    {
        const bool isLuaJitCode = code.startsWith(QByteArrayLiteral("\x1bLJ"));
#if defined(SHADCN_LUA_JIT)
        const bool isCompatible = isLuaJitCode;
#else
        const bool isCompatible = !isLuaJitCode;
#endif
        if (!isCompatible)
        {
            reportError(QStringLiteral("字节码的格式与当前运行时 %1 不兼容，LuaJIT 与 PUC-Lua 的字节码不能互相加载").arg(ShadLuaEngine::runtimeName()));
            return false;
        }
    }
    const int top = lua_gettop(_state);
    lua_pushcfunction(_state, traceback);
    const int handlerIndex = lua_gettop(_state);
    if (luaL_loadbuffer(_state, code.constData(), size_t(code.size()), chunkName.constData()) != 0 || lua_pcall(_state, 0, resultCount, handlerIndex) != 0)
    {
        QString message;
        ShadLuaStack::getString(_state, -1, message);
        lua_settop(_state, top);
        reportError(message);
        return false;
    }
    lua_remove(_state, handlerIndex);
    _lastError.clear();
    return true;
}

bool ShadLuaEnginePrivate::callReference(int functionReference, const QVariantList& arguments)
{
    const int top = lua_gettop(_state);
    lua_pushcfunction(_state, traceback);
    const int handlerIndex = lua_gettop(_state);
    lua_rawgeti(_state, LUA_REGISTRYINDEX, functionReference);
    for (const QVariant& argument : arguments)
    {
        ShadLuaStack::pushVariant(_state, argument);
    }
    const bool isOk = lua_pcall(_state, int(arguments.size()), 0, handlerIndex) == 0;
    QString message;
    if (!isOk)
    {
        ShadLuaStack::getString(_state, -1, message);
    }
    lua_settop(_state, top);
    if (!isOk)
    {
        reportError(message);
    }
    return isOk;
}

void ShadLuaEnginePrivate::reportError(const QString& message)
{
    Q_Q(ShadLuaEngine);
    _lastError = message;
    qWarning().noquote() << QStringLiteral("Lua 脚本出错：") << message;
    Q_EMIT q->errorOccurred(message);
}

ShadLuaHttp* ShadLuaEnginePrivate::http()
{
    Q_Q(ShadLuaEngine);
    if (_http == nullptr)
    {
        _http = new ShadLuaHttp(q);
    }
    return _http;
}

void ShadLuaEnginePrivate::printMessage(const QString& message)
{
    Q_Q(ShadLuaEngine);
    qInfo().noquote() << message;
    Q_EMIT q->messagePrinted(message);
}

void ShadLuaEnginePrivate::trackObject(QObject* object)
{
    QList<QPointer<QObject>>& objects = _scripts[_currentScript].objects;
    // 定期清除已销毁对象的空指针，避免列表无限增长
    if (objects.size() >= kTrackedObjectsPruneInterval && objects.size() % kTrackedObjectsPruneInterval == 0)
    {
        objects.removeIf([](const QPointer<QObject>& pointer) { return pointer.isNull(); });
    }
    objects.append(QPointer<QObject>(object));
}

void ShadLuaEnginePrivate::untrackObject(QObject* object)
{
    for (Script& script : _scripts)
    {
        script.objects.removeIf([object](const QPointer<QObject>& pointer) { return pointer.data() == object; });
    }
}

void ShadLuaEnginePrivate::releaseScript(int scriptId)
{
    Q_Q(ShadLuaEngine);
    _signalProxy->disconnectScript(_state, scriptId);
    if (_http != nullptr)
    {
        _http->cancelScript(scriptId);
    }
    const Script script = _scripts.take(scriptId);
    if (!script.widget.isNull())
    {
        script.widget->hide();
        script.widget->deleteLater();
    }
    for (const QPointer<QObject>& pointer : script.objects)
    {
        QObject* object = pointer.data();
        // 有其他父对象的由父对象管理，挂在引擎下的一并删除
        if (object == nullptr || (object->parent() != nullptr && object->parent() != q))
        {
            continue;
        }
        // 可能正处于自身的事件处理中，先隐藏再延迟删除
        QWidget* widget = qobject_cast<QWidget*>(object);
        if (widget != nullptr)
        {
            widget->hide();
        }
        object->deleteLater();
    }
}

ShadLuaEngine::ShadLuaEngine(QObject* parent)
    : ShadLuaEngine(ShadLuaType::SafeLibraries, parent)
{
}

ShadLuaEngine::ShadLuaEngine(ShadLuaType::Libraries libraries, QObject* parent)
    : QObject(parent), d_ptr(new ShadLuaEnginePrivate())
{
    Q_D(ShadLuaEngine);
    d->q_ptr = this;
    d->createState(libraries);
}

ShadLuaEngine::~ShadLuaEngine()
{
}

ShadLuaEngine* ShadLuaEngine::shared()
{
    static QPointer<ShadLuaEngine> engine;
    if (engine.isNull())
    {
        engine = new ShadLuaEngine(QCoreApplication::instance());
    }
    return engine.data();
}

ShadLuaEngine* ShadLuaEngine::fromState(lua_State* L)
{
    lua_pushlightuserdata(L, const_cast<char*>(&kEngineKey));
    lua_rawget(L, LUA_REGISTRYINDEX);
    ShadLuaEngine* engine = static_cast<ShadLuaEngine*>(lua_touserdata(L, -1));
    lua_pop(L, 1);
    return engine;
}

QString ShadLuaEngine::runtimeName()
{
#if defined(SHADCN_LUA_JIT)
    return QStringLiteral("LuaJIT %1.%2").arg(LUAJIT_VERSION_NUM / 10000).arg(LUAJIT_VERSION_NUM / 100 % 100);
#else
    return QString::fromLatin1(LUA_VERSION);
#endif
}

bool ShadLuaEnginePrivate::readScriptFile(const QString& filePath, QByteArray& content)
{
    QFile file(filePath);
    if (!file.open(QIODevice::ReadOnly))
    {
        reportError(QStringLiteral("无法打开脚本文件 %1：%2").arg(filePath, file.errorString()));
        return false;
    }
    content = file.readAll();
    return true;
}

int ShadLuaEnginePrivate::loadScript(const QByteArray& code, const QString& name)
{
    const int scriptId = _nextScriptId++;
    _scripts[scriptId].name = name;
    const int previousScript = _currentScript;
    _currentScript = scriptId;
    const bool isLoaded = execute(code, chunkNameOf(name), 1);
    _currentScript = previousScript;
    if (!isLoaded)
    {
        releaseScript(scriptId);
        return 0;
    }
    // 返回的组件由脚本记录管理，卸载时删除
    ShadLuaObject* handle = ShadLuaObject::handle(_state, -1);
    QWidget* widget = handle != nullptr ? qobject_cast<QWidget*>(handle->object()) : nullptr;
    if (widget != nullptr)
    {
        handle->release();
        _scripts[scriptId].widget = widget;
    }
    lua_pop(_state, 1);
    return scriptId;
}

QWidget* ShadLuaEnginePrivate::createWidget(const QByteArray& code, const QString& name, QWidget* parent)
{
    if (!execute(code, chunkNameOf(name), 1))
    {
        return nullptr;
    }
    ShadLuaObject* handle = ShadLuaObject::handle(_state, -1);
    QWidget* widget = handle != nullptr ? qobject_cast<QWidget*>(handle->object()) : nullptr;
    if (widget == nullptr)
    {
        lua_pop(_state, 1);
        return nullptr;
    }
    // 组件交给调用方，Lua 不再负责删除
    handle->release();
    untrackObject(widget);
    lua_pop(_state, 1);
    if (parent != nullptr)
    {
        widget->setParent(parent);
    }
    return widget;
}

bool ShadLuaEngine::run(const QString& code, const QString& chunkName)
{
    Q_D(ShadLuaEngine);
    return d->execute(code.toUtf8(), chunkNameOf(chunkName), 0);
}

bool ShadLuaEngine::runFile(const QString& filePath)
{
    Q_D(ShadLuaEngine);
    QByteArray content;
    return d->readScriptFile(filePath, content) && d->execute(content, chunkNameOf(QFileInfo(filePath).fileName()), 0);
}

QWidget* ShadLuaEngine::createWidget(const QString& code, QWidget* parent, const QString& chunkName)
{
    Q_D(ShadLuaEngine);
    return d->createWidget(code.toUtf8(), chunkName, parent);
}

QWidget* ShadLuaEngine::loadWidget(const QString& filePath, QWidget* parent)
{
    Q_D(ShadLuaEngine);
    QByteArray content;
    return d->readScriptFile(filePath, content) ? d->createWidget(content, QFileInfo(filePath).fileName(), parent) : nullptr;
}

int ShadLuaEngine::load(const QString& code, const QString& name)
{
    Q_D(ShadLuaEngine);
    return d->loadScript(code.toUtf8(), name);
}

int ShadLuaEngine::loadFile(const QString& filePath)
{
    Q_D(ShadLuaEngine);
    QByteArray content;
    return d->readScriptFile(filePath, content) ? d->loadScript(content, QFileInfo(filePath).fileName()) : 0;
}

void ShadLuaEngine::unload(int scriptId)
{
    Q_D(ShadLuaEngine);
    if (scriptId > 0 && d->_scripts.contains(scriptId))
    {
        d->releaseScript(scriptId);
    }
}

QObjectList ShadLuaEngine::getScriptObjects(int scriptId) const
{
    Q_D(const ShadLuaEngine);
    QObjectList objects;
    const auto it = d->_scripts.constFind(scriptId);
    if (it == d->_scripts.constEnd())
    {
        return objects;
    }
    for (const QPointer<QObject>& pointer : it->objects)
    {
        if (!pointer.isNull())
        {
            objects.append(pointer.data());
        }
    }
    return objects;
}

QWidget* ShadLuaEngine::getScriptWidget(int scriptId) const
{
    Q_D(const ShadLuaEngine);
    const auto it = d->_scripts.constFind(scriptId);
    return it != d->_scripts.constEnd() ? it->widget.data() : nullptr;
}

int ShadLuaEngine::getCurrentScript() const
{
    Q_D(const ShadLuaEngine);
    return d->_currentScript;
}

QString ShadLuaEngine::getLastError() const
{
    Q_D(const ShadLuaEngine);
    return d->_lastError;
}

lua_State* ShadLuaEngine::state() const
{
    Q_D(const ShadLuaEngine);
    return d->_state;
}

namespace ShadLua
{
    QWidget* createWidget(const QString& code, QWidget* parent, ShadLuaType::Scope scope, QString* errorMessage)
    {
        ShadLuaEngine* engine = engineFor(scope);
        return adoptWidget(engine, engine->createWidget(code, parent), parent, scope, errorMessage);
    }

    QWidget* loadWidget(const QString& filePath, QWidget* parent, ShadLuaType::Scope scope, QString* errorMessage)
    {
        ShadLuaEngine* engine = engineFor(scope);
        return adoptWidget(engine, engine->loadWidget(filePath, parent), parent, scope, errorMessage);
    }

    ShadLuaEngine* engineOf(const QWidget* widget)
    {
        return widget != nullptr ? widget->findChild<ShadLuaEngine*>(QString(), Qt::FindDirectChildrenOnly) : nullptr;
    }
} // namespace ShadLua
