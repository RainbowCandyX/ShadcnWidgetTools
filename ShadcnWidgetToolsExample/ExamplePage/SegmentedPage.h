#ifndef SEGMENTEDPAGE_H
#define SEGMENTEDPAGE_H

#include "ExampleBasePage.h"

class SegmentedPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit SegmentedPage(QWidget* parent = nullptr);
        ~SegmentedPage() override;
};

#endif // SEGMENTEDPAGE_H
