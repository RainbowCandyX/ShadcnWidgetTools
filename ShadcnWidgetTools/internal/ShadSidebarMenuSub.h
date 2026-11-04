#ifndef SHADSIDEBARMENUSUB_H
#define SHADSIDEBARMENUSUB_H

#include <QtGlobal>

#include "ShadSidebarCollapseArea.h"

class QEvent;
class QPaintEvent;
class QVBoxLayout;
class QWidget;
class ShadFrameAnimation;

// 菜单按钮的子菜单容器，组件库内部使用
class ShadSidebarMenuSub : public ShadSidebarCollapseArea
{
    Q_OBJECT

    public:
        explicit ShadSidebarMenuSub(QWidget* parent = nullptr);
        ~ShadSidebarMenuSub() override;

        // 子项所在的布局，布局属于 contentWidget()
        QVBoxLayout* itemLayout() const;

        // animated 为 false 时立即切换
        void setExpanded(bool expanded, bool animated);
        bool isExpanded() const;
        qreal contentOpacity() const;
        // 侧栏收起为图标的过渡进度
        void setIconProgress(qreal progress);
        qreal expandProgress() const;

    Q_SIGNALS:
        void progressChanged(qreal progress);

    protected:
        bool event(QEvent* event) override;
        void paintEvent(QPaintEvent* event) override;

    private:
        void applyMargins();
        void applyProgress(qreal progress);

        QVBoxLayout* _layout{nullptr};
        ShadFrameAnimation* _animation{nullptr};
        qreal _expandProgress{0.0};
        qreal _iconProgress{1.0};
        bool _isExpanded{false};
};

#endif // SHADSIDEBARMENUSUB_H
