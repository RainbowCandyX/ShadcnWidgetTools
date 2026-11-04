#ifndef SHADSTACKEDWIDGETPRIVATE_H
#define SHADSTACKEDWIDGETPRIVATE_H

#include <QPointer>

#include "ShadProperty.h"

class QWidget;
class ShadPageTransition;
class ShadStackedWidget;

class ShadStackedWidgetPrivate
{
    SHAD_D_CREATE(ShadStackedWidget)

    public:
        ShadStackedWidgetPrivate();
        ~ShadStackedWidgetPrivate();

        ShadPageTransition* _transition{nullptr};
        QPointer<QWidget> _shownWidget;
};

#endif // SHADSTACKEDWIDGETPRIVATE_H
