#ifndef SHADDOCKGROUP_H
#define SHADDOCKGROUP_H

#include <QList>
#include <QPoint>
#include <QPointer>
#include <QRect>
#include <QSize>
#include <QWidget>

#include "ShadDef.h"

class QEvent;
class QObject;
class QPaintEvent;
class QStackedLayout;
class QVBoxLayout;
class ShadDockHeader;
class ShadDockPanel;
class ShadPageTransition;
class ShadSeparator;
class ShadTabViewBar;

class ShadDockGroup : public QWidget
{
    Q_OBJECT

    public:
        enum class Kind
        {
            Document,
            Tool,
        };

        explicit ShadDockGroup(Kind kind, QWidget* parent = nullptr);
        // 同时删除仍在组内的面板
        ~ShadDockGroup() override;

        Kind kind() const;
        // 中央文档组为空时也保留
        void setCentral(bool central);
        bool isCentral() const;

        int insertPanel(int index, ShadDockPanel* panel);
        void removePanel(ShadDockPanel* panel);
        // 面板析构时调用
        bool forgetPanel(const QObject* panel);
        int count() const;
        ShadDockPanel* panel(int index) const;
        int indexOf(const ShadDockPanel* panel) const;
        QList<ShadDockPanel*> panels() const;

        void setCurrentPanel(ShadDockPanel* panel);
        ShadDockPanel* currentPanel() const;
        void updatePanel(ShadDockPanel* panel);
        void setActive(bool active);

        // 以下坐标均为本组件坐标
        QPoint dragAnchor(int index, const QPoint& grabOffset) const;
        QPoint headerAnchor(const QPoint& grabPos) const;
        QRect dropStripRect() const;
        int insertionIndexAt(const QPoint& pos) const;
        void setDropIndicator(int index);

        QSize sizeHint() const override;

    Q_SIGNALS:
        void currentChanged(ShadDockPanel* panel);
        void closeRequested(ShadDockPanel* panel);
        void panelDragRequested(ShadDockPanel* panel, const QPoint& grabOffset);
        void groupDragRequested(const QPoint& grabPos);
        void pressed();

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void updateChrome();
        static ShadTabViewType::TabFlags tabFlagsOf(const ShadDockPanel* panel);

        Kind _kind{Kind::Document};
        bool _isCentral{false};
        QList<ShadDockPanel*> _panels;
        QVBoxLayout* _layout{nullptr};
        ShadDockHeader* _header{nullptr};
        ShadSeparator* _topSeparator{nullptr};
        QWidget* _pageArea{nullptr};
        QStackedLayout* _pageStack{nullptr};
        ShadSeparator* _bottomSeparator{nullptr};
        ShadTabViewBar* _bar{nullptr};
        ShadPageTransition* _transition{nullptr};
        QPointer<ShadDockPanel> _shownPanel;
};

#endif // SHADDOCKGROUP_H
