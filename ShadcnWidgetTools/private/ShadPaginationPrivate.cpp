#include "ShadPaginationPrivate.h"

#include <QList>
#include <QObject>
#include <QRect>
#include <QSize>
#include <QString>
#include <QVariant>
#include <QWidget>
#include <QtGlobal>

#include <utility>

#include "ShadButton.h"
#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadInput.h"
#include "ShadLabel.h"
#include "ShadPagination.h"
#include "ShadPaginationNavButton.h"
#include "ShadStyleHelper.h"

namespace
{
    // 页码按钮上记录的页码
    constexpr const char* kPageProperty = "_shadPaginationPage";
    // 页码文字左右各留 8px 仍放不下时改用 default 尺寸（px-4），例如三位数页码
    constexpr int kPageTextPadding = 8;

    // 排列时的一项：组件、是否带焦点环边距、与前一项的可视间距
    struct LayoutEntry
    {
        QWidget* widget{nullptr};
        bool hasRing{true};
        int gapBefore{0};
    };

    // 按显示顺序列出可见的子组件：上一页、页码与省略号、下一页、跳转区
    QList<LayoutEntry> layoutEntries(const ShadPaginationPrivate* d)
    {
        QList<LayoutEntry> entries;
        const auto append = [&entries](QWidget* widget, bool hasRing, int gap) { entries.append({widget, hasRing, entries.isEmpty() ? 0 : gap}); };
        if (d->_isNavigationVisible)
        {
            append(d->_previousButton, true, ShadPaginationPrivate::kItemGap);
        }
        for (QWidget* widget : d->_itemWidgets)
        {
            append(widget, true, ShadPaginationPrivate::kItemGap);
        }
        if (d->_isNavigationVisible)
        {
            append(d->_nextButton, true, ShadPaginationPrivate::kItemGap);
        }
        if (d->_isJumpVisible)
        {
            append(d->_jumpPrefix, false, ShadPaginationPrivate::kJumpGap);
            append(d->_jumpInput, true, ShadPaginationPrivate::kJumpInnerGap);
            append(d->_jumpSuffix, false, ShadPaginationPrivate::kJumpInnerGap);
        }
        return entries;
    }

    // 实际占用尺寸：sizeHint 受最小与最大尺寸约束，例如固定宽度的跳转输入框
    QSize entrySize(const LayoutEntry& entry)
    {
        return entry.widget->sizeHint().expandedTo(entry.widget->minimumSize()).boundedTo(entry.widget->maximumSize());
    }

    int visualWidth(const LayoutEntry& entry)
    {
        const int width = entrySize(entry).width();
        return entry.hasRing ? width - 2 * ShadStyleHelper::kFocusRingWidth : width;
    }

    // 闭区间 [first, last] 的页码，first 大于 last 时为空
    void appendRange(QList<int>& items, int first, int last)
    {
        for (int page = first; page <= last; ++page)
        {
            items.append(page);
        }
    }
} // namespace

ShadPaginationPrivate::ShadPaginationPrivate()
{
}

ShadPaginationPrivate::~ShadPaginationPrivate()
{
}

QList<int> ShadPaginationPrivate::pageItems() const
{
    QList<int> items;
    const int count = _pageCount;
    if (count < 1)
    {
        return items;
    }
    const int page = _currentPage;
    const int boundary = qMax(0, _boundaryCount);
    const int sibling = qMax(0, _siblingCount);
    // 当前页两侧的连续区间：靠近首尾时向另一侧延伸，保持总数不变
    const int siblingsStart = qMax(qMin(page - sibling, count - boundary - sibling * 2 - 1), boundary + 2);
    const int siblingsEnd = qMin(qMax(page + sibling, boundary + sibling * 2 + 2), count - boundary - 1);
    appendRange(items, 1, qMin(boundary, count));
    // 起始侧：间隔大于一页时为省略号，只差一页时直接显示该页
    if (siblingsStart > boundary + 2)
    {
        items.append(kEllipsis);
    }
    else if (boundary + 1 < count - boundary)
    {
        items.append(boundary + 1);
    }
    appendRange(items, siblingsStart, siblingsEnd);
    if (siblingsEnd < count - boundary - 1)
    {
        items.append(kEllipsis);
    }
    else if (count - boundary > boundary)
    {
        items.append(count - boundary);
    }
    appendRange(items, qMax(count - boundary + 1, boundary + 1), count);
    return items;
}

void ShadPaginationPrivate::updateItems()
{
    Q_Q(ShadPagination);
    const QList<int> items = _isPageButtonsVisible ? pageItems() : QList<int>();
    _itemWidgets.clear();
    int pageIndex = 0;
    int ellipsisIndex = 0;
    for (const int item : items)
    {
        if (item == kEllipsis)
        {
            ShadButton* ellipsis = ellipsisAt(ellipsisIndex++);
            ellipsis->show();
            _itemWidgets.append(ellipsis);
            continue;
        }
        ShadButton* button = pageButtonAt(pageIndex++);
        const QString text = QString::number(item);
        button->setText(text);
        button->setProperty(kPageProperty, item);
        button->setAccessibleName(ShadPagination::tr("第 %1 页").arg(item));
        // PaginationLink：当前页 outline，其余 ghost
        button->setVariant(item == _currentPage ? ShadButtonType::Outline : ShadButtonType::Ghost);
        const bool isWideText = ShadStyleHelper::textWidth(button->font(), text) + 2 * kPageTextPadding > kButtonSize;
        button->setButtonSize(isWideText ? ShadButtonType::Medium : ShadButtonType::Icon);
        button->show();
        _itemWidgets.append(button);
    }
    // 多出的按钮隐藏；隐藏有焦点的按钮前清除焦点，避免焦点跳到页面中的其他组件
    for (int index = pageIndex; index < _pageButtons.size(); ++index)
    {
        ShadStyleHelper::clearFocusWithin(_pageButtons.at(index));
        _pageButtons.at(index)->hide();
    }
    for (int index = ellipsisIndex; index < _ellipses.size(); ++index)
    {
        _ellipses.at(index)->hide();
    }

    if (!_isNavigationVisible)
    {
        ShadStyleHelper::clearFocusWithin(_previousButton);
        ShadStyleHelper::clearFocusWithin(_nextButton);
    }
    _previousButton->setVisible(_isNavigationVisible);
    _nextButton->setVisible(_isNavigationVisible);
    ShadStyleHelper::setNavigationButtonsEnabled(_previousButton, _pageCount >= 1 && _currentPage > 1, _nextButton, _pageCount >= 1 && _currentPage < _pageCount);
    if (!_isJumpVisible)
    {
        ShadStyleHelper::clearFocusWithin(_jumpInput);
    }
    _jumpPrefix->setVisible(_isJumpVisible);
    _jumpInput->setVisible(_isJumpVisible);
    _jumpSuffix->setVisible(_isJumpVisible);

    // Tab 顺序与显示顺序一致：页码按钮在需要时才创建，默认排在后面
    QWidget* previous = nullptr;
    for (const LayoutEntry& entry : layoutEntries(this))
    {
        if (entry.widget->focusPolicy() == Qt::NoFocus)
        {
            continue;
        }
        if (previous != nullptr)
        {
            QWidget::setTabOrder(previous, entry.widget);
        }
        previous = entry.widget;
    }
    layoutItems();
    q->updateGeometry();
}

void ShadPaginationPrivate::layoutItems()
{
    Q_Q(ShadPagination);
    const QList<LayoutEntry> entries = layoutEntries(this);
    const int ring = ShadStyleHelper::kFocusRingWidth;
    const int total = contentWidth();
    // 对齐按逻辑方向计算：AlignLeft 为起始侧，AlignRight 为结束侧，之后整体镜像
    int x = (q->width() - total) / 2;
    if (_alignment.testFlag(Qt::AlignLeft))
    {
        x = ring;
    }
    else if (_alignment.testFlag(Qt::AlignRight))
    {
        x = q->width() - ring - total;
    }
    const Qt::LayoutDirection direction = q->layoutDirection();
    const QRect bounds = q->rect();
    for (const LayoutEntry& entry : entries)
    {
        x += entry.gapBefore;
        const QSize size = entrySize(entry);
        const int left = entry.hasRing ? x - ring : x;
        const QRect logicalRect(left, (q->height() - size.height()) / 2, size.width(), size.height());
        entry.widget->setGeometry(ShadStyleHelper::visualRect(direction, bounds, logicalRect));
        x += visualWidth(entry);
    }
}

int ShadPaginationPrivate::contentWidth() const
{
    int total = 0;
    for (const LayoutEntry& entry : layoutEntries(this))
    {
        total += entry.gapBefore + visualWidth(entry);
    }
    return total;
}

void ShadPaginationPrivate::changePage(int page, bool focusActive)
{
    Q_Q(ShadPagination);
    q->setCurrentPage(page);
    if (!focusActive)
    {
        return;
    }
    for (QWidget* widget : std::as_const(_itemWidgets))
    {
        if (widget->property(kPageProperty).toInt() == _currentPage)
        {
            widget->setFocus(Qt::OtherFocusReason);
            return;
        }
    }
}

void ShadPaginationPrivate::jumpToInput()
{
    Q_Q(ShadPagination);
    bool isNumber = false;
    const int page = _jumpInput->text().toInt(&isNumber);
    if (!isNumber || page < 1 || page > _pageCount)
    {
        _jumpInput->setInvalid(!_jumpInput->text().isEmpty());
        return;
    }
    _jumpInput->clear();
    _jumpInput->setInvalid(false);
    q->setCurrentPage(page);
}

void ShadPaginationPrivate::updateJumpInvalid()
{
    const QString text = _jumpInput->text();
    bool isNumber = false;
    const int page = text.toInt(&isNumber);
    _jumpInput->setInvalid(!text.isEmpty() && (!isNumber || page < 1 || page > _pageCount));
}

ShadButton* ShadPaginationPrivate::pageButtonAt(int index)
{
    Q_Q(ShadPagination);
    while (_pageButtons.size() <= index)
    {
        ShadButton* button = new ShadButton(QString(), ShadButtonType::Ghost, q);
        QObject::connect(button, &ShadButton::clicked, q, [this, button]() { changePage(button->property(kPageProperty).toInt(), button->hasFocus()); });
        _pageButtons.append(button);
    }
    return _pageButtons.at(index);
}

ShadButton* ShadPaginationPrivate::ellipsisAt(int index)
{
    Q_Q(ShadPagination);
    while (_ellipses.size() <= index)
    {
        // PaginationEllipsis：36x36 的 MoreHorizontal 图标，不可点击、不可聚焦；借用 ghost 图标按钮的尺寸与图标绘制
        ShadButton* ellipsis = new ShadButton(QString(), ShadButtonType::Ghost, q);
        ellipsis->setButtonSize(ShadButtonType::Icon);
        ellipsis->setShadIcon(ShadIconType::Ellipsis);
        ellipsis->setFocusPolicy(Qt::NoFocus);
        ellipsis->setAttribute(Qt::WA_TransparentForMouseEvents, true);
        _ellipses.append(ellipsis);
    }
    return _ellipses.at(index);
}
