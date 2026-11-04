#include "ShadTabView.h"

#include <QChildEvent>
#include <QEvent>
#include <QKeySequence>
#include <QObject>
#include <QPoint>
#include <QPointer>
#include <QShortcut>
#include <QStackedLayout>
#include <QString>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include "ShadPageTransition.h"
#include "ShadSeparator.h"
#include "ShadStyleHelper.h"
#include "ShadTabViewBar.h"
#include "ShadTabViewPrivate.h"

ShadTabView::ShadTabView(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadTabViewPrivate())
{
    Q_D(ShadTabView);
    d->q_ptr = this;
    d->_layout = new QVBoxLayout(this);
    d->_layout->setContentsMargins(0, 0, 0, 0);
    d->_layout->setSpacing(0);
    d->_bar = new ShadTabViewBar(this);
    d->_separator = new ShadSeparator(Qt::Horizontal, this);
    d->_pageArea = new QWidget(this);
    d->_pageStack = new QStackedLayout(d->_pageArea);
    d->_pageStack->setContentsMargins(0, 0, 0, 0);
    d->_transition = new ShadPageTransition(d->_pageArea);
    d->_layout->addWidget(d->_bar);
    d->_layout->addWidget(d->_separator);
    d->_layout->addWidget(d->_pageArea, 1);

    connect(d->_bar, &ShadTabViewBar::currentChanged, this, [this, d](int index)
    {
        d->updatePages();
        Q_EMIT currentChanged(index);
    });
    connect(d->_bar, &ShadTabViewBar::tabMoved, this, [this, d](int from, int to)
    {
        d->_pages.move(from, to);
        Q_EMIT tabMoved(from, to);
    });
    connect(d->_bar, &ShadTabViewBar::tabCloseRequested, this, &ShadTabView::tabCloseRequested);
    connect(d->_bar, &ShadTabViewBar::newTabRequested, this, &ShadTabView::newTabRequested);
    connect(d->_bar, &ShadTabViewBar::tabDragOutRequested, this, [d](int index, const QPoint& grabOffset) { d->startDrag(index, grabOffset); });
    // 放进标题栏后仍由本组件处理拖入
    d->_bar->installEventFilter(this);

    QShortcut* nextShortcut = new QShortcut(this);
    nextShortcut->setKeys(QKeySequence::NextChild);
    nextShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(nextShortcut, &QShortcut::activated, this, [d]() { d->stepCurrent(1); });
    QShortcut* previousShortcut = new QShortcut(this);
    previousShortcut->setKeys(QKeySequence::PreviousChild);
    previousShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(previousShortcut, &QShortcut::activated, this, [d]() { d->stepCurrent(-1); });
    QShortcut* closeShortcut = new QShortcut(this);
    closeShortcut->setKeys(QKeySequence::Close);
    closeShortcut->setContext(Qt::WidgetWithChildrenShortcut);
    connect(closeShortcut, &QShortcut::activated, this, [this]()
    {
        const int index = currentIndex();
        const ShadTabViewType::TabFlags flags = tabFlags(index);
        if (index >= 0 && isTabsClosable() && !flags.testFlag(ShadTabViewType::TabNotClosable) && !flags.testFlag(ShadTabViewType::TabDisabled))
        {
            Q_EMIT tabCloseRequested(index);
        }
    });
}

ShadTabView::~ShadTabView()
{
    Q_D(ShadTabView);
    // 放进标题栏的标签栏不会随本组件释放
    if (!d->_bar.isNull() && d->_bar->parent() != this)
    {
        delete d->_bar.data();
    }
}

int ShadTabView::addTab(const QString& text, QWidget* page)
{
    return insertTab(-1, ShadIconType::None, text, page);
}

int ShadTabView::addTab(ShadIconType::IconName icon, const QString& text, QWidget* page)
{
    return insertTab(-1, icon, text, page);
}

int ShadTabView::insertTab(int index, ShadIconType::IconName icon, const QString& text, QWidget* page)
{
    Q_D(ShadTabView);
    if (page == nullptr || d->_bar.isNull())
    {
        return -1;
    }
    const int position = index < 0 || index > count() ? count() : index;
    // 先记录页面，添加标签时会发出 currentChanged
    d->_pageStack->addWidget(page);
    d->_pages.insert(position, page);
    const int result = d->_bar->insertTab(position, icon, text);
    d->updatePages();
    return result;
}

void ShadTabView::removeTab(int index)
{
    Q_D(ShadTabView);
    if (index < 0 || index >= count())
    {
        return;
    }
    d->_transition->finish();
    const QPointer<QWidget> page = d->_pages.takeAt(index);
    if (!page.isNull())
    {
        ShadStyleHelper::clearFocusWithin(page);
        d->_pageStack->removeWidget(page);
        page->hide();
        page->setParent(nullptr);
    }
    d->_bar->removeTab(index);
    d->updatePages();
}

void ShadTabView::moveTab(int from, int to)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->moveTab(from, to);
    }
}

int ShadTabView::count() const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? 0 : d->_bar->count();
}

QWidget* ShadTabView::widget(int index) const
{
    Q_D(const ShadTabView);
    if (index < 0 || index >= d->_pages.size())
    {
        return nullptr;
    }
    return d->_pages.at(index).data();
}

int ShadTabView::indexOf(QWidget* page) const
{
    Q_D(const ShadTabView);
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

void ShadTabView::setTabText(int index, const QString& text)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setTabText(index, text);
    }
}

QString ShadTabView::tabText(int index) const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? QString() : d->_bar->tabText(index);
}

void ShadTabView::setTabIcon(int index, ShadIconType::IconName icon)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setTabIcon(index, icon);
    }
}

ShadIconType::IconName ShadTabView::tabIcon(int index) const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? ShadIconType::None : d->_bar->tabIcon(index);
}

void ShadTabView::setTabFlags(int index, ShadTabViewType::TabFlags flags)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setTabFlags(index, flags);
    }
}

ShadTabViewType::TabFlags ShadTabView::tabFlags(int index) const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? ShadTabViewType::TabFlags(ShadTabViewType::NoTabFlag) : d->_bar->tabFlags(index);
}

void ShadTabView::setTabFlag(int index, ShadTabViewType::TabFlag flag, bool on)
{
    ShadTabViewType::TabFlags flags = tabFlags(index);
    flags.setFlag(flag, on);
    setTabFlags(index, flags);
}

void ShadTabView::setTabToolTip(int index, const QString& toolTip)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setTabToolTip(index, toolTip);
    }
}

QString ShadTabView::tabToolTip(int index) const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? QString() : d->_bar->tabToolTip(index);
}

void ShadTabView::setTabData(int index, const QVariant& data)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setTabData(index, data);
    }
}

QVariant ShadTabView::tabData(int index) const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? QVariant() : d->_bar->tabData(index);
}

void ShadTabView::setCurrentIndex(int index)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setCurrentIndex(index);
    }
}

int ShadTabView::currentIndex() const
{
    Q_D(const ShadTabView);
    return d->_bar.isNull() ? -1 : d->_bar->currentIndex();
}

void ShadTabView::setTabsClosable(bool closable)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setTabsClosable(closable);
    }
}

bool ShadTabView::isTabsClosable() const
{
    Q_D(const ShadTabView);
    return !d->_bar.isNull() && d->_bar->isTabsClosable();
}

void ShadTabView::setAddButtonVisible(bool visible)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull())
    {
        d->_bar->setAddButtonVisible(visible);
    }
}

bool ShadTabView::isAddButtonVisible() const
{
    Q_D(const ShadTabView);
    return !d->_bar.isNull() && d->_bar->isAddButtonVisible();
}

QWidget* ShadTabView::tabBar() const
{
    Q_D(const ShadTabView);
    return d->_bar.data();
}

void ShadTabView::setDragGroup(const QString& group)
{
    Q_D(ShadTabView);
    d->_dragGroup = group;
    if (!d->_bar.isNull())
    {
        d->_bar->setDetachEnabled(!group.isEmpty());
        d->_bar->setAcceptDrops(!group.isEmpty());
    }
}

QString ShadTabView::getDragGroup() const
{
    Q_D(const ShadTabView);
    return d->_dragGroup;
}

void ShadTabView::setDetachHandler(const DetachHandler& handler)
{
    Q_D(ShadTabView);
    d->_detachHandler = handler;
}

bool ShadTabView::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadTabView);
    if (!d->_bar.isNull() && watched == d->_bar.data() && d->handleDragEvent(event))
    {
        return true;
    }
    return QWidget::eventFilter(watched, event);
}

void ShadTabView::childEvent(QChildEvent* event)
{
    QWidget::childEvent(event);
    Q_D(ShadTabView);
    if (d->_bar.isNull() || event->child() != d->_bar.data())
    {
        return;
    }
    // ChildRemoved 时父组件尚未改变，只能按事件类型判断
    if (event->type() == QEvent::ChildAdded)
    {
        d->_layout->insertWidget(0, d->_bar);
        d->_separator->setVisible(true);
    }
    else if (event->type() == QEvent::ChildRemoved)
    {
        d->_separator->setVisible(false);
    }
}
