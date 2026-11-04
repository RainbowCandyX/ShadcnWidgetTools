#ifndef SHADCARDPRIVATE_H
#define SHADCARDPRIVATE_H

#include <QPointer>
#include <QWidget>

#include "ShadProperty.h"

class QGridLayout;
class QHBoxLayout;
class QVBoxLayout;
class ShadCard;
class ShadLabel;

class ShadCardPrivate
{
    SHAD_D_CREATE(ShadCard)

    public:
        ShadCardPrivate();
        ~ShadCardPrivate();

        void applyThemeColors();
        // 标题、描述、操作都为空时隐藏 Header
        void updateHeaderVisibility();

        QWidget* _headerWidget{nullptr};
        QGridLayout* _headerLayout{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        // 外部可能提前删除操作组件，使用 QPointer 避免悬空指针
        QPointer<QWidget> _headerAction;
        QWidget* _contentWidget{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QWidget* _footerWidget{nullptr};
        QHBoxLayout* _footerLayout{nullptr};
};

#endif // SHADCARDPRIVATE_H
