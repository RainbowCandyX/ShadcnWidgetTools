#include "ShadTabs.h"

#include <QBoxLayout>
#include <QPointer>
#include <QVBoxLayout>
#include <QWidget>

#include "ShadMaxWidthBoxLayout.h"
#include "ShadPageTransition.h"
#include "ShadStyleHelper.h"
#include "ShadTabsList.h"
#include "ShadTabsPrivate.h"

ShadTabs::ShadTabs(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTabsPrivate())
{
    Q_D(ShadTabs);
    d->q_ptr = this;
    // 标签页设置固定或最大宽度时，页面中自动换行的文字按标签页的实际宽度计算高度
    d->_layout = new ShadMaxWidthBoxLayout(QBoxLayout::TopToBottom, this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_list = new ShadTabsList(this);
    d->_pageArea = new QWidget(this);
    d->_pageLayout = new QVBoxLayout(d->_pageArea);
    d->_pageLayout->setContentsMargins(0, 0, 0, 0);
    d->_pageLayout->setSpacing(0);
    d->_pageArea->hide();
    d->_transition = new ShadPageTransition(d->_pageArea);
    d->_layout->addWidget(d->_list);
    d->_layout->addWidget(d->_pageArea, 1);
    setFocusProxy(d->_list);
    connect(d->_list, &ShadTabsList::currentChanged, this, [this, d](int index)
    {
        d->updatePages();
        Q_EMIT currentChanged(index);
    });
    d->updateLayout();
}

ShadTabs::~ShadTabs()
{
}

int ShadTabs::addTab(const QString& text, QWidget* page)
{
    return insertTab(-1, ShadIconType::None, text, page);
}

int ShadTabs::addTab(ShadIconType::IconName icon, const QString& text, QWidget* page)
{
    return insertTab(-1, icon, text, page);
}

int ShadTabs::insertTab(int index, ShadIconType::IconName icon, const QString& text, QWidget* page)
{
    Q_D(ShadTabs);
    const int position = index < 0 || index > count() ? count() : index;
    if (page != nullptr)
    {
        page->setParent(d->_pageArea);
        page->hide();
        d->_pageLayout->addWidget(page);
    }
    // 先记录页面再添加标签：第一个标签会立即成为当前标签并发出 currentChanged
    d->_pages.insert(position, page);
    const int result = d->_list->insertTab(position, icon, text);
    d->updatePages();
    return result;
}

void ShadTabs::removeTab(int index)
{
    Q_D(ShadTabs);
    if (index < 0 || index >= count())
    {
        return;
    }
    d->_transition->finish();
    const QPointer<QWidget> page = d->_pages.takeAt(index);
    if (!page.isNull())
    {
        d->_pageLayout->removeWidget(page);
        ShadStyleHelper::clearFocusWithin(page);
        page->hide();
        page->setParent(nullptr);
    }
    d->_list->removeTab(index);
    d->updatePages();
}

int ShadTabs::count() const
{
    Q_D(const ShadTabs);
    return d->_list->count();
}

QWidget* ShadTabs::widget(int index) const
{
    Q_D(const ShadTabs);
    if (index < 0 || index >= d->_pages.size())
    {
        return nullptr;
    }
    return d->_pages.at(index).data();
}

int ShadTabs::indexOf(QWidget* page) const
{
    Q_D(const ShadTabs);
    if (page == nullptr)
    {
        return -1;
    }
    for (int index = 0; index < d->_pages.size(); ++index)
    {
        if (d->_pages.at(index).data() == page)
        {
            return index;
        }
    }
    return -1;
}

void ShadTabs::setTabText(int index, const QString& text)
{
    Q_D(ShadTabs);
    d->_list->setTabText(index, text);
}

QString ShadTabs::tabText(int index) const
{
    Q_D(const ShadTabs);
    return d->_list->tabText(index);
}

void ShadTabs::setTabIcon(int index, ShadIconType::IconName icon)
{
    Q_D(ShadTabs);
    d->_list->setTabIcon(index, icon);
}

ShadIconType::IconName ShadTabs::tabIcon(int index) const
{
    Q_D(const ShadTabs);
    return d->_list->tabIcon(index);
}

void ShadTabs::setTabEnabled(int index, bool enabled)
{
    Q_D(ShadTabs);
    d->_list->setTabEnabled(index, enabled);
}

bool ShadTabs::isTabEnabled(int index) const
{
    Q_D(const ShadTabs);
    return d->_list->isTabEnabled(index);
}

void ShadTabs::setCurrentIndex(int index)
{
    Q_D(ShadTabs);
    d->_list->setCurrentIndex(index);
}

int ShadTabs::currentIndex() const
{
    Q_D(const ShadTabs);
    return d->_list->currentIndex();
}

void ShadTabs::setVariant(ShadTabsType::Variant variant)
{
    Q_D(ShadTabs);
    d->_list->setVariant(variant);
    d->updateLayout();
}

ShadTabsType::Variant ShadTabs::getVariant() const
{
    Q_D(const ShadTabs);
    return d->_list->getVariant();
}

void ShadTabs::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadTabs);
    d->_list->setOrientation(orientation);
    d->updateLayout();
}

Qt::Orientation ShadTabs::getOrientation() const
{
    Q_D(const ShadTabs);
    return d->_list->getOrientation();
}

void ShadTabs::setActivateOnFocus(bool activateOnFocus)
{
    Q_D(ShadTabs);
    d->_list->setActivateOnFocus(activateOnFocus);
}

bool ShadTabs::isActivateOnFocus() const
{
    Q_D(const ShadTabs);
    return d->_list->isActivateOnFocus();
}
