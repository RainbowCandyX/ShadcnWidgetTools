#ifndef LUASCRIPTMANAGER_H
#define LUASCRIPTMANAGER_H

#include <QList>
#include <QPointer>
#include <QString>
#include <QWidget>

class QEvent;
class QObject;
class QStandardItem;
class QStandardItemModel;
class ShadCodeBlock;
class ShadLabel;
class ShadListView;
class ShadLuaEngine;
class ShadSwitch;

// Lua 脚本管理器：列出脚本，勾选加载，取消勾选卸载
class LuaScriptManager : public QWidget
{
    Q_OBJECT

    public:
        explicit LuaScriptManager(QWidget* parent = nullptr);
        ~LuaScriptManager() override;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;

    private:
        struct Script
        {
            QString name;
            QString filePath;
            bool isBuiltIn{false};
            QStandardItem* item{nullptr};
            QPointer<ShadLuaEngine> engine;
            int scriptId{0};
            bool isShared{false};
            bool isFailed{false};
        };

        static QString scriptDirectory();
        void refreshList();
        void addScript(const QString& filePath, bool isBuiltIn);
        void loadScript(int index);
        void unloadScript(int index);
        void unloadAll();
        void reloadScripts();
        void setChecked(int index, bool checked);
        void updateStatus(int index);
        void connectEngine(ShadLuaEngine* engine);
        QString sourceOf(ShadLuaEngine* engine) const;
        enum class LogKind
        {
            Output = 0,
            Status,
            Error,
        };

        void appendLog(const QString& source, const QString& message, LogKind kind);

        QList<Script> _scripts;
        ShadListView* _list{nullptr};
        QStandardItemModel* _model{nullptr};
        // 代码修改列表项时为 true，避免当作用户操作
        bool _isUpdatingItems{false};
        ShadLabel* _directoryLabel{nullptr};
        ShadSwitch* _sharedSwitch{nullptr};
        ShadCodeBlock* _log{nullptr};
        // 正在加载的脚本下标，-1 表示没有
        int _loadingIndex{-1};
};

#endif // LUASCRIPTMANAGER_H
