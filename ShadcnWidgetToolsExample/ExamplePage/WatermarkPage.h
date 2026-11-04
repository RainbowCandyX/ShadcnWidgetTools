#ifndef WATERMARKPAGE_H
#define WATERMARKPAGE_H

#include "ExampleBasePage.h"

class WatermarkPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit WatermarkPage(QWidget* parent = nullptr);
        ~WatermarkPage() override;
};

#endif
