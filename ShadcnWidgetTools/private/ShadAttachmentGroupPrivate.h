#ifndef SHADATTACHMENTGROUPPRIVATE_H
#define SHADATTACHMENTGROUPPRIVATE_H

#include <QList>
#include <QPointer>

#include "ShadProperty.h"

class QGraphicsOpacityEffect;
class QHBoxLayout;
class QScrollArea;
class QTimer;
class QVariantAnimation;
class QWheelEvent;
class QWidget;
class ShadAttachment;
class ShadAttachmentGroup;

class ShadAttachmentGroupPrivate
{
    SHAD_D_CREATE(ShadAttachmentGroup)

    public:
        ShadAttachmentGroupPrivate();
        ~ShadAttachmentGroupPrivate();

        // 滚动停止多久后吸附，以及吸附动画时长
        static constexpr int kSnapDelayMs = 150;
        static constexpr int kSnapDurationMs = 200;
        // scroll-px-1：吸附时附件起始边与视口起始边保持 4px
        static constexpr int kScrollPadding = 4;

        // 去掉已被外部删除的附件
        void pruneAttachments();
        // 内容尺寸取布局的理想尺寸，附件不会被压缩
        void updateContentGeometry();
        // 附件在内容中的逻辑起始位置（起始侧为 0，RTL 时从右侧算起），含焦点环边距
        int logicalLeft(const QWidget* widget) const;
        // 对齐到离当前位置最近的吸附点
        void snapToNearest();
        void animateTo(int value);
        // 按滚动位置更新两端的渐隐遮罩
        void updateFade();
        // 竖向滚轮转换为横向滚动，返回是否已处理
        bool handleWheel(QWheelEvent* event);

        QScrollArea* _scrollArea{nullptr};
        QWidget* _content{nullptr};
        QHBoxLayout* _layout{nullptr};
        QList<QPointer<ShadAttachment>> _attachments;
        QGraphicsOpacityEffect* _fadeEffect{nullptr};
        QTimer* _snapTimer{nullptr};
        QVariantAnimation* _snapAnimation{nullptr};
        // 吸附动画设置的滚动值不再触发吸附
        bool _isSnapping{false};
};

#endif // SHADATTACHMENTGROUPPRIVATE_H
