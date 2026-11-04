#ifndef SCREENSHOTPAGE_H
#define SCREENSHOTPAGE_H

#include "ExampleBasePage.h"

class QWidget;

class ScreenshotPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ScreenshotPage(QWidget* parent = nullptr);
        ~ScreenshotPage() override;
};

#endif
