#include "ShadToaster.h"

#include <QEnterEvent>
#include <QEvent>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPaintEvent>
#include <QPainter>
#include <QRectF>
#include <QString>
#include <QWidget>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadToastItem.h"
#include "ShadToasterPrivate.h"

ShadToaster::ShadToaster(QWidget* host)
    : QWidget(host), d_ptr(new ShadToasterPrivate())
{
    Q_D(ShadToaster);
    d->q_ptr = this;
    d->_host = host;
    hide();
    if (host != nullptr)
    {
        // 跟随宿主缩放重新定位，并在宿主新增子组件后保持最上层
        host->installEventFilter(this);
    }
    else
    {
        qWarning("ShadToaster 的宿主窗口为空，toast 将无法显示");
    }
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { update(); });
}

ShadToaster::~ShadToaster()
{
}

ShadToaster* ShadToaster::forWindow(QWidget* widget)
{
    if (widget == nullptr)
    {
        qWarning("ShadToaster::forWindow 传入的组件为空");
        return nullptr;
    }
    QWidget* window = widget->window();
    if (ShadToaster * toaster = window->findChild<ShadToaster*>(QString(), Qt::FindDirectChildrenOnly))
    {
        return toaster;
    }
    return new ShadToaster(window);
}

void ShadToaster::setPosition(ShadToastType::Position position)
{
    Q_D(ShadToaster);
    if (d->_position == position)
    {
        return;
    }
    d->_position = position;
    d->relayout(true);
}

ShadToastType::Position ShadToaster::getPosition() const
{
    Q_D(const ShadToaster);
    return d->_position;
}

void ShadToaster::setLimit(int limit)
{
    Q_D(ShadToaster);
    d->_limit = qMax(0, limit);
    while (d->_items.size() > d->effectiveLimit())
    {
        d->beginDismiss(d->_items.last());
    }
    d->relayout(true);
}

int ShadToaster::getLimit() const
{
    Q_D(const ShadToaster);
    return d->_limit;
}

void ShadToaster::setDuration(int duration)
{
    Q_D(ShadToaster);
    // 只影响之后新增或更新的 toast
    d->_duration = qMax(0, duration);
}

int ShadToaster::getDuration() const
{
    Q_D(const ShadToaster);
    return d->_duration;
}

int ShadToaster::add(const QString& title, const QString& description, ShadToastType::Type type)
{
    ShadToastOptions options;
    options.title = title;
    options.description = description;
    options.type = type;
    return add(options);
}

int ShadToaster::add(const ShadToastOptions& options)
{
    Q_D(ShadToaster);
    ShadToastItem* item = new ShadToastItem(d->_nextId++, this);
    item->hide();
    connect(item, &ShadToastItem::dismissRequested, this, [this](int id) { dismiss(id); });
    connect(item, &ShadToastItem::actionTriggered, this, &ShadToaster::actionTriggered);
    connect(item, &ShadToastItem::exitFinished, this, [d, item]() { d->onExitFinished(item); });
    connect(item, &ShadToastItem::geometryAnimating, this, [this, d]()
    {
        d->updateMask();
        update();
    });
    item->setOptions(options, d->_duration);
    d->_items.prepend(item);
    show();
    raise();
    // 超出数量限制时关闭最旧的
    while (d->_items.size() > d->effectiveLimit())
    {
        d->beginDismiss(d->_items.last());
    }
    d->relayout(true);
    return item->toastId();
}

bool ShadToaster::updateToast(int id, const ShadToastOptions& options)
{
    Q_D(ShadToaster);
    ShadToastItem* item = d->findItem(id);
    if (item == nullptr)
    {
        return false;
    }
    item->setOptions(options, d->_duration);
    d->relayout(true);
    return true;
}

void ShadToaster::dismiss(int id)
{
    Q_D(ShadToaster);
    d->beginDismiss(d->findItem(id));
}

void ShadToaster::dismissAll()
{
    Q_D(ShadToaster);
    const QList<ShadToastItem*> items = d->_items;
    for (ShadToastItem* item : items)
    {
        d->beginDismiss(item);
    }
}

bool ShadToaster::contains(int id) const
{
    Q_D(const ShadToaster);
    return d->findItem(id) != nullptr;
}

int ShadToaster::count() const
{
    Q_D(const ShadToaster);
    return static_cast<int>(d->_items.size());
}

bool ShadToaster::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadToaster);
    if (watched == d->_host.data())
    {
        if (event->type() == QEvent::Resize)
        {
            d->relayout(false);
        }
        else if (event->type() == QEvent::LayoutDirectionChange)
        {
            // 布局方向切换后左右位置互换，从原处动画移动过去
            d->relayout(true);
        }
        else if (event->type() == QEvent::ChildAdded)
        {
            // 新子组件会叠在上面，等它加入后再把自己提到最上层
            QMetaObject::invokeMethod(this, [this]() { raise(); }, Qt::QueuedConnection);
        }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadToaster::enterEvent(QEnterEvent* event)
{
    Q_D(ShadToaster);
    d->setExpanded(true);
    QWidget::enterEvent(event);
}

void ShadToaster::leaveEvent(QEvent* event)
{
    Q_D(ShadToaster);
    d->setExpanded(false);
    QWidget::leaveEvent(event);
}

void ShadToaster::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    Q_D(ShadToaster);
    // 卡片阴影统一画在所有 toast 下层（shadow-lg）
    QPainter painter(this);
    const qreal radius = ShadToastItem::cardRadius();
    QList<ShadToastItem*> items = d->_exitingItems;
    items.append(d->_items);
    for (ShadToastItem* item : items)
    {
        if (item->isHidden())
        {
            continue;
        }
        const QRectF cardRect(item->geometry());
        // 阴影与卡片同步淡出
        painter.setOpacity(item->opacity());
        ShadStyleHelper::drawShadowLg(&painter, cardRect, qMin(radius, cardRect.height() / 2.0));
    }
}
