#include "ShadListView.h"

#include <QAbstractItemModel>
#include <QAbstractItemView>
#include <QEvent>
#include <QFocusEvent>
#include <QFont>
#include <QFrame>
#include <QKeyEvent>
#include <QModelIndex>
#include <QMouseEvent>
#include <QObject>
#include <QResizeEvent>
#include <QScrollBar>
#include <QString>
#include <QtGlobal>

#include "ShadEmpty.h"
#include "ShadIconType.h"
#include "ShadListDelegate.h"
#include "ShadListViewPrivate.h"
#include "ShadScrollBar.h"
#include "ShadSpinner.h"
#include "ShadStyleHelper.h"
#include "ShadTheme.h"
#include "ShadTreeViewStyle.h"

namespace
{
    // 空状态的默认标题，使用者改过的保持不变
    void applyDefaultTexts(ShadEmpty* emptyState)
    {
        ShadStyleHelper::applyDefaultText(emptyState, "title", emptyState->getTitle(), ShadListView::tr("暂无数据"), [emptyState](const QString& text) { emptyState->setTitle(text); });
    }
} // namespace

ShadListView::ShadListView(QWidget* parent)
    : QListView(parent), d_ptr(new ShadListViewPrivate())
{
    Q_D(ShadListView);
    d->q_ptr = this;
    ShadStyleHelper::applyFont(this, 14, QFont::Normal);
    // 与 ShadTreeView 共用样式
    d->_style = new ShadTreeViewStyle(this);
    setStyle(d->_style);
    d->_delegate = new ShadListDelegate(this);
    setItemDelegate(d->_delegate);

    // 所有行等高，大量数据时只需计算可见行
    setUniformItemSizes(true);
    setEditTriggers(QAbstractItemView::NoEditTriggers);
    setFrameShape(QFrame::NoFrame);
    setHorizontalScrollMode(QAbstractItemView::ScrollPerPixel);
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    setHorizontalScrollBar(new ShadScrollBar(Qt::Horizontal, this));
    setVerticalScrollBar(new ShadScrollBar(Qt::Vertical, this));
    // 文字按宽度省略，不横向滚动
    setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    viewport()->setMouseTracking(true);
    viewport()->setAutoFillBackground(false);

    d->_emptyState = new ShadEmpty(ShadIconType::Inbox, QString(), QString(), viewport());
    applyDefaultTexts(d->_emptyState);
    d->_spinner = new ShadSpinner(this);
    d->_spinner->setSpinnerSize(ShadListViewPrivate::kSpinnerSize);
    d->_spinner->hide();

    connect(verticalScrollBar(), &QScrollBar::valueChanged, this, [d]()
    {
        d->updateHoverFromCursor();
        d->scheduleEndCheck();
    });
    connect(verticalScrollBar(), &QScrollBar::rangeChanged, this, [d]() { d->scheduleEndCheck(); });
    connect(shadTheme, &ShadTheme::themeChanged, this, [this]() { viewport()->update(); });
    d->updateEmptyState();
}

ShadListView::~ShadListView()
{
    Q_D(ShadListView);
    // 私有数据先于子对象释放，断开捕获 d 的连接
    d->disconnectModel();
    QObject::disconnect(verticalScrollBar(), nullptr, this, nullptr);
}

void ShadListView::setItemSize(ShadItemType::Size size)
{
    Q_D(ShadListView);
    if (d->_delegate->getItemSize() == size)
    {
        return;
    }
    d->_delegate->setItemSize(size);
    d->relayout();
}

ShadItemType::Size ShadListView::getItemSize() const
{
    Q_D(const ShadListView);
    return d->_delegate->getItemSize();
}

void ShadListView::setDescriptionVisible(bool visible)
{
    Q_D(ShadListView);
    if (d->_delegate->isDescriptionVisible() == visible)
    {
        return;
    }
    d->_delegate->setDescriptionVisible(visible);
    d->relayout();
}

bool ShadListView::isDescriptionVisible() const
{
    Q_D(const ShadListView);
    return d->_delegate->isDescriptionVisible();
}

void ShadListView::setImageShape(ShadTableType::ImageShape shape)
{
    Q_D(ShadListView);
    if (d->_delegate->getImageShape() == shape)
    {
        return;
    }
    d->_delegate->setImageShape(shape);
    viewport()->update();
}

ShadTableType::ImageShape ShadListView::getImageShape() const
{
    Q_D(const ShadListView);
    return d->_delegate->getImageShape();
}

ShadEmpty* ShadListView::emptyState() const
{
    Q_D(const ShadListView);
    return d->_emptyState;
}

void ShadListView::setLoadingMore(bool loading)
{
    Q_D(ShadListView);
    if (d->_isLoadingMore == loading)
    {
        return;
    }
    d->_isLoadingMore = loading;
    // 在视口下方预留加载指示区域
    setViewportMargins(0, 0, 0, loading ? ShadListViewPrivate::kLoadingHeight : 0);
    d->updateSpinnerGeometry();
    d->_spinner->setVisible(loading);
    if (!loading)
    {
        // 加载结束后允许再次发出 reachedEnd
        d->_isEndNotified = false;
        d->scheduleEndCheck();
    }
}

bool ShadListView::isLoadingMore() const
{
    Q_D(const ShadListView);
    return d->_isLoadingMore;
}

void ShadListView::setLoadMoreThreshold(int pixels)
{
    Q_D(ShadListView);
    const int threshold = qMax(0, pixels);
    if (d->_loadMoreThreshold == threshold)
    {
        return;
    }
    d->_loadMoreThreshold = threshold;
    d->scheduleEndCheck();
}

int ShadListView::getLoadMoreThreshold() const
{
    Q_D(const ShadListView);
    return d->_loadMoreThreshold;
}

void ShadListView::setModel(QAbstractItemModel* model)
{
    Q_D(ShadListView);
    d->setHoveredIndex(QModelIndex());
    QListView::setModel(model);
    d->connectModel(model);
    d->onRowCountChanged();
}

bool ShadListView::event(QEvent* event)
{
    Q_D(ShadListView);
    switch (event->type())
    {
        case QEvent::LanguageChange:
            applyDefaultTexts(d->_emptyState);
            break;
        case QEvent::Show:
            // 隐藏期间不检查是否到底，显示后补一次
            d->scheduleEndCheck();
            break;
        default:
            break;
    }
    return QListView::event(event);
}

bool ShadListView::viewportEvent(QEvent* event)
{
    Q_D(ShadListView);
    switch (event->type())
    {
        case QEvent::Leave:
            d->setHoveredIndex(QModelIndex());
            break;
        case QEvent::Resize:
            d->updateEmptyState();
            break;
        default:
            break;
    }
    return QListView::viewportEvent(event);
}

void ShadListView::mouseMoveEvent(QMouseEvent* event)
{
    Q_D(ShadListView);
    d->setHoveredIndex(indexAt(event->position().toPoint()));
    QListView::mouseMoveEvent(event);
}

void ShadListView::mousePressEvent(QMouseEvent* event)
{
    Q_D(ShadListView);
    // 鼠标操作不显示焦点环
    d->setFocusVisible(false);
    QListView::mousePressEvent(event);
}

void ShadListView::keyPressEvent(QKeyEvent* event)
{
    Q_D(ShadListView);
    // 键盘导航显示焦点环
    d->setFocusVisible(true);
    QListView::keyPressEvent(event);
    viewport()->update();
}

void ShadListView::focusInEvent(QFocusEvent* event)
{
    Q_D(ShadListView);
    d->setFocusVisible(ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event));
    QListView::focusInEvent(event);
    viewport()->update();
}

void ShadListView::focusOutEvent(QFocusEvent* event)
{
    Q_D(ShadListView);
    d->setFocusVisible(ShadStyleHelper::resolveFocusVisible(d->_isFocusVisible, event));
    QListView::focusOutEvent(event);
    viewport()->update();
}

void ShadListView::resizeEvent(QResizeEvent* event)
{
    Q_D(ShadListView);
    QListView::resizeEvent(event);
    d->updateSpinnerGeometry();
}
