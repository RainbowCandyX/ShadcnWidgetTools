#ifndef SHADRESULTPRIVATE_H
#define SHADRESULTPRIVATE_H

#include <QPointer>
#include <QWidget>

#include "ShadProperty.h"
#include "ShadStyleHelper.h"

class QHBoxLayout;
class QVBoxLayout;
class ShadLabel;
class ShadResult;
class ShadResultIcon;

class ShadResultPrivate
{
    SHAD_D_CREATE(ShadResult)

    public:
        ShadResultPrivate();
        ~ShadResultPrivate();

        static constexpr int kMaxContentWidth = 480;
        static constexpr int kPaddingX = 24;
        static constexpr int kPaddingY = 32;
        static constexpr int kTitleFontSize = 20;
        static constexpr int kTextFontSize = 14;
        static constexpr int kTitleGap = 16;
        static constexpr int kDescriptionGap = 8;
        static constexpr int kSectionGap = 24;
        static constexpr int kContentSpacing = 8;
        static constexpr int kActionSpacing = 8 - 2 * ShadStyleHelper::kFocusRingWidth;
        static constexpr int kColumnStretch = 100;

        QVBoxLayout* _columnLayout{nullptr};
        ShadResultIcon* _icon{nullptr};
        QPointer<QWidget> _mediaWidget;
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QWidget* _contentWidget{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QWidget* _actionsWidget{nullptr};
        QHBoxLayout* _actionsLayout{nullptr};
};

#endif // SHADRESULTPRIVATE_H
