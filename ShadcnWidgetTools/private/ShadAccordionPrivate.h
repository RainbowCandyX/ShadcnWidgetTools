#ifndef SHADACCORDIONPRIVATE_H
#define SHADACCORDIONPRIVATE_H

#include <QList>
#include <QString>
#include <QStringList>

#include "ShadDef.h"
#include "ShadProperty.h"

class QVBoxLayout;
class QWidget;
class ShadAccordion;
class ShadAccordionPanel;
class ShadAccordionTrigger;
class ShadFrameAnimation;

class ShadAccordionPrivate
{
    SHAD_D_CREATE(ShadAccordion)

    public:
        ShadAccordionPrivate();
        ~ShadAccordionPrivate();

        static constexpr int kDefaultDurationMs = 200;
        static constexpr int kMaxDurationMs = 1000;
        static constexpr int kBorderedPadding = 16;
        static constexpr int kSeparatorWidth = 1;

        struct Item
        {
            QString value;
            QWidget* container{nullptr};
            ShadAccordionTrigger* trigger{nullptr};
            ShadAccordionPanel* panel{nullptr};
            ShadFrameAnimation* animation{nullptr};
            bool isExpanded{false};
        };

        Item* findItem(const QString& value);
        const Item* findItem(const QString& value) const;
        bool setExpanded(Item& item, bool expanded);
        void applyExpanded(const QStringList& values);
        void toggle(const QString& value);
        void updateItemMargins();
        void moveFocus(int key);

        QVBoxLayout* _layout{nullptr};
        QList<Item> _items;
        bool _isMultiple{false};
        ShadAccordionType::Variant _variant{ShadAccordionType::Default};
        int _duration{kDefaultDurationMs};
};

#endif // SHADACCORDIONPRIVATE_H
