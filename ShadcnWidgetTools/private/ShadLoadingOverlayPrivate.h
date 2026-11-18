#ifndef SHADLOADINGOVERLAYPRIVATE_H
#define SHADLOADINGOVERLAYPRIVATE_H

#include <QPointer>
#include <QWidget>
#include <QtGlobal>

#include "ShadProperty.h"

class QGraphicsOpacityEffect;
class QTimer;
class QVariantAnimation;
class ShadLabel;
class ShadLoadingOverlay;
class ShadSpinner;

class ShadLoadingOverlayPrivate
{
    SHAD_D_CREATE(ShadLoadingOverlay)

    public:
        ShadLoadingOverlayPrivate();
        ~ShadLoadingOverlayPrivate();

        static constexpr int kFadeDurationMs = 150;
        static constexpr int kDefaultSpinnerSize = 24;
        static constexpr int kSpacing = 8;
        static constexpr qreal kMaskOpacity = 0.7;

        void attach(QWidget* target);
        void detach();
        void syncGeometry();
        void setShown(bool shown);
        void handleFocusChanged(QWidget* now);

        QPointer<QWidget> _target;
        QPointer<QWidget> _previousFocus;
        ShadSpinner* _spinner{nullptr};
        ShadLabel* _label{nullptr};
        QGraphicsOpacityEffect* _opacityEffect{nullptr};
        QVariantAnimation* _fadeAnimation{nullptr};
        QTimer* _delayTimer{nullptr};
        int _delayMs{0};
        int _radius{0};
        bool _isLoading{false};
        bool _isShown{false};
};

#endif // SHADLOADINGOVERLAYPRIVATE_H
