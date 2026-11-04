#ifndef SHADTABVIEW_H
#define SHADTABVIEW_H

#include <QString>
#include <QVariant>
#include <QWidget>

#include <functional>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QChildEvent;
class QEvent;
class QObject;

// 标签视图，对应 WinUI TabView：标签可关闭、新建、拖动排序，同组之间可拆分与合并
class ShadTabViewPrivate;

class SHADCN_EXPORT ShadTabView : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadTabView)
    Q_PROPERTY(int currentIndex READ currentIndex WRITE setCurrentIndex NOTIFY currentChanged)
    Q_PROPERTY(bool tabsClosable READ isTabsClosable WRITE setTabsClosable)
    Q_PROPERTY(bool addButtonVisible READ isAddButtonVisible WRITE setAddButtonVisible)
    Q_PROPERTY(QString dragGroup READ getDragGroup WRITE setDragGroup)

    public:
        // 返回接收拆分标签的标签视图，返回 nullptr 时取消
        using DetachHandler = std::function<ShadTabView*(ShadTabView* source)>;

        explicit ShadTabView(QWidget* parent = nullptr);
        ~ShadTabView() override;

        // page 为 nullptr 时返回 -1
        int addTab(const QString& text, QWidget* page);
        int addTab(ShadIconType::IconName icon, const QString& text, QWidget* page);
        int insertTab(int index, ShadIconType::IconName icon, const QString& text, QWidget* page);
        // 不删除页面，由调用方释放
        void removeTab(int index);
        void moveTab(int from, int to);
        int count() const;
        QWidget* widget(int index) const;
        int indexOf(QWidget* page) const;

        void setTabText(int index, const QString& text);
        QString tabText(int index) const;
        void setTabIcon(int index, ShadIconType::IconName icon);
        ShadIconType::IconName tabIcon(int index) const;

        // 单个标签的限制，默认 NoTabFlag
        void setTabFlags(int index, ShadTabViewType::TabFlags flags);
        ShadTabViewType::TabFlags tabFlags(int index) const;
        void setTabFlag(int index, ShadTabViewType::TabFlag flag, bool on = true);
        // 为空时显示标签文字
        void setTabToolTip(int index, const QString& toolTip);
        QString tabToolTip(int index) const;
        void setTabData(int index, const QVariant& data);
        QVariant tabData(int index) const;

        void setCurrentIndex(int index);
        int currentIndex() const;

        void setTabsClosable(bool closable);
        bool isTabsClosable() const;

        void setAddButtonVisible(bool visible);
        bool isAddButtonVisible() const;

        // 可放进 ShadWindow 标题栏
        QWidget* tabBar() const;

        // 同组且不为空的标签视图之间可以互相拖动标签
        void setDragGroup(const QString& group);
        QString getDragGroup() const;

        void setDetachHandler(const DetachHandler& handler);

    Q_SIGNALS:
        void currentChanged(int index);
        void tabCloseRequested(int index);
        void newTabRequested();
        void tabMoved(int from, int to);
        // 最后一个标签被拖走后发出
        void lastTabMovedOut();

    protected:
        void childEvent(QChildEvent* event) override;
        bool eventFilter(QObject* watched, QEvent* event) override;
};

#endif // SHADTABVIEW_H
