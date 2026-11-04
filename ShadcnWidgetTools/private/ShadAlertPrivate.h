#ifndef SHADALERTPRIVATE_H
#define SHADALERTPRIVATE_H

#include <QColor>
#include <QMargins>
#include <QPointer>

#include "ShadDef.h"
#include "ShadIconType.h"

class QLabel;
class QWidget;
class ShadAlert;

class ShadAlertPrivate
{
    SHAD_D_CREATE(ShadAlert)

    public:
        ShadAlertPrivate();
        ~ShadAlertPrivate();

        // 某个主题模式下的自定义颜色，无效颜色表示使用变体颜色
        struct CustomColors
        {
            QColor background;
            QColor border;
            QColor foreground;
        };

        // 当前主题下的背景、边框、标题（图标）与描述颜色
        QColor backgroundColor() const;
        QColor borderColor() const;
        QColor foregroundColor() const;
        QColor descriptionColor() const;
        const CustomColors& currentCustomColors() const;

        // 刷新文字颜色与可见性，并按图标与操作组件重新排版
        void updateContent();
        void updateActionGeometry();
        // 文字区域相对卡片边缘的逻辑距离（left 为起始侧，RTL 时对应右侧）：起始侧含图标宽度，结束侧有操作组件时为 pr-18
        QMargins textMargins() const;
        // 文字区域宽度为 textWidth 时标题与描述的总高度
        int textHeightForWidth(int textWidth) const;
        // 按当前宽度放置标题与描述
        void layoutLabels();

        ShadAlertType::Variant _variant{ShadAlertType::Default};
        ShadIconType::IconName _icon{ShadIconType::None};
        QLabel* _titleLabel{nullptr};
        QLabel* _descriptionLabel{nullptr};
        QPointer<QWidget> _action;
        CustomColors _lightColors;
        CustomColors _darkColors;
};

#endif // SHADALERTPRIVATE_H
