#ifndef SHADNAVIGATIONMENULINKPRIVATE_H
#define SHADNAVIGATIONMENULINKPRIVATE_H

#include "ShadProperty.h"

class ShadItemContent;
class ShadNavigationMenuLink;

class ShadNavigationMenuLinkPrivate
{
    SHAD_D_CREATE(ShadNavigationMenuLink)

    public:
        ShadNavigationMenuLinkPrivate();
        ~ShadNavigationMenuLinkPrivate();

        // p-2
        static constexpr int kPadding = 8;
        static constexpr int kPreferredWidth = 220;

        ShadItemContent* _content{nullptr};
        bool _isHovered{false};
        bool _isFocusVisible{false};
};

#endif // SHADNAVIGATIONMENULINKPRIVATE_H
