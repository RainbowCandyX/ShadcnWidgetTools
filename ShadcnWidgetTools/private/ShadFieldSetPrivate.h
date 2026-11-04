#ifndef SHADFIELDSETPRIVATE_H
#define SHADFIELDSETPRIVATE_H

#include "ShadDef.h"
#include "ShadProperty.h"

class QBoxLayout;
class QSpacerItem;
class QVBoxLayout;
class QWidget;
class ShadFieldSet;
class ShadLabel;

class ShadFieldSetPrivate
{
    SHAD_D_CREATE(ShadFieldSet)

    public:
        ShadFieldSetPrivate();
        ~ShadFieldSetPrivate();

        // 标题下方 mb-3，说明 -mt-1.5，内容区与上方 gap-6
        static constexpr int kLegendGap = 12;
        static constexpr int kDescriptionGap = 6;
        static constexpr int kContentGap = 24;

        // 标题字号随样式变化
        void applyLegendFont();
        // 按标题与说明是否为空设置可见性与间距
        void updateHeader();

        QBoxLayout* _layout{nullptr};
        // 标题之后、说明之后的间距
        QSpacerItem* _legendSpacer{nullptr};
        QSpacerItem* _descriptionSpacer{nullptr};
        ShadLabel* _legendLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QWidget* _content{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        ShadFieldType::LegendVariant _legendVariant{ShadFieldType::Legend};
};

#endif // SHADFIELDSETPRIVATE_H
