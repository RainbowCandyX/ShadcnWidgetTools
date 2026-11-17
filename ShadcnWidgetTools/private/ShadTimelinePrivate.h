#ifndef SHADTIMELINEPRIVATE_H
#define SHADTIMELINEPRIVATE_H

#include <QList>
#include <QRect>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"

class QVariantAnimation;
class QVBoxLayout;
class QWidget;
class ShadLabel;
class ShadTimeline;
class ShadTimelineItem;

class ShadTimelineItemPrivate
{
    SHAD_D_CREATE(ShadTimelineItem)

    public:
        ShadTimelineItemPrivate();
        ~ShadTimelineItemPrivate();

        static constexpr int kAxisWidth = 24;
        static constexpr int kAxisGap = 12;
        static constexpr int kItemGap = 24;
        static constexpr int kDotSize = 10;
        static constexpr int kIconSize = 16;
        static constexpr int kLineWidth = 2;
        static constexpr int kLineGap = 4;
        static constexpr int kTitleLineHeight = 20;
        static constexpr int kTimeLineHeight = 16;
        static constexpr int kTitleFontSize = 14;
        static constexpr int kTimeFontSize = 12;
        static constexpr int kTextFontSize = 14;
        static constexpr int kTextGap = 4;
        static constexpr int kContentGap = 8;
        static constexpr int kMaxNaturalTextWidth = 320;
        static constexpr int kMinTextWidth = 64;

        struct LayoutResult
        {
            QRect timeRect;
            QRect titleRect;
            QRect descriptionRect;
            QRect contentRect;
            qreal axisCenterX{0.0};
            qreal dotCenterY{0.0};
            int totalHeight{0};
        };

        bool hasTime() const;
        bool hasTitle() const;
        bool hasDescription() const;
        bool hasContent() const;
        bool isTimeOpposite() const;
        qreal markerRadius() const;
        int labelHeight(const ShadLabel* label, int width) const;
        int contentHeightForWidth(int width) const;
        int naturalTextWidth() const;
        LayoutResult computeLayout(int width) const;

        void updateContent();
        void layoutChildren();
        void setPlacement(ShadTimelineType::Mode mode, bool isContentAtStart, bool hasLineAbove, bool hasLineBelow, bool isLineAboveDashed, bool isLineBelowDashed);
        void setPending(bool pending);
        void updateSpin();

        ShadLabel* _timeLabel{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QWidget* _contentWidget{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QVariantAnimation* _spinAnimation{nullptr};
        ShadIconType::IconName _icon{ShadIconType::None};
        ShadThemeType::ThemeColor _color{ShadThemeType::Primary};
        ShadTimelineType::Mode _mode{ShadTimelineType::Start};
        bool _isContentAtStart{false};
        bool _hasLineAbove{false};
        bool _hasLineBelow{false};
        bool _isLineAboveDashed{false};
        bool _isLineBelowDashed{false};
        bool _isPending{false};
        qreal _spinAngle{0.0};
};

class ShadTimelinePrivate
{
    SHAD_D_CREATE(ShadTimeline)

    public:
        ShadTimelinePrivate();
        ~ShadTimelinePrivate();

        void rebuild();

        QVBoxLayout* _layout{nullptr};
        QList<ShadTimelineItem*> _items;
        ShadTimelineItem* _pendingItem{nullptr};
        QString _pendingText;
        ShadTimelineType::Mode _mode{ShadTimelineType::Start};
        bool _isReverse{false};
};

#endif // SHADTIMELINEPRIVATE_H
