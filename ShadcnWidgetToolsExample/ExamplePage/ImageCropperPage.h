#ifndef IMAGECROPPERPAGE_H
#define IMAGECROPPERPAGE_H

#include "ExampleBasePage.h"

class ImageCropperPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ImageCropperPage(QWidget* parent = nullptr);
        ~ImageCropperPage() override;
};

#endif // IMAGECROPPERPAGE_H
