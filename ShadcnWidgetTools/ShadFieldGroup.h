#ifndef SHADFIELDGROUP_H
#define SHADFIELDGROUP_H

#include <QString>
#include <QWidget>

#include "ShadProperty.h"

class QChildEvent;
class ShadFieldSeparator;

// 表单项组，对应 shadcn FieldGroup：纵向排列多个表单项，间距 gap-7（28px），可以插入分隔线
// 分隔线上下各收紧 8px（对应 FieldSeparator 的 -my-2）
class ShadFieldGroupPrivate;

class SHADCN_EXPORT ShadFieldGroup : public QWidget
{
    Q_OBJECT
    SHAD_Q_CREATE(ShadFieldGroup)
    Q_PROPERTY(int spacing READ getSpacing WRITE setSpacing)

    public:
        explicit ShadFieldGroup(QWidget* parent = nullptr);
        ~ShadFieldGroup() override;

        // 添加组件（通常为 ShadField 或嵌套的 ShadFieldSet / ShadFieldGroup），表单项组接管父子关系
        void addWidget(QWidget* widget);
        // 添加分隔线，text 为空时只有横线；返回分隔线以便修改文字
        ShadFieldSeparator* addSeparator(const QString& text = QString());
        // 移除组件但不删除，父组件置空，由调用方负责释放
        void removeWidget(QWidget* widget);

        // 相邻组件的间距，默认 28（gap-7）；嵌套在其他组内时常用 16（gap-4）
        void setSpacing(int spacing);
        int getSpacing() const;

    protected:
        void childEvent(QChildEvent* event) override;
};

#endif // SHADFIELDGROUP_H
