#ifndef HOVERCARDPAGE_H
#define HOVERCARDPAGE_H

#include "ExampleBasePage.h"

class QWidget;

// HoverCard 示例
class HoverCardPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit HoverCardPage(QWidget* parent = nullptr);
        ~HoverCardPage() override;
};

#endif // HOVERCARDPAGE_H
