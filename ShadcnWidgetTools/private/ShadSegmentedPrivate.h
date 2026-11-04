#ifndef SHADSEGMENTEDPRIVATE_H
#define SHADSEGMENTEDPRIVATE_H

#include <QList>
#include <QRectF>
#include <QString>

#include "ShadDef.h"
#include "ShadIconType.h"
#include "ShadProperty.h"

class QVariantAnimation;
class ShadSegmented;
class ShadTransition;

class ShadSegmentedPrivate
{
    SHAD_D_CREATE(ShadSegmented)

    public:
        struct Item
        {
            ShadIconType::IconName icon{ShadIconType::None};
            QString text;
            QString toolTip;
            bool isEnabled{true};
            ShadTransition* hoverTransition{nullptr};
            ShadTransition* activeTransition{nullptr};
        };

        ShadSegmentedPrivate();
        ~ShadSegmentedPrivate();

        QList<Item> _items;
        int _currentIndex{-1};
        int _hoverIndex{-1};
        int _pressedIndex{-1};
        bool _isBlock{false};
        bool _isFocusVisible{false};
        bool _isPositioned{false};
        ShadSegmentedType::Size _size{ShadSegmentedType::Default};
        QRectF _blockRect;
        QVariantAnimation* _slideAnimation{nullptr};
};

#endif // SHADSEGMENTEDPRIVATE_H
