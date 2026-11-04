#ifndef SHADTABSPRIVATE_H
#define SHADTABSPRIVATE_H

#include <QList>
#include <QPointer>

#include "ShadProperty.h"

class QBoxLayout;
class QVBoxLayout;
class QWidget;
class ShadPageTransition;
class ShadTabs;
class ShadTabsList;

class ShadTabsPrivate
{
    SHAD_D_CREATE(ShadTabs)

    public:
        ShadTabsPrivate();
        ~ShadTabsPrivate();

        static constexpr int kGap = 8;

        void updateLayout();
        // 只显示当前标签的页面；当前标签没有页面时隐藏内容区，不留出间距
        void updatePages();

        QBoxLayout* _layout{nullptr};
        ShadTabsList* _list{nullptr};
        QWidget* _pageArea{nullptr};
        QVBoxLayout* _pageLayout{nullptr};
        // 与标签一一对应，没有页面的标签为空
        QList<QPointer<QWidget>> _pages;
        ShadPageTransition* _transition{nullptr};
};

#endif // SHADTABSPRIVATE_H
