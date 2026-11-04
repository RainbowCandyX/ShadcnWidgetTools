#include "ShadTabViewPrivate.h"

#include <QByteArray>
#include <QCoreApplication>
#include <QCursor>
#include <QDrag>
#include <QDropEvent>
#include <QEvent>
#include <QMetaObject>
#include <QMimeData>
#include <QPixmap>
#include <QPoint>
#include <QPointer>
#include <QSize>
#include <QSizeF>
#include <QStackedLayout>
#include <QString>
#include <QVariant>
#include <QWidget>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadPageTransition.h"
#include "ShadStyleHelper.h"
#include "ShadTabView.h"
#include "ShadTabViewBar.h"

namespace
{
    struct DragSession
    {
        QPointer<ShadTabView> source;
        QPointer<QWidget> page;
        // 拖放结束后才移动标签，避免在拖放的调用栈中删除窗口
        QPointer<ShadTabView> target;
        int targetSlot{-1};
    };

    DragSession& dragSession()
    {
        static DragSession session;
        return session;
    }

    // 内容为进程号，拒绝其他进程的拖放
    QString mimeType()
    {
        return QStringLiteral("application/x-shadcnwidgettools-tab");
    }

    QByteArray processId()
    {
        return QByteArray::number(QCoreApplication::applicationPid());
    }
} // namespace

ShadTabViewPrivate::ShadTabViewPrivate()
{
}

ShadTabViewPrivate::~ShadTabViewPrivate()
{
}

void ShadTabViewPrivate::updatePages()
{
    const int current = _bar.isNull() ? -1 : _bar->currentIndex();
    QWidget* page = current >= 0 && current < _pages.size() ? _pages.at(current).data() : nullptr;
    // 移除当前页时 QStackedLayout 会自行切换，以上次显示的页面为准
    QWidget* previous = _shownPage.data();
    _shownPage = page;
    QWidget* stackPage = _pageStack->currentWidget();
    if (previous == page && stackPage == page)
    {
        return;
    }
    // 避免焦点跳到 Tab 顺序中的下一个组件
    if (stackPage != nullptr && stackPage != page)
    {
        ShadStyleHelper::clearFocusWithin(stackPage);
    }
    const bool isAnimated = previous != nullptr && page != nullptr && previous != page && _pages.contains(previous);
    if (isAnimated)
    {
        _transition->capture(previous);
    }
    if (page != nullptr)
    {
        _pageStack->setCurrentWidget(page);
    }
    if (isAnimated)
    {
        _transition->play(page, _pages.indexOf(page) >= _pages.indexOf(previous) ? 1 : -1);
    }
}

void ShadTabViewPrivate::stepCurrent(int step)
{
    if (!_bar.isNull())
    {
        _bar->selectAdjacent(step);
    }
}

void ShadTabViewPrivate::startDrag(int index, const QPoint& grabOffset)
{
    ShadTabView* q = q_ptr;
    QWidget* page = index >= 0 && index < _pages.size() ? _pages.at(index).data() : nullptr;
    if (_bar.isNull() || page == nullptr)
    {
        return;
    }
    // 拖放期间窗口允许被关闭
    const QPointer<ShadTabView> self(q);
    const QPointer<ShadTabViewBar> bar(_bar);
    const QPointer<QWidget> draggedPage(page);

    DragSession& session = dragSession();
    session = DragSession();
    session.source = q;
    session.page = page;
    QMimeData* mimeData = new QMimeData();
    mimeData->setData(mimeType(), processId());
    QDrag* drag = new QDrag(_bar);
    drag->setMimeData(mimeData);
    const QPixmap pixmap = _bar->dragPixmap(index, ShadStyleHelper::grabContent(page));
    const QSize pixmapSize = (QSizeF(pixmap.size()) / pixmap.devicePixelRatio()).toSize();
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(qBound(0, grabOffset.x(), pixmapSize.width() - 1), qBound(0, grabOffset.y(), pixmapSize.height() - 1)));
    _bar->setHiddenIndex(index);
    const Qt::DropAction action = drag->exec(Qt::MoveAction);

    const QPointer<ShadTabView> target = session.target;
    const int targetSlot = session.targetSlot;
    session = DragSession();
    if (self.isNull())
    {
        return;
    }
    if (!bar.isNull())
    {
        bar->setHiddenIndex(-1);
    }
    const int current = q->indexOf(draggedPage);
    if (current < 0)
    {
        return;
    }
    if (!target.isNull())
    {
        if (target.data() == q)
        {
            q->moveTab(current, qMin(targetSlot, q->count() - 1));
        }
        else
        {
            transferTab(q, current, target.data(), targetSlot);
        }
        return;
    }
    if (action != Qt::IgnoreAction || !_detachHandler)
    {
        return;
    }
    ShadTabView* detached = _detachHandler(q);
    if (self.isNull() || detached == nullptr || detached == q)
    {
        return;
    }
    const int detachedIndex = q->indexOf(draggedPage);
    if (detachedIndex < 0)
    {
        return;
    }
    QWidget* window = detached->window();
    const bool wasVisible = window->isVisible();
    transferTab(q, detachedIndex, detached, detached->count());
    // 显示后布局才确定，再让标签对准光标
    window->show();
    ShadTabViewBar* detachedBar = qobject_cast<ShadTabViewBar*>(detached->tabBar());
    if (!wasVisible && detachedBar != nullptr)
    {
        const QPoint tabOffset = detachedBar->mapTo(window, detachedBar->restingTabRect(detached->indexOf(draggedPage)).topLeft()) + grabOffset;
        window->move(QCursor::pos() - tabOffset);
    }
    window->raise();
    window->activateWindow();
}

ShadTabView* ShadTabViewPrivate::acceptedSource(const QMimeData* mimeData) const
{
    const DragSession& session = dragSession();
    if (mimeData == nullptr || _dragGroup.isEmpty() || session.source.isNull() || session.page.isNull())
    {
        return nullptr;
    }
    if (mimeData->data(mimeType()) != processId() || session.source->getDragGroup() != _dragGroup)
    {
        return nullptr;
    }
    return session.source.data();
}

bool ShadTabViewPrivate::handleDragEvent(QEvent* event)
{
    switch (event->type())
    {
        case QEvent::DragEnter:
        case QEvent::DragMove:
        case QEvent::Drop:
            {
                QDropEvent* dropEvent = static_cast<QDropEvent*>(event);
                ShadTabView* source = acceptedSource(dropEvent->mimeData());
                if (source == nullptr || _bar.isNull())
                {
                    event->ignore();
                    return true;
                }
                const int slot = _bar->insertionSlotAt(dropEvent->position().toPoint());
                if (event->type() == QEvent::Drop)
                {
                    DragSession& session = dragSession();
                    session.target = q_ptr;
                    session.targetSlot = slot;
                    _bar->setDropIndicator(-1);
                }
                else
                {
                    _bar->setDropIndicator(slot);
                }
                dropEvent->setDropAction(Qt::MoveAction);
                dropEvent->accept();
                return true;
            }
        case QEvent::DragLeave:
            if (!_bar.isNull())
            {
                _bar->setDropIndicator(-1);
            }
            return true;
        default:
            return false;
    }
}

void ShadTabViewPrivate::transferTab(ShadTabView* source, int index, ShadTabView* target, int targetIndex)
{
    QWidget* page = source->widget(index);
    if (page == nullptr)
    {
        return;
    }
    const ShadIconType::IconName icon = source->tabIcon(index);
    const QString text = source->tabText(index);
    const ShadTabViewType::TabFlags flags = source->tabFlags(index);
    const QString toolTip = source->tabToolTip(index);
    const QVariant data = source->tabData(index);
    source->removeTab(index);
    const int inserted = target->insertTab(targetIndex, icon, text, page);
    target->setTabFlags(inserted, flags);
    target->setTabToolTip(inserted, toolTip);
    target->setTabData(inserted, data);
    target->setCurrentIndex(inserted);
    if (source->count() == 0)
    {
        // 排队发出，接收方会关闭窗口
        QMetaObject::invokeMethod(source, [source]() { Q_EMIT source->lastTabMovedOut(); }, Qt::QueuedConnection);
    }
    target->window()->raise();
    target->window()->activateWindow();
}
