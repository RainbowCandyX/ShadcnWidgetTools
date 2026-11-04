#ifndef SHADINPUTPRIVATE_H
#define SHADINPUTPRIVATE_H

#include <QPoint>
#include <QRectF>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"

class QPainter;
class ShadInput;

class ShadInputPrivate
{
    SHAD_D_CREATE(ShadInput)

    public:
        ShadInputPrivate();
        ~ShadInputPrivate();

        // 根据当前主题刷新文字、占位符与选中颜色，并按图标与尾部按钮计算左右内边距
        void applyThemeColors();
        QRectF leadingIconRect() const;
        QRectF trailingButtonRect() const;
        // 尾部按钮当前是否可见且可点击
        bool isTrailingVisible() const;
        // 是否嵌入在 ShadInputGroup 中：不绘制背景、边框与焦点环，也不预留焦点环边距
        bool isEmbedded() const;
        // 文字区域距组件边缘的距离：独立时为 1px 边框 + 焦点环边距，嵌入时为 0
        int bodyInset() const;
        ShadIconType::IconName trailingIcon() const;
        QString trailingToolTip() const;
        void triggerTrailingAction();
        void updateTrailingHover(const QPoint& pos);
        void paintDecorations(QPainter* painter) const;

        bool _isInvalid{false};
        ShadIconType::IconName _leadingIcon{ShadIconType::None};
        ShadInputType::TrailingAction _trailingAction{ShadInputType::NoAction};
        bool _isTrailingHovered{false};
        bool _isTrailingPressed{false};
        bool _isPasswordVisible{false};
};

#endif // SHADINPUTPRIVATE_H
