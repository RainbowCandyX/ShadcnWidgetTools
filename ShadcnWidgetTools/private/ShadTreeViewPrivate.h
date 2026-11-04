#ifndef SHADTREEVIEWPRIVATE_H
#define SHADTREEVIEWPRIVATE_H

#include <QHash>
#include <QModelIndex>
#include <QPersistentModelIndex>
#include <QRect>
#include <QtGlobal>

#include "ShadProperty.h"

class QVariantAnimation;
class ShadTableHeader;
class ShadTreeDelegate;
class ShadTreeView;
class ShadTreeViewStyle;

class ShadTreeViewPrivate
{
    SHAD_D_CREATE(ShadTreeView)

    public:
        ShadTreeViewPrivate();
        ~ShadTreeViewPrivate();

        // 每级缩进
        static constexpr int kIndentation = 20;
        static constexpr int kItemHeight = 32;
        static constexpr int kChevronSize = 16;
        // 箭头旋转动画时长
        static constexpr int kChevronDurationMs = 150;

        // 节点相对根节点的层级，顶层为 0
        int depthOf(const QModelIndex& index) const;
        // 行背景区域：从该行展开箭头所在的缩进格开始，到最后一列的末尾；rowRect 为整行区域
        QRect rowBackgroundRect(const QRect& rowRect, const QModelIndex& index) const;
        // 展开箭头的旋转进度，0 为折叠、1 为展开
        qreal chevronProgress(const QModelIndex& index) const;
        void animateChevron(const QModelIndex& index, bool expanded);
        void clearChevronAnimations();
        void setHoveredIndex(const QModelIndex& index);
        // 重绘某一行
        void updateRow(const QModelIndex& index);

        ShadTreeDelegate* _delegate{nullptr};
        ShadTreeViewStyle* _style{nullptr};
        ShadTableHeader* _header{nullptr};
        QPersistentModelIndex _hoveredIndex;
        QHash<QPersistentModelIndex, QVariantAnimation*> _chevronAnimations;
        bool _isIndentGuidesVisible{true};
        bool _isFocusVisible{false};
};

#endif // SHADTREEVIEWPRIVATE_H
