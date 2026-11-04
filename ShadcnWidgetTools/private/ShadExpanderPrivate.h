#ifndef SHADEXPANDERPRIVATE_H
#define SHADEXPANDERPRIVATE_H

#include <QList>
#include <QtGlobal>

#include "ShadProperty.h"

class QVBoxLayout;
class QWidget;
class ShadCollapsePanel;
class ShadExpander;
class ShadExpanderHeader;
class ShadExpanderItem;
class ShadFrameAnimation;

class ShadExpanderPrivate
{
    SHAD_D_CREATE(ShadExpander)

    public:
        ShadExpanderPrivate();
        ~ShadExpanderPrivate();

        static constexpr int kBorderWidth = 1;
        static constexpr int kDefaultDuration = 200;

        void applyOpen(bool isAnimated);
        void setProgress(qreal progress);

        ShadExpanderHeader* _header{nullptr};
        ShadCollapsePanel* _panel{nullptr};
        QWidget* _itemsWidget{nullptr};
        QVBoxLayout* _itemsLayout{nullptr};
        QList<ShadExpanderItem*> _items;
        ShadFrameAnimation* _animation{nullptr};
        bool _isOpen{false};
        int _duration{kDefaultDuration};
};

#endif // SHADEXPANDERPRIVATE_H
