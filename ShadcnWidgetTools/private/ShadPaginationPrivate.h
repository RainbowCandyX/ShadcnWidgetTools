#ifndef SHADPAGINATIONPRIVATE_H
#define SHADPAGINATIONPRIVATE_H

#include <QList>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"

class QWidget;
class ShadButton;
class ShadInput;
class ShadLabel;
class ShadPagination;
class ShadPaginationNavButton;

class ShadPaginationPrivate
{
    SHAD_D_CREATE(ShadPagination)

    public:
        ShadPaginationPrivate();
        ~ShadPaginationPrivate();

        // 页码与省略号之间 gap-1；页码区与跳转区 gap-4；跳转区内 gap-2
        static constexpr int kItemGap = 4;
        static constexpr int kJumpGap = 16;
        static constexpr int kJumpInnerGap = 8;
        // 跳转输入框可视宽度
        static constexpr int kJumpInputWidth = 56;
        // 页码按钮 size="icon"（36x36）
        static constexpr int kButtonSize = 36;
        // 页码列表中表示省略号的值
        static constexpr int kEllipsis = 0;

        // 页码列表：首尾 boundaryCount 页、当前页两侧 siblingCount 页，不连续处为 kEllipsis
        QList<int> pageItems() const;
        // 按页码列表更新按钮、导航按钮的可用状态、Tab 顺序，并重新排列
        void updateItems();
        // 按显示顺序排列可见的子组件
        void layoutItems();
        // 内容的可视宽度（不含首尾焦点环边距）
        int contentWidth() const;
        // 点击页码或导航按钮：focusActive 为 true 时把焦点交给当前页按钮（按钮复用后被点击的按钮可能显示别的页码）
        void changePage(int page, bool focusActive);
        // 页码跳转输入框按 Enter
        void jumpToInput();
        // 输入内容变化时更新错误态
        void updateJumpInvalid();
        ShadButton* pageButtonAt(int index);
        ShadButton* ellipsisAt(int index);

        ShadPaginationNavButton* _previousButton{nullptr};
        ShadPaginationNavButton* _nextButton{nullptr};
        QList<ShadButton*> _pageButtons;
        QList<ShadButton*> _ellipses;
        ShadLabel* _jumpPrefix{nullptr};
        ShadInput* _jumpInput{nullptr};
        ShadLabel* _jumpSuffix{nullptr};
        // 当前显示的页码与省略号，按显示顺序
        QList<QWidget*> _itemWidgets;
        int _pageCount{1};
        int _currentPage{1};
        int _siblingCount{1};
        int _boundaryCount{1};
        bool _isNavigationVisible{true};
        bool _isPageButtonsVisible{true};
        bool _isJumpVisible{false};
        Qt::Alignment _alignment{Qt::AlignHCenter};
};

#endif // SHADPAGINATIONPRIVATE_H
