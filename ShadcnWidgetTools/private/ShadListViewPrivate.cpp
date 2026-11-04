#include "ShadListViewPrivate.h"

#include <QAbstractItemModel>
#include <QCursor>
#include <QMetaObject>
#include <QModelIndex>
#include <QObject>
#include <QRect>
#include <QScrollBar>
#include <QWidget>
#include <QtCore/qnamespace.h>

#include "ShadEmpty.h"
#include "ShadListDelegate.h"
#include "ShadListView.h"
#include "ShadSpinner.h"
#include "ShadStyleHelper.h"

ShadListViewPrivate::ShadListViewPrivate()
{
}

ShadListViewPrivate::~ShadListViewPrivate()
{
}

int ShadListViewPrivate::rowCount() const
{
    Q_Q(const ShadListView);
    const QAbstractItemModel* model = q->model();
    return model != nullptr ? model->rowCount(q->rootIndex()) : 0;
}

void ShadListViewPrivate::connectModel(QAbstractItemModel* model)
{
    Q_Q(ShadListView);
    disconnectModel();
    if (model == nullptr)
    {
        return;
    }
    const auto rowCountChanged = [this]() { onRowCountChanged(); };
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsInserted, q, rowCountChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::rowsRemoved, q, rowCountChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::modelReset, q, rowCountChanged));
    _modelConnections.append(QObject::connect(model, &QAbstractItemModel::layoutChanged, q, rowCountChanged));
    // 模型销毁后排队刷新空状态，等视图切换到空模型
    _modelConnections.append(QObject::connect(model, &QObject::destroyed, q, [this]() { QMetaObject::invokeMethod(q_ptr, [this]() { onRowCountChanged(); }, Qt::QueuedConnection); }));
}

void ShadListViewPrivate::disconnectModel()
{
    for (const QMetaObject::Connection& connection : _modelConnections)
    {
        QObject::disconnect(connection);
    }
    _modelConnections.clear();
}

void ShadListViewPrivate::onRowCountChanged()
{
    _isEndNotified = false;
    updateEmptyState();
    scheduleEndCheck();
}

void ShadListViewPrivate::updateEmptyState()
{
    Q_Q(ShadListView);
    if (rowCount() > 0)
    {
        ShadStyleHelper::clearFocusWithin(_emptyState);
        _emptyState->hide();
        return;
    }
    _emptyState->setGeometry(q->viewport()->rect());
    _emptyState->show();
    _emptyState->raise();
}

void ShadListViewPrivate::updateSpinnerGeometry()
{
    Q_Q(ShadListView);
    // 加载指示居中于视口下方的预留区域
    const QRect viewportRect = q->viewport()->geometry();
    const QRect area(viewportRect.left(), viewportRect.bottom() + 1, viewportRect.width(), kLoadingHeight);
    _spinner->setGeometry(area.left() + (area.width() - kSpinnerSize) / 2, area.top() + (area.height() - kSpinnerSize) / 2, kSpinnerSize, kSpinnerSize);
}

void ShadListViewPrivate::relayout()
{
    Q_Q(ShadListView);
    // doItemsLayout 会清除缓存的行尺寸，使新行高生效
    q->doItemsLayout();
    q->viewport()->update();
}

void ShadListViewPrivate::setHoveredIndex(const QModelIndex& index)
{
    QModelIndex rowIndex = index;
    if (rowIndex.isValid() && !rowIndex.flags().testFlag(Qt::ItemIsEnabled))
    {
        rowIndex = QModelIndex();
    }
    const QModelIndex oldIndex = _delegate->getHoveredIndex();
    if (oldIndex == rowIndex)
    {
        return;
    }
    _delegate->setHoveredIndex(rowIndex);
    updateRow(oldIndex);
    updateRow(rowIndex);
}

void ShadListViewPrivate::updateHoverFromCursor()
{
    Q_Q(ShadListView);
    QWidget* viewport = q->viewport();
    if (!viewport->underMouse())
    {
        setHoveredIndex(QModelIndex());
        return;
    }
    setHoveredIndex(q->indexAt(viewport->mapFromGlobal(QCursor::pos())));
}

void ShadListViewPrivate::updateRow(const QModelIndex& index)
{
    Q_Q(ShadListView);
    if (!index.isValid())
    {
        return;
    }
    const QRect itemRect = q->visualRect(index);
    if (itemRect.isValid())
    {
        q->viewport()->update(itemRect);
    }
}

void ShadListViewPrivate::setFocusVisible(bool visible)
{
    _isFocusVisible = visible;
    _delegate->setFocusVisible(visible);
}

void ShadListViewPrivate::scheduleEndCheck()
{
    Q_Q(ShadListView);
    if (_isEndCheckQueued)
    {
        return;
    }
    _isEndCheckQueued = true;
    QMetaObject::invokeMethod(q, [this]()
    {
        _isEndCheckQueued = false;
        checkReachedEnd();
    }, Qt::QueuedConnection);
}

void ShadListViewPrivate::checkReachedEnd()
{
    Q_Q(ShadListView);
    // 隐藏时尺寸与滚动范围不可靠，显示后再检查
    if (!q->isVisible() || rowCount() == 0)
    {
        return;
    }
    // 先完成排队中的布局，确保滚动条范围是最新的
    q->executeDelayedItemsLayout();
    const QScrollBar* scrollBar = q->verticalScrollBar();
    if (scrollBar->maximum() - scrollBar->value() > _loadMoreThreshold)
    {
        // 离开底部后允许下次到底时再次发出
        _isEndNotified = false;
        return;
    }
    if (_isLoadingMore || _isEndNotified)
    {
        return;
    }
    _isEndNotified = true;
    Q_EMIT q->reachedEnd();
}
