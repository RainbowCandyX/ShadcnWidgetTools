#ifndef SHADTREESELECTPRIVATE_H
#define SHADTREESELECTPRIVATE_H

#include <QIcon>
#include <QList>
#include <QMetaObject>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QString>
#include <QtCore/qnamespace.h>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QFontMetricsF;
class QKeyEvent;
class QObject;
class QStandardItem;
class QStandardItemModel;
class ShadPopover;
class ShadTransition;
class ShadTreeSelect;
class ShadTreeSelectPanel;

class ShadTreeSelectPrivate
{
    SHAD_D_CREATE(ShadTreeSelect)

    public:
        ShadTreeSelectPrivate();
        ~ShadTreeSelectPrivate();

        bool isMultiSelection() const;
        // 当前使用的模型（用户模型或内部模型）
        QAbstractItemModel* currentModel() const;
        // 节点是否显示复选框，判断与 ShadTreeView 一致
        static bool hasCheckbox(const QModelIndex& index);
        static Qt::CheckState checkState(const QModelIndex& index);
        // 读取 Qt::DecorationRole 图标，兼容 QPixmap
        static QIcon decorationIcon(const QModelIndex& index);
        // 节点在触发框中显示的文字，可为完整路径
        QString displayText(const QModelIndex& index) const;

        void connectModel(QAbstractItemModel* model);
        void disconnectModel();
        void onModelStructureChanged();
        void onModelDataChanged(const QList<int>& roles);
        // 当前项失效后清空并发出 currentIndexChanged
        void checkCurrentIndexValid();

        // 按选择模式设置内部模型节点的复选框
        void configureInternalItem(QStandardItem* item) const;
        void configureInternalItems(QStandardItem* parent) const;

        // 设置勾选状态并联动父子节点
        void applyCheckState(const QModelIndex& index, Qt::CheckState state);
        void setDescendantsCheckState(const QModelIndex& index, Qt::CheckState state);
        void updateAncestorsCheckState(const QModelIndex& index);
        // 按 CheckedStrategy 收集勾选节点
        void collectCheckedIndexes(const QModelIndex& parent, QList<QModelIndex>* indexes) const;
        // 把 parent 子树中全部带复选框的节点设为未勾选
        void uncheckRows(const QModelIndex& parent);
        // 把多次勾选变化合并为一次 checkedIndexesChanged
        void notifyCheckedChanged();
        // 勾选结果与上次发出时不同才发出 checkedIndexesChanged
        void emitCheckedChanged();

        // 触发框内容的最大宽度，结果带缓存
        int contentWidth() const;
        void measureRows(const QModelIndex& parent, const QFontMetricsF& metrics, qreal* textWidth, bool* hasIcon) const;
        void invalidateTextWidth();

        void onTreePressed(const QModelIndex& index);
        void onTreeClicked(const QModelIndex& index);
        // 单选时选中节点，多选时切换勾选
        void activateIndex(const QModelIndex& index);
        void onFiltered(const QModelIndex& firstMatch);
        // 打开弹出层或清空搜索时默认高亮的节点
        QModelIndex initialHighlightIndex() const;
        // 高亮节点并滚动到可见位置
        void highlightIndex(const QModelIndex& index);
        // 处理弹出层中树与搜索框的按键，返回 true 表示已处理
        bool handleTreeKey(QKeyEvent* event);
        bool handleSearchKey(QKeyEvent* event);

        ShadComboBoxType::Size _size{ShadComboBoxType::Medium};
        ShadComboBoxType::SelectionMode _selectionMode{ShadComboBoxType::SingleSelection};
        ShadTreeSelectType::CheckedStrategy _checkedStrategy{ShadTreeSelectType::ShowChild};
        bool _isInvalid{false};
        bool _isFocusVisible{false};
        bool _isLeafOnly{false};
        bool _isFullPathVisible{false};
        bool _isSearchEnabled{false};
        QString _placeholderText;
        QString _searchPlaceholderText;
        QString _emptyText;
        QStandardItemModel* _internalModel{nullptr};
        QList<QMetaObject::Connection> _modelConnections;
        QPersistentModelIndex _currentIndex;
        // 用于当前项被删除后发出一次变化信号
        bool _hasCurrentIndex{false};
        // 按下时的勾选状态，用于判断是否点在复选框以外
        QPersistentModelIndex _pressedIndex;
        int _pressedCheckState{-1};
        // 合并批量勾选变化
        int _checkBatchDepth{0};
        bool _isCheckedDirty{false};
        bool _isCheckedSignalQueued{false};
        // 上次发出 checkedIndexesChanged 时的勾选结果
        QList<QPersistentModelIndex> _lastCheckedIndexes;
        mutable int _cachedContentWidth{-1};
        ShadTransition* _hoverTransition{nullptr};
        ShadPopover* _popover{nullptr};
        ShadTreeSelectPanel* _panel{nullptr};
};

#endif // SHADTREESELECTPRIVATE_H
