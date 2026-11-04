#ifndef SHADSIDEBARCOLLAPSEAREA_H
#define SHADSIDEBARCOLLAPSEAREA_H

#include <QSize>
#include <QWidget>
#include <QtGlobal>

class QEvent;
class QObject;
class QResizeEvent;

// 高度过渡用的裁剪容器，组件库内部使用：内容组件始终保持完整高度并贴顶放置，容器高度为内容高度乘以进度，超出部分被裁掉
// 直接限制带布局组件的最大高度时，布局会把子项一起压扁，过渡期间子项互相重叠；用裁剪代替压缩，子项尺寸始终不变
class ShadSidebarCollapseArea : public QWidget
{
    Q_OBJECT

    public:
        explicit ShadSidebarCollapseArea(QWidget* parent = nullptr);
        ~ShadSidebarCollapseArea() override;

        // 内容组件，在其上设置布局并添加子项
        QWidget* contentWidget() const;

        // 0 为完全折叠（高度 0），1 为完全展开（内容高度）
        void setProgress(qreal progress);
        qreal progress() const;

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;

    private:
        int contentHeight() const;
        int visibleHeight() const;
        // 高度变化后立即重排外层布局，避免外层容器以旧高度绘制一帧
        void syncAncestorLayouts();
        void layoutContent();

        QWidget* _content{nullptr};
        qreal _progress{1.0};
};

#endif // SHADSIDEBARCOLLAPSEAREA_H
