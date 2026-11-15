#ifndef SHADLUAENGINE_H
#define SHADLUAENGINE_H

#include <QObject>
#include <QString>

#include "ShadLuaDef.h"
#include "ShadProperty.h"

struct lua_State;
class QWidget;
class ShadLuaEnginePrivate;

// Lua 脚本引擎，每个引擎持有独立的 Lua 状态
// 脚本只能在 GUI 线程执行，出错时返回 false 或 nullptr
class SHADCN_LUA_EXPORT ShadLuaEngine : public QObject
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadLuaEngine)

    public:
        explicit ShadLuaEngine(QObject* parent = nullptr);
        // 开放的标准库创建后不能修改
        explicit ShadLuaEngine(ShadLuaType::Libraries libraries, QObject* parent = nullptr);
        ~ShadLuaEngine() override;

        // 进程内共享的引擎，随应用对象销毁
        static ShadLuaEngine* shared();
        static ShadLuaEngine* fromState(lua_State* L);
        static QString runtimeName();

        // 文件加载同时接受源码与字节码，不校验字节码，只加载可信来源的文件
        bool run(const QString& code, const QString& chunkName = QString());
        bool runFile(const QString& filePath);

        // 返回脚本返回的组件，不再由 Lua 负责删除
        // 无返回组件不算错误，出错时返回 nullptr，以 getLastError 区分
        QWidget* createWidget(const QString& code, QWidget* parent = nullptr, const QString& chunkName = QString());
        QWidget* loadWidget(const QString& filePath, QWidget* parent = nullptr);

        // 加载可单独卸载的脚本，成功返回大于 0 的脚本编号，失败返回 0
        int load(const QString& code, const QString& name = QString());
        int loadFile(const QString& filePath);
        // 卸载脚本，断开信号并删除它创建的组件与无父对象的对象
        void unload(int scriptId);
        // 没有返回组件时为 nullptr，卸载脚本时会被删除
        QWidget* getScriptWidget(int scriptId) const;
        QObjectList getScriptObjects(int scriptId) const;
        // 仅在脚本加载与信号回调期间有效，其余时间为 0
        int getCurrentScript() const;

        // 没有错误时为空
        QString getLastError() const;

        lua_State* state() const;

    Q_SIGNALS:
        void errorOccurred(const QString& message);
        void messagePrinted(const QString& message);
};

namespace ShadLua
{
    // Isolated 的引擎随返回的组件销毁，无返回组件或出错时返回 nullptr
    SHADCN_LUA_EXPORT QWidget* createWidget(const QString& code, QWidget* parent = nullptr, ShadLuaType::Scope scope = ShadLuaType::Isolated, QString* errorMessage = nullptr);
    SHADCN_LUA_EXPORT QWidget* loadWidget(const QString& filePath, QWidget* parent = nullptr, ShadLuaType::Scope scope = ShadLuaType::Isolated, QString* errorMessage = nullptr);
    // 仅 Isolated 方式创建的组件有所属引擎，其余返回 nullptr
    SHADCN_LUA_EXPORT ShadLuaEngine* engineOf(const QWidget* widget);
} // namespace ShadLua

#endif // SHADLUAENGINE_H
