#ifndef SHADEMPTYPRIVATE_H
#define SHADEMPTYPRIVATE_H

#include <QPointer>
#include <QRect>
#include <QSize>

#include "ShadDef.h"
#include "ShadIconType.h"

class QLabel;
class QVBoxLayout;
class QWidget;
class ShadEmpty;

class ShadEmptyPrivate
{
    SHAD_D_CREATE(ShadEmpty)

    public:
        ShadEmptyPrivate();
        ~ShadEmptyPrivate();

        // 标题、描述与操作区的最大宽度
        static constexpr int kMaxContentWidth = 384;

        // 指定宽度下各部分的位置，未做垂直居中
        struct LayoutResult
        {
            QRect mediaRect;
            QRect titleRect;
            QRect descriptionRect;
            QRect contentRect;
            int totalHeight{0};
        };

        // 边框加内边距
        int padding() const;
        // 媒体区尺寸，没有媒体时为空
        QSize mediaSize() const;
        bool hasTitle() const;
        bool hasDescription() const;
        // 操作区有可见内容
        bool hasContent() const;
        int contentHeightForWidth(int width) const;
        LayoutResult computeLayout(int width) const;

        void updateColors();
        // 链接悬停状态，影响链接颜色
        void setLinkHovered(bool hovered);
        // 刷新可见性并重新排版
        void updateContent();
        // 按当前尺寸放置各部分并垂直居中
        void layoutChildren();

        ShadEmptyType::MediaVariant _mediaVariant{ShadEmptyType::Icon};
        ShadIconType::IconName _icon{ShadIconType::None};
        QPointer<QWidget> _mediaWidget;
        QLabel* _titleLabel{nullptr};
        QLabel* _descriptionLabel{nullptr};
        QWidget* _contentWidget{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        bool _isBorderVisible{false};
        bool _isLinkHovered{false};
        // 图标媒体的绘制区域
        QRect _mediaRect;
};

#endif // SHADEMPTYPRIVATE_H
