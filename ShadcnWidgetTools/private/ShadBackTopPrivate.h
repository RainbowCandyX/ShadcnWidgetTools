#ifndef SHADBACKTOPPRIVATE_H
#define SHADBACKTOPPRIVATE_H

#include <QList>
#include <QMetaObject>
#include <QPointer>

class QAbstractScrollArea;
class QGraphicsOpacityEffect;
class QVariantAnimation;
class QWidget;
class ShadBackTop;

class ShadBackTopPrivate
{
    public:
        ShadBackTopPrivate();
        ~ShadBackTopPrivate();

        static constexpr int kDefaultVisibilityHeight = 400;
        static constexpr int kDefaultOffset = 24;
        static constexpr int kDefaultDurationMs = 450;
        static constexpr int kFadeDurationMs = 150;

        void attach(QAbstractScrollArea* target);
        void detach();
        void updatePosition();
        void updateVisibility();
        void setShown(bool shown);
        void retranslate();

        ShadBackTop* q{nullptr};
        QPointer<QAbstractScrollArea> _target;
        QPointer<QWidget> _viewport;
        QList<QMetaObject::Connection> _targetConnections;
        QGraphicsOpacityEffect* _opacityEffect{nullptr};
        QVariantAnimation* _fadeAnimation{nullptr};
        QVariantAnimation* _scrollAnimation{nullptr};
        int _visibilityHeight{kDefaultVisibilityHeight};
        int _offset{kDefaultOffset};
        int _durationMs{kDefaultDurationMs};
        bool _isShown{false};
        bool _isAnimatingScroll{false};
};

#endif // SHADBACKTOPPRIVATE_H
