#ifndef SHADLUAENGINEPRIVATE_H
#define SHADLUAENGINEPRIVATE_H

#include <QByteArray>
#include <QHash>
#include <QList>
#include <QObject>
#include <QPointer>
#include <QString>
#include <QVariantList>
#include <QWidget>

#include "ShadLuaDef.h"
#include "ShadProperty.h"

struct lua_State;
class ShadLuaEngine;
class ShadLuaHttp;
class ShadLuaSignalProxy;

class ShadLuaEnginePrivate
{
    SHAD_D_CREATE(ShadLuaEngine)

    public:
        ShadLuaEnginePrivate();
        ~ShadLuaEnginePrivate();

        // 编号 0 记录不属于任何脚本的对象
        struct Script
        {
            QString name;
            QList<QPointer<QObject>> objects;
            QPointer<QWidget> widget;
        };

        static ShadLuaEnginePrivate* get(ShadLuaEngine* engine);

        void createState(ShadLuaType::Libraries libraries);
        // 成功时返回值留在栈上，失败时记录错误并恢复栈
        bool execute(const QByteArray& code, const QByteArray& chunkName, int resultCount);
        // 读取原始内容，字节码不能按文本转换
        bool readScriptFile(const QString& filePath, QByteArray& content);
        // 失败返回 0
        int loadScript(const QByteArray& code, const QString& name);
        QWidget* createWidget(const QByteArray& code, const QString& name, QWidget* parent);
        bool callReference(int functionReference, const QVariantList& arguments);
        void reportError(const QString& message);
        void printMessage(const QString& message);

        void trackObject(QObject* object);
        void untrackObject(QObject* object);
        // 断开信号，中止 HTTP 请求，删除创建的对象并移除记录
        void releaseScript(int scriptId);
        // 第一次调用时创建
        ShadLuaHttp* http();

        lua_State* _state{nullptr};
        ShadLuaSignalProxy* _signalProxy{nullptr};
        ShadLuaHttp* _http{nullptr};
        QString _lastError;
        QHash<int, Script> _scripts;
        int _currentScript{0};
        int _nextScriptId{1};
};

#endif // SHADLUAENGINEPRIVATE_H
