#ifndef SHADDIFFVIEWPRIVATE_H
#define SHADDIFFVIEWPRIVATE_H

#include <QSize>

#include "ShadProperty.h"

class ShadDiffContent;
class ShadDiffView;
class ShadScrollArea;

class ShadDiffViewPrivate
{
    SHAD_D_CREATE(ShadDiffView)

    public:
        ShadDiffViewPrivate();
        ~ShadDiffViewPrivate();

        static constexpr int kBorderWidth = 1;

        void updateLayout();

        ShadScrollArea* _scrollArea{nullptr};
        ShadDiffContent* _content{nullptr};
        int _maximumContentHeight{0};
        bool _isUpdatingLayout{false};
        int _preferredHeight{0};
        QSize _lastSizeHint;
};

#endif // SHADDIFFVIEWPRIVATE_H
