#ifndef SHADSTEPSPRIVATE_H
#define SHADSTEPSPRIVATE_H

#include <QFont>
#include <QLineF>
#include <QList>
#include <QPoint>
#include <QRectF>
#include <QString>
#include <QtCore/qnamespace.h>

#include "ShadDef.h"
#include "ShadIconType.h"

class ShadSteps;

class ShadStepsPrivate
{
    SHAD_D_CREATE(ShadSteps)

    public:
        ShadStepsPrivate();
        ~ShadStepsPrivate();

        static constexpr int kIndicatorSize = 24;
        static constexpr int kNumberFontSize = 12;
        static constexpr int kIconSize = 14;
        static constexpr int kTextFontSize = 14;
        static constexpr int kLineHeight = 20;
        static constexpr int kTextGap = 4;
        static constexpr int kHorizontalTextGap = 8;
        static constexpr int kLineGap = 8;
        static constexpr int kMinLineLength = 24;
        static constexpr int kPreferredLineLength = 48;
        static constexpr int kVerticalTextGap = 12;
        static constexpr int kVerticalStepGap = 24;
        static constexpr int kVerticalLineGap = 4;
        static constexpr int kLineWidth = 2;
        static constexpr int kMaxNaturalTextWidth = 320;
        static constexpr int kMinTextWidth = 64;

        struct Step
        {
            QString title;
            QString description;
            ShadIconType::IconName icon{ShadIconType::None};
            bool isDisabled{false};
        };

        struct StepLayout
        {
            QRectF indicator;
            QRectF title;
            QRectF description;
            QRectF hit;
            QLineF line;
            QString titleText;
            QString descriptionText;
        };

        struct LayoutResult
        {
            QList<StepLayout> steps;
            int height{0};
        };

        QFont titleFont() const;
        QFont descriptionFont() const;
        QFont numberFont() const;
        int naturalTextWidth(int index) const;
        int wrappedHeight(const QFont& font, const QString& text, int width) const;
        bool hasDescription() const;
        LayoutResult computeLayout(int width) const;
        LayoutResult computeHorizontalLayout(int width) const;
        LayoutResult computeVerticalLayout(int width) const;
        int indexAt(const QPoint& pos) const;
        bool isSelectable(int index) const;
        void activate(int index);
        void moveFocus(int step);
        void updateFocusPolicy();

        QList<Step> _steps;
        int _currentIndex{-1};
        ShadStepsType::Status _status{ShadStepsType::Process};
        Qt::Orientation _orientation{Qt::Horizontal};
        bool _isClickable{false};
        int _hoverIndex{-1};
        int _pressedIndex{-1};
        int _focusIndex{0};
        bool _isFocusVisible{false};
};

#endif // SHADSTEPSPRIVATE_H
