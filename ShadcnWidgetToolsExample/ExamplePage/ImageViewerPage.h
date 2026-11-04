#ifndef IMAGEVIEWERPAGE_H
#define IMAGEVIEWERPAGE_H

#include "ExampleBasePage.h"

// 图片预览示例
class ImageViewerPage : public ExampleBasePage
{
    Q_OBJECT

    public:
        explicit ImageViewerPage(QWidget* parent = nullptr);
        ~ImageViewerPage() override;
};

#endif // IMAGEVIEWERPAGE_H
