#include "ShadTabsPrivate.h"

#include <QBoxLayout>
#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>

#include "ShadPageTransition.h"
#include "ShadStyleHelper.h"
#include "ShadTabs.h"
#include "ShadTabsList.h"

ShadTabsPrivate::ShadTabsPrivate()
{
}

ShadTabsPrivate::~ShadTabsPrivate()
{
}

void ShadTabsPrivate::updateLayout()
{
    const bool isHorizontal = _list->getOrientation() == Qt::Horizontal;
    _layout->setDirection(isHorizontal ? QBoxLayout::TopToBottom : QBoxLayout::LeftToRight);
    _layout->setAlignment(_list, Qt::AlignLeft | Qt::AlignTop);
    // Line 变体的指示线超出标签栏，需扣除
    _layout->setSpacing(kGap - _list->indicatorOverflow());
}

void ShadTabsPrivate::updatePages()
{
    const int current = _list->currentIndex();
    QWidget* next = current >= 0 && current < _pages.size() ? _pages.at(current).data() : nullptr;
    QWidget* previous = nullptr;
    for (const QPointer<QWidget>& page : _pages)
    {
        if (!page.isNull() && !page->isHidden())
        {
            previous = page.data();
        }
    }
    const bool isAnimated = previous != nullptr && next != nullptr && previous != next;
    if (isAnimated)
    {
        _transition->capture(previous);
    }
    bool hasPage = false;
    for (int index = 0; index < _pages.size(); ++index)
    {
        QWidget* page = _pages.at(index).data();
        if (page == nullptr)
        {
            continue;
        }
        const bool isCurrent = index == current;
        if (!isCurrent)
        {
            ShadStyleHelper::clearFocusWithin(page);
        }
        page->setVisible(isCurrent);
        hasPage = hasPage || isCurrent;
    }
    _pageArea->setVisible(hasPage);
    if (isAnimated)
    {
        _transition->play(next, _pages.indexOf(next) >= _pages.indexOf(previous) ? 1 : -1);
    }
}
