#ifndef SHADDOCKPANEL_H
#define SHADDOCKPANEL_H

#include <QString>
#include <QWidget>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QAction;
class ShadDockManager;

// 可停靠面板，id 用于保存与恢复布局
class ShadDockPanelPrivate;

class SHADCN_EXPORT ShadDockPanel : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadDockPanel)
    Q_PROPERTY(QString title READ getTitle WRITE setTitle NOTIFY titleChanged)
    Q_PROPERTY(ShadDockType::PanelFlags panelFlags READ getPanelFlags WRITE setPanelFlags NOTIFY panelFlagsChanged)
    Q_PROPERTY(bool defaultCloseEnabled READ isDefaultCloseEnabled WRITE setDefaultCloseEnabled)

    public:
        explicit ShadDockPanel(const QString& id, QWidget* parent = nullptr);
        ShadDockPanel(const QString& id, ShadIconType::IconName icon, const QString& title, QWidget* widget, QWidget* parent = nullptr);
        ~ShadDockPanel() override;

        QString getId() const;

        // 旧组件会被删除
        void setWidget(QWidget* widget);
        QWidget* getWidget() const;

        void setTitle(const QString& title);
        QString getTitle() const;
        void setIcon(ShadIconType::IconName icon);
        ShadIconType::IconName getIcon() const;
        // 为空时显示标题
        void setTitleToolTip(const QString& toolTip);
        QString getTitleToolTip() const;

        void setPanelFlags(ShadDockType::PanelFlags flags);
        ShadDockType::PanelFlags getPanelFlags() const;
        void setPanelFlag(ShadDockType::PanelFlag flag, bool on = true);

        // 为 false 时只发出 closeRequested
        void setDefaultCloseEnabled(bool enabled);
        bool isDefaultCloseEnabled() const;

        ShadDockManager* getDockManager() const;
        bool isDocument() const;
        bool isFloating() const;
        bool isOpen() const;

        QAction* toggleViewAction() const;

    Q_SIGNALS:
        void titleChanged(const QString& title);
        void iconChanged(ShadIconType::IconName icon);
        void titleToolTipChanged(const QString& toolTip);
        void panelFlagsChanged(ShadDockType::PanelFlags flags);
        void closeRequested();
        void closed();

    private:
        friend class ShadDockManager;
        friend class ShadDockManagerPrivate;
};

#endif // SHADDOCKPANEL_H
