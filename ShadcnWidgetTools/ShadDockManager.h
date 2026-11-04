#ifndef SHADDOCKMANAGER_H
#define SHADDOCKMANAGER_H

#include <QByteArray>
#include <QList>
#include <QRect>
#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadProperty.h"

class ShadDockPanel;

// 停靠管理器，对应 Visual Studio 式的停靠布局
class ShadDockManagerPrivate;

class SHADCN_EXPORT ShadDockManager : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDockManager)

    public:
        explicit ShadDockManager(QWidget* parent = nullptr);
        // 同时删除面板与浮动窗口
        ~ShadDockManager() override;

        // relativeTo 为空时停靠到边缘，CenterArea 合并为标签
        void addPanel(ShadDockPanel* panel, ShadDockType::DockArea area, ShadDockPanel* relativeTo = nullptr);
        // 加入最近使用的文档组
        void addDocument(ShadDockPanel* panel);
        void tabifyPanel(ShadDockPanel* panel, ShadDockPanel* existing);
        void floatPanel(ShadDockPanel* panel, const QRect& geometry = QRect());
        // 调用方负责释放面板
        void removePanel(ShadDockPanel* panel);

        void closePanel(ShadDockPanel* panel);
        // 回到关闭前的位置
        void showPanel(ShadDockPanel* panel);

        void setCurrentPanel(ShadDockPanel* panel);
        ShadDockPanel* getCurrentPanel() const;
        QList<ShadDockPanel*> panels() const;
        ShadDockPanel* findPanel(const QString& id) const;

        // 只恢复已加入的面板
        QByteArray saveState() const;
        bool restoreState(const QByteArray& state);

    Q_SIGNALS:
        void currentPanelChanged(ShadDockPanel* panel);

    private:
        friend class ShadDockPanel;
};

#endif // SHADDOCKMANAGER_H
