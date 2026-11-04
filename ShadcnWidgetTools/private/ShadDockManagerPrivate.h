#ifndef SHADDOCKMANAGERPRIVATE_H
#define SHADDOCKMANAGERPRIVATE_H

#include <QJsonObject>
#include <QList>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSet>
#include <QSize>

#include "ShadDef.h"
#include "ShadDockGroup.h"
#include "ShadProperty.h"

class QEvent;
class QVBoxLayout;
class QWidget;
class ShadDockContainer;
class ShadDockFloatingWindow;
class ShadDockManager;
class ShadDockPanel;

// 只用于事件过滤，不需要 Q_OBJECT
class ShadDockManagerPrivate : public QObject
{
    SHAD_D_CREATE(ShadDockManager)

    public:
        ShadDockManagerPrivate();
        ~ShadDockManagerPrivate() override;

        static constexpr int kStateVersion = 1;
        static constexpr int kDefaultFloatingWidth = 480;
        static constexpr int kDefaultFloatingHeight = 360;

        struct DropTarget
        {
            QPointer<ShadDockContainer> container;
            // 为 nullptr 时相对整个容器
            QPointer<ShadDockGroup> group;
            ShadDockType::DockArea area{ShadDockType::CenterArea};
            bool isValid{false};
            // -1 表示末尾
            int tabIndex{-1};
            int extent{0};
        };

        struct DragState
        {
            bool isActive{false};
            bool isSystemMove{false};
            QPointer<ShadDockFloatingWindow> window;
            QPoint anchor;
            // 用于重新计算 anchor
            QPointer<ShadDockGroup> anchorGroup;
            int anchorIndex{0};
            QPoint anchorOffset;
            bool isHeaderAnchor{false};
            // 拖出前的位置
            QPointer<ShadDockGroup> sourceGroup;
            int sourceIndex{-1};
            QList<QPointer<ShadDockPanel>> panels;
            bool hasDocuments{false};
            DropTarget target;
            QPointer<ShadDockGroup> indicatorGroup;
        };

        void registerPanel(ShadDockPanel* panel, bool isDocument);
        void placePanel(ShadDockPanel* panel, ShadDockType::DockArea area, ShadDockGroup* relativeGroup);
        void insertIntoGroup(ShadDockGroup* group, ShadDockPanel* panel, int index);
        void takeFromGroup(ShadDockPanel* panel);
        void setPanelOpen(ShadDockPanel* panel, bool open);
        void panelChanged(ShadDockPanel* panel);
        // 面板析构时调用
        void panelDestroyed(ShadDockPanel* panel);
        void requestClose(ShadDockPanel* panel);

        ShadDockGroup* createGroup(ShadDockGroup::Kind kind);
        ShadDockGroup* documentGroup() const;
        void releaseGroupIfEmpty(ShadDockGroup* group);
        ShadDockFloatingWindow* createFloatingWindow();
        void releaseWindowIfEmpty(ShadDockFloatingWindow* window);
        void updateWindow(ShadDockFloatingWindow* window);
        void requestCloseWindow(ShadDockFloatingWindow* window);
        bool isWindowMovable(const ShadDockFloatingWindow* window) const;
        QList<ShadDockContainer*> containers() const;
        static ShadDockContainer* containerOf(const QWidget* widget);
        static ShadDockFloatingWindow* windowOf(const QWidget* widget);
        static QList<ShadDockPanel*> panelsIn(const ShadDockContainer* container);

        void setActiveGroup(ShadDockGroup* group);
        void activateGroup(ShadDockGroup* group);
        void updateCurrentPanel();
        int defaultExtent(const ShadDockContainer* container, const ShadDockGroup* relativeGroup, ShadDockType::DockArea area, const ShadDockGroup* group) const;

        void startPanelDrag(ShadDockGroup* group, ShadDockPanel* panel, const QPoint& grabOffset);
        void startGroupDrag(ShadDockGroup* group, const QPoint& grabPos);
        void startWindowDrag(ShadDockFloatingWindow* window, const QPoint& pressGlobalPos, bool isSystemMove);
        ShadDockFloatingWindow* detachToWindow(const QList<ShadDockPanel*>& panels, ShadDockGroup::Kind kind, const QSize& size);
        void beginDrag(ShadDockFloatingWindow* window, bool isSystemMove);
        void alignWindow(const QPoint& globalPos);
        void updateDrag(const QPoint& globalPos);
        void finishDrag(bool isCancelled);
        void clearDropFeedback();
        QRect previewRect(const ShadDockContainer* container, DropTarget* target) const;
        void dock(ShadDockFloatingWindow* window, const DropTarget& target);

        QJsonObject saveNode(QWidget* node) const;
        QWidget* buildNode(const QJsonObject& object, bool isMain, QSet<const ShadDockPanel*>* placed);
        void restoreRoot(ShadDockContainer* container, const QJsonObject& object, bool isMain, QSet<const ShadDockPanel*>* placed);

        bool eventFilter(QObject* watched, QEvent* event) override;

        QVBoxLayout* _layout{nullptr};
        ShadDockContainer* _mainContainer{nullptr};
        QPointer<ShadDockGroup> _centralGroup;
        QList<ShadDockPanel*> _panels;
        QList<QPointer<ShadDockFloatingWindow>> _windows;
        QPointer<ShadDockGroup> _activeGroup;
        QPointer<ShadDockGroup> _lastDocumentGroup;
        QPointer<ShadDockPanel> _currentPanel;
        DragState _drag;
};

#endif // SHADDOCKMANAGERPRIVATE_H
