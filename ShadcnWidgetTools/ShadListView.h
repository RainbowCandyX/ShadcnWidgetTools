#ifndef SHADLISTVIEW_H
#define SHADLISTVIEW_H

#include <QListView>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadProperty.h"

class QAbstractItemModel;
class QEvent;
class QFocusEvent;
class QKeyEvent;
class QMouseEvent;
class QResizeEvent;
class ShadEmpty;

// 虚拟列表：继承 QListView，只绘制可见行，适合大量等高行数据
// 行外观与 shadcn Item 一致，支持复选框、空状态与滚动加载
class ShadListViewPrivate;

class SHADCN_EXPORT ShadListView : public QListView
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadListView)
    Q_PROPERTY(ShadItemType::Size itemSize READ getItemSize WRITE setItemSize)
    Q_PROPERTY(bool descriptionVisible READ isDescriptionVisible WRITE setDescriptionVisible)
    Q_PROPERTY(ShadTableType::ImageShape imageShape READ getImageShape WRITE setImageShape)
    Q_PROPERTY(bool loadingMore READ isLoadingMore WRITE setLoadingMore)
    Q_PROPERTY(int loadMoreThreshold READ getLoadMoreThreshold WRITE setLoadMoreThreshold)

    public:
        // 媒体区的 Lucide 图标，优先于 Qt::DecorationRole 的图片
        static constexpr int IconRole = Qt::UserRole + 0x300;
        // 标题下方的描述
        static constexpr int DescriptionRole = Qt::UserRole + 0x301;
        // 结束侧的辅助文字
        static constexpr int TrailingTextRole = Qt::UserRole + 0x302;

        explicit ShadListView(QWidget* parent = nullptr);
        ~ShadListView() override;

        // 行尺寸，默认 Medium
        void setItemSize(ShadItemType::Size size);
        ShadItemType::Size getItemSize() const;

        // 是否显示描述，默认 true
        void setDescriptionVisible(bool visible);
        bool isDescriptionVisible() const;

        // 图片形状，默认 Rounded
        void setImageShape(ShadTableType::ImageShape shape);
        ShadTableType::ImageShape getImageShape() const;

        // 没有行时显示的空状态
        ShadEmpty* emptyState() const;

        // 是否正在加载更多，加载期间在底部显示加载指示
        void setLoadingMore(bool loading);
        bool isLoadingMore() const;

        // 触发 reachedEnd 的距底部距离，默认 200
        void setLoadMoreThreshold(int pixels);
        int getLoadMoreThreshold() const;

        void setModel(QAbstractItemModel* model) override;

    Q_SIGNALS:
        // 滚动到接近底部时发出，每次到底只发出一次
        void reachedEnd();

    protected:
        bool event(QEvent* event) override;
        bool viewportEvent(QEvent* event) override;
        void mouseMoveEvent(QMouseEvent* event) override;
        void mousePressEvent(QMouseEvent* event) override;
        void keyPressEvent(QKeyEvent* event) override;
        void focusInEvent(QFocusEvent* event) override;
        void focusOutEvent(QFocusEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADLISTVIEW_H
