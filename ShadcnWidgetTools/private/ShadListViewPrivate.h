#ifndef SHADLISTVIEWPRIVATE_H
#define SHADLISTVIEWPRIVATE_H

#include <QList>
#include <QMetaObject>
#include <QModelIndex>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class ShadEmpty;
class ShadListDelegate;
class ShadListView;
class ShadSpinner;
class ShadTreeViewStyle;

class ShadListViewPrivate
{
    SHAD_D_CREATE(ShadListView)

    public:
        ShadListViewPrivate();
        ~ShadListViewPrivate();

        // 滚动加载相关的尺寸
        static constexpr int kLoadingHeight = 48;
        static constexpr int kSpinnerSize = 16;
        static constexpr int kDefaultLoadMoreThreshold = 200;

        int rowCount() const;
        // 监听模型的行数变化
        void connectModel(QAbstractItemModel* model);
        void disconnectModel();
        // 行数变化后刷新空状态并重新检查是否到底
        void onRowCountChanged();
        void updateEmptyState();
        void updateSpinnerGeometry();
        // 行高变化后重新布局
        void relayout();
        // 记录悬停行，禁用的行不高亮
        void setHoveredIndex(const QModelIndex& index);
        // 滚动后按光标位置刷新悬停行
        void updateHoverFromCursor();
        void updateRow(const QModelIndex& index);
        // 只有键盘导航时显示焦点环
        void setFocusVisible(bool visible);
        // 排队检查是否到底，避免在布局过程中发出信号
        void scheduleEndCheck();
        void checkReachedEnd();

        ShadListDelegate* _delegate{nullptr};
        ShadTreeViewStyle* _style{nullptr};
        ShadEmpty* _emptyState{nullptr};
        ShadSpinner* _spinner{nullptr};
        QList<QMetaObject::Connection> _modelConnections;
        bool _isFocusVisible{false};
        bool _isLoadingMore{false};
        int _loadMoreThreshold{kDefaultLoadMoreThreshold};
        // 本次到底已经发出过 reachedEnd
        bool _isEndNotified{false};
        bool _isEndCheckQueued{false};
};

#endif // SHADLISTVIEWPRIVATE_H
