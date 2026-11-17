#ifndef SHADANCHORPRIVATE_H
#define SHADANCHORPRIVATE_H

#include <QFont>
#include <QList>
#include <QMetaObject>
#include <QPoint>
#include <QPointer>
#include <QRectF>
#include <QString>
#include <QWidget>

#include "ShadDef.h"

class QAbstractScrollArea;
class QVariantAnimation;
class ShadAnchor;

class ShadAnchorPrivate
{
    SHAD_D_CREATE(ShadAnchor)

    public:
        ShadAnchorPrivate();
        ~ShadAnchorPrivate();

        static constexpr int kRowHeight = 28;
        static constexpr int kFontPixelSize = 14;
        static constexpr int kRailWidth = 2;
        static constexpr int kTextPadding = 12;
        static constexpr int kLevelIndent = 12;
        static constexpr int kEndPadding = 4;
        static constexpr int kIndicatorInset = 4;
        static constexpr int kIndicatorDurationMs = 200;
        static constexpr int kDefaultDurationMs = 450;
        static constexpr int kMinTextWidth = 48;

        struct Link
        {
            QString title;
            QPointer<QWidget> target;
            int level{0};
        };

        QFont textFont() const;
        QRectF rowRect(int index) const;
        QRectF textRect(int index) const;
        int indexAt(const QPoint& pos) const;
        bool targetTop(int index, int* top) const;
        int computeSpyIndex() const;
        void updateFromScroll();
        void setCurrent(int index);
        void scrollToLink(int index);
        void activate(int index);
        void connectScrollArea();
        void disconnectScrollArea();

        QList<Link> _links;
        QPointer<QAbstractScrollArea> _scrollArea;
        QList<QMetaObject::Connection> _scrollConnections;
        QVariantAnimation* _indicatorAnimation{nullptr};
        QVariantAnimation* _scrollAnimation{nullptr};
        int _currentIndex{-1};
        qreal _indicatorPosition{0.0};
        int _hoverIndex{-1};
        int _pressedIndex{-1};
        int _focusIndex{0};
        bool _isFocusVisible{false};
        int _offset{0};
        int _durationMs{kDefaultDurationMs};
        bool _isAnimatingScroll{false};
};

#endif // SHADANCHORPRIVATE_H
