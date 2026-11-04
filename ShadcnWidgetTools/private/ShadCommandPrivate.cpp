#include "ShadCommandPrivate.h"

#include <QAction>
#include <QChar>
#include <QColor>
#include <QKeyEvent>
#include <QLineEdit>
#include <QList>
#include <QPalette>
#include <QRect>
#include <QScrollBar>
#include <QSizePolicy>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <algorithm>
#include <utility>

#include "ShadComboBoxSearch.h"
#include "ShadCommand.h"
#include "ShadCommandList.h"
#include "ShadScrollArea.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 去掉助记符 &，&& 还原为 &
    QString displayText(const QAction* action)
    {
        const QString text = action->text();
        QString result;
        result.reserve(text.size());
        for (int index = 0; index < int(text.size()); ++index)
        {
            if (text.at(index) == QLatin1Char('&') && index + 1 < int(text.size()))
            {
                ++index;
            }
            result.append(text.at(index));
        }
        return result;
    }

    struct Block
    {
        bool isSeparator{false};
        QString heading;
        QList<int> entryIndexes;
    };

    using ScoredRows = std::pair<int, QList<ShadCommandRow>>;
} // namespace

ShadCommandPrivate::ShadCommandPrivate()
{
}

ShadCommandPrivate::~ShadCommandPrivate()
{
}

int ShadCommandPrivate::chromeMargin() const
{
    return _isDialogStyle ? 0 : ShadStyleHelper::kFocusRingWidth + kBorderWidth;
}

int ShadCommandPrivate::searchHeight() const
{
    return _isDialogStyle ? kLargeSearchHeight : kSearchHeight;
}

int ShadCommandPrivate::searchIconSize() const
{
    return _isDialogStyle ? kLargeSearchIconSize : kSearchIconSize;
}

int ShadCommandPrivate::listHeight() const
{
    return qMin(_list->sizeHint().height(), kListMaxHeight);
}

void ShadCommandPrivate::rebuildRows()
{
    Q_Q(ShadCommand);
    QList<Block> blocks;
    int groupIndex = -1;
    for (int index = 0; index < int(_entries.size()); ++index)
    {
        const Entry& entry = _entries.at(index);
        switch (entry.kind)
        {
            case EntryKind::Group:
                {
                    Block block;
                    block.heading = entry.heading;
                    blocks.append(block);
                    groupIndex = int(blocks.size()) - 1;
                    break;
                }
            case EntryKind::Separator:
                {
                    Block block;
                    block.isSeparator = true;
                    blocks.append(block);
                    groupIndex = -1;
                    break;
                }
            case EntryKind::Item:
                {
                    if (groupIndex < 0)
                    {
                        blocks.append(Block());
                        groupIndex = int(blocks.size()) - 1;
                    }
                    blocks[groupIndex].entryIndexes.append(index);
                    break;
                }
        }
    }

    const QString query = _searchEdit->text().trimmed();
    const auto itemRow = [this](int entryIndex)
    {
        const Entry& entry = _entries.at(entryIndex);
        ShadCommandRow row;
        row.kind = ShadCommandRow::Kind::Item;
        row.text = displayText(entry.action);
        row.action = entry.action;
        row.icon = entry.icon;
        row.shortcutText = entry.shortcutText;
        return row;
    };
    // 搜索时隐藏分隔线并按相关度排序
    QList<ScoredRows> groups;
    for (const Block& block : std::as_const(blocks))
    {
        if (block.isSeparator)
        {
            if (query.isEmpty())
            {
                ShadCommandRow row;
                row.kind = ShadCommandRow::Kind::Separator;
                groups.append(ScoredRows(0, {row}));
            }
            continue;
        }
        QList<std::pair<int, ShadCommandRow>> items;
        for (int entryIndex : block.entryIndexes)
        {
            const Entry& entry = _entries.at(entryIndex);
            if (!entry.action->isVisible())
            {
                continue;
            }
            int score = 1;
            if (!query.isEmpty())
            {
                score = ShadComboBoxSearch::fuzzyScore(displayText(entry.action), query);
                for (const QString& keyword : entry.keywords)
                {
                    score = qMax(score, ShadComboBoxSearch::fuzzyScore(keyword, query));
                }
            }
            if (score > 0)
            {
                items.append(std::make_pair(score, itemRow(entryIndex)));
            }
        }
        if (items.isEmpty())
        {
            continue;
        }
        if (!query.isEmpty())
        {
            std::stable_sort(items.begin(), items.end(), [](const std::pair<int, ShadCommandRow>& left, const std::pair<int, ShadCommandRow>& right) { return left.first > right.first; });
        }
        QList<ShadCommandRow> rows;
        ShadCommandRow top;
        top.kind = ShadCommandRow::Kind::GroupTop;
        rows.append(top);
        if (!block.heading.isEmpty())
        {
            ShadCommandRow heading;
            heading.kind = ShadCommandRow::Kind::Heading;
            heading.text = block.heading;
            rows.append(heading);
        }
        for (const std::pair<int, ShadCommandRow>& item : std::as_const(items))
        {
            rows.append(item.second);
        }
        ShadCommandRow bottom;
        bottom.kind = ShadCommandRow::Kind::GroupBottom;
        rows.append(bottom);
        groups.append(ScoredRows(items.first().first, rows));
    }
    if (!query.isEmpty())
    {
        std::stable_sort(groups.begin(), groups.end(), [](const ScoredRows& left, const ScoredRows& right) { return left.first > right.first; });
    }

    QList<ShadCommandRow> rows;
    bool isFirstGroup = true;
    for (ScoredRows& group : groups)
    {
        if (group.second.first().kind == ShadCommandRow::Kind::GroupTop)
        {
            group.second.first().isFirstGroup = isFirstGroup;
            isFirstGroup = false;
        }
        rows.append(group.second);
    }
    _list->setRows(rows);
    _list->highlightFirst();
    q->updateGeometry();
    layoutChildren();
    // 回到顶部
    _scrollArea->verticalScrollBar()->setValue(0);
}

void ShadCommandPrivate::layoutChildren()
{
    Q_Q(ShadCommand);
    const int margin = chromeMargin();
    const QRect inner = q->rect().adjusted(margin, margin, -margin, -margin);
    const int editLeft = kSearchPaddingX + searchIconSize() + kSearchIconGap;
    const QRect editRect(inner.left() + editLeft, inner.top(), qMax(0, inner.width() - editLeft - kSearchPaddingX), searchHeight());
    _searchEdit->setGeometry(ShadStyleHelper::visualRect(q->layoutDirection(), inner, editRect));
    _scrollArea->setGeometry(inner.left(), inner.top() + searchHeight() + 1, inner.width(), qMax(0, inner.height() - searchHeight() - 1));
}

void ShadCommandPrivate::scrollToHighlight()
{
    const int index = _list->getHighlightIndex();
    if (index < 0)
    {
        return;
    }
    const QRect rowRect = _list->rowRect(index);
    _scrollArea->ensureVisible(rowRect.center().x(), rowRect.center().y(), 0, rowRect.height() / 2 + 4);
}

void ShadCommandPrivate::activateHighlighted()
{
    Q_Q(ShadCommand);
    const int index = _list->getHighlightIndex();
    if (!_list->isSelectable(index))
    {
        return;
    }
    QAction* action = _list->getRows().at(index).action;
    Q_EMIT q->itemTriggered(action);
    action->trigger();
}

bool ShadCommandPrivate::handleNavigationKey(QKeyEvent* event)
{
    switch (event->key())
    {
        case Qt::Key_Down:
        case Qt::Key_Up:
            {
                _list->moveHighlight(event->key() == Qt::Key_Down ? 1 : -1);
                break;
            }
        case Qt::Key_PageDown:
        case Qt::Key_PageUp:
            {
                const int pageItems = qMax(1, _scrollArea->viewport()->height() / _list->itemHeight() - 1);
                for (int step = 0; step < pageItems; ++step)
                {
                    _list->moveHighlight(event->key() == Qt::Key_PageDown ? 1 : -1);
                }
                break;
            }
        case Qt::Key_Home:
            {
                _list->highlightFirst();
                break;
            }
        case Qt::Key_End:
            {
                _list->highlightLast();
                break;
            }
        case Qt::Key_Return:
        case Qt::Key_Enter:
            {
                activateHighlighted();
                event->accept();
                return true;
            }
        default:
            {
                return false;
            }
    }
    scrollToHighlight();
    event->accept();
    return true;
}

void ShadCommandPrivate::applyThemeColors()
{
    using ShadStyleHelper::withAlpha;
    const QColor foreground = shadTheme->getColor(ShadThemeType::PopoverForeground);
    const QColor placeholder = shadTheme->getColor(ShadThemeType::MutedForeground);
    const QColor selection = shadTheme->getColor(ShadThemeType::Primary);
    const QColor selectionText = shadTheme->getColor(ShadThemeType::PrimaryForeground);
    const QString styleSheet = QStringLiteral("QLineEdit{background-color:transparent;border:none;padding:0px;color:%1;selection-background-color:%2;selection-color:%3;}").arg(foreground.name(QColor::HexArgb), selection.name(QColor::HexArgb), selectionText.name(QColor::HexArgb));
    _searchEdit->setStyleSheet(styleSheet);
    QPalette palette = _searchEdit->palette();
    palette.setColor(QPalette::Active, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Inactive, QPalette::PlaceholderText, placeholder);
    palette.setColor(QPalette::Disabled, QPalette::PlaceholderText, withAlpha(placeholder, ShadStyleHelper::kDisabledOpacity));
    _searchEdit->setPalette(palette);
}

void ShadCommandPrivate::setDialogStyle(bool isDialogStyle)
{
    Q_Q(ShadCommand);
    _isDialogStyle = isDialogStyle;
    _list->setLarge(isDialogStyle);
    // 对话框中列表随窗口高度伸缩
    q->setSizePolicy(QSizePolicy::Preferred, isDialogStyle ? QSizePolicy::Expanding : QSizePolicy::Fixed);
    q->updateGeometry();
    layoutChildren();
    q->update();
}
