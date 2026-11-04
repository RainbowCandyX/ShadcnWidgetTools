#ifndef SHADDRAWERPRIVATE_H
#define SHADDRAWERPRIVATE_H

#include <QElapsedTimer>
#include <QList>
#include <QMetaObject>
#include <QObject>
#include <QPixmap>
#include <QPointF>
#include <QPointer>
#include <QRect>
#include <QRectF>
#include <QtGlobal>

#include "ShadDef.h"
#include "ShadProperty.h"

class QEvent;
class QPainterPath;
class QVBoxLayout;
class QWidget;
class ShadDrawer;
class ShadDrawerOverlay;
class ShadFrameAnimation;
class ShadLabel;

// 只用于事件过滤，不需要 Q_OBJECT
class ShadDrawerPrivate : public QObject
{
    SHAD_D_CREATE(ShadDrawer)

    public:
        ShadDrawerPrivate();
        ~ShadDrawerPrivate() override;

        static constexpr int kPadding = 16;
        static constexpr int kHeaderGap = 6;
        static constexpr int kNarrowHeaderGap = 2;
        static constexpr int kFooterGap = 8;
        static constexpr int kWideBreakpoint = 768;
        static constexpr int kVerticalMargin = 96;
        static constexpr qreal kSideRatio = 0.75;
        static constexpr int kSideBreakpoint = 640;
        static constexpr int kSideWidth = 384;
        static constexpr int kBorderWidth = 1;
        static constexpr int kHandleArea = 12;
        static constexpr int kHandleLength = 100;
        static constexpr int kHandleThickness = 6;
        static constexpr int kBleed = 48;
        static constexpr int kClosedGap = 2;
        static constexpr int kOpenDurationMs = 450;
        static constexpr int kCloseDurationMs = 400;
        static constexpr qreal kMinCloseStrength = 0.25;
        static constexpr qreal kStackStep = 0.05;
        static constexpr int kStackPeek = 16;
        static constexpr qreal kStackDim = 0.05;
        static constexpr qreal kDismissRatio = 0.4;
        // 像素每毫秒
        static constexpr qreal kDismissVelocity = 0.5;
        static constexpr int kProjectionMs = 150;
        static constexpr int kVelocityTimeoutMs = 50;
        static constexpr qreal kSnapOverlayMinLevel = 0.5;

        bool isVertical() const;
        bool hasSnapPoints() const;
        QWidget* resolveHost() const;
        // 按打开顺序
        static QList<ShadDrawer*> visibleStack(QWidget* host);
        static ShadDrawerOverlay* findOverlay(QWidget* host);
        static void restack(QWidget* host);
        bool isTopmost() const;

        void attachHost(QWidget* host);
        void detachHost();
        void updateHeader();
        void updateHeaderStyle();
        void updateExtent();
        qreal closedOffset() const;
        qreal snapOffset(qreal snapPoint) const;
        qreal restingOffset() const;
        qreal minimumOffset() const;
        qreal openness() const;
        qreal overlayLevel() const;
        void place();
        // 同时同步遮罩与叠放
        void refresh();
        static void syncStack(QWidget* host);
        static void syncOverlay(QWidget* host);
        void setStack(qreal level, qreal frontExtent);
        void animateTo(qreal offset, int duration);
        void requestClose();
        void finishClose();
        void settle();
        void setCurrentSnapPoint(qreal snapPoint);
        void focusFirstChild();
        QList<QWidget*> focusableChildren() const;
        QPainterPath panelPath(const QRectF& panel, qreal radius) const;
        QPainterPath innerEdgePath(const QRectF& panel, qreal radius) const;

        bool eventFilter(QObject* watched, QEvent* event) override;

        QPointer<QWidget> _anchor;
        QPointer<QWidget> _host;
        QWidget* _body{nullptr};
        QVBoxLayout* _bodyLayout{nullptr};
        QWidget* _header{nullptr};
        QVBoxLayout* _headerLayout{nullptr};
        ShadLabel* _titleLabel{nullptr};
        ShadLabel* _descriptionLabel{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        QWidget* _footer{nullptr};
        QVBoxLayout* _footerLayout{nullptr};
        ShadFrameAnimation* _animation{nullptr};

        ShadDrawerType::SwipeDirection _direction{ShadDrawerType::Down};
        bool _isHandleVisible{false};
        bool _isModal{true};
        bool _isPointerDismissal{true};
        int _preferredSize{0};
        QList<qreal> _snapPoints;
        qreal _snapPoint{0.0};
        bool _isSequentialSnap{false};

        bool _isOpen{false};
        quint64 _openSerial{0};
        qreal _extent{0.0};
        // 朝关闭方向的位移，负值为越界拖动
        qreal _offset{0.0};
        bool _isOpening{false};
        bool _isClosing{false};
        qreal _animationStart{0.0};
        qreal _animationEnd{0.0};
        qreal _appear{1.0};
        qreal _closeStartLevel{0.0};
        qreal _closeStartOffset{0.0};

        qreal _stackLevel{0.0};
        qreal _stackExtent{0.0};
        QPixmap _bodySnapshot;
        QRectF _snapshotRect;
        QRect _panelRect;
        qreal _panelScale{1.0};

        bool _isPressed{false};
        bool _isSwiping{false};
        QPointF _pressPos;
        qreal _pressOffset{0.0};
        qreal _lastOffset{0.0};
        qint64 _lastTime{0};
        qreal _velocity{0.0};
        QElapsedTimer _clock;

        QMetaObject::Connection _overlayConnection;
        QPointer<QWidget> _focusBeforeOpen;
        // 忽略同一次外部点击触发的 open
        QElapsedTimer _outsideDismissClock;

        static quint64 s_openSerial;
};

#endif // SHADDRAWERPRIVATE_H
