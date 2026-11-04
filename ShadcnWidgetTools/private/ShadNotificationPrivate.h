#ifndef SHADNOTIFICATIONPRIVATE_H
#define SHADNOTIFICATIONPRIVATE_H

#include <QColor>
#include <QFont>
#include <QList>
#include <QPoint>
#include <QRect>
#include <QRectF>
#include <QSize>
#include <QString>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QVariantAnimation;
class QWidget;
class ShadButton;
class ShadNotification;

class ShadNotificationPrivate
{
    SHAD_D_CREATE(ShadNotification)

    public:
        ShadNotificationPrivate();
        ~ShadNotificationPrivate();

        static constexpr int kShadowMargin = 20;
        static constexpr int kViewportOffset = 16;
        static constexpr int kMaxCardWidth = 500;
        static constexpr int kMaxTitleWidth = 160;
        static constexpr int kStackGap = 12;
        static constexpr int kPaddingX = 16;
        static constexpr int kPaddingY = 14;
        static constexpr int kIconSize = 16;
        static constexpr int kContentGap = 12;
        static constexpr int kTextGap = 8;
        static constexpr int kLineHeight = 20;
        static constexpr int kButtonSize = 32;
        static constexpr int kCountdownHeight = 3;
        static constexpr int kSlideDistance = 25;
        static constexpr int kEnterDuration = 350;
        static constexpr int kFadeEnterDuration = 250;
        static constexpr int kMoveDuration = 220;
        static constexpr int kExitDuration = 250;

        struct ContentLayout
        {
            QRectF cardRect;
            QRectF iconRect;
            QRectF titleRect;
            QRectF textRect;
            QRectF closeRect;
        };

        static void notify(ShadNotificationType::Type type, const QString& title, const QString& text, int duration, ShadNotificationType::Position position, QWidget* parent);
        static void closeAll(QWidget* parent);
        static void repositionStack(QWidget* host, ShadNotificationType::Position position);
        static void repositionHost(QWidget* host);
        static bool isFirstOnHost(const ShadNotification* notification);
        static int cardRadius();

        void start();
        void close();
        void unregister();
        void updateSize();
        void layoutCloseButton();
        void updateCloseButton();
        void moveTo(const QPoint& target, int duration);
        void startCountdown();
        bool isFadePosition() const;
        bool isBottomPosition() const;
        bool isMiddlePosition() const;
        bool isStartSide() const;
        bool isEndSide() const;
        int titleBarInset() const;
        QRect availableRect() const;
        int titleWidth() const;
        int textHeight(int width) const;
        QSize computeCardSize() const;
        QList<ShadNotification*> stack() const;
        QRect logicalCardRect() const;
        QRect visualCardRect(const QRect& logicalRect) const;
        QPoint widgetPos(const QRect& logicalRect) const;
        QPoint enterStartPos() const;
        bool isOverflowing() const;
        ContentLayout computeGeometry() const;
        QColor statusColor() const;
        ShadIconType::IconName statusIcon() const;

        QWidget* _host{nullptr};
        ShadNotificationType::Type _type{ShadNotificationType::Info};
        ShadNotificationType::Position _position{ShadNotificationType::TopRight};
        QString _title;
        QString _text;
        int _duration{0};
        QSize _cardSize;
        QFont _titleFont;
        QFont _textFont;
        ShadButton* _closeButton{nullptr};
        QVariantAnimation* _moveAnimation{nullptr};
        QVariantAnimation* _opacityAnimation{nullptr};
        QVariantAnimation* _countdownAnimation{nullptr};
        qreal _opacity{1.0};
        qreal _countdownProgress{1.0};
        bool _isPlaced{false};
        bool _isCountdownStarted{false};
        bool _isClosing{false};
};

#endif
