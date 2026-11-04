#include "ShadCommand.h"

#include <QAction>
#include <QEvent>
#include <QFont>
#include <QKeyEvent>
#include <QLineEdit>
#include <QList>
#include <QPaintEvent>
#include <QPainter>
#include <QRect>
#include <QRectF>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include <utility>

#include "ShadCommandList.h"
#include "ShadCommandPrivate.h"
#include "ShadIcon.h"
#include "ShadScrollArea.h"
#include "ShadScrollBar.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"

namespace
{
    // 占位提示与空结果文字使用默认文字，使用者改过的保持不变
    void applyDefaultTexts(ShadCommand* command)
    {
        ShadStyleHelper::applyDefaultText(command, "placeholderText", command->getPlaceholderText(), ShadCommand::tr("输入命令或搜索…"), [command](const QString& text) { command->setPlaceholderText(text); });
        ShadStyleHelper::applyDefaultText(command, "emptyText", command->getEmptyText(), ShadCommand::tr("没有找到结果。"), [command](const QString& text) { command->setEmptyText(text); });
    }
} // namespace

ShadCommand::ShadCommand(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCommandPrivate())
{
    Q_D(ShadCommand);
    d->q_ptr = this;
    setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Fixed);
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);

    d->_searchEdit = new QLineEdit(this);
    d->_searchEdit->setFrame(false);
#ifdef Q_OS_MACOS
    d->_searchEdit->setAttribute(Qt::WA_MacShowFocusRect, false);
#endif
    ShadStyleHelper::applyFont(d->_searchEdit, 14, QFont::Normal);
    d->_searchEdit->installEventFilter(this);
    setFocusProxy(d->_searchEdit);

    d->_scrollArea = new ShadScrollArea(this);
    d->_scrollArea->setFocusPolicy(Qt::NoFocus);
    d->_scrollArea->getOverlayScrollBar(Qt::Vertical)->setTrackInset(4);
    d->_list = new ShadCommandList();
    d->_scrollArea->setWidget(d->_list);

    connect(d->_searchEdit, &QLineEdit::textChanged, this, [d]() { d->rebuildRows(); });
    connect(d->_list, &ShadCommandList::itemClicked, this, [d](int index)
    {
        d->_list->setHighlightIndex(index);
        d->activateHighlighted();
    });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this, d]()
    {
        d->applyThemeColors();
        update();
    });
    d->applyThemeColors();
    // setEmptyText 会调用 rebuildRows
    applyDefaultTexts(this);
}

ShadCommand::~ShadCommand()
{
}

int ShadCommand::addGroup(const QString& heading)
{
    Q_D(ShadCommand);
    int groupIndex = 0;
    for (const ShadCommandPrivate::Entry& existing : std::as_const(d->_entries))
    {
        if (existing.kind == ShadCommandPrivate::EntryKind::Group)
        {
            ++groupIndex;
        }
    }
    ShadCommandPrivate::Entry entry;
    entry.kind = ShadCommandPrivate::EntryKind::Group;
    entry.heading = heading;
    d->_entries.append(entry);
    d->rebuildRows();
    return groupIndex;
}

void ShadCommand::setGroupHeading(int groupIndex, const QString& heading)
{
    Q_D(ShadCommand);
    int current = 0;
    for (ShadCommandPrivate::Entry& entry : d->_entries)
    {
        if (entry.kind != ShadCommandPrivate::EntryKind::Group)
        {
            continue;
        }
        if (current == groupIndex)
        {
            entry.heading = heading;
            // 与菜单项文字变化（QAction::changed）的处理一致，按当前搜索重建行，高亮项不变
            d->rebuildRows();
            return;
        }
        ++current;
    }
}

QAction* ShadCommand::addItem(const QString& text, ShadIconType::IconName icon, const QString& shortcutText)
{
    Q_D(ShadCommand);
    QAction* action = new QAction(text, this);
    ShadCommandPrivate::Entry entry;
    entry.kind = ShadCommandPrivate::EntryKind::Item;
    entry.action = action;
    entry.icon = icon;
    entry.shortcutText = shortcutText;
    d->_entries.append(entry);
    connect(action, &QAction::changed, this, [d]() { d->rebuildRows(); });
    d->rebuildRows();
    return action;
}

void ShadCommand::setItemKeywords(QAction* action, const QStringList& keywords)
{
    Q_D(ShadCommand);
    for (ShadCommandPrivate::Entry& entry : d->_entries)
    {
        if (entry.action == action)
        {
            entry.keywords = keywords;
            d->rebuildRows();
            return;
        }
    }
}

void ShadCommand::addSeparator()
{
    Q_D(ShadCommand);
    ShadCommandPrivate::Entry entry;
    entry.kind = ShadCommandPrivate::EntryKind::Separator;
    d->_entries.append(entry);
    d->rebuildRows();
}

void ShadCommand::clear()
{
    Q_D(ShadCommand);
    const QList<ShadCommandPrivate::Entry> entries = d->_entries;
    d->_entries.clear();
    for (const ShadCommandPrivate::Entry& entry : entries)
    {
        delete entry.action;
    }
    d->rebuildRows();
}

void ShadCommand::setPlaceholderText(const QString& text)
{
    Q_D(ShadCommand);
    d->_searchEdit->setPlaceholderText(text);
    updateGeometry();
}

QString ShadCommand::getPlaceholderText() const
{
    Q_D(const ShadCommand);
    return d->_searchEdit->placeholderText();
}

void ShadCommand::setEmptyText(const QString& text)
{
    Q_D(ShadCommand);
    d->_emptyText = text;
    d->_list->setEmptyText(text);
    d->rebuildRows();
}

QString ShadCommand::getEmptyText() const
{
    Q_D(const ShadCommand);
    return d->_emptyText;
}

void ShadCommand::setSearchText(const QString& text)
{
    Q_D(ShadCommand);
    d->_searchEdit->setText(text);
}

QString ShadCommand::getSearchText() const
{
    Q_D(const ShadCommand);
    return d->_searchEdit->text();
}

QSize ShadCommand::sizeHint() const
{
    Q_D(const ShadCommand);
    const int margin = d->chromeMargin();
    const int editLeft = ShadCommandPrivate::kSearchPaddingX + d->searchIconSize() + ShadCommandPrivate::kSearchIconGap;
    const int searchWidth = editLeft + ShadStyleHelper::textWidth(d->_searchEdit->font(), d->_searchEdit->placeholderText()) + ShadCommandPrivate::kSearchPaddingX;
    const int width = qMax(searchWidth, d->_list->sizeHint().width()) + 2 * margin;
    return QSize(width, 2 * margin + d->searchHeight() + 1 + d->listHeight());
}

QSize ShadCommand::minimumSizeHint() const
{
    Q_D(const ShadCommand);
    const int margin = d->chromeMargin();
    const int editLeft = ShadCommandPrivate::kSearchPaddingX + d->searchIconSize() + ShadCommandPrivate::kSearchIconGap;
    // 至少显示一项
    const int listHeight = qMin(d->listHeight(), d->_list->itemHeight() + 8);
    return QSize(editLeft + ShadCommandPrivate::kSearchPaddingX + 2 * margin, 2 * margin + d->searchHeight() + 1 + listHeight);
}

bool ShadCommand::event(QEvent* event)
{
    Q_D(ShadCommand);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutChildren();
        update();
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        applyDefaultTexts(this);
    }
    return QWidget::event(event);
}

bool ShadCommand::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadCommand);
    if (watched == d->_searchEdit && event->type() == QEvent::KeyPress && d->handleNavigationKey(static_cast<QKeyEvent*>(event)))
    {
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ShadCommand::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadCommand);
    QWidget::resizeEvent(event);
    d->layoutChildren();
}

void ShadCommand::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadCommand);
    using namespace ShadStyleHelper;
    QPainter painter(this);
    painter.setRenderHints(QPainter::Antialiasing | QPainter::TextAntialiasing);
    if (!d->_isDialogStyle)
    {
        // rounded-lg border bg-popover shadow-md
        const QRectF card = insetRect(rect(), kFocusRingWidth);
        const qreal radius = shadTheme->getRadius(ShadThemeType::Large);
        drawShadowMd(&painter, card, radius);
        fillRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Popover));
        strokeRoundedRect(&painter, card, radius, shadTheme->getColor(ShadThemeType::Border));
    }
    const int margin = d->chromeMargin();
    const QRect inner = rect().adjusted(margin, margin, -margin, -margin);
    const int iconSize = d->searchIconSize();
    // 搜索图标 opacity-50，搜索栏 border-b
    const QRectF iconRect(inner.left() + ShadCommandPrivate::kSearchPaddingX, inner.top() + (d->searchHeight() - iconSize) / 2.0, iconSize, iconSize);
    ShadIcon::paint(&painter, visualRect(layoutDirection(), QRectF(inner), iconRect), ShadIconType::Search, withAlpha(shadTheme->getColor(ShadThemeType::PopoverForeground), 0.5), iconSize);
    painter.fillRect(QRect(inner.left(), inner.top() + d->searchHeight(), inner.width(), 1), shadTheme->getColor(ShadThemeType::Border));
}
