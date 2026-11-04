#ifndef SHADFIELDPRIVATE_H
#define SHADFIELDPRIVATE_H

#include <QMetaObject>
#include <QPointer>
#include <QStringList>

#include "ShadDef.h"
#include "ShadProperty.h"

class QBoxLayout;
class QVBoxLayout;
class QWidget;
class ShadField;
class ShadLabel;

class ShadFieldPrivate
{
    SHAD_D_CREATE(ShadField)

    public:
        ShadFieldPrivate();
        ~ShadFieldPrivate();

        // gap-3
        static constexpr int kGap = 12;
        // FieldContent 的 gap-1
        static constexpr int kContentGap = 4;
        // 选择卡片的 p-3 与 1px 边框
        static constexpr int kCardPadding = 12;
        static constexpr int kCardBorder = 1;
        // Responsive 横排的宽度阈值，对应 @md 容器断点（28rem）
        static constexpr int kResponsiveBreakpoint = 448;

        // 当前是否横排（Responsive 按宽度决定）
        bool isHorizontal() const;
        // 按方向、控件位置与各部分是否为空重建布局
        void rebuildLayout();
        // 错误文字：一条时直接显示，多条时每条前加圆点
        void updateErrorText();
        // 标签颜色与控件的 invalid 属性
        void applyInvalid();
        // 选择卡片的外边距（焦点环 + 边框 + 内边距）
        void applyMargins();
        // 选择卡片中的控件是否为选中状态
        bool isControlChecked() const;

        QBoxLayout* _layout{nullptr};
        // 横排时标签、说明与错误所在的一列
        QWidget* _content{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        ShadLabel* _label{nullptr};
        ShadLabel* _description{nullptr};
        ShadLabel* _error{nullptr};
        QPointer<QWidget> _control;
        QMetaObject::Connection _toggledConnection;
        QStringList _errors;
        ShadFieldType::Orientation _orientation{ShadFieldType::Vertical};
        ShadFieldType::ControlPosition _controlPosition{ShadFieldType::End};
        bool _isInvalid{false};
        bool _isChoiceCard{false};
        bool _isHovered{false};
        // 控件通过键盘获得焦点（focus-visible）
        bool _isControlFocusVisible{false};
        // 上次布局时是否横排，Responsive 宽度变化跨过阈值时重建
        bool _isLaidOutHorizontal{false};
};

#endif // SHADFIELDPRIVATE_H
