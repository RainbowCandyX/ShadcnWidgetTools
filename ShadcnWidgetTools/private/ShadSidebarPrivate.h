#ifndef SHADSIDEBARPRIVATE_H
#define SHADSIDEBARPRIVATE_H

#include <QPointer>
#include <QRect>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QVBoxLayout;
class ShadFrameAnimation;
class QWidget;
class ShadScrollArea;
class ShadSidebar;
class ShadSidebarMenuButton;
class ShadSidebarRail;

class ShadSidebarPrivate
{
    SHAD_D_CREATE(ShadSidebar)

    public:
        ShadSidebarPrivate();
        ~ShadSidebarPrivate();

        static constexpr int kDefaultExpandedWidth = 256;
        static constexpr int kIconWidth = 48;
        static constexpr int kVariantPadding = 8;
        static constexpr int kSectionPadding = 8;
        static constexpr int kSectionGap = 8;
        static constexpr int kBorderWidth = 1;
        static constexpr int kAnimationDurationMs = 250;
        static constexpr int kDefaultMarkDurationMs = 300;
        static constexpr int kMaxMarkDurationMs = 1000;

        int variantPadding() const;
        int collapsedWidth() const;
        // 不小于 Floating / Inset 变体收起为图标后的宽度
        int boundedExpandedWidth(int width) const;
        void applyWidth();
        void layoutChildren();
        // LTR 逻辑坐标
        QRect logicalPanelRect() const;
        QRect visualRect(const QRect& logicalRect) const;
        void refreshIconState();
        // 同步收起为图标的过渡进度
        void syncIconTransition();
        void updateSections();
        void setProgress(qreal progress);
        void startMarkTransition(ShadSidebarMenuButton* from, ShadSidebarMenuButton* to);
        void stopMarkTransition();
        void applyMark(qreal value);
        void finishMarkPhase();

        QWidget* _panel{nullptr};
        QWidget* _header{nullptr};
        QVBoxLayout* _headerLayout{nullptr};
        ShadScrollArea* _scrollArea{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QWidget* _footer{nullptr};
        QVBoxLayout* _footerLayout{nullptr};
        ShadSidebarRail* _rail{nullptr};
        ShadFrameAnimation* _animation{nullptr};
        QPointer<ShadSidebarMenuButton> _activeButton;
        ShadFrameAnimation* _markAnimation{nullptr};
        int _markDuration{kDefaultMarkDurationMs};
        QPointer<ShadSidebarMenuButton> _markFrom;
        QPointer<ShadSidebarMenuButton> _markTo;
        bool _isMarkDown{false};
        bool _isMarkSecondPhase{false};
        ShadSidebarType::Side _side{ShadSidebarType::Start};
        ShadSidebarType::Variant _variant{ShadSidebarType::Sidebar};
        ShadSidebarType::Collapsible _collapsible{ShadSidebarType::Offcanvas};
        int _expandedWidth{kDefaultExpandedWidth};
        int _minimumExpandedWidth{0};
        int _maximumExpandedWidth{QWIDGETSIZE_MAX};
        bool _isResizable{false};
        bool _isSubMenuExclusive{false};
        int _dragStartWidth{0};
        bool _isExpanded{true};
        // 菜单项当前是否为图标模式
        bool _isIconApplied{false};
        bool _isRailVisible{true};
        qreal _progress{1.0};
};

#endif // SHADSIDEBARPRIVATE_H
