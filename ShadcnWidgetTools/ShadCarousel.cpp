#include "ShadCarousel.h"

#include <QAbstractButton>
#include <QChildEvent>
#include <QEasingCurve>
#include <QEnterEvent>
#include <QEvent>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QResizeEvent>
#include <QSize>
#include <QString>
#include <QTimer>
#include <QWidget>
#include <QtGlobal>

#include <cmath>
#include <initializer_list>

#include "ShadButton.h"
#include "ShadCarouselPrivate.h"
#include "ShadFrameAnimation.h"

ShadCarousel::ShadCarousel(QWidget* parent)
    : QWidget(parent), d_ptr(new ShadCarouselPrivate())
{
    Q_D(ShadCarousel);
    d->q_ptr = this;
    // 点击幻灯片区域时轮播获得焦点（Qt 从被点击的组件向上查找第一个接受点击聚焦的组件），之后方向键直接交给轮播；
    // 不参与 Tab 顺序，键盘用户通过翻页按钮进入
    setFocusPolicy(Qt::ClickFocus);

    // 视口裁剪超出范围的幻灯片，对应 CarouselContent 的 overflow-hidden
    d->_viewport = new QWidget(this);
    d->installFilter(d->_viewport);

    // 对应 CarouselPrevious / CarouselNext：variant="outline" size="icon-sm" rounded-full
    d->_prevButton = new ShadButton(this);
    d->_nextButton = new ShadButton(this);
    for (ShadButton* button : {d->_prevButton, d->_nextButton})
    {
        button->setVariant(ShadButtonType::Outline);
        button->setButtonSize(ShadButtonType::IconSmall);
        button->setRoundedFull(true);
    }
    d->_prevButton->setAccessibleName(tr("上一张"));
    d->_nextButton->setAccessibleName(tr("下一张"));
    // 只用于拦截方向键，见 eventFilter
    d->_prevButton->installEventFilter(this);
    d->_nextButton->installEventFilter(this);
    connect(d->_prevButton, &ShadButton::clicked, this, [this]() { scrollPrev(); });
    connect(d->_nextButton, &ShadButton::clicked, this, [this]() { scrollNext(); });

    d->_animation = new ShadFrameAnimation(this);
    d->_animation->setDuration(ShadCarouselPrivate::kScrollDuration);
    d->_animation->setEasingCurve(QEasingCurve::OutCubic);
    connect(d->_animation, &ShadFrameAnimation::valueChanged, this, [d](qreal value) { d->setOffset(value); });
    connect(d->_animation, &ShadFrameAnimation::finished, this, [d]()
    {
        // 循环时把位置收回到一圈之内，绘制结果不变
        if (d->isLoopActive())
        {
            const qreal period = d->loopPeriod();
            qreal offset = std::fmod(d->_offset, period);
            if (offset < 0.0)
            {
                offset += period;
            }
            d->setOffset(offset);
        }
    });

    d->_autoplayTimer = new QTimer(this);
    d->_autoplayTimer->setSingleShot(true);
    connect(d->_autoplayTimer, &QTimer::timeout, this, [d]() { d->autoplayStep(); });

    d->updateLayout();
}

ShadCarousel::~ShadCarousel()
{
}

int ShadCarousel::addItem(QWidget* item)
{
    Q_D(ShadCarousel);
    return insertItem(int(d->_items.size()), item);
}

int ShadCarousel::insertItem(int index, QWidget* item)
{
    Q_D(ShadCarousel);
    if (item == nullptr)
    {
        return -1;
    }
    d->_items.removeAll(item);
    if (index < 0 || index > d->_items.size())
    {
        index = int(d->_items.size());
    }
    // 先加入列表再设置父组件：视口收到 ChildAdded 时为幻灯片安装事件过滤器
    d->_items.insert(index, item);
    if (item->parentWidget() != d->_viewport)
    {
        item->setParent(d->_viewport);
    }
    item->show();
    d->updateLayout();
    updateGeometry();
    return index;
}

void ShadCarousel::removeItem(int index)
{
    Q_D(ShadCarousel);
    if (index < 0 || index >= d->_items.size())
    {
        return;
    }
    QWidget* item = d->_items.takeAt(index);
    // 父组件置空时视口收到 ChildRemoved，在那里移除事件过滤器
    item->setParent(nullptr);
    d->updateLayout();
    updateGeometry();
}

int ShadCarousel::count() const
{
    Q_D(const ShadCarousel);
    return int(d->_items.size());
}

QWidget* ShadCarousel::item(int index) const
{
    Q_D(const ShadCarousel);
    return index >= 0 && index < d->_items.size() ? d->_items.at(index) : nullptr;
}

int ShadCarousel::indexOf(QWidget* item) const
{
    Q_D(const ShadCarousel);
    return int(d->_items.indexOf(item));
}

void ShadCarousel::setOrientation(Qt::Orientation orientation)
{
    Q_D(ShadCarousel);
    if (d->_orientation == orientation)
    {
        return;
    }
    d->_orientation = orientation;
    d->updateLayout();
    updateGeometry();
}

Qt::Orientation ShadCarousel::getOrientation() const
{
    Q_D(const ShadCarousel);
    return d->_orientation;
}

void ShadCarousel::setItemsPerView(int itemsPerView)
{
    Q_D(ShadCarousel);
    itemsPerView = qMax(1, itemsPerView);
    if (d->_itemsPerView == itemsPerView)
    {
        return;
    }
    d->_itemsPerView = itemsPerView;
    d->updateLayout();
    updateGeometry();
}

int ShadCarousel::getItemsPerView() const
{
    Q_D(const ShadCarousel);
    return d->_itemsPerView;
}

void ShadCarousel::setSpacing(int spacing)
{
    Q_D(ShadCarousel);
    spacing = qMax(0, spacing);
    if (d->_spacing == spacing)
    {
        return;
    }
    d->_spacing = spacing;
    d->updateLayout();
    updateGeometry();
}

int ShadCarousel::getSpacing() const
{
    Q_D(const ShadCarousel);
    return d->_spacing;
}

void ShadCarousel::setAlign(ShadCarouselType::Align align)
{
    Q_D(ShadCarousel);
    if (d->_align == align)
    {
        return;
    }
    d->_align = align;
    d->updateLayout();
}

ShadCarouselType::Align ShadCarousel::getAlign() const
{
    Q_D(const ShadCarousel);
    return d->_align;
}

void ShadCarousel::setLoop(bool loop)
{
    Q_D(ShadCarousel);
    if (d->_isLoop == loop)
    {
        return;
    }
    d->_isLoop = loop;
    d->updateLayout();
}

bool ShadCarousel::isLoop() const
{
    Q_D(const ShadCarousel);
    return d->_isLoop;
}

void ShadCarousel::setAutoplayInterval(int milliseconds)
{
    Q_D(ShadCarousel);
    d->_autoplayInterval = qMax(0, milliseconds);
    d->restartAutoplay();
}

int ShadCarousel::getAutoplayInterval() const
{
    Q_D(const ShadCarousel);
    return d->_autoplayInterval;
}

void ShadCarousel::setNavigationVisible(bool visible)
{
    Q_D(ShadCarousel);
    if (d->_isNavigationVisible == visible)
    {
        return;
    }
    d->_isNavigationVisible = visible;
    d->updateLayout();
    updateGeometry();
}

bool ShadCarousel::isNavigationVisible() const
{
    Q_D(const ShadCarousel);
    return d->_isNavigationVisible;
}

void ShadCarousel::scrollPrev()
{
    Q_D(ShadCarousel);
    if (!canScrollPrev())
    {
        return;
    }
    d->scrollToSnap(d->_currentIndex - 1, ShadCarouselPrivate::Backward, true);
    d->restartAutoplay();
}

void ShadCarousel::scrollNext()
{
    Q_D(ShadCarousel);
    if (!canScrollNext())
    {
        return;
    }
    d->scrollToSnap(d->_currentIndex + 1, ShadCarouselPrivate::Forward, true);
    d->restartAutoplay();
}

void ShadCarousel::scrollTo(int index, bool animated)
{
    Q_D(ShadCarousel);
    if (d->_snaps.isEmpty())
    {
        return;
    }
    d->scrollToSnap(qBound(0, index, int(d->_snaps.size()) - 1), ShadCarouselPrivate::Nearest, animated);
    d->restartAutoplay();
}

bool ShadCarousel::canScrollPrev() const
{
    Q_D(const ShadCarousel);
    return d->_snaps.size() > 1 && (d->isLoopActive() || d->_currentIndex > 0);
}

bool ShadCarousel::canScrollNext() const
{
    Q_D(const ShadCarousel);
    return d->_snaps.size() > 1 && (d->isLoopActive() || d->_currentIndex < int(d->_snaps.size()) - 1);
}

int ShadCarousel::currentIndex() const
{
    Q_D(const ShadCarousel);
    return d->_currentIndex;
}

int ShadCarousel::snapCount() const
{
    Q_D(const ShadCarousel);
    return int(d->_snaps.size());
}

QSize ShadCarousel::sizeHint() const
{
    Q_D(const ShadCarousel);
    const int margin = 2 * d->navigationMargin();
    if (d->_orientation == Qt::Horizontal)
    {
        int itemExtent = 0;
        for (const QWidget* item : d->_items)
        {
            itemExtent = qMax(itemExtent, d->itemMainHint(item));
        }
        const int width = margin + d->_itemsPerView * itemExtent + (d->_itemsPerView - 1) * d->_spacing;
        return QSize(width, d->contentCrossHint(width));
    }
    int width = d->_isNavigationVisible ? d->_prevButton->sizeHint().width() : 0;
    for (const QWidget* item : d->_items)
    {
        width = qMax(width, d->itemCrossHint(item, 0));
    }
    return QSize(width, margin + d->contentMainHintVertical());
}

QSize ShadCarousel::minimumSizeHint() const
{
    Q_D(const ShadCarousel);
    if (d->_orientation == Qt::Vertical)
    {
        return sizeHint();
    }
    int itemExtent = 0;
    for (const QWidget* item : d->_items)
    {
        itemExtent = qMax(itemExtent, item->minimumSizeHint().expandedTo(item->minimumSize()).width());
    }
    const int width = 2 * d->navigationMargin() + d->_itemsPerView * itemExtent + (d->_itemsPerView - 1) * d->_spacing;
    return QSize(width, d->contentCrossHint(width));
}

bool ShadCarousel::hasHeightForWidth() const
{
    Q_D(const ShadCarousel);
    return d->_orientation == Qt::Horizontal;
}

int ShadCarousel::heightForWidth(int width) const
{
    Q_D(const ShadCarousel);
    if (d->_orientation == Qt::Horizontal)
    {
        return d->contentCrossHint(width);
    }
    return sizeHint().height();
}

bool ShadCarousel::event(QEvent* event)
{
    Q_D(ShadCarousel);
    switch (event->type())
    {
        case QEvent::LayoutDirectionChange:
            {
                d->updateLayout();
                break;
            }
        case QEvent::LanguageChange:
            {
                d->_prevButton->setAccessibleName(tr("上一张"));
                d->_nextButton->setAccessibleName(tr("下一张"));
                break;
            }
        case QEvent::Show:
            {
                d->restartAutoplay();
                break;
            }
        case QEvent::Hide:
            {
                d->_autoplayTimer->stop();
                // 隐藏时放弃拖动并停到当前停靠点
                if (d->_isPointerDown)
                {
                    d->resetPointer();
                }
                d->updateLayout();
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::event(event);
}

bool ShadCarousel::eventFilter(QObject* watched, QEvent* event)
{
    Q_D(ShadCarousel);
    if (d->_isSendingCancel || !watched->isWidgetType())
    {
        return QWidget::eventFilter(watched, event);
    }
    QWidget* widget = static_cast<QWidget*>(watched);
    // 按钮会把方向键用于在相邻按钮之间移动焦点，焦点在翻页按钮或幻灯片内的按钮上时由轮播先处理方向键
    if (event->type() == QEvent::KeyPress && qobject_cast<QAbstractButton*>(widget) != nullptr)
    {
        const int step = d->keyStep(static_cast<QKeyEvent*>(event)->key());
        if (step != 0)
        {
            if (step < 0)
            {
                scrollPrev();
            }
            else
            {
                scrollNext();
            }
            event->accept();
            return true;
        }
    }
    // 翻页按钮只处理方向键，鼠标事件不参与拖动
    if (widget == d->_prevButton || widget == d->_nextButton)
    {
        return QWidget::eventFilter(watched, event);
    }
    switch (event->type())
    {
        case QEvent::ChildAdded:
            {
                d->installFilter(static_cast<QChildEvent*>(event)->child());
                break;
            }
        case QEvent::ChildRemoved:
            {
                QObject* child = static_cast<QChildEvent*>(event)->child();
                d->removeFilter(child);
                if (widget != d->_viewport)
                {
                    break;
                }
                // 幻灯片被删除或移到别处时从列表中移除；只按 QObject 指针比较，析构过程中的子组件不会被访问
                bool isItemRemoved = false;
                for (int index = int(d->_items.size()) - 1; index >= 0; --index)
                {
                    if (static_cast<QObject*>(d->_items.at(index)) == child)
                    {
                        d->_items.removeAt(index);
                        isItemRemoved = true;
                    }
                }
                if (isItemRemoved)
                {
                    d->updateLayout();
                    updateGeometry();
                }
                break;
            }
        case QEvent::LayoutRequest:
            {
                // 幻灯片的尺寸提示变化时视口收到布局请求
                if (widget == d->_viewport)
                {
                    updateGeometry();
                    d->updateLayout();
                }
                break;
            }
        case QEvent::MouseButtonPress:
        case QEvent::MouseButtonDblClick:
        case QEvent::MouseMove:
        case QEvent::MouseButtonRelease:
            {
                if (d->handleMouseEvent(widget, static_cast<QMouseEvent*>(event)))
                {
                    event->accept();
                    return true;
                }
                break;
            }
        default:
            {
                break;
            }
    }
    return QWidget::eventFilter(watched, event);
}

void ShadCarousel::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadCarousel);
    QWidget::resizeEvent(event);
    d->updateLayout();
}

void ShadCarousel::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadCarousel);
    // 焦点在轮播内部的子组件上且子组件没有处理方向键时，事件交给这里
    const int step = d->keyStep(event->key());
    if (step == 0)
    {
        QWidget::keyPressEvent(event);
        return;
    }
    if (step < 0)
    {
        scrollPrev();
    }
    else
    {
        scrollNext();
    }
    event->accept();
}

void ShadCarousel::enterEvent(QEnterEvent* event)
{
    Q_D(ShadCarousel);
    d->_isHovered = true;
    d->restartAutoplay();
    QWidget::enterEvent(event);
}

void ShadCarousel::leaveEvent(QEvent* event)
{
    Q_D(ShadCarousel);
    d->_isHovered = false;
    d->restartAutoplay();
    QWidget::leaveEvent(event);
}
