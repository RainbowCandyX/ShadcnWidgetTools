#ifndef SHADATTACHMENTGROUP_H
#define SHADATTACHMENTGROUP_H

#include <QSize>
#include <QWidget>

#include "ShadProperty.h"

class QEvent;
class QObject;
class QResizeEvent;
class ShadAttachment;

// 附件分组，对应 shadcn AttachmentGroup：附件横向排列（gap-3 py-1），超出宽度时横向滚动，不显示滚动条（scrollbar-none）
// 滚动停止后自动对齐到最近一个附件的起始边（snap-x snap-mandatory，scroll-px-1）；可滚动的一端边缘渐隐（scroll-fade-x）
// 竖向滚轮转换为横向滚动，已滚到头时交给父组件处理；附件保持自身宽度，不会被压缩（flex-none）
class ShadAttachmentGroupPrivate;

class SHADCN_EXPORT ShadAttachmentGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadAttachmentGroup)

    public:
        explicit ShadAttachmentGroup(QWidget* parent = nullptr);
        ~ShadAttachmentGroup() override;

        // 添加到末尾，分组接管附件的父子关系
        void addAttachment(ShadAttachment* attachment);
        // index 超出范围时添加到末尾
        void insertAttachment(int index, ShadAttachment* attachment);
        // 从分组中移除但不删除，附件的父组件置空，由调用方负责释放
        void removeAttachment(ShadAttachment* attachment);
        // 删除全部附件
        void clear();
        int count() const;
        ShadAttachment* attachmentAt(int index) const;

        // 滚动到使该附件完整可见的吸附位置
        void scrollToAttachment(ShadAttachment* attachment);

        QSize sizeHint() const override;
        QSize minimumSizeHint() const override;

    protected:
        bool eventFilter(QObject* watched, QEvent* event) override;
        void changeEvent(QEvent* event) override;
        void resizeEvent(QResizeEvent* event) override;
};

#endif // SHADATTACHMENTGROUP_H
