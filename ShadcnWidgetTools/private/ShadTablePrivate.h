#ifndef SHADTABLEPRIVATE_H
#define SHADTABLEPRIVATE_H

#include <QHash>
#include <QList>
#include <QMetaObject>
#include <QModelIndex>
#include <QPoint>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadProperty.h"
#include "ShadTableDelegate.h"

class QPainter;
class ShadTable;
class ShadTableHeader;

class ShadTablePrivate
{
    SHAD_D_CREATE(ShadTable)

    public:
        ShadTablePrivate();
        ~ShadTablePrivate();

        // 表尾的一个单元格
        struct FooterCell
        {
            int column{0};
            QString text;
            Qt::Alignment alignment{Qt::AlignLeft | Qt::AlignVCenter};
            int span{1};
        };

        // 空状态行 h-24
        static constexpr int kEmptyHeight = 96;
        // 表尾行：p-2 + text-sm 行高
        static constexpr int kFooterHeight = 36;
        // 说明文字 mt-4 + text-sm 行高
        static constexpr int kCaptionTop = 16;
        static constexpr int kCaptionHeight = 20;
        // 计算列宽时最多测量的行数，避免大数据量时卡顿
        static constexpr int kMaxMeasureRows = 1000;

        // 行高：指定了统一行高时使用指定值；否则取 36px（有行操作列时 48px）与单元格内容所需高度的较大值
        int rowHeight() const;
        // 按单元格的 sizeHint 测量内容所需的行高（最多测量 kMaxMeasureRows 行）
        void measureContentRowHeight();
        int footerHeight() const;
        int captionHeight() const;
        int rowCount() const;
        // 表体（含空状态行）、表尾与说明文字的总高度，不含表头
        int contentHeight() const;
        int naturalColumnWidth(int column) const;
        int totalNaturalWidth() const;

        // 合并同一轮事件中的多次布局请求
        void scheduleLayout();
        // 重新测量行高与各列自然宽度，再分配列宽
        void updateLayout();
        // 按已测量的自然宽度分配列宽，不重新测量；拖动调整列宽时每次移动调用
        // 列宽优先级：固定宽度 > 拖动后的宽度 > 自然宽度；只有自然宽度的列分配剩余宽度
        void applyColumnWidths();
        // 列的当前目标宽度（未分配剩余宽度前）
        int baseColumnWidth(int column) const;
        void updateHover(const QPoint& pos);
        void clearHover();
        void toggleRowChecked(const QModelIndex& index);
        // 表头全选：已全部勾选时全部取消，否则全部勾选
        void toggleAllChecked();
        void connectModel();
        void disconnectModel();

        void paintFooter(QPainter* painter, int top) const;
        void paintCaption(QPainter* painter, int top) const;

        ShadTableHeader* _header{nullptr};
        ShadTableDelegate* _delegate{nullptr};
        QHash<int, int> _fixedWidths;
        QHash<int, ShadTableDelegate::ImageColumn> _imageColumns;
        // 拖动或 setColumnUserWidth 设置的列宽
        QHash<int, int> _userWidths;
        // 上次 updateLayout 测量的各列自然宽度
        QHash<int, int> _naturalWidths;
        bool _isColumnsResizable{true};
        // setDefaultRowHeight 指定的行高，-1 为自动
        int _defaultRowHeight{-1};
        // 单元格内容所需的行高，由 measureContentRowHeight 计算
        int _contentRowHeight{0};
        QList<FooterCell> _footerCells;
        QList<QMetaObject::Connection> _modelConnections;
        QString _caption;
        QString _emptyText;
        bool _isAutoHeight{true};
        bool _isLayoutPending{false};
        int _selectionColumn{-1};
        int _actionColumn{-1};
};

#endif // SHADTABLEPRIVATE_H
