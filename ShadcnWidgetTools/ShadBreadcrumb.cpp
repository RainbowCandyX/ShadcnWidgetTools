#include "ShadBreadcrumb.h"

#include <QEvent>
#include <QResizeEvent>
#include <QSizePolicy>
#include <QString>
#include <QtGlobal>

#include <limits>
#include <utility>

#include "ShadBreadcrumbItem.h"
#include "ShadBreadcrumbPrivate.h"
#include "ShadStyleHelper.h"

ShadBreadcrumb::ShadBreadcrumb(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadBreadcrumbPrivate())
{
    Q_D(ShadBreadcrumb);
    d->q_ptr = this;
    QSizePolicy sizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
    sizePolicy.setHeightForWidth(true);
    setSizePolicy(sizePolicy);
    ShadStyleHelper::applyDefaultText(this, "accessibleName", accessibleName(), tr("面包屑导航"), [this](const QString& text) { setAccessibleName(text); });
}

ShadBreadcrumb::~ShadBreadcrumb()
{
}

int ShadBreadcrumb::addLink(const QString& text)
{
    Q_D(ShadBreadcrumb);
    return d->appendEntry(ShadBreadcrumbPrivate::EntryKind::Link, text, nullptr);
}

int ShadBreadcrumb::addPage(const QString& text)
{
    Q_D(ShadBreadcrumb);
    return d->appendEntry(ShadBreadcrumbPrivate::EntryKind::Page, text, nullptr);
}

int ShadBreadcrumb::addEllipsis(ShadDropdownMenu* menu)
{
    Q_D(ShadBreadcrumb);
    return d->appendEntry(ShadBreadcrumbPrivate::EntryKind::Ellipsis, QString(), menu);
}

int ShadBreadcrumb::addDropdown(const QString& text, ShadDropdownMenu* menu)
{
    Q_D(ShadBreadcrumb);
    return d->appendEntry(ShadBreadcrumbPrivate::EntryKind::Dropdown, text, menu);
}

void ShadBreadcrumb::setPath(const QStringList& texts)
{
    Q_D(ShadBreadcrumb);
    d->_entries.clear();
    for (int index = 0; index < texts.size(); ++index)
    {
        ShadBreadcrumbPrivate::Entry entry;
        entry.kind = index + 1 == texts.size() ? ShadBreadcrumbPrivate::EntryKind::Page : ShadBreadcrumbPrivate::EntryKind::Link;
        entry.text = texts.at(index);
        d->_entries.append(entry);
    }
    d->rebuildItems();
}

void ShadBreadcrumb::clear()
{
    Q_D(ShadBreadcrumb);
    d->_entries.clear();
    d->rebuildItems();
}

int ShadBreadcrumb::count() const
{
    Q_D(const ShadBreadcrumb);
    return int(d->_entries.size());
}

void ShadBreadcrumb::setItemText(int index, const QString& text)
{
    Q_D(ShadBreadcrumb);
    if (index < 0 || index >= d->_entries.size() || d->_entries.at(index).text == text)
    {
        return;
    }
    d->_entries[index].text = text;
    d->rebuildItems();
}

QString ShadBreadcrumb::getItemText(int index) const
{
    Q_D(const ShadBreadcrumb);
    if (index < 0 || index >= d->_entries.size())
    {
        return QString();
    }
    return d->_entries.at(index).text;
}

void ShadBreadcrumb::setSeparatorIcon(ShadIconType::IconName icon)
{
    Q_D(ShadBreadcrumb);
    if (d->_separatorIcon == icon)
    {
        return;
    }
    d->_separatorIcon = icon;
    for (ShadBreadcrumbItem* item : std::as_const(d->_items))
    {
        if (item->kind() == ShadBreadcrumbItem::Kind::Separator)
        {
            item->setShadIcon(icon);
        }
    }
}

ShadIconType::IconName ShadBreadcrumb::getSeparatorIcon() const
{
    Q_D(const ShadBreadcrumb);
    return d->_separatorIcon;
}

void ShadBreadcrumb::setMaximumVisibleItems(int count)
{
    Q_D(ShadBreadcrumb);
    const int value = qMax(0, count);
    if (d->_maximumVisibleItems == value)
    {
        return;
    }
    d->_maximumVisibleItems = value;
    d->rebuildItems();
}

int ShadBreadcrumb::getMaximumVisibleItems() const
{
    Q_D(const ShadBreadcrumb);
    return d->_maximumVisibleItems;
}

void ShadBreadcrumb::setMaximumItemWidth(int width)
{
    Q_D(ShadBreadcrumb);
    const int value = qMax(0, width);
    if (d->_maximumItemWidth == value)
    {
        return;
    }
    d->_maximumItemWidth = value;
    updateGeometry();
    d->layoutItems(this->width(), true);
}

int ShadBreadcrumb::getMaximumItemWidth() const
{
    Q_D(const ShadBreadcrumb);
    return d->_maximumItemWidth;
}

bool ShadBreadcrumb::hasHeightForWidth() const
{
    return true;
}

int ShadBreadcrumb::heightForWidth(int width) const
{
    Q_D(const ShadBreadcrumb);
    return d->layoutItems(width, false);
}

QSize ShadBreadcrumb::sizeHint() const
{
    Q_D(const ShadBreadcrumb);
    // 所有项放在一行时的宽度（文字项受 maximumItemWidth 限制）
    int width = 0;
    for (const ShadBreadcrumbItem* item : d->_items)
    {
        width += d->itemWidth(item, std::numeric_limits<int>::max() / 2);
    }
    width += qMax(0, int(d->_items.size()) - 1) * ShadBreadcrumbPrivate::kItemSpacing;
    return QSize(width, d->layoutItems(width, false));
}

QSize ShadBreadcrumb::minimumSizeHint() const
{
    Q_D(const ShadBreadcrumb);
    // 最窄时每行一项，宽度取各项最小宽度中的最大值；高度由 heightForWidth 决定，这里给出单行高度
    int width = 0;
    int height = 0;
    for (const ShadBreadcrumbItem* item : d->_items)
    {
        const QSize minimum = item->minimumSizeHint();
        width = qMax(width, minimum.width());
        height = qMax(height, minimum.height());
    }
    return QSize(width, height);
}

void ShadBreadcrumb::changeEvent(QEvent* event)
{
    Q_D(ShadBreadcrumb);
    if (event->type() == QEvent::LayoutDirectionChange)
    {
        d->layoutItems(width(), true);
    }
    else if (event->type() == QEvent::FontChange)
    {
        updateGeometry();
        d->layoutItems(width(), true);
    }
    else if (event->type() == QEvent::LanguageChange)
    {
        ShadStyleHelper::applyDefaultText(this, "accessibleName", accessibleName(), tr("面包屑导航"), [this](const QString& text) { setAccessibleName(text); });
        // 重建子组件，刷新折叠按钮的"显示更多"提示
        d->rebuildItems();
    }
    QWidget::changeEvent(event);
}

void ShadBreadcrumb::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadBreadcrumb);
    QWidget::resizeEvent(event);
    d->layoutItems(width(), true);
}
