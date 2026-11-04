#ifndef SHADSTATISTICPRIVATE_H
#define SHADSTATISTICPRIVATE_H

#include <QColor>
#include <QDateTime>
#include <QFont>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QTimer;
class QVariantAnimation;
class ShadStatistic;

class ShadStatisticPrivate
{
    SHAD_D_CREATE(ShadStatistic)

    public:
        ShadStatisticPrivate();
        ~ShadStatisticPrivate();

        static constexpr int kTextSize = 14;
        static constexpr int kTitleLineHeight = 20;
        static constexpr int kIconSize = 16;
        static constexpr int kTitleIconGap = 6;
        static constexpr int kRowGap = 4;
        static constexpr int kSegmentGap = 4;

        struct RowLayout
        {
            QRectF titleIconRect;
            QRectF titleTextRect;
            QRectF trendIconRect;
            QRectF prefixIconRect;
            QRectF prefixRect;
            QRectF valueRect;
            QRectF suffixRect;
            qreal titleHeight{0.0};
            qreal valueHeight{0.0};
            qreal baseline{0.0};
            qreal width{0.0};
            qreal height{0.0};
        };

        QFont titleFont() const;
        QFont valueFont() const;
        QFont affixFont() const;
        QString formatValue(double value) const;
        QString displayText() const;
        QString countdownText(qint64 remainingSeconds) const;
        QColor upColor() const;
        QColor valueColor() const;
        ShadIconType::IconName trendIcon() const;
        int trendIconSize() const;
        RowLayout computeRows(const QString& text) const;
        QSize computeSize() const;
        void refresh();
        void startValueAnimation(double target);
        void stopCountdown();
        void tickCountdown();
        void scheduleCountdown();

        QString _title;
        ShadIconType::IconName _titleIcon{ShadIconType::None};
        QString _prefix;
        QString _suffix;
        ShadIconType::IconName _prefixIcon{ShadIconType::None};
        ShadStatisticType::Trend _trend{ShadStatisticType::None};
        double _value{0.0};
        double _displayValue{0.0};
        int _precision{0};
        int _valueFontSize{24};
        bool _isGroupSeparatorShown{true};
        int _animationDuration{800};
        QVariantAnimation* _valueAnimation{nullptr};

        bool _isCountdownMode{false};
        bool _isCountdownActive{false};
        QDateTime _countdownTarget;
        QString _countdownFormat;
        QString _countdownText;
        QTimer* _countdownTimer{nullptr};
};

#endif
