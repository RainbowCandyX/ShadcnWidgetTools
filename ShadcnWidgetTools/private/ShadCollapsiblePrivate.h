#ifndef SHADCOLLAPSIBLEPRIVATE_H
#define SHADCOLLAPSIBLEPRIVATE_H

#include <QMetaObject>
#include <QPointer>

#include "ShadProperty.h"

class QAbstractButton;
class QVBoxLayout;
class QWidget;
class ShadCollapsePanel;
class ShadCollapsible;
class ShadFrameAnimation;

class ShadCollapsiblePrivate
{
    SHAD_D_CREATE(ShadCollapsible)

    public:
        ShadCollapsiblePrivate();
        ~ShadCollapsiblePrivate();

        // gap-2
        static constexpr int kGap = 8;
        static constexpr int kDefaultDuration = 200;

        void applyOpen(bool isAnimated);

        QWidget* _headerWidget{nullptr};
        QVBoxLayout* _headerLayout{nullptr};
        ShadCollapsePanel* _panel{nullptr};
        QWidget* _contentWidget{nullptr};
        QVBoxLayout* _contentLayout{nullptr};
        ShadFrameAnimation* _animation{nullptr};
        QPointer<QAbstractButton> _trigger;
        QMetaObject::Connection _triggerConnection;
        bool _isOpen{false};
        int _duration{kDefaultDuration};
};

#endif // SHADCOLLAPSIBLEPRIVATE_H
